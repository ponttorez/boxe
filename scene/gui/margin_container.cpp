/**************************************************************************/
/*  margin_container.cpp                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "margin_container.h"

#include "core/config/engine.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "scene/main/viewport.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"

// Tolerance for detecting an edge touching the safe area edge. Containers such as BoxContainer round child sizes to whole units.
static constexpr real_t SAFE_AREA_EDGE_TOLERANCE = 1.0;

Size2 MarginContainer::get_minimum_size() const {
	Size2 max;

	for (int i = 0; i < get_child_count(); i++) {
		Control *c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}

		Size2 s = c->get_bound_minimum_size();
		max = max.max(s);
	}

	max.width += _get_minimum_margin(SIDE_LEFT) + _get_minimum_margin(SIDE_RIGHT);
	max.height += _get_minimum_margin(SIDE_TOP) + _get_minimum_margin(SIDE_BOTTOM);

	return max;
}

Size2 MarginContainer::get_desired_size() const {
	Size2 ds;

	for (int i = 0; i < get_child_count(); i++) {
		Control *c = as_sortable_control(get_child(i), SortableVisibilityMode::VISIBLE);
		if (!c) {
			continue;
		}

		Size2 s = c->get_desired_size();
		ds = ds.max(s);
	}

	ds.width += _get_minimum_margin(SIDE_LEFT) + _get_minimum_margin(SIDE_RIGHT);
	ds.height += _get_minimum_margin(SIDE_TOP) + _get_minimum_margin(SIDE_BOTTOM);

	return ds;
}

Size2 MarginContainer::get_inner_combined_maximum_size() const {
	Size2 ms = Container::get_inner_combined_maximum_size();

	ms.width -= _get_margin(SIDE_LEFT) + _get_margin(SIDE_RIGHT);
	ms.height -= _get_margin(SIDE_TOP) + _get_margin(SIDE_BOTTOM);

	return ms;
}

Vector<int> MarginContainer::get_allowed_size_flags_horizontal() const {
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> MarginContainer::get_allowed_size_flags_vertical() const {
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

int MarginContainer::get_margin_size(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);

	switch (p_side) {
		case SIDE_LEFT:
			return theme_cache.margin_left;
		case SIDE_RIGHT:
			return theme_cache.margin_right;
		case SIDE_TOP:
			return theme_cache.margin_top;
		case SIDE_BOTTOM:
			return theme_cache.margin_bottom;
	}

	return 0;
}

void MarginContainer::set_safe_area(SafeArea p_safe_area) {
	if (safe_area == p_safe_area) {
		return;
	}

	safe_area = p_safe_area;
	_update_safe_area_tracking();
	update_minimum_size();
	queue_sort();
}

MarginContainer::SafeArea MarginContainer::get_safe_area() const {
	return safe_area;
}

real_t MarginContainer::_get_margin(Side p_side) const {
	return get_margin_size(p_side) + safe_area_offsets[p_side];
}

// The theme margin, negative ones included like in a plain MarginContainer, plus the space kept free by the safe area.
// Content extended outside of the safe area doesn't make the container smaller.
real_t MarginContainer::_get_minimum_margin(Side p_side) const {
	return get_margin_size(p_side) + MAX(0, safe_area_offsets[p_side]);
}

real_t MarginContainer::_get_safe_area_offset(Side p_side) const {
	// Only mobile platforms report a meaningful safe area. Elsewhere it excludes e.g. the taskbar.
	if (safe_area == SAFE_AREA_NONE || !is_inside_tree() || Engine::get_singleton()->is_editor_hint() || !OS::get_singleton()->has_feature("mobile")) {
		return 0;
	}

	DisplayServer *display_server = DisplayServer::get_singleton();
	const Rect2 screen_safe_area = display_server->get_display_safe_area();
	if (!screen_safe_area.has_area()) {
		return 0;
	}

	// Distances from this container's edges to the safe area edges, positive when the edge is outside of the safe area.
	// The viewport transform includes the content scale of the window, unlike get_screen_transform() with embedded subwindows.
	const Transform2D screen_to_local = (get_viewport()->get_screen_transform() * get_global_transform_with_canvas()).affine_inverse();
	const Rect2 safe_rect = screen_to_local.xform(screen_safe_area);
	const Size2 size = get_size();
	real_t unsafe_distance = 0;
	switch (p_side) {
		case SIDE_LEFT:
			unsafe_distance = safe_rect.position.x;
			break;
		case SIDE_TOP:
			unsafe_distance = safe_rect.position.y;
			break;
		case SIDE_RIGHT:
			unsafe_distance = size.width - safe_rect.get_end().x;
			break;
		case SIDE_BOTTOM:
			unsafe_distance = size.height - safe_rect.get_end().y;
			break;
	}

	if (safe_area == SAFE_AREA_INSIDE) {
		// The margin is the larger of the theme margin and the unsafe area overlapping this container.
		const real_t limit = (p_side == SIDE_LEFT || p_side == SIDE_RIGHT) ? size.width : size.height;
		return MAX(0, CLAMP(unsafe_distance, 0, limit) - get_margin_size(p_side));
	}

	// Outside: extend to the screen edge, but only from an edge that touches or crosses the safe area edge.
	if (unsafe_distance < -SAFE_AREA_EDGE_TOLERANCE) {
		return 0;
	}

	const Rect2 screen_rect = screen_to_local.xform(Rect2(Point2(), display_server->window_get_size()));
	real_t screen_distance = 0;
	switch (p_side) {
		case SIDE_LEFT:
			screen_distance = -screen_rect.position.x;
			break;
		case SIDE_TOP:
			screen_distance = -screen_rect.position.y;
			break;
		case SIDE_RIGHT:
			screen_distance = screen_rect.get_end().x - size.width;
			break;
		case SIDE_BOTTOM:
			screen_distance = screen_rect.get_end().y - size.height;
			break;
	}
	return -MAX(0, screen_distance);
}

void MarginContainer::_update_safe_area_offsets() {
	bool changed = false;
	for (int side = 0; side < 4; side++) {
		const real_t offset = _get_safe_area_offset(Side(side));
		if (!Math::is_equal_approx(safe_area_offsets[side], offset)) {
			safe_area_offsets[side] = offset;
			changed = true;
		}
	}

	if (changed) {
		update_minimum_size();
		queue_sort();
	}
}

void MarginContainer::_update_safe_area_tracking() {
	if (!is_inside_tree()) {
		return;
	}

	const bool tracking = safe_area != SAFE_AREA_NONE;
	set_notify_transform(tracking);

	DisplayServer *display_server = DisplayServer::get_singleton();
	const Callable callable = callable_mp(this, &MarginContainer::_update_safe_area_offsets);
	if (tracking && !display_server->is_connected(SNAME("safe_area_changed"), callable)) {
		display_server->connect(SNAME("safe_area_changed"), callable);
	} else if (!tracking && display_server->is_connected(SNAME("safe_area_changed"), callable)) {
		display_server->disconnect(SNAME("safe_area_changed"), callable);
	}

	_update_safe_area_offsets();
}

void MarginContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_SORT_CHILDREN: {
			Size2 s = get_size();

			for (int i = 0; i < get_child_count(); i++) {
				Control *c = as_sortable_control(get_child(i));
				if (!c) {
					continue;
				}

				const real_t margin_left = _get_margin(SIDE_LEFT);
				const real_t margin_top = _get_margin(SIDE_TOP);
				const real_t w = s.width - margin_left - _get_margin(SIDE_RIGHT);
				const real_t h = s.height - margin_top - _get_margin(SIDE_BOTTOM);
				fit_child_in_rect(c, Rect2(margin_left, margin_top, w, h));
			}
		} break;

		case NOTIFICATION_ENTER_TREE: {
			_update_safe_area_tracking();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			DisplayServer *display_server = DisplayServer::get_singleton();
			const Callable callable = callable_mp(this, &MarginContainer::_update_safe_area_offsets);
			if (display_server->is_connected(SNAME("safe_area_changed"), callable)) {
				display_server->disconnect(SNAME("safe_area_changed"), callable);
			}
		} break;

		case NOTIFICATION_RESIZED:
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (safe_area != SAFE_AREA_NONE) {
				_update_safe_area_offsets();
			}
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			_update_safe_area_offsets();
			update_minimum_size();
		} break;
	}
}

void MarginContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_safe_area", "safe_area"), &MarginContainer::set_safe_area);
	ClassDB::bind_method(D_METHOD("get_safe_area"), &MarginContainer::get_safe_area);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "safe_area", PROPERTY_HINT_ENUM, "None,Inside,Outside"), "set_safe_area", "get_safe_area");

	BIND_ENUM_CONSTANT(SAFE_AREA_NONE);
	BIND_ENUM_CONSTANT(SAFE_AREA_INSIDE);
	BIND_ENUM_CONSTANT(SAFE_AREA_OUTSIDE);

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, MarginContainer, margin_left);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, MarginContainer, margin_top);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, MarginContainer, margin_right);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, MarginContainer, margin_bottom);
}

MarginContainer::MarginContainer() {
}
