/**************************************************************************/
/*  style_box.cpp                                                         */
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

#include "style_box.h"

#include "core/object/class_db.h"
#include "scene/main/canvas_item.h"

Size2 StyleBox::get_minimum_size() const {
	Size2 min_size = Size2(get_margin(SIDE_LEFT) + get_margin(SIDE_RIGHT), get_margin(SIDE_TOP) + get_margin(SIDE_BOTTOM));
	Size2 custom_size;
	GDVIRTUAL_CALL(_get_minimum_size, custom_size);

	if (min_size.x < custom_size.x) {
		min_size.x = custom_size.x;
	}
	if (min_size.y < custom_size.y) {
		min_size.y = custom_size.y;
	}

	return min_size;
}

void StyleBox::set_content_margin(Side p_side, float p_value) {
	ERR_FAIL_INDEX((int)p_side, 4);

	content_margin[p_side] = p_value;
	emit_changed();
}

void StyleBox::set_content_margin_all(float p_value) {
	for (int i = 0; i < 4; i++) {
		content_margin[i] = p_value;
	}
	emit_changed();
}

void StyleBox::set_content_margin_individual(float p_left, float p_top, float p_right, float p_bottom) {
	content_margin[SIDE_LEFT] = p_left;
	content_margin[SIDE_TOP] = p_top;
	content_margin[SIDE_RIGHT] = p_right;
	content_margin[SIDE_BOTTOM] = p_bottom;
	emit_changed();
}

float StyleBox::get_content_margin(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	return content_margin[p_side];
}

float StyleBox::get_margin(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0.0);

	if (content_margin[p_side] < 0) {
		return get_style_margin(p_side);
	} else {
		return content_margin[p_side];
	}
}

Point2 StyleBox::get_offset() const {
	return Point2(get_margin(SIDE_LEFT), get_margin(SIDE_TOP));
}

void StyleBox::draw(RID p_canvas_item, const Rect2 &p_rect) const {
	GDVIRTUAL_CALL(_draw, p_canvas_item, p_rect);
}

Rect2 StyleBox::get_draw_rect(const Rect2 &p_rect) const {
	Rect2 ret;
	if (GDVIRTUAL_CALL(_get_draw_rect, p_rect, ret)) {
		return ret;
	}
	return p_rect;
}

CanvasItem *StyleBox::get_current_item_drawn() const {
	return CanvasItem::get_current_item_drawn();
}

bool StyleBox::test_mask(const Point2 &p_point, const Rect2 &p_rect) const {
	bool ret = true;
	GDVIRTUAL_CALL(_test_mask, p_point, p_rect, ret);
	return ret;
}

void StyleBox::set_transition_duration(float p_duration) {
	ERR_FAIL_COND_MSG(p_duration < 0.0, "Transition duration cannot be negative.");
	transition_duration = p_duration;
	emit_changed();
}

float StyleBox::get_transition_duration() const {
	return transition_duration;
}

void StyleBox::set_transition_type(Tween::TransitionType p_type) {
	ERR_FAIL_INDEX(p_type, Tween::TRANS_MAX);
	transition_type = p_type;
	emit_changed();
}

Tween::TransitionType StyleBox::get_transition_type() const {
	return transition_type;
}

void StyleBox::set_transition_ease(Tween::EaseType p_ease) {
	ERR_FAIL_INDEX(p_ease, Tween::EASE_MAX);
	transition_ease = p_ease;
	emit_changed();
}

Tween::EaseType StyleBox::get_transition_ease() const {
	return transition_ease;
}

real_t StyleBox::ease_transition(real_t p_progress) const {
	return Tween::run_equation(transition_type, transition_ease, CLAMP(p_progress, 0, 1), 0, 1, 1);
}

void StyleBox::_interpolate_content_margins(const Ref<StyleBox> &p_from, const Ref<StyleBox> &p_to, real_t p_weight) {
	for (int i = 0; i < 4; i++) {
		const float from = p_from->content_margin[i];
		const float to = p_to->content_margin[i];
		if (from < 0 && to < 0) {
			// Both use the style margin, which follows the interpolated style.
			content_margin[i] = -1;
			continue;
		}
		// A negative margin means "use the style margin", so the margin it gives is blended instead.
		const float resolved_from = from < 0 ? p_from->get_style_margin((Side)i) : from;
		const float resolved_to = to < 0 ? p_to->get_style_margin((Side)i) : to;
		// A margin going past the target can't become negative, as that would mean "use the style margin".
		content_margin[i] = MAX(0, Math::lerp(resolved_from, resolved_to, (float)p_weight));
	}
}

Ref<StyleBox> StyleBox::duplicate_for_transition() const {
	return duplicate(false);
}

void StyleBox::set_content_margins_from(const Ref<StyleBox> &p_style_box) {
	ERR_FAIL_COND(p_style_box.is_null());
	for (int i = 0; i < 4; i++) {
		content_margin[i] = p_style_box->get_margin((Side)i);
	}
	emit_changed();
}

void StyleBox::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_minimum_size"), &StyleBox::get_minimum_size);

	ClassDB::bind_method(D_METHOD("set_content_margin", "margin", "offset"), &StyleBox::set_content_margin);
	ClassDB::bind_method(D_METHOD("set_content_margin_all", "offset"), &StyleBox::set_content_margin_all);
	ClassDB::bind_method(D_METHOD("get_content_margin", "margin"), &StyleBox::get_content_margin);

	ClassDB::bind_method(D_METHOD("get_margin", "margin"), &StyleBox::get_margin);
	ClassDB::bind_method(D_METHOD("get_offset"), &StyleBox::get_offset);

	ClassDB::bind_method(D_METHOD("draw", "canvas_item", "rect"), &StyleBox::draw);
	ClassDB::bind_method(D_METHOD("get_current_item_drawn"), &StyleBox::get_current_item_drawn);

	ClassDB::bind_method(D_METHOD("test_mask", "point", "rect"), &StyleBox::test_mask);

	ADD_GROUP("Content Margins", "content_margin_");
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "content_margin_left", PROPERTY_HINT_RANGE, "-1,2048,1,suffix:px"), "set_content_margin", "get_content_margin", SIDE_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "content_margin_top", PROPERTY_HINT_RANGE, "-1,2048,1,suffix:px"), "set_content_margin", "get_content_margin", SIDE_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "content_margin_right", PROPERTY_HINT_RANGE, "-1,2048,1,suffix:px"), "set_content_margin", "get_content_margin", SIDE_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "content_margin_bottom", PROPERTY_HINT_RANGE, "-1,2048,1,suffix:px"), "set_content_margin", "get_content_margin", SIDE_BOTTOM);

	ClassDB::bind_method(D_METHOD("set_transition_duration", "duration"), &StyleBox::set_transition_duration);
	ClassDB::bind_method(D_METHOD("get_transition_duration"), &StyleBox::get_transition_duration);
	ClassDB::bind_method(D_METHOD("set_transition_type", "type"), &StyleBox::set_transition_type);
	ClassDB::bind_method(D_METHOD("get_transition_type"), &StyleBox::get_transition_type);
	ClassDB::bind_method(D_METHOD("set_transition_ease", "ease"), &StyleBox::set_transition_ease);
	ClassDB::bind_method(D_METHOD("get_transition_ease"), &StyleBox::get_transition_ease);

	ADD_GROUP("Transition", "transition_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "transition_duration", PROPERTY_HINT_RANGE, "0,2,0.01,or_greater,suffix:s"), "set_transition_duration", "get_transition_duration");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "transition_type", PROPERTY_HINT_ENUM, "Linear,Sine,Quint,Quart,Quad,Expo,Elastic,Cubic,Circ,Bounce,Back,Spring"), "set_transition_type", "get_transition_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "transition_ease", PROPERTY_HINT_ENUM, "In,Out,In Out,Out In"), "set_transition_ease", "get_transition_ease");

	GDVIRTUAL_BIND(_draw, "to_canvas_item", "rect")
	GDVIRTUAL_BIND(_get_draw_rect, "rect")
	GDVIRTUAL_BIND(_get_minimum_size)
	GDVIRTUAL_BIND(_test_mask, "point", "rect")
}

StyleBox::StyleBox() {
	for (int i = 0; i < 4; i++) {
		content_margin[i] = -1;
	}
}
