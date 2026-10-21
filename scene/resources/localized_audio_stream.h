/**************************************************************************/
/*  localized_audio_stream.h                                              */
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

#include "scene/resources/localized_variants.h"
#include "servers/audio/audio_stream.h"

// An audio stream with a stream per locale, which plays the one of the current locale.
class LocalizedAudioStream : public AudioStream {
	GDCLASS(LocalizedAudioStream, AudioStream);

	LocalizedVariants<AudioStream> streams;

	void _locale_changed();
	void _stream_changed();

protected:
	static void _bind_methods();

public:
	void set_default_stream(const Ref<AudioStream> &p_stream);
	Ref<AudioStream> get_default_stream() const;
	void set_streams(const Dictionary &p_streams);
	// Used to prevent cycles of localized resources.
	bool uses_resource(const Object *p_resource) const;
	Dictionary get_streams() const;
	Ref<AudioStream> get_current_stream() const;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual String get_stream_name() const override;
	virtual double get_length() const override;
	virtual bool is_monophonic() const override;
	virtual double get_bpm() const override;
	virtual bool has_loop() const override;
	virtual int get_bar_beats() const override;
	virtual int get_beat_count() const override;
	virtual Dictionary get_tags() const override;
	virtual void get_parameter_list(List<Parameter> *r_parameters) override;
	virtual bool can_be_sampled() const override;
	virtual Ref<AudioSample> generate_sample() const override;
	virtual bool is_meta_stream() const override { return true; }

	LocalizedAudioStream();
};
