/**************************************************************************/
/*  test_style_box_transitions.cpp                                        */
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

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_style_box_transitions)

#include "core/templates/hash_map.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_layered.h"
#include "scene/resources/style_box_line.h"
#include "scene/resources/style_box_texture.h"

namespace TestStyleBoxTransitions {

TEST_CASE("[StyleBoxFlat] Interpolation past the target") {
	Ref<StyleBoxFlat> from;
	from.instantiate();
	from->set_bg_color(Color(0.2, 0.4, 0.6));
	from->set_corner_radius_all(2);
	Ref<StyleBoxFlat> to;
	to.instantiate();
	to->set_bg_color(Color(0.3, 0.5, 0.9));
	to->set_corner_radius_all(10);

	Ref<StyleBoxFlat> live = to->duplicate();
	// Elastic and Back curves return weights outside of 0 and 1.
	REQUIRE(live->interpolate(from, to, 1.5));
	CHECK_MESSAGE(live->get_bg_color().is_equal_approx(to->get_bg_color()), "Colors don't go past the target, so their hue doesn't change.");
	CHECK_MESSAGE(live->get_corner_radius(CORNER_TOP_LEFT) == doctest::Approx(14), "Shapes go past the target.");

	REQUIRE(live->interpolate(from, to, -0.5));
	CHECK(live->get_bg_color().is_equal_approx(from->get_bg_color()));
	CHECK_MESSAGE(live->get_corner_radius(CORNER_TOP_LEFT) == doctest::Approx(0), "Sizes don't become negative.");
}

TEST_CASE("[StyleBoxFlat] Parts shown on one side fade") {
	Ref<StyleBoxFlat> from;
	from.instantiate();
	from->set_draw_center(false);
	from->set_bg_color(Color(1, 0, 0));
	Ref<StyleBoxFlat> to;
	to.instantiate();
	to->set_bg_color(Color(0, 0, 1));
	to->set_border_width_all(2);
	to->set_border_color(Color(0, 1, 0));
	to->set_shadow_size(4);
	to->set_shadow_color(Color(1, 1, 0));

	Ref<StyleBoxFlat> live = to->duplicate();
	REQUIRE(live->interpolate(from, to, 0.5));
	CHECK(live->is_draw_center_enabled());
	CHECK_MESSAGE(live->get_bg_color().is_equal_approx(Color(0, 0, 1, 0.5)), "The center fades in with its own color.");
	CHECK_MESSAGE(live->get_border_color().is_equal_approx(Color(0, 1, 0, 0.5)), "The border fades in with its own color.");
	CHECK_MESSAGE(live->get_shadow_color().is_equal_approx(Color(1, 1, 0, 0.5)), "The shadow fades in with its own color.");

	REQUIRE(live->interpolate(to, from, 0.5));
	CHECK(live->is_draw_center_enabled());
	CHECK_MESSAGE(live->get_bg_color().is_equal_approx(Color(0, 0, 1, 0.5)), "The center fades out with its own color.");
	CHECK_MESSAGE(live->get_border_color().is_equal_approx(Color(0, 1, 0, 0.5)), "The border fades out with its own color.");
}

TEST_CASE("[StyleBoxFlat] Content margins") {
	Ref<StyleBoxFlat> from;
	from.instantiate();
	from->set_border_width_all(4);
	Ref<StyleBoxFlat> to;
	to.instantiate();
	to->set_content_margin_all(10);

	Ref<StyleBoxFlat> live = to->duplicate();
	REQUIRE(live->interpolate(from, to, 0.5));
	CHECK_MESSAGE(live->get_content_margin(SIDE_LEFT) == doctest::Approx(7), "A margin taken from the border is blended with a set margin.");

	REQUIRE(live->interpolate(from, from, 0.5));
	CHECK_MESSAGE(live->get_content_margin(SIDE_LEFT) < 0, "Margins taken from the border stay so.");
	CHECK(live->get_margin(SIDE_LEFT) == doctest::Approx(4));

	Ref<StyleBoxFlat> zero = to->duplicate();
	zero->set_content_margin_all(0);
	REQUIRE(live->interpolate(to, zero, 1.5));
	CHECK_MESSAGE(live->get_content_margin(SIDE_LEFT) == doctest::Approx(0), "Margins going past the target don't become negative.");
}

TEST_CASE("[StyleBox] Content margins from another style box") {
	Ref<StyleBoxFlat> source;
	source.instantiate();
	source->set_border_width_all(3);
	source->set_content_margin(SIDE_TOP, 8);
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_content_margins_from(source);
	CHECK(style->get_margin(SIDE_LEFT) == doctest::Approx(3));
	CHECK(style->get_margin(SIDE_TOP) == doctest::Approx(8));
}

TEST_CASE("[StyleBox] Transparent copies") {
	Ref<StyleBoxFlat> flat;
	flat.instantiate();
	REQUIRE(flat->make_transparent());
	CHECK(flat->get_bg_color().a == 0);
	CHECK(flat->get_border_color().a == 0);
	CHECK(flat->get_shadow_color().a == 0);

	Ref<StyleBoxLine> line;
	line.instantiate();
	REQUIRE(line->make_transparent());
	CHECK(line->get_color().a == 0);

	Ref<StyleBoxTexture> texture;
	texture.instantiate();
	REQUIRE(texture->make_transparent());
	CHECK(texture->get_modulate().a == 0);

	Ref<StyleBoxEmpty> empty;
	empty.instantiate();
	CHECK_FALSE(empty->make_transparent());
}

TEST_CASE("[StyleBoxLine] Interpolation") {
	Ref<StyleBoxLine> from;
	from.instantiate();
	from->set_color(Color(0, 0, 0));
	from->set_thickness(2);
	from->set_grow_begin(0);
	Ref<StyleBoxLine> to;
	to.instantiate();
	to->set_color(Color(1, 1, 1));
	to->set_thickness(6);
	to->set_grow_begin(4);

	Ref<StyleBoxLine> live = to->duplicate();
	REQUIRE(live->interpolate(from, to, 0.5));
	CHECK(live->get_color().is_equal_approx(Color(0.5, 0.5, 0.5)));
	CHECK(live->get_thickness() == 4);
	CHECK(live->get_grow_begin() == doctest::Approx(2));

	to->set_vertical(true);
	CHECK_FALSE_MESSAGE(live->interpolate(from, to, 0.5), "Lines with different orientations aren't interpolated.");
}

TEST_CASE("[StyleBoxTexture] Interpolation") {
	Ref<Image> image = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
	Ref<ImageTexture> texture = ImageTexture::create_from_image(image);
	Ref<StyleBoxTexture> from;
	from.instantiate();
	from->set_texture(texture);
	from->set_modulate(Color(0, 0, 0));
	from->set_texture_margin_all(0);
	Ref<StyleBoxTexture> to;
	to.instantiate();
	to->set_texture(texture);
	to->set_modulate(Color(1, 1, 1));
	to->set_texture_margin_all(2);

	Ref<StyleBoxTexture> live = to->duplicate();
	REQUIRE(live->interpolate(from, to, 0.5));
	CHECK(live->get_modulate().is_equal_approx(Color(0.5, 0.5, 0.5)));
	CHECK(live->get_texture_margin(SIDE_LEFT) == doctest::Approx(1));

	to->set_texture(ImageTexture::create_from_image(image));
	CHECK_FALSE_MESSAGE(live->interpolate(from, to, 0.5), "Style boxes with different textures aren't interpolated.");
}

TEST_CASE("[StyleBoxTexture] Copies for transitions share the texture") {
	// A texture made in memory, which a deep copy would duplicate.
	Ref<ImageTexture> texture = ImageTexture::create_from_image(Image::create_empty(4, 4, false, Image::FORMAT_RGBA8));
	Ref<StyleBoxTexture> from;
	from.instantiate();
	from->set_texture(texture);
	from->set_modulate(Color(0, 0, 0));
	Ref<StyleBoxTexture> to;
	to.instantiate();
	to->set_texture(texture);
	to->set_modulate(Color(1, 1, 1));

	const Ref<StyleBoxTexture> from_copy = from->duplicate_for_transition();
	CHECK(from_copy->get_texture() == texture);
	const Ref<StyleBoxTexture> live = to->duplicate_for_transition();
	CHECK(live->interpolate(from_copy, to, 0.5));
	CHECK(live->get_modulate().is_equal_approx(Color(0.5, 0.5, 0.5)));
}

TEST_CASE("[StyleBox] Transition properties") {
	Ref<StyleBoxEmpty> style;
	style.instantiate();
	style->set_transition_duration(0.5);
	ERR_PRINT_OFF;
	style->set_transition_duration(-1);
	ERR_PRINT_ON;
	CHECK_MESSAGE(style->get_transition_duration() == doctest::Approx(0.5), "Negative durations are rejected.");

	style->set_transition_type(Tween::TRANS_LINEAR);
	style->set_transition_ease(Tween::EASE_IN);
	CHECK(style->ease_transition(0.25) == doctest::Approx(0.25));
	CHECK_MESSAGE(style->ease_transition(2) == doctest::Approx(1), "The progress is clamped.");
}

TEST_CASE("[StyleBoxLayered] Interpolation past the target") {
	Ref<StyleBoxLayer> from_layer;
	from_layer.instantiate();
	from_layer->set_color(Color(0.2, 0.4, 0.6));
	from_layer->set_margin(SIDE_LEFT, 2);
	Ref<StyleBoxLayer> to_layer;
	to_layer.instantiate();
	to_layer->set_color(Color(0.3, 0.5, 0.9));
	to_layer->set_margin(SIDE_LEFT, 6);

	Ref<StyleBoxLayered> from;
	from.instantiate();
	from->set_layers({ from_layer });
	Ref<StyleBoxLayered> to;
	to.instantiate();
	to->set_layers({ to_layer });

	Ref<StyleBoxLayered> live = to->duplicate();
	REQUIRE(live->interpolate(from, to, 1.5));
	const Ref<StyleBoxLayer> layer = live->get_layers()[0];
	CHECK_MESSAGE(layer->get_color().is_equal_approx(to_layer->get_color()), "Colors don't go past the target, so their hue doesn't change.");
	CHECK_MESSAGE(layer->get_margin(SIDE_LEFT) == doctest::Approx(8), "Shapes go past the target.");
}

static Ref<StyleBoxLayer> make_layer(StyleBoxLayer::Mode p_mode, const Color &p_color) {
	Ref<StyleBoxLayer> layer;
	layer.instantiate();
	layer->set_mode(p_mode);
	layer->set_color(p_color);
	return layer;
}

static Ref<StyleBoxLayered> make_layered(const TypedArray<StyleBoxLayer> &p_layers) {
	Ref<StyleBoxLayered> style;
	style.instantiate();
	style->set_layers(p_layers);
	return style;
}

TEST_CASE("[StyleBoxLayered] Copies for transitions have their own layers") {
	Ref<StyleBoxLayer> layer = make_layer(StyleBoxLayer::MODE_FILL, Color(1, 0, 0));
	Ref<StyleBoxLayered> style = make_layered({ layer });
	const Ref<StyleBoxLayered> copy = style->duplicate_for_transition();
	REQUIRE(copy->get_layers().size() == 1);
	CHECK(copy->get_layers()[0] != Variant(layer));
	REQUIRE(copy->make_transparent());
	CHECK_MESSAGE(layer->get_color() == Color(1, 0, 0), "The layers of the style box aren't changed.");
}

TEST_CASE("[StyleBoxLayered] Layers of different modes aren't interpolated") {
	Ref<StyleBoxLayered> from = make_layered({ make_layer(StyleBoxLayer::MODE_FILL, Color(1, 1, 1)) });
	Ref<StyleBoxLayered> to = make_layered({ make_layer(StyleBoxLayer::MODE_OUTLINE, Color(1, 1, 1)) });
	Ref<StyleBoxLayered> live = to->duplicate(true);
	CHECK_FALSE(live->interpolate(from, to, 0.5));

	// Nor layers drawn on top on one side only.
	Ref<StyleBoxLayer> on_top = make_layer(StyleBoxLayer::MODE_FILL, Color(1, 1, 1));
	on_top->set_draw_on_top(true);
	to = make_layered({ on_top });
	live = to->duplicate(true);
	CHECK_FALSE(live->interpolate(from, to, 0.5));
	to = make_layered({ on_top->duplicate() });
	live = to->duplicate(true);
	CHECK(live->interpolate(make_layered({ on_top }), to, 0.5));
	CHECK(Ref<StyleBoxLayer>(live->get_layers()[0])->is_draw_on_top_enabled());
}

TEST_CASE("[StyleBoxLayered] Layers on one side fade") {
	Ref<StyleBoxLayer> fill = make_layer(StyleBoxLayer::MODE_FILL, Color(1, 0, 0));
	Ref<StyleBoxLayer> outline = make_layer(StyleBoxLayer::MODE_OUTLINE, Color(0, 1, 0));
	outline->set_width(4);
	Ref<StyleBoxLayered> from = make_layered({ fill });
	Ref<StyleBoxLayered> to = make_layered({ fill, outline });

	Ref<StyleBoxLayered> live = to->duplicate(true);
	REQUIRE(live->interpolate(from, to, 0.5));
	REQUIRE(live->get_layers().size() == 2);
	const Ref<StyleBoxLayer> faded = live->get_layers()[1];
	CHECK_MESSAGE(faded->get_color().is_equal_approx(Color(0, 1, 0, 0.5)), "A layer only in the target fades in with its own color.");
	CHECK(faded->get_width() == doctest::Approx(4));
	CHECK_MESSAGE(outline->get_color().is_equal_approx(Color(0, 1, 0)), "The source layers aren't modified.");

	REQUIRE(live->interpolate(from, to, 0.75));
	CHECK_MESSAGE(live->get_layers()[1] == Variant(faded), "The layers are kept between frames.");

	REQUIRE(live->interpolate(to, from, 0.75));
	const Ref<StyleBoxLayer> fading = live->get_layers()[1];
	CHECK_MESSAGE(fading->get_color().is_equal_approx(Color(0, 1, 0, 0.25)), "A layer missing in the target fades out with its own color.");
}

TEST_CASE("[StyleBoxLayered] Transparent copies") {
	Ref<StyleBoxLayered> style = make_layered({ make_layer(StyleBoxLayer::MODE_FILL, Color(1, 0, 0)), make_layer(StyleBoxLayer::MODE_OUTLINE, Color(0, 1, 0)) });
	REQUIRE(style->make_transparent());
	for (const Variant &v : style->get_layers()) {
		const Ref<StyleBoxLayer> layer = v;
		CHECK(layer->get_color().a == 0);
	}
}

TEST_CASE("[StyleBoxLayered] Content margins") {
	Ref<StyleBoxLayer> outline = make_layer(StyleBoxLayer::MODE_OUTLINE, Color(1, 1, 1));
	outline->set_margin(SIDE_LEFT, 2);
	outline->set_width(3);
	Ref<StyleBoxLayered> style = make_layered({ make_layer(StyleBoxLayer::MODE_FILL, Color(1, 1, 1)), outline });
	CHECK_MESSAGE(style->get_margin(SIDE_LEFT) == doctest::Approx(5), "The content is placed inside the outlines.");
	CHECK(style->get_margin(SIDE_TOP) == doctest::Approx(3));

	style->set_content_offset(Vector2(1, 2));
	CHECK_MESSAGE(style->get_margin(SIDE_LEFT) == doctest::Approx(6), "The content offset moves the content.");
	CHECK(style->get_margin(SIDE_RIGHT) == doctest::Approx(2));
	CHECK(style->get_margin(SIDE_TOP) == doctest::Approx(5));
	CHECK(style->get_margin(SIDE_BOTTOM) == doctest::Approx(1));

	Ref<StyleBoxFlat> flat;
	flat.instantiate();
	flat->set_content_margin_all(10);
	style->set_content_margins_from(flat);
	CHECK_MESSAGE(style->get_margin(SIDE_LEFT) == doctest::Approx(10), "Copied margins place the content like in the other style box.");
	CHECK(style->get_margin(SIDE_BOTTOM) == doctest::Approx(10));
}

TEST_CASE("[StyleBoxLayered] Draw rect") {
	Ref<StyleBoxLayer> shadow = make_layer(StyleBoxLayer::MODE_SHADOW, Color(0, 0, 0));
	shadow->set_blur(4);
	Ref<StyleBoxLayered> style = make_layered({ shadow });
	CHECK_MESSAGE(style->get_draw_rect(Rect2(0, 0, 10, 10)).is_equal_approx(Rect2(-6, -6, 22, 22)), "Shadows grow by their blur, a pixel and the antialiasing.");

	style->set_layers({ make_layer(StyleBoxLayer::MODE_FILL, Color(1, 1, 1)) });
	style->set_skew(Vector2(0.5, 0));
	CHECK_MESSAGE(style->get_draw_rect(Rect2(0, 0, 10, 10)).is_equal_approx(Rect2(-3.5, -1, 17, 12)), "Skewed layers grow by their skewed corners.");
}

// The cells covered by the rectangles of a pixel layer, row by row.
static HashMap<int, Vector2i> get_pixel_rows(const Ref<StyleBoxLayered> &p_style, const Ref<StyleBoxLayer> &p_layer, const Rect2 &p_rect) {
	LocalVector<StyleBoxLayered::PixelRect> rects;
	p_style->get_pixel_rects(p_layer, p_rect, rects);
	HashMap<int, Vector2i> rows;
	for (const StyleBoxLayered::PixelRect &rect : rects) {
		for (int y = rect.rect.position.y; y < rect.rect.get_end().y; y++) {
			Vector2i *row = rows.getptr(y);
			const Vector2i span = Vector2i(rect.rect.position.x, rect.rect.get_end().x);
			if (row) {
				*row = Vector2i(MIN(row->x, span.x), MAX(row->y, span.y));
			} else {
				rows.insert(y, span);
			}
		}
	}
	return rows;
}

TEST_CASE("[StyleBoxLayered] Pixel corners") {
	Ref<StyleBoxLayer> fill = make_layer(StyleBoxLayer::MODE_FILL, Color(1, 1, 1));
	Ref<StyleBoxLayered> style = make_layered({ fill });
	style->set_style(StyleBoxLayered::STYLE_PIXEL);
	style->set_corner_radius(CORNER_TOP_LEFT, 2);
	style->set_corner_radius(CORNER_TOP_RIGHT, 2);
	style->set_corner_radius(CORNER_BOTTOM_RIGHT, 2);
	style->set_corner_radius(CORNER_BOTTOM_LEFT, 2);

	HashMap<int, Vector2i> rows = get_pixel_rows(style, fill, Rect2(0, 0, 8, 8));
	REQUIRE(rows.size() == 8);
	CHECK_MESSAGE(rows[0] == Vector2i(2, 6), "The corners are cut in steps of a pixel.");
	CHECK(rows[1] == Vector2i(1, 7));
	CHECK(rows[3] == Vector2i(0, 8));
	CHECK(rows[6] == Vector2i(1, 7));
	CHECK(rows[7] == Vector2i(2, 6));

	style->set_corner_radius(CORNER_TOP_LEFT, 0);
	style->set_corner_radius(CORNER_TOP_RIGHT, 0);
	style->set_corner_radius(CORNER_BOTTOM_RIGHT, 0);
	style->set_corner_radius(CORNER_BOTTOM_LEFT, 0);
	style->set_skew(Vector2(1, 0));
	rows = get_pixel_rows(style, fill, Rect2(0, 0, 4, 2));
	REQUIRE(rows.size() == 2);
	CHECK_MESSAGE(rows[0] == Vector2i(0, 5), "Skewed rows move sideways in whole pixels.");
	CHECK(rows[1] == Vector2i(-1, 4));
}

// Drawing reuses the rectangles of a size at other positions with the same fraction of their position.
TEST_CASE("[StyleBoxLayered] Pixel rectangles move with the position") {
	Ref<StyleBoxLayer> fill = make_layer(StyleBoxLayer::MODE_FILL, Color(1, 1, 1));
	fill->set_blend(true);
	Ref<StyleBoxLayered> style = make_layered({ fill });
	style->set_style(StyleBoxLayered::STYLE_PIXEL);
	style->set_pixel_size(2);
	style->set_skew(Vector2(0.3, 0.2));
	for (int i = 0; i < 4; i++) {
		style->set_corner_radius(Corner(i), 5);
	}

	for (const bool snap : { false, true }) {
		style->set_snap(snap);
		for (const Rect2 &rect : { Rect2(10.25, 3.4, 37, 21), Rect2(10.5, 3.5, 37.3, 21.6) }) {
			for (const Vector2 &offset : { Vector2(1, 0), Vector2(-40, 17), Vector2(300, -250) }) {
				LocalVector<StyleBoxLayered::PixelRect> at_rect;
				style->get_pixel_rects(fill, rect, at_rect);
				LocalVector<StyleBoxLayered::PixelRect> moved;
				style->get_pixel_rects(fill, Rect2(rect.position + offset, rect.size), moved);
				REQUIRE(at_rect.size() == moved.size());
				bool same = true;
				for (uint32_t i = 0; i < moved.size(); i++) {
					same = same && moved[i].rect.position.is_equal_approx(at_rect[i].rect.position + offset) && moved[i].rect.size.is_equal_approx(at_rect[i].rect.size) && Math::is_equal_approx(moved[i].alpha, at_rect[i].alpha);
				}
				CHECK_MESSAGE(same, vformat("%s moved by %s, with snap %s.", rect, offset, snap));
			}
		}
	}
}

} // namespace TestStyleBoxTransitions
