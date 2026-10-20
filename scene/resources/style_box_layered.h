/**************************************************************************/
/*  style_box_layered.h                                                   */
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

#pragma once

#include "core/templates/local_vector.h"
#include "core/variant/typed_array.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"

class StyleBoxLayer : public Resource {
	GDCLASS(StyleBoxLayer, Resource);
	friend class StyleBoxLayered;

public:
	enum Mode {
		MODE_FILL,
		MODE_OUTLINE,
		MODE_SHADOW,
		MODE_MAX,
	};

private:
	Mode mode = MODE_FILL;
	Color color = Color(1, 1, 1);
	real_t width = 1.0;
	bool blend = false;
	real_t blur = 8.0;
	real_t margin[4] = {};
	Vector2 offset;
	real_t corner_radius_adjust = 0.0;
	bool draw_on_top = false;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const;

	void set_color(const Color &p_color);
	Color get_color() const;

	void set_width(real_t p_width);
	real_t get_width() const;

	void set_blend(bool p_blend);
	bool get_blend() const;

	void set_blur(real_t p_blur);
	real_t get_blur() const;

	void set_margin(Side p_side, real_t p_margin);
	real_t get_margin(Side p_side) const;

	void set_offset(const Vector2 &p_offset);
	Vector2 get_offset() const;

	void set_corner_radius_adjust(real_t p_adjust);
	real_t get_corner_radius_adjust() const;

	void set_draw_on_top(bool p_draw_on_top);
	bool is_draw_on_top_enabled() const;

	// Sets this layer to the state between two layers of the same mode. A null layer is a transparent copy of the other one.
	void interpolate(const Ref<StyleBoxLayer> &p_from, const Ref<StyleBoxLayer> &p_to, real_t p_weight);
};

class StyleBoxLayered : public StyleBox {
	GDCLASS(StyleBoxLayered, StyleBox);

public:
	enum Style {
		STYLE_SMOOTH,
		STYLE_PIXEL,
		STYLE_MAX,
	};

private:
	TypedArray<StyleBoxLayer> layers;
	Style style = STYLE_SMOOTH;
	real_t corner_radius[4] = {};
	int corner_detail = 8;
	bool anti_aliased = true;
	real_t anti_aliasing_size = 1.0;
	Vector2 skew;
	real_t pixel_size = 1.0;
	bool snap = false;
	Vector2 content_offset;

	mutable Ref<StyleBoxFlat> smooth_drawer;

	void _layer_changed();
	void _set_layers(const TypedArray<StyleBoxLayer> &p_layers);
	real_t _snap(real_t p_value) const;
	Rect2 _get_layer_rect(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const;
	void _draw_smooth_layer(RID p_canvas_item, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const;
	Vector2 _get_skew_shift(const Rect2 &p_rect, const Vector2 &p_center) const;

protected:
	virtual float get_style_margin(Side p_side) const override;
	virtual Vector2 get_style_content_offset() const override;
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	struct PixelRect {
		Rect2 rect;
		real_t alpha = 1.0;
	};

	void set_layers(const TypedArray<StyleBoxLayer> &p_layers);
	TypedArray<StyleBoxLayer> get_layers() const;

	void set_style(Style p_style);
	Style get_style() const;

	void set_corner_radius(Corner p_corner, real_t p_radius);
	real_t get_corner_radius(Corner p_corner) const;

	void set_corner_detail(int p_detail);
	int get_corner_detail() const;

	void set_anti_aliased(bool p_anti_aliased);
	bool is_anti_aliased() const;

	void set_anti_aliasing_size(real_t p_size);
	real_t get_anti_aliasing_size() const;

	void set_skew(const Vector2 &p_skew);
	Vector2 get_skew() const;

	void set_pixel_size(real_t p_size);
	real_t get_pixel_size() const;

	void set_snap(bool p_snap);
	bool is_snap_enabled() const;

	void set_content_offset(const Vector2 &p_offset);
	Vector2 get_content_offset() const;

	virtual Rect2 get_draw_rect(const Rect2 &p_rect) const override;
	virtual void draw(RID p_canvas_item, const Rect2 &p_rect) const override;
	virtual bool interpolate(const Ref<StyleBox> &p_from, const Ref<StyleBox> &p_to, real_t p_weight) override;
	virtual bool make_transparent() override;
	virtual Ref<StyleBox> duplicate_for_transition() const override;

	// Returns the rectangles drawing a layer with STYLE_PIXEL in the given style box rectangle, with their opacity.
	void get_pixel_rects(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect, LocalVector<PixelRect> &r_rects) const;

private:
	// What the pixel rectangles of a layer are made from. Drawing reuses them while it stays the same,
	// as making them can check every cell of the area, e.g. with a skew on both axes. The rectangles are kept relative
	// to the pixel origin of the style box rectangle, so the same size at other positions reuses them.
	struct PixelKey {
		Vector2 size;
		Vector2 origin_offset; // Of the rectangle from its pixel origin.
		Vector2 skew;
		real_t pixel_size = 0.0;
		bool snap = false;
		real_t corner_radius[4] = {};
		StyleBoxLayer::Mode mode = StyleBoxLayer::MODE_FILL;
		real_t width = 0.0;
		bool blend = false;
		real_t blur = 0.0;
		real_t margin[4] = {};
		Vector2 offset;
		real_t corner_radius_adjust = 0.0;

		bool operator==(const PixelKey &p_other) const;
	};
	struct PixelCache {
		PixelKey key;
		LocalVector<PixelRect> rects; // Relative to the pixel origin.
		uint64_t last_used = 0;
	};
	// The rectangles of the sizes drawn last, for each layer, as a style box of a theme is drawn by many controls.
	static constexpr uint32_t PIXEL_CACHE_SIZES = 8;
	mutable LocalVector<LocalVector<PixelCache>> pixel_caches;
	mutable uint64_t pixel_cache_uses = 0;

	void _draw_layers(RID p_canvas_item, const Rect2 &p_rect, bool p_on_top) const;
	Vector2 _get_pixel_origin(const Rect2 &p_rect) const;
	void _make_pixel_rects(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect, LocalVector<PixelRect> &r_rects) const;
	PixelKey _make_pixel_key(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const;
	const PixelCache &_get_pixel_cache(int p_index, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const;
};

VARIANT_ENUM_CAST(StyleBoxLayer::Mode);
VARIANT_ENUM_CAST(StyleBoxLayered::Style);
