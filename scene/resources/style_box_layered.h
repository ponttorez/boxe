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

#include "core/variant/typed_array.h"
#include "scene/resources/style_box.h"
#include "scene/resources/style_box_flat.h"

class StyleBoxLayer : public Resource {
	GDCLASS(StyleBoxLayer, Resource);

public:
	enum Mode {
		MODE_FILL,
		MODE_OUTLINE,
		MODE_SHADOW,
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

	bool interpolate(const Ref<StyleBoxLayer> &p_from, const Ref<StyleBoxLayer> &p_to, real_t p_weight);
};

class StyleBoxLayered : public StyleBox {
	GDCLASS(StyleBoxLayered, StyleBox);

public:
	enum CornerStyle {
		CORNER_STYLE_SMOOTH,
		CORNER_STYLE_PIXEL,
	};

private:
	TypedArray<StyleBoxLayer> layers;
	CornerStyle corner_style = CORNER_STYLE_SMOOTH;
	real_t corner_radius[4] = {};
	int corner_detail = 8;
	bool anti_aliased = true;
	real_t anti_aliasing_size = 1.0;
	Vector2 skew;
	real_t pixel_size = 1.0;
	bool pixel_snap = true;
	Vector2 content_offset;

	mutable Ref<StyleBoxFlat> smooth_drawer;

	void _layer_changed();
	real_t _snap(real_t p_value) const;
	Rect2 _get_layer_rect(const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const;
	void _draw_smooth_layer(RID p_canvas_item, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) const;
	void _draw_pixel_layer(RID p_canvas_item, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect, const Vector2 &p_center) const;
	Vector2 _get_skew_shift(const Rect2 &p_rect, const Vector2 &p_center) const;

protected:
	virtual float get_style_margin(Side p_side) const override;
	virtual Vector2 get_style_content_offset() const override;
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;
#ifndef DISABLE_DEPRECATED
	bool _set(const StringName &p_name, const Variant &p_value);
#endif

public:
	void set_layers(const TypedArray<StyleBoxLayer> &p_layers);
	TypedArray<StyleBoxLayer> get_layers() const;

	void set_corner_style(CornerStyle p_style);
	CornerStyle get_corner_style() const;

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

	void set_pixel_snap(bool p_snap);
	bool is_pixel_snap() const;

	void set_content_offset(const Vector2 &p_offset);
	Vector2 get_content_offset() const;

	virtual Rect2 get_draw_rect(const Rect2 &p_rect) const override;
	virtual void draw(RID p_canvas_item, const Rect2 &p_rect) const override;
	virtual bool interpolate(const Ref<StyleBox> &p_from, const Ref<StyleBox> &p_to, real_t p_weight) override;
	virtual bool make_transparent() override;
};

VARIANT_ENUM_CAST(StyleBoxLayer::Mode);
VARIANT_ENUM_CAST(StyleBoxLayered::CornerStyle);
