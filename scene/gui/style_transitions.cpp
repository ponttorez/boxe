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

bool StyleTransitions::draws_nothing(const Ref<StyleBox> &p_style_box) {
	return p_style_box.is_null() || Object::cast_to<StyleBoxEmpty>(p_style_box.ptr());
}

bool StyleTransitions::_is_enabled() const {
	// The edited scene isn't animated, so the editor always shows its final state.
	// The other controls of the editor are, e.g. the preview of the theme editor.
	return !owner->is_part_of_edited_scene();
}

double StyleTransitions::_get_time() {
	const uint64_t current_frame = Engine::get_singleton()->get_process_frames();
	if (current_frame != time_frame) {
		time_frame = current_frame;
		time += Engine::get_singleton()->get_process_step();
	}
	return time;
}

void StyleTransitions::skip_time(double p_seconds) {
	ERR_FAIL_COND(p_seconds < 0.0);
	time += p_seconds;
}

void StyleTransitions::_start_timing(TransitionTiming &r_timing, const Ref<StyleBox> &p_style_box, real_t p_covered) {
	r_timing.start(p_style_box->get_transition_duration(), p_style_box->get_transition_type(), p_style_box->get_transition_ease(), p_covered);
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
		LocalVector<Key> unused;
		for (const KeyValue<Key, StyleTrack> &E : styles) {
			if (p_frame - E.value.last_used_frame > MAX_UNUSED_FRAMES) {
				unused.push_back(E.key);
			}
		}
		for (const Key &key : unused) {
			styles.erase(key);
		}
	}
	if (colors.size() > 32) {
		LocalVector<Key> unused;
		for (const KeyValue<Key, ColorTrack> &E : colors) {
			if (p_frame - E.value.last_used_frame > MAX_UNUSED_FRAMES) {
				unused.push_back(E.key);
			}
		}
		for (const Key &key : unused) {
			colors.erase(key);
		}
	}
}

// Only a playing transition keeps the style boxes it goes between, so old themes and their textures aren't kept.
void StyleTransitions::_stop_style_track(StyleTrack &r_track) {
	r_track.active = false;
	r_track.origin.unref();
	r_track.from.unref();
	r_track.to.unref();
	r_track.live.unref();
}

Ref<StyleBox> StyleTransitions::get_style(const Key &p_key, const Ref<StyleBox> &p_target) {
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
			track->timing.set_elapsed(_get_time() - track->start_time);
			previous_position = track->timing.get_position();
		}
		const Ref<StyleBox> current = track->active ? track->live : previous;

		const bool reversing = track->active && p_target == track->origin;
		_stop_style_track(*track);
		track->target = p_target;

		// The transition into a state is timed by its style box, and fading out by the style box that disappears.
		const Ref<StyleBox> timing = draws_nothing(p_target) ? previous : p_target;
		if (!_is_enabled() || timing.is_null() || timing->get_transition_duration() <= 0.0 || (draws_nothing(current) && draws_nothing(p_target))) {
			return p_target;
		}

		Ref<StyleBox> from;
		Ref<StyleBox> to;
		if (draws_nothing(current)) {
			from = p_target->duplicate_for_transition();
			if (!from->make_transparent()) {
				return p_target;
			}
			if (current.is_valid()) {
				from->set_content_margins_from(current);
			}
			to = p_target;
		} else {
			from = current->duplicate_for_transition();
			if (!draws_nothing(p_target)) {
				to = p_target;
			} else {
				to = current->duplicate_for_transition();
				if (!to->make_transparent()) {
					return p_target;
				}
				if (p_target.is_valid()) {
					to->set_content_margins_from(p_target);
				}
			}
		}

		Ref<StyleBox> live = to->duplicate_for_transition();
		if (!live->interpolate(from, to, 0.0)) {
			return p_target;
		}

		track->origin = previous;
		track->from = from;
		track->to = to;
		track->live = live;
		// Going back before the transition ends continues on the curve from the same point,
		// so it takes as long as the part already travelled, without stopping or jumping.
		_start_timing(track->timing, timing, reversing ? 1.0 - previous_position : 0.0);
		track->start_time = _get_time();
		track->active = track->timing.is_playing();
		if (!track->active) {
			_stop_style_track(*track);
		}
	}

	if (!track->active) {
		return p_target;
	}

	track->timing.set_elapsed(_get_time() - track->start_time);
	if (!track->timing.is_playing()) {
		_stop_style_track(*track);
		return p_target;
	}

	track->live->interpolate(track->from, track->to, track->timing.get_weight());
	_queue_redraw();
	return track->live;
}

Color StyleTransitions::get_color(const Key &p_key, const Color &p_target, const Ref<StyleBox> &p_timing) {
	const uint64_t frame = Engine::get_singleton()->get_process_frames();
	_prune(frame);

	ColorTrack *track = colors.getptr(p_key);
	if (!track) {
		ColorTrack new_track;
		new_track.target = p_target;
		new_track.target_timing = p_timing;
		new_track.last_used_frame = frame;
		colors.insert(p_key, new_track);
		return p_target;
	}
	track->last_used_frame = frame;

	if (p_target == track->target) {
		track->target_timing = p_timing;
	} else {
		Color current = track->target;
		if (track->active) {
			track->timing.set_elapsed(_get_time() - track->start_time);
			current = track->from.lerp(track->target, CLAMP(track->timing.get_weight(), 0, 1));
		}

		// Continues on the curve from the point that has covered the part of the way from the previous target already
		// travelled, e.g. back to the color of the previous state, or on to a closer one.
		real_t covered = 0.0;
		if (track->active) {
			const real_t origin[4] = { track->target.r, track->target.g, track->target.b, track->target.a };
			const real_t at[4] = { current.r, current.g, current.b, current.a };
			const real_t to[4] = { p_target.r, p_target.g, p_target.b, p_target.a };
			covered = TransitionTiming::get_covered(origin, at, to, 4);
		}
		track->origin = track->target;
		track->target = p_target;
		track->active = false;

		// Timed like the style box of the state, which fades out with the style box that disappears when it draws nothing.
		const Ref<StyleBox> timing = draws_nothing(p_timing) ? track->target_timing : p_timing;
		track->target_timing = p_timing;
		if (!_is_enabled() || timing.is_null() || timing->get_transition_duration() <= 0.0) {
			return p_target;
		}

		track->from = current;
		_start_timing(track->timing, timing, covered);
		track->start_time = _get_time();
		track->active = track->timing.is_playing();
	}

	if (!track->active) {
		return p_target;
	}

	track->timing.set_elapsed(_get_time() - track->start_time);
	if (!track->timing.is_playing()) {
		track->active = false;
		return p_target;
	}

	_queue_redraw();
	// Curves like Elastic and Back overshoot the target, which would change the hue of colors.
	return track->from.lerp(p_target, CLAMP(track->timing.get_weight(), 0, 1));
}

void StyleTransitions::clear() {
	styles.clear();
	colors.clear();
}

StyleTransitions::StyleTransitions(CanvasItem *p_owner) {
	owner = p_owner;
}
