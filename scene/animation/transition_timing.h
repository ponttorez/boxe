/**************************************************************************/
/*  transition_timing.h                                                   */
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

#include "scene/animation/tween.h"

// The timing of a transition along an easing curve, for transitions that can change their target while they play.
// A transition going to another target continues on the curve from the point that has covered the part of the way
// already travelled, so it neither stops nor jumps, and going back takes as long as the part travelled.
// Used by the transitions of theme style boxes and by property animations.
struct TransitionTiming {
	Tween::TransitionType transition = Tween::TRANS_LINEAR;
	Tween::EaseType ease = Tween::EASE_IN_OUT;
	double duration = 0.0;
	// Where the transition started on its curve, and where it is now, from 0 to 1 at its end.
	real_t start_curve_time = 0.0;
	real_t curve_time = 1.0;

	static real_t ease_curve(Tween::TransitionType p_transition, Tween::EaseType p_ease, real_t p_curve_time);
	// The time on the curve where it reaches p_value. Curves going past their end, like Elastic and Back, give the
	// first time reaching it.
	static real_t find_curve_time(Tween::TransitionType p_transition, Tween::EaseType p_ease, real_t p_value);
	// How much of the way from p_origin, where the previous transition went, to p_target is covered at p_current, from
	// 0 to 1, by projecting p_current on that way.
	static real_t get_covered(const real_t *p_origin, const real_t *p_current, const real_t *p_target, int p_count);

	// Starts at the point of the curve that has covered p_covered of the way, 0 for the start of the curve.
	void start(double p_duration, Tween::TransitionType p_transition, Tween::EaseType p_ease, real_t p_covered = 0.0);
	// Sets the time since the start, or moves it forward.
	void set_elapsed(double p_seconds);
	void advance(double p_seconds);
	void finish() { curve_time = 1.0; }

	bool is_playing() const { return curve_time < 1.0; }
	// How far the transition is on its curve, from 0 at the start of the curve to 1 at its end.
	real_t get_position() const;
	// The weight between the value where the transition started and its target.
	real_t get_weight() const;
};
