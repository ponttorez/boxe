/**************************************************************************/
/*  style_transitions.cpp                                                 */
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

#include "style_transitions.h"

#include "core/config/engine.h"
#include "core/object/callable_mp.h"
#include "scene/main/canvas_item.h"
#include "scene/main/scene_tree.h"

bool StyleTransitions::_is_enabled() {
	// The edited scene isn't animated, so the editor always shows the final state.
	return !Engine::get_singleton()->is_editor_hint();
}

double StyleTransitions::_get_time() {
	static uint64_t frame = UINT64_MAX;
	static double time = 0.0;
	const uint64_t current_frame = Engine::get_singleton()->get_process_frames();
	if (current_frame != frame) {
		frame = current_frame;
		time += Engine::get_singleton()->get_process_step();
	}
	return time;
}

real_t StyleTransitions::_get_curve_time(double p_start_time, double p_duration, real_t p_start_curve_time) {
	if (p_duration <= 0.0) {
		return 1.0;
	}
	return MIN(1.0, p_start_curve_time + (_get_time() - p_start_time) / p_duration);
}

real_t StyleTransitions::_get_weight(const Ref<StyleBox> &p_timing, real_t p_curve_time, real_t p_start_curve_time) {
	const real_t start = p_timing->ease_transition(p_start_curve_time);
	if (1.0 - start < CMP_EPSILON) {
		return 1.0;
	}
	return (p_timing->ease_transition(p_curve_time) - start) / (1.0 - start);
}

real_t StyleTransitions::_find_curve_time(const Ref<StyleBox> &p_timing, real_t p_weight) {
	// Curves going past the target, like Elastic and Back, give one of the times reaching the weight.
	real_t low = 0.0;
	real_t high = 1.0;
	for (int i = 0; i < 20; i++) {
		const real_t middle = (low + high) * 0.5;
		if (p_timing->ease_transition(middle) < p_weight) {
			low = middle;
		} else {
			high = middle;
		}
	}
	return (low + high) * 0.5;
}

void StyleTransitions::_queue_redraw() {
	if (!owner->is_inside_tree()) {
		return;
	}

	SceneTree *tree = owner->get_tree();
	const Callable callable = callable_mp(owner, &CanvasItem::queue_redraw);
	if (!tree->is_connected(SNAME("process_frame"), callable)) {
		tree->connect(SNAME("process_frame"), callable, Object::CONNECT_ONE_SHOT);
	}
}

void StyleTransitions::_prune(uint64_t p_frame) {
	// Once per frame, as controls drawing many parts get their styles many times per frame.
	if (p_frame == pruned_frame) {
		return;
	}
	pruned_frame = p_frame;

	// Parts that weren't drawn for a while, e.g. removed tabs or items, are forgotten.
	static constexpr uint64_t MAX_UNUSED_FRAMES = 60;
	if (styles.size() > 32) {
		LocalVector<uint64_t> unused;
		for (const KeyValue<uint64_t, StyleTrack> &E : styles) {
			if (p_frame - E.value.last_used_frame > MAX_UNUSED_FRAMES) {
				unused.push_back(E.key);
			}
		}
		for (uint64_t key : unused) {
			styles.erase(key);
		}
	}
	if (colors.size() > 32) {
		LocalVector<uint64_t> unused;
		for (const KeyValue<uint64_t, ColorTrack> &E : colors) {
			if (p_frame - E.value.last_used_frame > MAX_UNUSED_FRAMES) {
				unused.push_back(E.key);
			}
		}
		for (uint64_t key : unused) {
			colors.erase(key);
		}
	}
}

Ref<StyleBox> StyleTransitions::get_style(uint64_t p_key, const Ref<StyleBox> &p_target) {
	const uint64_t frame = Engine::get_singleton()->get_process_frames();
	_prune(frame);

	StyleTrack *track = styles.getptr(p_key);
	if (!track) {
		StyleTrack new_track;
		new_track.target = p_target;
		new_track.last_used_frame = frame;
		styles.insert(p_key, new_track);
		return p_target;
	}
	track->last_used_frame = frame;

	if (p_target != track->target) {
		const Ref<StyleBox> previous = track->target;
		// How far the interrupted transition went from the state where its curve starts.
		real_t previous_position = 1.0;
		if (track->active) {
			previous_position = track->timing->ease_transition(_get_curve_time(track->start_time, track->duration, track->start_curve_time));
		}
		const Ref<StyleBox> current = track->active ? track->live : previous;

		const bool reversing = track->active && p_target == track->origin;
		track->origin = previous;
		track->target = p_target;
		track->active = false;
		track->from.unref();
		track->to.unref();
		track->live.unref();

		// The transition into a state is timed by its style box, and fading out by the style box that disappears.
		const Ref<StyleBox> timing = p_target.is_valid() ? p_target : previous;
		if (!_is_enabled() || timing.is_null() || timing->get_transition_duration() <= 0.0 || (current.is_null() && p_target.is_null())) {
			return p_target;
		}

		Ref<StyleBox> from;
		Ref<StyleBox> to;
		if (current.is_null()) {
			from = p_target->duplicate(true);
			if (!from->make_transparent()) {
				return p_target;
			}
			to = p_target;
		} else {
			from = current->duplicate(true);
			if (p_target.is_valid()) {
				to = p_target;
			} else {
				to = current->duplicate(true);
				if (!to->make_transparent()) {
					return p_target;
				}
			}
		}

		Ref<StyleBox> live = to->duplicate(true);
		if (!live->interpolate(from, to, 0.0)) {
			return p_target;
		}

		track->from = from;
		track->to = to;
		track->live = live;
		track->timing = timing;
		// Going back before the transition ends continues on the curve from the same point,
		// so it takes as long as the part already travelled, without stopping or jumping.
		track->start_curve_time = reversing ? _find_curve_time(timing, 1.0 - previous_position) : 0.0;
		track->duration = timing->get_transition_duration();
		track->start_time = _get_time();
		track->active = track->duration > 0.0 && track->start_curve_time < 1.0;
	}

	if (!track->active) {
		return p_target;
	}

	const real_t curve_time = _get_curve_time(track->start_time, track->duration, track->start_curve_time);
	if (curve_time >= 1.0) {
		track->active = false;
		track->from.unref();
		track->to.unref();
		track->live.unref();
		return p_target;
	}

	track->live->interpolate(track->from, track->to, _get_weight(track->timing, curve_time, track->start_curve_time));
	_queue_redraw();
	return track->live;
}

Color StyleTransitions::get_color(uint64_t p_key, const Color &p_target, const Ref<StyleBox> &p_timing) {
	const uint64_t frame = Engine::get_singleton()->get_process_frames();
	_prune(frame);

	ColorTrack *track = colors.getptr(p_key);
	if (!track) {
		ColorTrack new_track;
		new_track.target = p_target;
		new_track.last_used_frame = frame;
		colors.insert(p_key, new_track);
		return p_target;
	}
	track->last_used_frame = frame;

	if (p_target != track->target) {
		real_t previous_position = 1.0;
		Color current = track->target;
		if (track->active) {
			const real_t curve_time = _get_curve_time(track->start_time, track->duration, track->start_curve_time);
			previous_position = track->timing->ease_transition(curve_time);
			current = track->from.lerp(track->target, CLAMP(_get_weight(track->timing, curve_time, track->start_curve_time), 0, 1));
		}

		const bool reversing = track->active && p_target == track->origin;
		track->origin = track->target;
		track->target = p_target;
		track->active = false;

		if (!_is_enabled() || p_timing.is_null() || p_timing->get_transition_duration() <= 0.0) {
			return p_target;
		}

		track->from = current;
		track->timing = p_timing;
		track->start_curve_time = reversing ? _find_curve_time(p_timing, 1.0 - previous_position) : 0.0;
		track->duration = p_timing->get_transition_duration();
		track->start_time = _get_time();
		track->active = track->duration > 0.0 && track->start_curve_time < 1.0;
	}

	if (!track->active) {
		return p_target;
	}

	const real_t curve_time = _get_curve_time(track->start_time, track->duration, track->start_curve_time);
	if (curve_time >= 1.0) {
		track->active = false;
		return p_target;
	}

	_queue_redraw();
	// Curves like Elastic and Back overshoot the target, which would change the hue of colors.
	return track->from.lerp(p_target, CLAMP(_get_weight(track->timing, curve_time, track->start_curve_time), 0, 1));
}

bool StyleTransitions::is_animating() const {
	for (const KeyValue<uint64_t, StyleTrack> &E : styles) {
		if (E.value.active) {
			return true;
		}
	}
	for (const KeyValue<uint64_t, ColorTrack> &E : colors) {
		if (E.value.active) {
			return true;
		}
	}
	return false;
}

void StyleTransitions::clear() {
	styles.clear();
	colors.clear();
}

StyleTransitions::StyleTransitions(CanvasItem *p_owner) {
	owner = p_owner;
}
