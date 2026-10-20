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

#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
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

} // namespace TestStyleBoxTransitions
