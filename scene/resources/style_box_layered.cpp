/**************************************************************************/
/*  style_box_layered.cpp                                                 */
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

#include "style_box_layered.h"

#include "core/core_string_names.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/main/canvas_item.h"
#include "servers/rendering/rendering_server.h"

// StyleBoxLayer.

void StyleBoxLayer::set_mode(Mode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, MODE_MAX);
	mode = p_mode;
	notify_property_list_changed();
	emit_changed();
}

StyleBoxLayer::Mode StyleBoxLayer::get_mode() const {
	return mode;
}

void StyleBoxLayer::set_color(const Color &p_color) {
	color = p_color;
	emit_changed();
}

Color StyleBoxLayer::get_color() const {
	return color;
}

void StyleBoxLayer::set_width(real_t p_width) {
	width = MAX(0, p_width);
	emit_changed();
}

real_t StyleBoxLayer::get_width() const {
	return width;
}

void StyleBoxLayer::set_blend(bool p_blend) {
	blend = p_blend;
	emit_changed();
}

bool StyleBoxLayer::get_blend() const {
	return blend;
}

void StyleBoxLayer::set_blur(real_t p_blur) {
	blur = MAX(0, p_blur);
	emit_changed();
}

real_t StyleBoxLayer::get_blur() const {
	return blur;
}

void StyleBoxLayer::set_margin(Side p_side, real_t p_margin) {
	ERR_FAIL_INDEX((int)p_side, 4);
	margin[p_side] = p_margin;
	emit_changed();
}

real_t StyleBoxLayer::get_margin(Side p_side) const {
	ERR_FAIL_INDEX_V((int)p_side, 4, 0);
	return margin[p_side];
}

void StyleBoxLayer::set_offset(const Vector2 &p_offset) {
	offset = p_offset;
	emit_changed();
}

Vector2 StyleBoxLayer::get_offset() const {
	return offset;
}

void StyleBoxLayer::set_corner_radius_adjust(real_t p_adjust) {
	corner_radius_adjust = p_adjust;
	emit_changed();
}

real_t StyleBoxLayer::get_corner_radius_adjust() const {
	return corner_radius_adjust;
}

void StyleBoxLayer::set_draw_on_top(bool p_draw_on_top) {
	draw_on_top = p_draw_on_top;
	emit_changed();
}

bool StyleBoxLayer::is_draw_on_top_enabled() const {
	return draw_on_top;
}

void StyleBoxLayer::interpolate(const Ref<StyleBoxLayer> &p_from, const Ref<StyleBoxLayer> &p_to, real_t p_weight) {
	ERR_FAIL_COND(p_from.is_null() && p_to.is_null());
	// A layer missing on one side fades in or out with the shape of the other side.
	const StyleBoxLayer *from = p_from.is_valid() ? p_from.ptr() : p_to.ptr();
	const StyleBoxLayer *to = p_to.is_valid() ? p_to.ptr() : p_from.ptr();
	const Color from_color = p_from.is_valid() ? from->color : Color(to->color, 0);
	const Color to_color = p_to.is_valid() ? to->color : Color(from->color, 0);

	mode = to->mode;
	// Curves like Elastic and Back overshoot the target. Shapes may go past it, but colors would leave their range and change hue.
	color = from_color.lerp(to_color, CLAMP(p_weight, 0, 1));
	width = MAX(0, Math::lerp(from->width, to->width, p_weight));
	blur = MAX(0, Math::lerp(from->blur, to->blur, p_weight));
	blend = to->blend;
	for (int i = 0; i < 4; i++) {
		margin[i] = Math::lerp(from->margin[i], to->margin[i], p_weight);
	}
	offset = from->offset.lerp(to->offset, p_weight);
	corner_radius_adjust = Math::lerp(from->corner_radius_adjust, to->corner_radius_adjust, p_weight);
	draw_on_top = to->draw_on_top;
}

void StyleBoxLayer::_validate_property(PropertyInfo &p_property) const {
	if ((p_property.name == "width" || p_property.name == "blend") && mode != MODE_OUTLINE) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
	if (p_property.name == "blur" && mode != MODE_SHADOW) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
}

void StyleBoxLayer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mode", "mode"), &StyleBoxLayer::set_mode);
	ClassDB::bind_method(D_METHOD("get_mode"), &StyleBoxLayer::get_mode);
	ClassDB::bind_method(D_METHOD("set_color", "color"), &StyleBoxLayer::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &StyleBoxLayer::get_color);
	ClassDB::bind_method(D_METHOD("set_width", "width"), &StyleBoxLayer::set_width);
	ClassDB::bind_method(D_METHOD("get_width"), &StyleBoxLayer::get_width);
	ClassDB::bind_method(D_METHOD("set_blend", "blend"), &StyleBoxLayer::set_blend);
	ClassDB::bind_method(D_METHOD("get_blend"), &StyleBoxLayer::get_blend);
	ClassDB::bind_method(D_METHOD("set_blur", "blur"), &StyleBoxLayer::set_blur);
	ClassDB::bind_method(D_METHOD("get_blur"), &StyleBoxLayer::get_blur);
	ClassDB::bind_method(D_METHOD("set_margin", "side", "margin"), &StyleBoxLayer::set_margin);
	ClassDB::bind_method(D_METHOD("get_margin", "side"), &StyleBoxLayer::get_margin);
	ClassDB::bind_method(D_METHOD("set_offset", "offset"), &StyleBoxLayer::set_offset);
	ClassDB::bind_method(D_METHOD("get_offset"), &StyleBoxLayer::get_offset);
	ClassDB::bind_method(D_METHOD("set_corner_radius_adjust", "adjust"), &StyleBoxLayer::set_corner_radius_adjust);
	ClassDB::bind_method(D_METHOD("get_corner_radius_adjust"), &StyleBoxLayer::get_corner_radius_adjust);

	ClassDB::bind_method(D_METHOD("set_draw_on_top", "draw_on_top"), &StyleBoxLayer::set_draw_on_top);
	ClassDB::bind_method(D_METHOD("is_draw_on_top_enabled"), &StyleBoxLayer::is_draw_on_top_enabled);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Fill,Outline,Shadow"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "width", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "blend"), "set_blend", "get_blend");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blur", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_blur", "get_blur");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset", PROPERTY_HINT_NONE, "suffix:px"), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "corner_radius_adjust", PROPERTY_HINT_RANGE, "-64,64,1,or_less,or_greater,suffix:px"), "set_corner_radius_adjust", "get_corner_radius_adjust");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_on_top"), "set_draw_on_top", "is_draw_on_top_enabled");

	ADD_GROUP("Margin", "margin_");
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "margin_left", PROPERTY_HINT_RANGE, "-256,256,1,or_less,or_greater,suffix:px"), "set_margin", "get_margin", SIDE_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "margin_top", PROPERTY_HINT_RANGE, "-256,256,1,or_less,or_greater,suffix:px"), "set_margin", "get_margin", SIDE_TOP);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "margin_right", PROPERTY_HINT_RANGE, "-256,256,1,or_less,or_greater,suffix:px"), "set_margin", "get_margin", SIDE_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "margin_bottom", PROPERTY_HINT_RANGE, "-256,256,1,or_less,or_greater,suffix:px"), "set_margin", "get_margin", SIDE_BOTTOM);

	BIND_ENUM_CONSTANT(MODE_FILL);
	BIND_ENUM_CONSTANT(MODE_OUTLINE);
	BIND_ENUM_CONSTANT(MODE_SHADOW);
}

// StyleBoxLayered.

void StyleBoxLayered::_layer_changed() {
	emit_changed();
}

void StyleBoxLayered::_set_layers(const TypedArray<StyleBoxLayer> &p_layers) {
	const Callable callable = callable_mp(this, &StyleBoxLayered::_layer_changed);
	for (const Variant &v : layers) {
		Ref<StyleBoxLayer> layer = v;
		if (layer.is_valid() && layer->is_connected(CoreStringName(changed), callable)) {
			layer->disconnect(CoreStringName(changed), callable);
		}
	}

	layers = p_layers;
	for (const Variant &v : layers) {
		Ref<StyleBoxLayer> layer = v;
		if (layer.is_valid() && !layer->is_connected(CoreStringName(changed), callable)) {
			layer->connect(CoreStringName(changed), callable);
		}
	}
}

void StyleBoxLayered::set_layers(const TypedArray<StyleBoxLayer> &p_layers) {
	_set_layers(p_layers);
	emit_changed();
}

TypedArray<StyleBoxLayer> StyleBoxLayered::get_layers() const {
	return layers;
}

void StyleBoxLayered::set_style(Style p_style) {
	ERR_FAIL_INDEX((int)p_style, STYLE_MAX);
	style = p_style;
	notify_property_list_changed();
	emit_changed();
}

StyleBoxLayered::Style StyleBoxLayered::get_style() const {
	return style;
}

void StyleBoxLayered::set_corner_radius(Corner p_corner, real_t p_radius) {
	ERR_FAIL_INDEX((int)p_corner, 4);
	corner_radius[p_corner] = MAX(0, p_radius);
	emit_changed();
}

real_t StyleBoxLayered::get_corner_radius(Corner p_corner) const {
	ERR_FAIL_INDEX_V((int)p_corner, 4, 0);
	return corner_radius[p_corner];
}

void StyleBoxLayered::set_corner_detail(int p_detail) {
	corner_detail = CLAMP(p_detail, 1, 20);
	emit_changed();
}

int StyleBoxLayered::get_corner_detail() const {
	return corner_detail;
}

void StyleBoxLayered::set_anti_aliased(bool p_anti_aliased) {
	anti_aliased = p_anti_aliased;
	emit_changed();
}

bool StyleBoxLayered::is_anti_aliased() const {
	return anti_aliased;
}

void StyleBoxLayered::set_anti_aliasing_size(real_t p_size) {
	anti_aliasing_size = CLAMP(p_size, 0.01, 10);
	emit_changed();
}

real_t StyleBoxLayered::get_anti_aliasing_size() const {
	return anti_aliasing_size;
}

void StyleBoxLayered::set_skew(const Vector2 &p_skew) {
	skew = p_skew;
	emit_changed();
}

Vector2 StyleBoxLayered::get_skew() const {
	return skew;
}

void StyleBoxLayered::set_pixel_size(real_t p_size) {
	pixel_size = MAX(1, p_size);
	emit_changed();
}

real_t StyleBoxLayered::get_pixel_size() const {
	return pixel_size;
}

void StyleBoxLayered::set_snap(bool p_snap) {
	snap = p_snap;
	emit_changed();
}

bool StyleBoxLayered::is_snap_enabled() const {
	return snap;
}

void StyleBoxLayered::set_content_offset(const Vector2 &p_offset) {
	content_offset = p_offset;
	emit_changed();
}

Vector2 StyleBoxLayered::get_content_offset() const {
	return content_offset;
}

float StyleBoxLayered::get_style_margin(Side p_side) const {
	// The content is placed inside the outlines, like inside the border of StyleBoxFlat.
	real_t ret = 0;
	for (const Variant &v : layers) {
		const Ref<StyleBoxLayer> layer = v;
		if (layer.is_valid() && layer->get_mode() == StyleBoxLayer::MODE_OUTLINE) {
			ret = MAX(ret, _snap(layer->get_margin(p_side)) + _snap(layer->get_width()));
		}
	}
	return ret;
}

Vector2 StyleBoxLayered::get_style_content_offset() const {
	return content_offset;
}

real_t StyleBoxLayered::_snap(real_t p_value) const {
	return snap ? Math::snapped(p_value, pixel_size) : p_value;
}

Rect2 StyleBoxLayered::_get_layer_rect(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const {
	Rect2 rect = p_rect;
	if (snap) {
		rect.position = rect.position.round();
		rect.size = rect.size.round();
	}

	rect = rect.grow_individual(-_snap(p_layer->get_margin(SIDE_LEFT)), -_snap(p_layer->get_margin(SIDE_TOP)), -_snap(p_layer->get_margin(SIDE_RIGHT)), -_snap(p_layer->get_margin(SIDE_BOTTOM)));
	rect.position += Vector2(_snap(p_layer->get_offset().x), _snap(p_layer->get_offset().y));
	return rect;
}

void StyleBoxLayered::_draw_smooth_layer(RID p_canvas_item, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const {
	if (smooth_drawer.is_null()) {
		smooth_drawer.instantiate();
		smooth_drawer->set_content_margin_all(0);
	}

	// Reuses the StyleBoxFlat drawing code for antialiased rounded shapes.
	const bool outline = p_layer->get_mode() == StyleBoxLayer::MODE_OUTLINE;
	const bool shadow = p_layer->get_mode() == StyleBoxLayer::MODE_SHADOW && p_layer->get_blur() > 0;
	const Color color = p_layer->get_color();
	smooth_drawer->bg_color = shadow ? Color(color.r, color.g, color.b, 0) : color;
	smooth_drawer->border_color = color;
	smooth_drawer->draw_center = !outline;
	smooth_drawer->blend_border = outline && p_layer->get_blend();
	smooth_drawer->shadow_color = color;
	smooth_drawer->shadow_size = shadow ? Math::round(p_layer->get_blur()) : 0;
	smooth_drawer->shadow_offset = Point2();
	smooth_drawer->skew = skew;
	for (int i = 0; i < 4; i++) {
		smooth_drawer->border_width[i] = outline ? _snap(p_layer->get_width()) : 0;
		smooth_drawer->corner_radius[i] = MAX(0, corner_radius[i] + p_layer->get_corner_radius_adjust());
	}
	smooth_drawer->corner_detail = corner_detail;
	smooth_drawer->anti_aliased = anti_aliased;
	smooth_drawer->aa_size = anti_aliasing_size;
	smooth_drawer->draw(p_canvas_item, p_rect);
}

namespace {

// A rectangle with stepped corners, like pixel art.
struct PixelShape {
	Rect2 rect;
	real_t radius[4] = {};
	real_t step = 1;

	bool is_empty() const {
		return rect.size.x <= 0 || rect.size.y <= 0;
	}

	real_t corner_inset(real_t p_radius, real_t p_distance) const {
		if (p_radius <= 0 || p_distance >= p_radius) {
			return 0;
		}
		// The row starting at p_distance from the edge is inset to the circle, rounded up to whole steps.
		const real_t row = Math::floor(p_distance / step) * step;
		const real_t inset = p_radius - Math::sqrt(MAX(0, p_radius * p_radius - (p_radius - row) * (p_radius - row)));
		return Math::ceil(inset / step - CMP_EPSILON) * step;
	}

	// Returns the horizontal span covered at the given height, or an empty span.
	Vector2 get_span(real_t p_y) const {
		const real_t top = rect.position.y;
		const real_t bottom = rect.get_end().y;
		if (is_empty() || p_y < top || p_y > bottom) {
			return Vector2();
		}

		real_t left = rect.position.x;
		real_t right = rect.get_end().x;
		left += MAX(corner_inset(radius[CORNER_TOP_LEFT], p_y - top), corner_inset(radius[CORNER_BOTTOM_LEFT], bottom - p_y));
		right -= MAX(corner_inset(radius[CORNER_TOP_RIGHT], p_y - top), corner_inset(radius[CORNER_BOTTOM_RIGHT], bottom - p_y));
		return right > left ? Vector2(left, right) : Vector2();
	}

	void add_rows(LocalVector<real_t> &r_rows) const {
		if (is_empty()) {
			return;
		}

		const real_t top = rect.position.y;
		const real_t bottom = rect.get_end().y;
		r_rows.push_back(top);
		r_rows.push_back(bottom);
		const real_t max_radius = MAX(MAX(radius[0], radius[1]), MAX(radius[2], radius[3]));
		// Every step of the corners is a row, up to where the corners end.
		for (real_t d = step; d <= max_radius; d += step) {
			r_rows.push_back(top + d);
			r_rows.push_back(bottom - d);
		}
	}
};

// The shape grown (or shrunk with a negative amount) by p_amount on all sides, keeping the corners concentric.
PixelShape grow_pixel_shape(const PixelShape &p_shape, real_t p_amount) {
	PixelShape ret = p_shape;
	ret.rect = p_shape.rect.grow(p_amount);
	for (int i = 0; i < 4; i++) {
		ret.radius[i] = MAX(0, p_shape.radius[i] + p_amount);
	}
	return ret;
}

// The area between two pixel shapes, with its opacity.
struct PixelRing {
	PixelShape outer;
	PixelShape inner;
	real_t alpha = 1;
};

bool is_in_pixel_ring(const PixelRing &p_ring, const Point2 &p_point) {
	const Vector2 outer_span = p_ring.outer.get_span(p_point.y);
	if (p_point.x < outer_span.x || p_point.x >= outer_span.y) {
		return false;
	}
	const Vector2 inner_span = p_ring.inner.get_span(p_point.y);
	return p_point.x < inner_span.x || p_point.x >= inner_span.y;
}

// Adds the parts of the outer span that aren't in the inner span, as up to two ranges.
int get_ring_ranges(const Vector2 &p_outer, const Vector2 &p_inner, Vector2 r_ranges[2]) {
	if (p_outer.y <= p_outer.x) {
		return 0;
	}
	if (p_inner.y <= p_inner.x) {
		r_ranges[0] = p_outer;
		return 1;
	}
	int count = 0;
	const real_t left_end = MIN(p_inner.x, p_outer.y);
	if (left_end > p_outer.x) {
		r_ranges[count++] = Vector2(p_outer.x, left_end);
	}
	const real_t right_start = MAX(p_inner.y, p_outer.x);
	if (p_outer.y > right_start) {
		r_ranges[count++] = Vector2(right_start, p_outer.y);
	}
	return count;
}

// The first and last of the cells whose centers are in the range, for cells of p_step starting at p_origin.
Vector2i get_cells_in_range(const Vector2 &p_range, real_t p_origin, real_t p_step) {
	return Vector2i(Math::ceil((p_range.x - p_origin) / p_step - 0.5), Math::ceil((p_range.y - p_origin) / p_step - 0.5) - 1);
}

// Whether the center of a cell of a column is in the shape, for cells of p_step starting at p_base.
bool is_column_cell_in_shape(const PixelShape &p_shape, real_t p_column, real_t p_base, real_t p_step, int p_cell) {
	const Vector2 span = p_shape.get_span(p_base + (p_cell + 0.5) * p_step);
	return p_column >= span.x && p_column < span.y;
}

// The first and last of the cells along the axis whose centers are in the shape, at the given position across the axis,
// for cells of p_step starting at p_base. The range is empty (first > last) if there are none.
Vector2i get_shape_cells(const PixelShape &p_shape, Vector2::Axis p_axis, real_t p_across, real_t p_base, real_t p_step) {
	const Vector2i none = Vector2i(0, -1);
	if (p_shape.is_empty()) {
		return none;
	}
	if (p_axis == Vector2::AXIS_X) {
		const Vector2 span = p_shape.get_span(p_across);
		return span.y > span.x ? get_cells_in_range(span, p_base, p_step) : none;
	}

	const Rect2 &rect = p_shape.rect;
	if (p_across < rect.position.x || p_across >= rect.get_end().x) {
		return none;
	}
	const int first = Math::ceil((rect.position.y - p_base) / p_step - 0.5);
	const int last = Math::floor((rect.get_end().y - p_base) / p_step - 0.5);
	if (last < first) {
		return none;
	}

	// The shape is made of rows, which the corners only shorten towards the edges, so the cells of a column
	// inside it are consecutive. They're found from a cell inside, usually the middle one.
	int inside = CLAMP((int)Math::round((rect.get_center().y - p_base) / p_step - 0.5), first, last);
	if (!is_column_cell_in_shape(p_shape, p_across, p_base, p_step, inside)) {
		inside = first;
		while (inside <= last && !is_column_cell_in_shape(p_shape, p_across, p_base, p_step, inside)) {
			inside++;
		}
		if (inside > last) {
			return none;
		}
	}

	Vector2i ret;
	int low = first;
	int high = inside;
	while (low < high) {
		const int middle = low + (high - low) / 2;
		if (is_column_cell_in_shape(p_shape, p_across, p_base, p_step, middle)) {
			high = middle;
		} else {
			low = middle + 1;
		}
	}
	ret.x = low;
	low = inside;
	high = last;
	while (low < high) {
		const int middle = low + (high - low + 1) / 2;
		if (is_column_cell_in_shape(p_shape, p_across, p_base, p_step, middle)) {
			low = middle;
		} else {
			high = middle - 1;
		}
	}
	ret.y = low;
	return ret;
}

// The cells of the outer range that aren't in the inner range, as up to two ranges.
int get_ring_cells(const Vector2i &p_outer, const Vector2i &p_inner, Vector2i r_ranges[2]) {
	if (p_outer.y < p_outer.x) {
		return 0;
	}
	if (p_inner.y < p_inner.x) {
		r_ranges[0] = p_outer;
		return 1;
	}
	int count = 0;
	if (p_inner.x > p_outer.x) {
		r_ranges[count++] = Vector2i(p_outer.x, MIN(p_inner.x - 1, p_outer.y));
	}
	if (p_inner.y < p_outer.y) {
		r_ranges[count++] = Vector2i(MAX(p_inner.y + 1, p_outer.x), p_outer.y);
	}
	return count;
}

// Rows of cells with their opacity, where overlapping runs keep the highest opacity instead of adding up.
struct PixelRows {
	struct Run {
		int first = 0;
		int last = 0;
		real_t alpha = 1;
	};

	int first_row = 0;
	LocalVector<LocalVector<Run>> rows;

	void add(int p_row, int p_first, int p_last, real_t p_alpha) {
		const int row = p_row - first_row;
		if (p_last < p_first || row < 0 || row >= (int)rows.size()) {
			return;
		}
		rows[row].push_back({ p_first, p_last, p_alpha });
	}

	// Rows run along the axis, from the origin, with cells of p_step.
	void get_rects(Vector2::Axis p_axis, const Vector2 &p_origin, real_t p_step, LocalVector<StyleBoxLayered::PixelRect> &r_rects) const {
		const Vector2::Axis across = p_axis == Vector2::AXIS_X ? Vector2::AXIS_Y : Vector2::AXIS_X;
		LocalVector<int> points;
		for (uint32_t row = 0; row < rows.size(); row++) {
			const LocalVector<Run> &runs = rows[row];
			points.clear();
			for (const Run &run : runs) {
				points.push_back(run.first);
				points.push_back(run.last + 1);
			}
			points.sort();

			// Consecutive cells with the same opacity make one rectangle.
			int rect_first = 0;
			int rect_end = 0;
			real_t rect_alpha = -1;
			for (uint32_t i = 0; i + 1 < points.size(); i++) {
				if (points[i + 1] == points[i]) {
					continue;
				}
				real_t alpha = -1;
				for (const Run &run : runs) {
					if (run.first <= points[i] && run.last >= points[i + 1] - 1) {
						alpha = MAX(alpha, run.alpha);
					}
				}
				if (alpha == rect_alpha && points[i] == rect_end) {
					rect_end = points[i + 1];
					continue;
				}
				if (rect_alpha >= 0) {
					_add_rect(p_axis, across, p_origin, p_step, row, rect_first, rect_end, rect_alpha, r_rects);
				}
				rect_first = points[i];
				rect_end = points[i + 1];
				rect_alpha = alpha;
			}
			if (rect_alpha >= 0) {
				_add_rect(p_axis, across, p_origin, p_step, row, rect_first, rect_end, rect_alpha, r_rects);
			}
		}
	}

	void _add_rect(Vector2::Axis p_axis, Vector2::Axis p_across, const Vector2 &p_origin, real_t p_step, uint32_t p_row, int p_first, int p_end, real_t p_alpha, LocalVector<StyleBoxLayered::PixelRect> &r_rects) const {
		Rect2 rect;
		rect.position[p_axis] = p_origin[p_axis] + p_first * p_step;
		rect.position[p_across] = p_origin[p_across] + (first_row + (int)p_row) * p_step;
		rect.size[p_axis] = (p_end - p_first) * p_step;
		rect.size[p_across] = p_step;
		r_rects.push_back({ rect, p_alpha });
	}
};

// Fills the area between two pixel shapes, row by row.
void add_pixel_ring_rects(const PixelRing &p_ring, LocalVector<StyleBoxLayered::PixelRect> &r_rects) {
	if (p_ring.outer.is_empty()) {
		return;
	}

	LocalVector<real_t> rows;
	p_ring.outer.add_rows(rows);
	p_ring.inner.add_rows(rows);
	rows.sort();

	for (uint32_t i = 0; i + 1 < rows.size(); i++) {
		const real_t y0 = rows[i];
		const real_t y1 = rows[i + 1];
		if (y1 <= y0) {
			continue;
		}

		const real_t y = (y0 + y1) / 2;
		Vector2 ranges[2];
		const int count = get_ring_ranges(p_ring.outer.get_span(y), p_ring.inner.get_span(y), ranges);
		for (int j = 0; j < count; j++) {
			r_rects.push_back({ Rect2(ranges[j].x, y0, ranges[j].y - ranges[j].x, y1 - y0), p_ring.alpha });
		}
	}
}

// Keeps the highest opacity of a cell of the grid, cells outside of its bounds are ignored.
void add_grid_cell(LocalVector<real_t> &r_alphas, const Rect2i &p_bounds, const Vector2i &p_cell, real_t p_alpha) {
	const Vector2i cell = p_cell - p_bounds.position;
	if (cell.x >= 0 && cell.y >= 0 && cell.x < p_bounds.size.x && cell.y < p_bounds.size.y) {
		real_t &alpha = r_alphas[cell.y * p_bounds.size.x + cell.x];
		alpha = MAX(alpha, p_alpha);
	}
}

// Skewed shapes are drawn on a grid of cells, so their edges make stairs. A cell is drawn when a cell of the shape
// moves into it with the skew, which keeps thin outlines connected, or when its center moves back into the shape,
// which fills the gaps between them.
struct SkewedPixelGrid {
	Vector2 origin;
	real_t step = 1;
	Vector2 skew;
	Vector2 center;

	Vector2 skew_point(const Vector2 &p_point) const {
		const Vector2 rel = p_point - center;
		return center + Vector2(rel.x - skew.x * rel.y, rel.y - skew.y * rel.x);
	}

	Vector2i to_cell(const Vector2 &p_point) const {
		return Vector2i(Math::floor((p_point.x - origin.x) / step), Math::floor((p_point.y - origin.y) / step));
	}

	Rect2i get_cell_bounds(const PixelShape &p_shape) const {
		Rect2 bounds;
		for (int i = 0; i < 4; i++) {
			const Vector2 skewed = skew_point(p_shape.rect.position + p_shape.rect.size * Vector2(i & 1, i >> 1));
			bounds = i == 0 ? Rect2(skewed, Size2()) : bounds.expand(skewed);
		}
		const Vector2i from = to_cell(bounds.position);
		return Rect2i(from, to_cell(bounds.get_end()) - from + Vector2i(1, 1));
	}

	// Skewed along one axis only, rows of cells along that axis keep their cells together, so each row is made of ranges.
	void add_rects_skewed_on_axis(Vector2::Axis p_axis, const LocalVector<PixelRing> &p_rings, LocalVector<StyleBoxLayered::PixelRect> &r_rects) const {
		const Vector2::Axis across = p_axis == Vector2::AXIS_X ? Vector2::AXIS_Y : Vector2::AXIS_X;
		const real_t amount = skew[p_axis];

		PixelRows rows;
		Rect2i all_bounds;
		for (uint32_t i = 0; i < p_rings.size(); i++) {
			const Rect2i bounds = get_cell_bounds(p_rings[i].outer);
			all_bounds = i == 0 ? bounds : all_bounds.merge(bounds);
		}
		rows.first_row = all_bounds.position[across];
		rows.rows.resize(all_bounds.size[across]);

		for (const PixelRing &ring : p_rings) {
			if (ring.outer.is_empty()) {
				continue;
			}
			const Rect2 &rect = ring.outer.rect;
			Vector2i ranges[2];

			// The cells of the shape, moved along the axis by the skew.
			const int cell_count = Math::ceil(rect.size[p_axis] / step);
			for (real_t row = rect.position[across]; row < rect.get_end()[across]; row += step) {
				const real_t row_center = row + step / 2;
				const int target_row = Math::floor((row_center - origin[across]) / step);
				const int shift = Math::floor((rect.position[p_axis] - origin[p_axis] - amount * (row_center - center[across])) / step + 0.5);
				const Vector2i outer_cells = get_shape_cells(ring.outer, p_axis, row_center, rect.position[p_axis], step);
				const Vector2i inner_cells = get_shape_cells(ring.inner, p_axis, row_center, rect.position[p_axis], step);
				const int count = get_ring_cells(outer_cells, inner_cells, ranges);
				for (int i = 0; i < count; i++) {
					rows.add(target_row, MAX(ranges[i].x, 0) + shift, MIN(ranges[i].y, cell_count - 1) + shift, ring.alpha);
				}
			}

			// The cells whose centers move back into the shape.
			const Rect2i bounds = get_cell_bounds(ring.outer);
			for (int row = bounds.position[across]; row < bounds.get_end()[across]; row++) {
				const real_t row_center = origin[across] + (row + 0.5) * step;
				const real_t base = origin[p_axis] + amount * (row_center - center[across]);
				const Vector2i outer_cells = get_shape_cells(ring.outer, p_axis, row_center, base, step);
				const Vector2i inner_cells = get_shape_cells(ring.inner, p_axis, row_center, base, step);
				const int count = get_ring_cells(outer_cells, inner_cells, ranges);
				for (int i = 0; i < count; i++) {
					rows.add(row, MAX(ranges[i].x, bounds.position[p_axis]), MIN(ranges[i].y, bounds.get_end()[p_axis] - 1), ring.alpha);
				}
			}
		}
		rows.get_rects(p_axis, origin, step, r_rects);
	}

	// Skewed along both axes, every cell is checked.
	void add_rects_skewed(const LocalVector<PixelRing> &p_rings, LocalVector<StyleBoxLayered::PixelRect> &r_rects) const {
		const real_t det = 1.0 - skew.x * skew.y;
		if (Math::is_zero_approx(det)) {
			return;
		}

		Rect2i all_bounds;
		for (uint32_t i = 0; i < p_rings.size(); i++) {
			const Rect2i bounds = get_cell_bounds(p_rings[i].outer);
			all_bounds = i == 0 ? bounds : all_bounds.merge(bounds);
		}
		LocalVector<real_t> alphas;
		alphas.resize(all_bounds.size.x * all_bounds.size.y);
		for (real_t &alpha : alphas) {
			alpha = -1;
		}

		for (const PixelRing &ring : p_rings) {
			if (ring.outer.is_empty()) {
				continue;
			}
			const Rect2 &rect = ring.outer.rect;
			Vector2i ranges[2];

			// The cells of the shape, moved by the skew.
			const int cell_count = Math::ceil(rect.size.x / step);
			for (real_t y = rect.position.y; y < rect.get_end().y; y += step) {
				const real_t row_center = y + step / 2;
				const Vector2i outer_cells = get_shape_cells(ring.outer, Vector2::AXIS_X, row_center, rect.position.x, step);
				const Vector2i inner_cells = get_shape_cells(ring.inner, Vector2::AXIS_X, row_center, rect.position.x, step);
				const int count = get_ring_cells(outer_cells, inner_cells, ranges);
				for (int i = 0; i < count; i++) {
					for (int x = MAX(ranges[i].x, 0); x <= MIN(ranges[i].y, cell_count - 1); x++) {
						add_grid_cell(alphas, all_bounds, to_cell(skew_point(Vector2(rect.position.x + (x + 0.5) * step, row_center))), ring.alpha);
					}
				}
			}

			// The cells whose centers move back into the shape.
			const Rect2i bounds = get_cell_bounds(ring.outer);
			for (int y = bounds.position.y; y < bounds.get_end().y; y++) {
				for (int x = bounds.position.x; x < bounds.get_end().x; x++) {
					const Vector2 rel = origin + (Vector2(x, y) + Vector2(0.5, 0.5)) * step - center;
					const real_t u = (rel.x + skew.x * rel.y) / det;
					const real_t v = rel.y + skew.y * u;
					if (is_in_pixel_ring(ring, center + Vector2(u, v))) {
						add_grid_cell(alphas, all_bounds, Vector2i(x, y), ring.alpha);
					}
				}
			}
		}

		// Consecutive cells with the same opacity make one rectangle.
		for (int y = 0; y < all_bounds.size.y; y++) {
			for (int x = 0; x < all_bounds.size.x;) {
				const real_t alpha = alphas[y * all_bounds.size.x + x];
				int end = x + 1;
				while (end < all_bounds.size.x && alphas[y * all_bounds.size.x + end] == alpha) {
					end++;
				}
				if (alpha >= 0) {
					const Vector2 position = origin + Vector2(all_bounds.position + Vector2i(x, y)) * step;
					r_rects.push_back({ Rect2(position, Size2((end - x) * step, step)), alpha });
				}
				x = end;
			}
		}
	}
};

} // namespace

void StyleBoxLayered::get_pixel_rects(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect, LocalVector<PixelRect> &r_rects) const {
	ERR_FAIL_COND(p_layer.is_null());
	const uint32_t first = r_rects.size();
	const Vector2 origin = _get_pixel_origin(p_rect);
	_make_pixel_rects(p_layer, Rect2(p_rect.position - origin, p_rect.size), r_rects);
	for (uint32_t i = first; i < r_rects.size(); i++) {
		r_rects[i].rect.position += origin;
	}
}

// Made around the pixel origin, so the rectangles are the same at any whole pixel offset, and drawing reuses them.
void StyleBoxLayered::_make_pixel_rects(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect, LocalVector<PixelRect> &r_rects) const {
	const Rect2 rect = _get_layer_rect(p_layer, p_rect);

	PixelShape outer;
	outer.rect = rect;
	outer.step = pixel_size;
	for (int i = 0; i < 4; i++) {
		outer.radius[i] = MIN(_snap(MAX(0, corner_radius[i] + p_layer->get_corner_radius_adjust())), MIN(rect.size.x, rect.size.y) / 2);
	}
	if (outer.is_empty()) {
		return;
	}

	// The layer as rings of opacity, drawn together.
	LocalVector<PixelRing> rings;
	switch (p_layer->get_mode()) {
		case StyleBoxLayer::MODE_FILL: {
			rings.push_back({ outer, PixelShape(), 1 });
		} break;
		case StyleBoxLayer::MODE_OUTLINE: {
			const real_t width = _snap(p_layer->get_width());
			// With blend, steps fade out towards the inner edge.
			const int steps = p_layer->get_blend() ? MAX(1, (int)Math::ceil(width / pixel_size)) : 1;
			for (int i = 0; i < steps; i++) {
				PixelShape ring_outer = outer;
				ring_outer.rect = rect.grow(-(width * i / steps));
				PixelShape ring_inner = outer;
				ring_inner.rect = rect.grow(-(width * (i + 1) / steps));
				rings.push_back({ ring_outer, ring_inner, 1.0f - (real_t)i / steps });
			}
		} break;
		case StyleBoxLayer::MODE_SHADOW: {
			rings.push_back({ outer, PixelShape(), 1 });
			// Rings fading out around the shape.
			const int steps = (int)Math::ceil(p_layer->get_blur() / pixel_size);
			for (int i = 0; i < steps; i++) {
				rings.push_back({ grow_pixel_shape(outer, (i + 1) * pixel_size), grow_pixel_shape(outer, i * pixel_size), (real_t)(steps - i) / (steps + 1) });
			}
		} break;
		case StyleBoxLayer::MODE_MAX: {
		} break;
	}

	if (skew.is_zero_approx()) {
		for (const PixelRing &ring : rings) {
			add_pixel_ring_rects(ring, r_rects);
		}
		return;
	}

	SkewedPixelGrid grid;
	grid.origin = rect.position;
	grid.step = pixel_size;
	grid.skew = skew;
	grid.center = p_rect.get_center();
	if (Math::is_zero_approx(skew.y)) {
		grid.skew.y = 0;
		grid.add_rects_skewed_on_axis(Vector2::AXIS_X, rings, r_rects);
	} else if (Math::is_zero_approx(skew.x)) {
		grid.skew.x = 0;
		grid.add_rects_skewed_on_axis(Vector2::AXIS_Y, rings, r_rects);
	} else {
		grid.add_rects_skewed(rings, r_rects);
	}
}

Vector2 StyleBoxLayered::_get_skew_shift(const Rect2 &p_rect, const Vector2 &p_center) const {
	// StyleBoxFlat skews around the center of each layer, layers are skewed around the center of the box instead.
	const Vector2 layer_center = p_rect.get_center();
	return Vector2(-skew.x * (layer_center.y - p_center.y), -skew.y * (layer_center.x - p_center.x));
}

Rect2 StyleBoxLayered::get_draw_rect(const Rect2 &p_rect) const {
	Rect2 ret = p_rect;
	const Vector2 center = p_rect.get_center();
	for (const Variant &v : layers) {
		const Ref<StyleBoxLayer> layer = v;
		if (layer.is_null()) {
			continue;
		}
		Rect2 rect = _get_layer_rect(layer, p_rect);
		if (layer->get_mode() == StyleBoxLayer::MODE_SHADOW) {
			rect = rect.grow(layer->get_blur() + pixel_size);
		}
		// Room for the skewed corners.
		rect = rect.grow_individual(Math::abs(skew.x) * Math::abs(rect.position.y - center.y), Math::abs(skew.y) * Math::abs(rect.position.x - center.x), Math::abs(skew.x) * Math::abs(rect.get_end().y - center.y), Math::abs(skew.y) * Math::abs(rect.get_end().x - center.x));
		ret = ret.merge(rect.grow(anti_aliasing_size));
	}
	return ret;
}

bool StyleBoxLayered::PixelKey::operator==(const PixelKey &p_other) const {
	for (int i = 0; i < 4; i++) {
		if (corner_radius[i] != p_other.corner_radius[i] || margin[i] != p_other.margin[i]) {
			return false;
		}
	}
	return size == p_other.size && origin_offset == p_other.origin_offset && skew == p_other.skew && pixel_size == p_other.pixel_size && snap == p_other.snap && mode == p_other.mode && width == p_other.width && blend == p_other.blend && blur == p_other.blur && offset == p_other.offset && corner_radius_adjust == p_other.corner_radius_adjust;
}

// The whole pixel position the pixel rectangles are made around.
Vector2 StyleBoxLayered::_get_pixel_origin(const Rect2 &p_rect) const {
	return p_rect.position.floor();
}

StyleBoxLayered::PixelKey StyleBoxLayered::_make_pixel_key(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const {
	PixelKey key;
	key.size = p_rect.size;
	key.origin_offset = p_rect.position - _get_pixel_origin(p_rect);
	key.skew = skew;
	key.pixel_size = pixel_size;
	key.snap = snap;
	for (int i = 0; i < 4; i++) {
		key.corner_radius[i] = corner_radius[i];
		key.margin[i] = p_layer->get_margin((Side)i);
	}
	key.mode = p_layer->get_mode();
	key.width = p_layer->get_width();
	key.blend = p_layer->get_blend();
	key.blur = p_layer->get_blur();
	key.offset = p_layer->get_offset();
	key.corner_radius_adjust = p_layer->get_corner_radius_adjust();
	return key;
}

const StyleBoxLayered::PixelCache &StyleBoxLayered::_get_pixel_cache(int p_index, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const {
	LocalVector<PixelCache> &caches = pixel_caches[p_index];
	const PixelKey key = _make_pixel_key(p_layer, p_rect);
	pixel_cache_uses++;
	uint32_t oldest = 0;
	for (uint32_t i = 0; i < caches.size(); i++) {
		if (caches[i].key == key) {
			caches[i].last_used = pixel_cache_uses;
			return caches[i];
		}
		if (caches[i].last_used < caches[oldest].last_used) {
			oldest = i;
		}
	}
	if (caches.size() < PIXEL_CACHE_SIZES) {
		oldest = caches.size();
		caches.push_back(PixelCache());
	}
	PixelCache &cache = caches[oldest];
	cache.key = key;
	cache.last_used = pixel_cache_uses;
	cache.rects.clear();
	_make_pixel_rects(p_layer, Rect2(p_rect.position - _get_pixel_origin(p_rect), p_rect.size), cache.rects);
	return cache;
}

void StyleBoxLayered::draw(RID p_canvas_item, const Rect2 &p_rect) const {
	if (style == STYLE_PIXEL && pixel_caches.size() < (uint32_t)layers.size()) {
		pixel_caches.resize(layers.size());
	}
	_draw_layers(p_canvas_item, p_rect, false);

	bool has_layers_on_top = false;
	for (int i = 0; i < layers.size() && !has_layers_on_top; i++) {
		const Ref<StyleBoxLayer> layer = layers[i];
		has_layers_on_top = layer.is_valid() && layer->is_draw_on_top_enabled();
	}
	if (has_layers_on_top) {
		// Over the content and the children of the node drawing the style box. Drawn by something else, e.g. a Viewport
		// drawing the borders of embedded windows, they're drawn over the other layers.
		CanvasItem *drawn = CanvasItem::get_current_item_drawn();
		_draw_layers((drawn && drawn->get_canvas_item() == p_canvas_item) ? drawn->get_overlay_canvas_item() : p_canvas_item, p_rect, true);
	}
}

void StyleBoxLayered::_draw_layers(RID p_canvas_item, const Rect2 &p_rect, bool p_on_top) const {
	for (int i = 0; i < layers.size(); i++) {
		const Ref<StyleBoxLayer> layer = layers[i];
		if (layer.is_null() || layer->get_color().a <= 0 || layer->is_draw_on_top_enabled() != p_on_top) {
			continue;
		}

		const Rect2 rect = _get_layer_rect(layer, p_rect);
		if (rect.size.x <= 0 || rect.size.y <= 0) {
			continue;
		}

		if (style == STYLE_PIXEL) {
			const PixelCache &cache = _get_pixel_cache(i, layer, p_rect);
			const Vector2 origin = _get_pixel_origin(p_rect);
			const Color color = layer->get_color();
			for (const PixelRect &pixel_rect : cache.rects) {
				RenderingServer::get_singleton()->canvas_item_add_rect(p_canvas_item, Rect2(pixel_rect.rect.position + origin, pixel_rect.rect.size), color * Color(1, 1, 1, pixel_rect.alpha));
			}
		} else {
			Rect2 skewed_rect = rect;
			skewed_rect.position += _get_skew_shift(rect, p_rect.get_center());
			_draw_smooth_layer(p_canvas_item, layer, skewed_rect);
		}
	}
}

bool StyleBoxLayered::interpolate(const Ref<StyleBox> &p_from, const Ref<StyleBox> &p_to, real_t p_weight) {
	const Ref<StyleBoxLayered> from = p_from;
	const Ref<StyleBoxLayered> to = p_to;
	if (from.is_null() || to.is_null()) {
		return false;
	}

	// Layers are matched by their index. Layers only on one side fade in or out, layers of different modes, or drawn on top
	// on one side only, can't be interpolated.
	const int count = MAX(from->layers.size(), to->layers.size());
	for (int i = 0; i < MIN(from->layers.size(), to->layers.size()); i++) {
		const Ref<StyleBoxLayer> from_layer = from->layers[i];
		const Ref<StyleBoxLayer> to_layer = to->layers[i];
		if (from_layer.is_valid() && to_layer.is_valid() && (from_layer->get_mode() != to_layer->get_mode() || from_layer->is_draw_on_top_enabled() != to_layer->is_draw_on_top_enabled())) {
			return false;
		}
	}

	// Each layer is interpolated into a layer owned by this style box, so the source layers are never modified.
	// The owned layers are kept between frames.
	bool own_layers_valid = layers.size() == count;
	for (int i = 0; i < count && own_layers_valid; i++) {
		const Ref<StyleBoxLayer> layer = layers[i];
		const Ref<StyleBoxLayer> from_layer = i < from->layers.size() ? from->layers[i] : Variant();
		const Ref<StyleBoxLayer> to_layer = i < to->layers.size() ? to->layers[i] : Variant();
		const bool needed = from_layer.is_valid() || to_layer.is_valid();
		own_layers_valid = layer.is_valid() == needed && (layer.is_null() || (layer != from_layer && layer != to_layer));
	}
	if (!own_layers_valid) {
		TypedArray<StyleBoxLayer> own_layers;
		for (int i = 0; i < count; i++) {
			const Ref<StyleBoxLayer> from_layer = i < from->layers.size() ? from->layers[i] : Variant();
			const Ref<StyleBoxLayer> to_layer = i < to->layers.size() ? to->layers[i] : Variant();
			Ref<StyleBoxLayer> layer;
			if (from_layer.is_valid() || to_layer.is_valid()) {
				layer.instantiate();
			}
			own_layers.push_back(layer);
		}
		_set_layers(own_layers);
	}

	for (int i = 0; i < count; i++) {
		const Ref<StyleBoxLayer> layer = layers[i];
		if (layer.is_valid()) {
			const Ref<StyleBoxLayer> from_layer = i < from->layers.size() ? from->layers[i] : Variant();
			const Ref<StyleBoxLayer> to_layer = i < to->layers.size() ? to->layers[i] : Variant();
			layer->interpolate(from_layer, to_layer, p_weight);
		}
	}

	_interpolate_content_margins(p_from, p_to, p_weight);
	for (int i = 0; i < 4; i++) {
		corner_radius[i] = MAX(0, Math::lerp(from->corner_radius[i], to->corner_radius[i], p_weight));
	}
	content_offset = from->content_offset.lerp(to->content_offset, p_weight);
	skew = from->skew.lerp(to->skew, p_weight);
	anti_aliasing_size = MAX(0, Math::lerp(from->anti_aliasing_size, to->anti_aliasing_size, p_weight));
	style = to->style;
	corner_detail = MAX(from->corner_detail, to->corner_detail);
	anti_aliased = to->anti_aliased;
	pixel_size = to->pixel_size;
	snap = to->snap;
	emit_changed();
	return true;
}

Ref<StyleBox> StyleBoxLayered::duplicate_for_transition() const {
	Ref<StyleBoxLayered> copy = duplicate(false);
	// The transitions change the layers, which would change the ones of the theme if they were shared.
	TypedArray<StyleBoxLayer> own_layers;
	for (const Variant &v : layers) {
		const Ref<StyleBoxLayer> layer = v;
		own_layers.push_back(layer.is_valid() ? Ref<StyleBoxLayer>(layer->duplicate()) : layer);
	}
	copy->_set_layers(own_layers);
	return copy;
}

bool StyleBoxLayered::make_transparent() {
	for (const Variant &v : layers) {
		Ref<StyleBoxLayer> layer = v;
		if (layer.is_valid()) {
			layer->color.a = 0;
		}
	}
	emit_changed();
	return true;
}

void StyleBoxLayered::_validate_property(PropertyInfo &p_property) const {
	if (style == STYLE_PIXEL && (p_property.name == "corner_detail" || p_property.name == "anti_aliasing" || p_property.name == "anti_aliasing_size")) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
}

void StyleBoxLayered::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_layers", "layers"), &StyleBoxLayered::set_layers);
	ClassDB::bind_method(D_METHOD("get_layers"), &StyleBoxLayered::get_layers);
	ClassDB::bind_method(D_METHOD("set_style", "style"), &StyleBoxLayered::set_style);
	ClassDB::bind_method(D_METHOD("get_style"), &StyleBoxLayered::get_style);
	ClassDB::bind_method(D_METHOD("set_corner_radius", "corner", "radius"), &StyleBoxLayered::set_corner_radius);
	ClassDB::bind_method(D_METHOD("get_corner_radius", "corner"), &StyleBoxLayered::get_corner_radius);
	ClassDB::bind_method(D_METHOD("set_corner_detail", "detail"), &StyleBoxLayered::set_corner_detail);
	ClassDB::bind_method(D_METHOD("get_corner_detail"), &StyleBoxLayered::get_corner_detail);
	ClassDB::bind_method(D_METHOD("set_anti_aliased", "anti_aliased"), &StyleBoxLayered::set_anti_aliased);
	ClassDB::bind_method(D_METHOD("is_anti_aliased"), &StyleBoxLayered::is_anti_aliased);
	ClassDB::bind_method(D_METHOD("set_anti_aliasing_size", "size"), &StyleBoxLayered::set_anti_aliasing_size);
	ClassDB::bind_method(D_METHOD("get_anti_aliasing_size"), &StyleBoxLayered::get_anti_aliasing_size);
	ClassDB::bind_method(D_METHOD("set_skew", "skew"), &StyleBoxLayered::set_skew);
	ClassDB::bind_method(D_METHOD("get_skew"), &StyleBoxLayered::get_skew);
	ClassDB::bind_method(D_METHOD("set_pixel_size", "size"), &StyleBoxLayered::set_pixel_size);
	ClassDB::bind_method(D_METHOD("get_pixel_size"), &StyleBoxLayered::get_pixel_size);
	ClassDB::bind_method(D_METHOD("set_snap", "snap"), &StyleBoxLayered::set_snap);
	ClassDB::bind_method(D_METHOD("is_snap_enabled"), &StyleBoxLayered::is_snap_enabled);
	ClassDB::bind_method(D_METHOD("set_content_offset", "offset"), &StyleBoxLayered::set_content_offset);
	ClassDB::bind_method(D_METHOD("get_content_offset"), &StyleBoxLayered::get_content_offset);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "style", PROPERTY_HINT_ENUM, "Smooth,Pixel"), "set_style", "get_style");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layers", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("StyleBoxLayer")), "set_layers", "get_layers");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixel_size", PROPERTY_HINT_RANGE, "1,16,1,or_greater,suffix:px"), "set_pixel_size", "get_pixel_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "snap"), "set_snap", "is_snap_enabled");

	ADD_GROUP("Corner Radius", "corner_radius_");
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_top_left", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_TOP_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_top_right", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_TOP_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_bottom_right", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_BOTTOM_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_bottom_left", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_BOTTOM_LEFT);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "corner_detail", PROPERTY_HINT_RANGE, "1,20,1"), "set_corner_detail", "get_corner_detail");

	ADD_GROUP("Anti Aliasing", "anti_aliasing_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "anti_aliasing"), "set_anti_aliased", "is_anti_aliased");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "anti_aliasing_size", PROPERTY_HINT_RANGE, "0.01,10,0.001,suffix:px"), "set_anti_aliasing_size", "get_anti_aliasing_size");

	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "skew"), "set_skew", "get_skew");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "content_offset", PROPERTY_HINT_NONE, "suffix:px"), "set_content_offset", "get_content_offset");

	BIND_ENUM_CONSTANT(STYLE_SMOOTH);
	BIND_ENUM_CONSTANT(STYLE_PIXEL);
}
