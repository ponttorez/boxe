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
#include "core/templates/hash_set.h"
#include "servers/rendering/rendering_server.h"

// StyleBoxLayer.

void StyleBoxLayer::set_mode(Mode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 3);
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

bool StyleBoxLayer::interpolate(const Ref<StyleBoxLayer> &p_from, const Ref<StyleBoxLayer> &p_to, real_t p_weight) {
	if (p_from.is_null() || p_to.is_null() || p_from->mode != p_to->mode) {
		return false;
	}

	mode = p_to->mode;
	// Curves like Elastic and Back overshoot the target. Shapes may go past it, but colors would leave their range and change hue.
	color = p_from->color.lerp(p_to->color, CLAMP(p_weight, 0, 1));
	width = MAX(0, Math::lerp(p_from->width, p_to->width, p_weight));
	blur = MAX(0, Math::lerp(p_from->blur, p_to->blur, p_weight));
	blend = p_to->blend;
	for (int i = 0; i < 4; i++) {
		margin[i] = Math::lerp(p_from->margin[i], p_to->margin[i], p_weight);
	}
	offset = p_from->offset.lerp(p_to->offset, p_weight);
	corner_radius_adjust = Math::lerp(p_from->corner_radius_adjust, p_to->corner_radius_adjust, p_weight);
	emit_changed();
	return true;
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

	ADD_PROPERTY(PropertyInfo(Variant::INT, "mode", PROPERTY_HINT_ENUM, "Fill,Outline,Shadow"), "set_mode", "get_mode");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "width", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_width", "get_width");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "blend"), "set_blend", "get_blend");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "blur", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_blur", "get_blur");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset", PROPERTY_HINT_NONE, "suffix:px"), "set_offset", "get_offset");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "corner_radius_adjust", PROPERTY_HINT_RANGE, "-64,64,1,or_less,or_greater,suffix:px"), "set_corner_radius_adjust", "get_corner_radius_adjust");

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

void StyleBoxLayered::set_layers(const TypedArray<StyleBoxLayer> &p_layers) {
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
	emit_changed();
}

TypedArray<StyleBoxLayer> StyleBoxLayered::get_layers() const {
	return layers;
}

void StyleBoxLayered::set_corner_style(CornerStyle p_style) {
	ERR_FAIL_INDEX((int)p_style, 2);
	corner_style = p_style;
	notify_property_list_changed();
	emit_changed();
}

StyleBoxLayered::CornerStyle StyleBoxLayered::get_corner_style() const {
	return corner_style;
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

void StyleBoxLayered::set_pixel_snap(bool p_snap) {
	pixel_snap = p_snap;
	emit_changed();
}

bool StyleBoxLayered::is_pixel_snap() const {
	return pixel_snap;
}

void StyleBoxLayered::set_content_offset(const Vector2 &p_offset) {
	content_offset = p_offset;
	emit_changed();
}

Vector2 StyleBoxLayered::get_content_offset() const {
	return content_offset;
}

float StyleBoxLayered::get_style_margin(Side p_side) const {
	return 0;
}

Vector2 StyleBoxLayered::get_style_content_offset() const {
	return content_offset;
}

real_t StyleBoxLayered::_snap(real_t p_value) const {
	return pixel_snap ? Math::snapped(p_value, pixel_size) : p_value;
}

Rect2 StyleBoxLayered::_get_layer_rect(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const {
	Rect2 rect = p_rect;
	if (pixel_snap) {
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

} // namespace

static bool _is_in_pixel_ring(const PixelShape &p_outer, const PixelShape &p_inner, const Point2 &p_point) {
	const Vector2 outer_span = p_outer.get_span(p_point.y);
	if (p_point.x < outer_span.x || p_point.x >= outer_span.y) {
		return false;
	}
	const Vector2 inner_span = p_inner.get_span(p_point.y);
	return p_point.x < inner_span.x || p_point.x >= inner_span.y;
}

// Fills the area between two pixel shapes, row by row.
static void _draw_pixel_ring(RID p_canvas_item, const PixelShape &p_outer, const PixelShape &p_inner, const Color &p_color) {
	if (p_outer.is_empty() || p_color.a <= 0) {
		return;
	}

	LocalVector<real_t> rows;
	p_outer.add_rows(rows);
	p_inner.add_rows(rows);
	rows.sort();

	RenderingServer *rs = RenderingServer::get_singleton();
	for (uint32_t i = 0; i + 1 < rows.size(); i++) {
		const real_t y0 = rows[i];
		const real_t y1 = rows[i + 1];
		if (y1 <= y0) {
			continue;
		}

		const real_t y = (y0 + y1) / 2;
		const Vector2 outer_span = p_outer.get_span(y);
		if (outer_span.y <= outer_span.x) {
			continue;
		}

		const Vector2 inner_span = p_inner.get_span(y);
		if (inner_span.y <= inner_span.x) {
			rs->canvas_item_add_rect(p_canvas_item, Rect2(outer_span.x, y0, outer_span.y - outer_span.x, y1 - y0), p_color);
			continue;
		}

		const real_t left_end = MIN(inner_span.x, outer_span.y);
		if (left_end > outer_span.x) {
			rs->canvas_item_add_rect(p_canvas_item, Rect2(outer_span.x, y0, left_end - outer_span.x, y1 - y0), p_color);
		}
		const real_t right_start = MAX(inner_span.y, outer_span.x);
		if (outer_span.y > right_start) {
			rs->canvas_item_add_rect(p_canvas_item, Rect2(right_start, y0, outer_span.y - right_start, y1 - y0), p_color);
		}
	}
}

// Cells of the pixel grid with their opacity, for skewed shapes, whose edges make stairs.
struct SkewedPixelCells {
	Vector2 origin;
	real_t step = 1;
	Vector2 skew;
	Vector2 center;
	HashMap<Vector2i, real_t> alphas;

	Vector2 skew_point(const Vector2 &p_point) const {
		const Vector2 rel = p_point - center;
		return center + Vector2(rel.x - skew.x * rel.y, rel.y - skew.y * rel.x);
	}

	Vector2i to_cell(const Vector2 &p_point) const {
		return Vector2i(Math::floor((p_point.x - origin.x) / step), Math::floor((p_point.y - origin.y) / step));
	}

	void add_cell(const Vector2i &p_cell, real_t p_alpha) {
		real_t *alpha = alphas.getptr(p_cell);
		if (alpha) {
			*alpha = MAX(*alpha, p_alpha);
		} else {
			alphas.insert(p_cell, p_alpha);
		}
	}

	// Overlapping rings keep the highest opacity, instead of adding up.
	void add_ring(const PixelShape &p_outer, const PixelShape &p_inner, real_t p_alpha) {
		const real_t det = 1.0 - skew.x * skew.y;
		if (p_outer.is_empty() || Math::is_zero_approx(det)) {
			return;
		}

		// Cells of the shape moved by the skew keep thin outlines connected.
		for (real_t y = p_outer.rect.position.y; y < p_outer.rect.get_end().y; y += step) {
			for (real_t x = p_outer.rect.position.x; x < p_outer.rect.get_end().x; x += step) {
				const Vector2 cell_center = Vector2(x, y) + Vector2(step, step) / 2;
				if (_is_in_pixel_ring(p_outer, p_inner, cell_center)) {
					add_cell(to_cell(skew_point(cell_center)), p_alpha);
				}
			}
		}

		// Cells whose center unskews into the shape fill the gaps between them.
		Rect2 bounds;
		for (int i = 0; i < 4; i++) {
			const Vector2 skewed = skew_point(p_outer.rect.position + p_outer.rect.size * Vector2(i & 1, i >> 1));
			bounds = i == 0 ? Rect2(skewed, Size2()) : bounds.expand(skewed);
		}
		const Vector2i from = to_cell(bounds.position);
		const Vector2i to = to_cell(bounds.get_end());
		for (int cy = from.y; cy <= to.y; cy++) {
			for (int cx = from.x; cx <= to.x; cx++) {
				const Vector2 rel = origin + (Vector2(cx, cy) + Vector2(0.5, 0.5)) * step - center;
				const real_t u = (rel.x + skew.x * rel.y) / det;
				const real_t v = rel.y + skew.y * u;
				if (_is_in_pixel_ring(p_outer, p_inner, center + Vector2(u, v))) {
					add_cell(Vector2i(cx, cy), p_alpha);
				}
			}
		}
	}

	struct CellSort {
		_FORCE_INLINE_ bool operator()(const Vector2i &p_a, const Vector2i &p_b) const {
			return p_a.y != p_b.y ? p_a.y < p_b.y : p_a.x < p_b.x;
		}
	};

	// Draws each row as runs of consecutive cells with the same opacity.
	void draw(RID p_canvas_item, const Color &p_color) const {
		LocalVector<Vector2i> cells;
		for (const KeyValue<Vector2i, real_t> &E : alphas) {
			cells.push_back(E.key);
		}
		cells.sort_custom<CellSort>();

		RenderingServer *rs = RenderingServer::get_singleton();
		for (uint32_t i = 0; i < cells.size();) {
			const real_t alpha = alphas[cells[i]];
			uint32_t j = i + 1;
			while (j < cells.size() && cells[j].y == cells[i].y && cells[j].x == cells[j - 1].x + 1 && alphas[cells[j]] == alpha) {
				j++;
			}
			const Vector2 position = origin + Vector2(cells[i].x, cells[i].y) * step;
			rs->canvas_item_add_rect(p_canvas_item, Rect2(position, Size2((j - i) * step, step)), p_color * Color(1, 1, 1, alpha));
			i = j;
		}
	}
};

// The shape grown (or shrunk with a negative amount) by p_amount on all sides, keeping the corners concentric.
static PixelShape _grow_pixel_shape(const PixelShape &p_shape, real_t p_amount) {
	PixelShape ret = p_shape;
	ret.rect = p_shape.rect.grow(p_amount);
	for (int i = 0; i < 4; i++) {
		ret.radius[i] = MAX(0, p_shape.radius[i] + p_amount);
	}
	return ret;
}

void StyleBoxLayered::_draw_pixel_layer(RID p_canvas_item, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect, const Vector2 &p_center) const {
	PixelShape outer;
	outer.rect = p_rect;
	outer.step = pixel_size;
	for (int i = 0; i < 4; i++) {
		outer.radius[i] = MIN(_snap(MAX(0, corner_radius[i] + p_layer->get_corner_radius_adjust())), MIN(p_rect.size.x, p_rect.size.y) / 2);
	}
	if (outer.is_empty()) {
		return;
	}

	// The layer as rings of opacity, drawn together.
	struct Ring {
		PixelShape outer;
		PixelShape inner;
		real_t alpha = 1;
	};
	LocalVector<Ring> rings;
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
				ring_outer.rect = p_rect.grow(-(width * i / steps));
				PixelShape ring_inner = outer;
				ring_inner.rect = p_rect.grow(-(width * (i + 1) / steps));
				rings.push_back({ ring_outer, ring_inner, 1.0f - (real_t)i / steps });
			}
		} break;
		case StyleBoxLayer::MODE_SHADOW: {
			rings.push_back({ outer, PixelShape(), 1 });
			// Rings fading out around the shape.
			const int steps = (int)Math::ceil(p_layer->get_blur() / pixel_size);
			for (int i = 0; i < steps; i++) {
				rings.push_back({ _grow_pixel_shape(outer, (i + 1) * pixel_size), _grow_pixel_shape(outer, i * pixel_size), (real_t)(steps - i) / (steps + 1) });
			}
		} break;
	}

	const Color color = p_layer->get_color();
	if (skew.is_zero_approx()) {
		for (const Ring &ring : rings) {
			_draw_pixel_ring(p_canvas_item, ring.outer, ring.inner, color * Color(1, 1, 1, ring.alpha));
		}
		return;
	}

	SkewedPixelCells cells;
	cells.origin = p_rect.position;
	cells.step = pixel_size;
	cells.skew = skew;
	cells.center = p_center;
	for (const Ring &ring : rings) {
		cells.add_ring(ring.outer, ring.inner, ring.alpha);
	}
	cells.draw(p_canvas_item, color);
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

void StyleBoxLayered::draw(RID p_canvas_item, const Rect2 &p_rect) const {
	for (const Variant &v : layers) {
		const Ref<StyleBoxLayer> layer = v;
		if (layer.is_null() || layer->get_color().a <= 0) {
			continue;
		}

		const Rect2 rect = _get_layer_rect(layer, p_rect);
		if (rect.size.x <= 0 || rect.size.y <= 0) {
			continue;
		}

		if (corner_style == CORNER_STYLE_PIXEL) {
			_draw_pixel_layer(p_canvas_item, layer, rect, p_rect.get_center());
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
	if (from.is_null() || to.is_null() || from->layers.size() != to->layers.size()) {
		return false;
	}

	// Each layer is interpolated into a layer owned by this style box, so the source layers are never modified.
	TypedArray<StyleBoxLayer> own_layers;
	for (int i = 0; i < to->layers.size(); i++) {
		const Ref<StyleBoxLayer> from_layer = from->layers[i];
		const Ref<StyleBoxLayer> to_layer = to->layers[i];
		if (from_layer.is_null() || to_layer.is_null() || from_layer->get_mode() != to_layer->get_mode()) {
			return false;
		}

		Ref<StyleBoxLayer> layer;
		if (i < layers.size()) {
			layer = layers[i];
		}
		if (layer.is_null() || layer == from_layer || layer == to_layer) {
			layer = to_layer->duplicate();
		}
		layer->interpolate(from_layer, to_layer, p_weight);
		own_layers.push_back(layer);
	}

	if (own_layers != layers) {
		set_layers(own_layers);
	}

	_interpolate_content_margins(p_from, p_to, p_weight);
	for (int i = 0; i < 4; i++) {
		corner_radius[i] = MAX(0, Math::lerp(from->corner_radius[i], to->corner_radius[i], p_weight));
	}
	content_offset = from->content_offset.lerp(to->content_offset, p_weight);
	skew = from->skew.lerp(to->skew, p_weight);
	anti_aliasing_size = MAX(0, Math::lerp(from->anti_aliasing_size, to->anti_aliasing_size, p_weight));
	corner_style = to->corner_style;
	corner_detail = MAX(from->corner_detail, to->corner_detail);
	anti_aliased = to->anti_aliased;
	pixel_size = to->pixel_size;
	pixel_snap = to->pixel_snap;
	emit_changed();
	return true;
}

bool StyleBoxLayered::make_transparent() {
	for (const Variant &v : layers) {
		Ref<StyleBoxLayer> layer = v;
		if (layer.is_valid()) {
			layer->set_color(layer->get_color() * Color(1, 1, 1, 0));
		}
	}
	return true;
}

#ifndef DISABLE_DEPRECATED
bool StyleBoxLayered::_set(const StringName &p_name, const Variant &p_value) {
	if (p_name == SNAME("anti_aliased")) {
		set_anti_aliased(p_value);
		return true;
	}
	return false;
}
#endif

void StyleBoxLayered::_validate_property(PropertyInfo &p_property) const {
	if (corner_style == CORNER_STYLE_PIXEL && (p_property.name == "corner_detail" || p_property.name == "anti_aliasing" || p_property.name == "anti_aliasing_size")) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
}

void StyleBoxLayered::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_layers", "layers"), &StyleBoxLayered::set_layers);
	ClassDB::bind_method(D_METHOD("get_layers"), &StyleBoxLayered::get_layers);
	ClassDB::bind_method(D_METHOD("set_corner_style", "style"), &StyleBoxLayered::set_corner_style);
	ClassDB::bind_method(D_METHOD("get_corner_style"), &StyleBoxLayered::get_corner_style);
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
	ClassDB::bind_method(D_METHOD("set_pixel_snap", "snap"), &StyleBoxLayered::set_pixel_snap);
	ClassDB::bind_method(D_METHOD("is_pixel_snap"), &StyleBoxLayered::is_pixel_snap);
	ClassDB::bind_method(D_METHOD("set_content_offset", "offset"), &StyleBoxLayered::set_content_offset);
	ClassDB::bind_method(D_METHOD("get_content_offset"), &StyleBoxLayered::get_content_offset);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "layers", PROPERTY_HINT_ARRAY_TYPE, MAKE_RESOURCE_TYPE_HINT("StyleBoxLayer")), "set_layers", "get_layers");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "corner_style", PROPERTY_HINT_ENUM, "Smooth,Pixel"), "set_corner_style", "get_corner_style");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "skew"), "set_skew", "get_skew");

	ADD_GROUP("Corner Radius", "corner_radius_");
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_top_left", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_TOP_LEFT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_top_right", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_TOP_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_bottom_right", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_BOTTOM_RIGHT);
	ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "corner_radius_bottom_left", PROPERTY_HINT_RANGE, "0,64,1,or_greater,suffix:px"), "set_corner_radius", "get_corner_radius", CORNER_BOTTOM_LEFT);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "corner_detail", PROPERTY_HINT_RANGE, "1,20,1"), "set_corner_detail", "get_corner_detail");

	ADD_GROUP("Anti Aliasing", "anti_aliasing_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "anti_aliasing"), "set_anti_aliased", "is_anti_aliased");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "anti_aliasing_size", PROPERTY_HINT_RANGE, "0.01,10,0.001,suffix:px"), "set_anti_aliasing_size", "get_anti_aliasing_size");

	ADD_GROUP("Pixel", "pixel_");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixel_size", PROPERTY_HINT_RANGE, "1,16,1,or_greater,suffix:px"), "set_pixel_size", "get_pixel_size");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "pixel_snap"), "set_pixel_snap", "is_pixel_snap");

	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "content_offset", PROPERTY_HINT_NONE, "suffix:px"), "set_content_offset", "get_content_offset");

	BIND_ENUM_CONSTANT(CORNER_STYLE_SMOOTH);
	BIND_ENUM_CONSTANT(CORNER_STYLE_PIXEL);
}
