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

#include "core/config/engine.h"
#include "core/object/object.h"
#include "core/object/object_id.h"
#include "scene/main/node.h"
#include "servers/audio/audio_server.h"

namespace {

// The sound started in the current frame.
uint64_t last_frame = UINT64_MAX;
ThemeSound::Priority last_priority = ThemeSound::PRIORITY_HOVER;
ObjectID last_playback;

} // namespace

void ThemeSound::play(const Ref<AudioStream> &p_sound, const Node *p_context, Priority p_priority) {
	// The edited scene doesn't play theme sounds.
	if (p_sound.is_null() || p_context == nullptr || Engine::get_singleton()->is_editor_hint() || !AudioServer::get_singleton()) {
		return;
	}

	const uint64_t frame = Engine::get_singleton()->get_process_frames();
	if (frame == last_frame) {
		if (p_priority <= last_priority) {
			return;
		}
		// A sound of a higher priority replaces the one already started for the same action.
		AudioStreamPlayback *previous = ObjectDB::get_instance<AudioStreamPlayback>(last_playback);
		if (previous) {
			AudioServer::get_singleton()->stop_playback_stream(Ref<AudioStreamPlayback>(previous));
		}
	}

	Ref<AudioStreamPlayback> playback = p_sound->instantiate_playback();
	if (playback.is_null()) {
		return;
	}

	// Front speakers at full volume. The AudioServer frees the playback when it ends.
	Vector<AudioFrame> volume_vector;
	volume_vector.resize(4);
	for (AudioFrame &channel_volume : volume_vector) {
		channel_volume = AudioFrame(0, 0);
	}
	volume_vector.write[0] = AudioFrame(1, 1);
	AudioServer::get_singleton()->start_playback_stream(playback, p_context->get_sound_bus_in_tree(), volume_vector);

	last_frame = frame;
	last_priority = p_priority;
	last_playback = playback->get_instance_id();
}
