/**************************************************************************/
/*  transition_timing.cpp                                                 */
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

#include "transition_timing.h"

real_t TransitionTiming::ease_curve(Tween::TransitionType p_transition, Tween::EaseType p_ease, real_t p_curve_time) {
	return Tween::run_equation(p_transition, p_ease, CLAMP(p_curve_time, 0, 1), 0, 1, 1);
}

real_t TransitionTiming::find_curve_time(Tween::TransitionType p_transition, Tween::EaseType p_ease, real_t p_value) {
	if (p_value <= 0.0) {
		return 0.0;
	}
	if (p_value >= 1.0) {
		return 1.0;
	}

	// Samples find the first part of the curve that reaches the value, as curves going past their end reach it several
	// times, then bisection finds the time in that part.
	static constexpr int SAMPLES = 64;
	static constexpr int ITERATIONS = 30;
	real_t low = 0.0;
	real_t low_delta = ease_curve(p_transition, p_ease, 0.0) - p_value;
	real_t best = 0.0;
	real_t best_delta = Math::abs(low_delta);
	for (int i = 1; i <= SAMPLES; i++) {
		const real_t high = real_t(i) / SAMPLES;
		const real_t high_delta = ease_curve(p_transition, p_ease, high) - p_value;
		if (Math::abs(high_delta) < best_delta) {
			best = high;
			best_delta = Math::abs(high_delta);
		}
		if ((low_delta <= 0.0) != (high_delta < 0.0) || high_delta == 0.0) {
			real_t from = low;
			real_t to = high;
			real_t from_delta = low_delta;
			for (int j = 0; j < ITERATIONS; j++) {
				const real_t middle = (from + to) * 0.5;
				const real_t middle_delta = ease_curve(p_transition, p_ease, middle) - p_value;
				if (middle_delta == 0.0) {
					return middle;
				}
				if ((middle_delta < 0.0) == (from_delta < 0.0)) {
					from = middle;
					from_delta = middle_delta;
				} else {
					to = middle;
				}
			}
			return (from + to) * 0.5;
		}
		low = high;
		low_delta = high_delta;
	}
	return best;
}

real_t TransitionTiming::get_covered(const real_t *p_origin, const real_t *p_current, const real_t *p_target, int p_count) {
	real_t dot = 0.0;
	real_t length_squared = 0.0;
	for (int i = 0; i < p_count; i++) {
		const real_t direction = p_target[i] - p_origin[i];
		dot += (p_current[i] - p_origin[i]) * direction;
		length_squared += direction * direction;
	}
	if (length_squared < CMP_EPSILON2) {
		return 0.0;
	}
	return CLAMP(dot / length_squared, 0, 1);
}

void TransitionTiming::start(double p_duration, Tween::TransitionType p_transition, Tween::EaseType p_ease, real_t p_covered) {
	transition = p_transition;
	ease = p_ease;
	duration = p_duration;
	start_curve_time = p_duration > 0.0 ? find_curve_time(p_transition, p_ease, p_covered) : 1.0;
	curve_time = start_curve_time;
	// A transition that has nothing left to cover ends at once.
	if (1.0 - ease_curve(transition, ease, start_curve_time) < CMP_EPSILON) {
		curve_time = 1.0;
	}
}

void TransitionTiming::set_elapsed(double p_seconds) {
	curve_time = duration > 0.0 ? MIN(1.0, start_curve_time + p_seconds / duration) : 1.0;
}

void TransitionTiming::advance(double p_seconds) {
	if (is_playing()) {
		curve_time = duration > 0.0 ? MIN(1.0, curve_time + p_seconds / duration) : 1.0;
	}
}

real_t TransitionTiming::get_position() const {
	return ease_curve(transition, ease, curve_time);
}

real_t TransitionTiming::get_weight() const {
	if (curve_time >= 1.0) {
		return 1.0;
	}
	const real_t start = ease_curve(transition, ease, start_curve_time);
	if (1.0 - start < CMP_EPSILON) {
		return 1.0;
	}
	return (get_position() - start) / (1.0 - start);
}
