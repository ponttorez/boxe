/**************************************************************************/
/*  style_transitions.h                                                   */
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

#include "core/templates/hash_map.h"
#include "scene/resources/style_box.h"

class CanvasItem;
class Node;

// Animates the state changes of theme style boxes and colors, using the transition settings of the target style box.
// Each animated part of a control uses its own key, e.g. the background of a button or the background of a tab.
class StyleTransitions {
	struct StyleTrack {
		Ref<StyleBox> target;
		Ref<StyleBox> origin;
		Ref<StyleBox> from;
		// What the transition goes to: the target, or a transparent copy of the previous style when there is no target.
		Ref<StyleBox> to;
		// The style box whose transition settings are used.
		Ref<StyleBox> timing;
		Ref<StyleBox> live;
		double start_time = 0.0;
		double duration = 0.0;
		// Where the transition starts on its curve, after an interrupted transition in the other direction.
		real_t start_curve_time = 0.0;
		bool active = false;
		uint64_t last_used_frame = 0;
	};

	struct ColorTrack {
		Color target;
		Color origin;
		Color from;
		Ref<StyleBox> timing;
		double start_time = 0.0;
		double duration = 0.0;
		real_t start_curve_time = 0.0;
		bool active = false;
		uint64_t last_used_frame = 0;
	};

	Node *owner = nullptr;
	Callable redraw; // Called each frame while a transition plays.
	HashMap<uint64_t, StyleTrack> styles;
	HashMap<uint64_t, ColorTrack> colors;
	uint64_t pruned_frame = UINT64_MAX;

	static bool _is_enabled();
	// Seconds advanced by the unscaled process step of each frame, so transitions keep their speed when the time scale changes and in Movie Maker mode.
	static double _get_time();
	static real_t _get_curve_time(double p_start_time, double p_duration, real_t p_start_curve_time);
	// The weight between the state where the transition started and its target.
	static real_t _get_weight(const Ref<StyleBox> &p_timing, real_t p_curve_time, real_t p_start_curve_time);
	static real_t _find_curve_time(const Ref<StyleBox> &p_timing, real_t p_weight);
	void _queue_redraw();
	void _prune(uint64_t p_frame);

public:
	static uint64_t make_key(uint32_t p_part, uint32_t p_index = 0) { return (uint64_t(p_part) << 32) | p_index; }

	// Returns the style box to draw for the target style box of the current state.
	// The target can be null when nothing is drawn in a state, then the previous style box fades out, and in when it comes back.
	Ref<StyleBox> get_style(uint64_t p_key, const Ref<StyleBox> &p_target);
	// Returns the color to use for the target color of the current state, timed like p_timing.
	Color get_color(uint64_t p_key, const Color &p_target, const Ref<StyleBox> &p_timing);

	bool is_animating() const;
	void clear();

	StyleTransitions(CanvasItem *p_owner);
	StyleTransitions(Node *p_owner, const Callable &p_redraw);
};
