/**************************************************************************/
/*  localized_audio_stream.cpp                                            */
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

#include "localized_audio_stream.h"

#include "core/object/class_db.h"
#include "servers/audio/audio_server.h"

void LocalizedAudioStream::_locale_changed() {
	if (streams.update()) {
		_stream_changed();
	}
}

void LocalizedAudioStream::_stream_changed() {
	// A sample registered for sample playback, e.g. on the web, holds the audio of the previous stream.
	// It's registered again with the current stream at the next play.
	AudioServer *audio_server = AudioServer::get_singleton();
	if (audio_server && get_reference_count() > 0 && audio_server->is_stream_registered_as_sample(this)) {
		Ref<AudioSample> sample;
		sample.instantiate();
		sample->stream = this;
		audio_server->unregister_sample(sample);
	}
	emit_changed();
	emit_signal(SNAME("parameter_list_changed"));
}

void LocalizedAudioStream::set_default_stream(const Ref<AudioStream> &p_stream) {
	ERR_FAIL_COND_MSG(streams.makes_cycle(p_stream, this), "A LocalizedAudioStream can't contain itself.");
	streams.default_variant = p_stream;
	streams.update();
	_stream_changed();
}

Ref<AudioStream> LocalizedAudioStream::get_default_stream() const {
	return streams.default_variant;
}

void LocalizedAudioStream::set_streams(const Dictionary &p_streams) {
	streams.set_dictionary(p_streams, this);
	streams.update();
	_stream_changed();
}

bool LocalizedAudioStream::uses_resource(const Object *p_resource) const {
	return streams.uses_resource<LocalizedAudioStream>(p_resource);
}

Dictionary LocalizedAudioStream::get_streams() const {
	return streams.get_dictionary();
}

Ref<AudioStream> LocalizedAudioStream::get_current_stream() const {
	return streams.current;
}

// The stream of the current locale plays, and a playing stream keeps playing when the locale changes.
Ref<AudioStreamPlayback> LocalizedAudioStream::instantiate_playback() {
	return streams.current.is_valid() ? streams.current->instantiate_playback() : Ref<AudioStreamPlayback>();
}

String LocalizedAudioStream::get_stream_name() const {
	return streams.current.is_valid() ? streams.current->get_stream_name() : String();
}

double LocalizedAudioStream::get_length() const {
	return streams.current.is_valid() ? streams.current->get_length() : 0.0;
}

bool LocalizedAudioStream::is_monophonic() const {
	return streams.current.is_valid() ? streams.current->is_monophonic() : false;
}

double LocalizedAudioStream::get_bpm() const {
	return streams.current.is_valid() ? streams.current->get_bpm() : 0.0;
}

bool LocalizedAudioStream::has_loop() const {
	return streams.current.is_valid() ? streams.current->has_loop() : false;
}

int LocalizedAudioStream::get_bar_beats() const {
	return streams.current.is_valid() ? streams.current->get_bar_beats() : 4;
}

int LocalizedAudioStream::get_beat_count() const {
	return streams.current.is_valid() ? streams.current->get_beat_count() : 0;
}

Dictionary LocalizedAudioStream::get_tags() const {
	return streams.current.is_valid() ? streams.current->get_tags() : Dictionary();
}

void LocalizedAudioStream::get_parameter_list(List<Parameter> *r_parameters) {
	if (streams.current.is_valid()) {
		streams.current->get_parameter_list(r_parameters);
	}
}

bool LocalizedAudioStream::can_be_sampled() const {
	return streams.current.is_valid() && streams.current->can_be_sampled();
}

// The sample has the audio of the current stream, but belongs to this stream, which players register and play.
Ref<AudioSample> LocalizedAudioStream::generate_sample() const {
	if (streams.current.is_null()) {
		return Ref<AudioSample>();
	}
	Ref<AudioSample> sample = streams.current->generate_sample();
	if (sample.is_valid()) {
		sample->stream = this;
	}
	return sample;
}

void LocalizedAudioStream::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_default_stream", "stream"), &LocalizedAudioStream::set_default_stream);
	ClassDB::bind_method(D_METHOD("get_default_stream"), &LocalizedAudioStream::get_default_stream);
	ClassDB::bind_method(D_METHOD("set_streams", "streams"), &LocalizedAudioStream::set_streams);
	ClassDB::bind_method(D_METHOD("get_streams"), &LocalizedAudioStream::get_streams);
	ClassDB::bind_method(D_METHOD("get_current_stream"), &LocalizedAudioStream::get_current_stream);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_stream", PROPERTY_HINT_RESOURCE_TYPE, AudioStream::get_class_static()), "set_default_stream", "get_default_stream");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "streams", PROPERTY_HINT_DICTIONARY_TYPE, vformat("%d/%d:;%s", Variant::STRING, PROPERTY_HINT_LOCALE_ID, AudioStream::get_class_static())), "set_streams", "get_streams");
}

LocalizedAudioStream::LocalizedAudioStream() {
	streams.setup(callable_mp(this, &LocalizedAudioStream::_locale_changed), callable_mp(this, &LocalizedAudioStream::_stream_changed));
}
