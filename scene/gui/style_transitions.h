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

// Animates the state changes of theme style boxes and colors, using the transition settings of the target style box.
// Each animated part of a control uses its own key, e.g. the background of a button or the background of a tab.
class StyleTransitions {
public:
	// The animated parts of controls. A control uses each part once, repeated parts like tabs are told apart by their indices.
	enum Part : uint32_t {
		PART_BACKGROUND,
		PART_FOCUS,
		PART_FONT_COLOR,
		PART_ICON_COLOR,
		PART_ARROW_COLOR,
		PART_CURSOR,
		PART_TITLEBAR,
		PART_GRABBER,
		PART_GRABBER_AREA,
		PART_TRACK,
		PART_TIP,
		PART_UP_BUTTON,
		PART_DOWN_BUTTON,
		PART_UP_ICON_COLOR,
		PART_DOWN_ICON_COLOR,
		PART_ITEM,
		PART_ITEM_FONT_COLOR,
		PART_ITEM_ICON_COLOR,
		PART_ITEM_FOCUS,
		PART_CELL,
		PART_CELL_BUTTON,
		PART_CELL_BUTTON_COLOR,
		PART_BUTTON,
		PART_HEADER,
		PART_CHECK,
		PART_ITEM_CHECK,
		// Parts animated by scripts, see Control.get_transition_style().
		PART_SCRIPT,
	};

	// Identifies an animated part. The indices tell apart repeated parts, e.g. the item and the column of a tree cell.
	struct Key {
		Part part = PART_BACKGROUND;
		uint64_t index = 0;
		uint64_t sub_index = 0;

		bool operator==(const Key &p_other) const { return part == p_other.part && index == p_other.index && sub_index == p_other.sub_index; }
		uint32_t hash() const { return hash_murmur3_one_64(sub_index, hash_murmur3_one_64(index, hash_murmur3_one_32(part))); }

		Key() = default;
		Key(Part p_part, uint64_t p_index = 0, uint64_t p_sub_index = 0) :
				part(p_part), index(p_index), sub_index(p_sub_index) {}
	};

private:
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
		// The style box of the state of the target color.
		Ref<StyleBox> target_timing;
		Color origin;
		Color from;
		Ref<StyleBox> timing;
		double start_time = 0.0;
		double duration = 0.0;
		real_t start_curve_time = 0.0;
		bool active = false;
		uint64_t last_used_frame = 0;
	};

	CanvasItem *owner = nullptr;
	HashMap<Key, StyleTrack> styles;
	HashMap<Key, ColorTrack> colors;
	uint64_t pruned_frame = UINT64_MAX;

	static inline uint64_t time_frame = UINT64_MAX;
	static inline double time = 0.0;

	bool _is_enabled() const;
	// Seconds advanced by the unscaled process step of each frame, so transitions keep their speed when the time scale changes and in Movie Maker mode.
	static double _get_time();
	static real_t _get_curve_time(double p_start_time, double p_duration, real_t p_start_curve_time);
	// The weight between the state where the transition started and its target.
	static real_t _get_weight(const Ref<StyleBox> &p_timing, real_t p_curve_time, real_t p_start_curve_time);
	static real_t _find_curve_time(const Ref<StyleBox> &p_timing, real_t p_weight);
	void _queue_redraw();
	static void _stop_style_track(StyleTrack &r_track);
	void _prune(uint64_t p_frame);

public:
	// Returns the style box to draw for the target style box of the current state.
	// The target can be null when nothing is drawn in a state, then the previous style box fades out, and in when it comes back.
	Ref<StyleBox> get_style(const Key &p_key, const Ref<StyleBox> &p_target);
	// Returns the color to use for the target color of the current state, timed like the style box of that state.
	Color get_color(const Key &p_key, const Color &p_target, const Ref<StyleBox> &p_timing);

	void clear();

	// Whether the style box shows nothing: it's null or a StyleBoxEmpty. A StyleBoxEmpty fades like a missing style box, but keeps its content margins.
	static bool draws_nothing(const Ref<StyleBox> &p_style_box);

	// Moves all transitions forward in time without waiting for frames, e.g. in tests.
	static void skip_time(double p_seconds);

	StyleTransitions(CanvasItem *p_owner);
};
