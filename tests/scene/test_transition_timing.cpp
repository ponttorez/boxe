/**************************************************************************/
/*  test_transition_timing.cpp                                            */
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

TEST_FORCE_LINK(test_transition_timing)

#include "scene/animation/transition_timing.h"

namespace TestTransitionTiming {

TEST_CASE("[TransitionTiming] Curves and the time reaching a value") {
	TransitionTiming timing;
	timing.start(2.0, Tween::TRANS_LINEAR, Tween::EASE_IN_OUT);
	CHECK(timing.is_playing());
	timing.set_elapsed(0.5);
	CHECK(timing.get_weight() == doctest::Approx(0.25));
	timing.advance(0.5);
	CHECK(timing.get_weight() == doctest::Approx(0.5));
	timing.advance(10.0);
	CHECK_FALSE(timing.is_playing());
	CHECK(timing.get_weight() == doctest::Approx(1.0));

	for (const Tween::TransitionType transition : { Tween::TRANS_LINEAR, Tween::TRANS_CUBIC, Tween::TRANS_SINE, Tween::TRANS_EXPO }) {
		for (const Tween::EaseType ease : { Tween::EASE_IN, Tween::EASE_OUT, Tween::EASE_IN_OUT }) {
			const real_t time = TransitionTiming::find_curve_time(transition, ease, 0.3);
			CHECK(TransitionTiming::ease_curve(transition, ease, time) == doctest::Approx(0.3).epsilon(0.001));
		}
	}

	// Curves going past their end reach a value several times, the first time is found.
	const real_t time = TransitionTiming::find_curve_time(Tween::TRANS_BACK, Tween::EASE_OUT, 0.9);
	CHECK(TransitionTiming::ease_curve(Tween::TRANS_BACK, Tween::EASE_OUT, time) == doctest::Approx(0.9).epsilon(0.001));
	CHECK(time < 0.6);
}

TEST_CASE("[TransitionTiming] Continuing a transition covers what is left of the way") {
	const real_t origin[2] = { 0, 0 };
	const real_t current[2] = { 3, 4 };
	const real_t target[2] = { 10, 0 };
	CHECK(TransitionTiming::get_covered(origin, current, target, 2) == doctest::Approx(0.3));
	CHECK(TransitionTiming::get_covered(origin, origin, origin, 2) == doctest::Approx(0.0));

	// Going back halfway takes half the duration.
	TransitionTiming timing;
	timing.start(1.0, Tween::TRANS_LINEAR, Tween::EASE_IN_OUT, 0.5);
	CHECK(timing.get_weight() == doctest::Approx(0.0));
	timing.advance(0.25);
	CHECK(timing.get_weight() == doctest::Approx(0.5));
	timing.advance(0.25);
	CHECK_FALSE(timing.is_playing());

	// Nothing left to cover.
	timing.start(1.0, Tween::TRANS_LINEAR, Tween::EASE_IN_OUT, 1.0);
	CHECK_FALSE(timing.is_playing());
}

} // namespace TestTransitionTiming
