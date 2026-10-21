/**************************************************************************/
/*  theme_sound.cpp                                                       */
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

#include "theme_sound.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "scene/gui/control.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/audio/audio_server.h"

namespace ThemeSound {

// Theme sounds play once, a sound that loops is stopped this long after its length.
static constexpr uint64_t LOOP_STOP_DELAY_USEC = 100000;

struct Voice {
	Ref<AudioStreamPlayback> playback; // Null when there's no audio server, e.g. in tests.
	uint64_t start_usec = 0;
	uint64_t length_usec = 0; // 0 when the length isn't known.
};

// The playing voices of each sound, by the ID of the sound.
static HashMap<ObjectID, LocalVector<Voice>> *voices = nullptr;
static uint64_t played_count = 0;
static ObjectID last_played;

// The same sound doesn't start again sooner than this, see the gui/theme/sound_min_interval_msec project setting.
static uint64_t _get_min_interval_usec() {
	return MAX(0, GLOBAL_GET_CACHED(int, "gui/theme/sound_min_interval_msec")) * 1000ULL;
}

// The most times the same sound plays at once, 0 for no limit, see the gui/theme/sound_max_voices project setting.
static int _get_max_voices() {
	return MAX(0, GLOBAL_GET_CACHED(int, "gui/theme/sound_max_voices"));
}

static bool _is_voice_playing(Voice &r_voice, uint64_t p_now) {
	const uint64_t elapsed = p_now - r_voice.start_usec;
	if (r_voice.playback.is_null() || !AudioServer::get_singleton()) {
		return elapsed < MAX(r_voice.length_usec, _get_min_interval_usec());
	}
	if (!AudioServer::get_singleton()->is_playback_active(r_voice.playback)) {
		return false;
	}
	if (r_voice.length_usec > 0 && elapsed >= r_voice.length_usec + LOOP_STOP_DELAY_USEC) {
		// A looping stream would play until the game ends.
		AudioServer::get_singleton()->stop_playback_stream(r_voice.playback);
		return false;
	}
	return true;
}

// Forgets the voices that ended, and stops the looping ones. Runs each frame while voices play.
static void _update_voices() {
	if (!voices) {
		return;
	}
	const uint64_t now = OS::get_singleton()->get_ticks_usec();
	LocalVector<ObjectID> ended;
	for (KeyValue<ObjectID, LocalVector<Voice>> &E : *voices) {
		LocalVector<Voice> &sound_voices = E.value;
		for (uint32_t i = 0; i < sound_voices.size();) {
			if (_is_voice_playing(sound_voices[i], now)) {
				i++;
			} else {
				sound_voices.remove_at_unordered(i);
			}
		}
		if (sound_voices.is_empty()) {
			ended.push_back(E.key);
		}
	}
	for (const ObjectID &id : ended) {
		voices->erase(id);
	}

	SceneTree *tree = SceneTree::get_singleton();
	const Callable update = callable_mp_static(&_update_voices);
	if (!voices->is_empty() && tree && !tree->is_connected(SNAME("process_frame"), update)) {
		tree->connect(SNAME("process_frame"), update, Object::CONNECT_ONE_SHOT);
	}
}

StringName get_bus(const Node *p_context) {
	for (const Node *node = p_context; node; node = node->get_parent()) {
		StringName bus;
		if (const Control *control = Object::cast_to<Control>(node)) {
			bus = control->get_theme_sound_bus();
		} else if (const Viewport *viewport = Object::cast_to<Viewport>(node)) {
			bus = viewport->get_theme_sound_bus();
		} else if (const CanvasLayer *layer = Object::cast_to<CanvasLayer>(node)) {
			bus = layer->get_theme_sound_bus();
		}
		if (!bus.is_empty()) {
			return bus;
		}
	}
	return GLOBAL_GET("gui/theme/sound_bus");
}

String get_bus_hint() {
	String hint;
	if (AudioServer::get_singleton()) {
		for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
			hint += (i > 0 ? "," : "") + AudioServer::get_singleton()->get_bus_name(i);
		}
	}
	return hint;
}

// Whether the sound can start now.
static bool _can_start(const Ref<AudioStream> &p_sound) {
	_update_voices();
	if (!voices) {
		return true;
	}
	const LocalVector<Voice> *sound_voices = voices->getptr(p_sound->get_instance_id());
	if (!sound_voices) {
		return true;
	}
	const uint64_t now = OS::get_singleton()->get_ticks_usec();
	const uint64_t min_interval = _get_min_interval_usec();
	for (const Voice &voice : *sound_voices) {
		if (now - voice.start_usec < min_interval) {
			return false;
		}
	}
	const int max_voices = _get_max_voices();
	return max_voices == 0 || (int)sound_voices->size() < max_voices;
}

static void _add_voice(const Ref<AudioStream> &p_sound, const Ref<AudioStreamPlayback> &p_playback) {
	if (!voices) {
		voices = memnew((HashMap<ObjectID, LocalVector<Voice>>));
	}
	Voice voice;
	voice.playback = p_playback;
	voice.start_usec = OS::get_singleton()->get_ticks_usec();
	voice.length_usec = MAX(0.0, p_sound->get_length()) * 1000000.0;
	(*voices)[p_sound->get_instance_id()].push_back(voice);
	_update_voices();
}

int get_voice_count(const Ref<AudioStream> &p_sound) {
	_update_voices();
	const LocalVector<Voice> *sound_voices = (voices && p_sound.is_valid()) ? voices->getptr(p_sound->get_instance_id()) : nullptr;
	return sound_voices ? sound_voices->size() : 0;
}

bool play(const Ref<AudioStream> &p_sound, const Node *p_context) {
	// The edited scene doesn't play theme sounds, the other controls of the editor do, e.g. the preview of the theme editor.
	if (p_sound.is_null() || p_context == nullptr || p_context->is_part_of_edited_scene()) {
		return false;
	}
	if (!_can_start(p_sound)) {
		return false;
	}
	played_count++;
	last_played = p_sound->get_instance_id();

	Ref<AudioStreamPlayback> playback = AudioServer::get_singleton() ? p_sound->instantiate_playback() : Ref<AudioStreamPlayback>();
	if (playback.is_valid()) {
		// Front speakers at full volume. The AudioServer frees the playback when it ends.
		Vector<AudioFrame> volume_vector;
		volume_vector.resize(4);
		for (AudioFrame &channel_volume : volume_vector) {
			channel_volume = AudioFrame(0, 0);
		}
		volume_vector.write[0] = AudioFrame(1, 1);
		AudioServer::get_singleton()->start_playback_stream(playback, get_bus(p_context), volume_vector);
	}
	// Added once it plays, as the voices that don't play are forgotten.
	_add_voice(p_sound, playback);
	return true;
}

bool play_hover(const Ref<AudioStream> &p_sound, const Node *p_context) {
	if (p_context && p_context->get_viewport() && p_context->get_viewport()->gui_is_mouse_emulated_from_touch()) {
		return false;
	}
	return play(p_sound, p_context);
}

uint64_t get_played_count() {
	return played_count;
}

ObjectID get_last_played() {
	return last_played;
}

void clear() {
	if (voices) {
		memdelete(voices);
		voices = nullptr;
	}
}

} // namespace ThemeSound
