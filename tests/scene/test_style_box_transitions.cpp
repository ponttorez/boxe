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

#include "scene/resources/style_box_flat.h"
#include "scene/resources/style_box_layered.h"

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

} // namespace TestStyleBoxTransitions
