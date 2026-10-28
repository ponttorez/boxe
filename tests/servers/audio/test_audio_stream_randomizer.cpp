/**************************************************************************/
/*  test_audio_stream_randomizer.cpp                                      */
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

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_audio_stream_randomizer)

#include "servers/audio/audio_stream.h"

namespace TestAudioStreamRandomizer {

// Plays full volume frames, and keeps the pitch it's played with.
class PitchPlayback : public AudioStreamPlayback {
	GDCLASS(PitchPlayback, AudioStreamPlayback);

public:
	float pitch_scale = 0.0;

	virtual void start(double p_from_pos = 0.0) override {}
	virtual void stop() override {}
	virtual bool is_playing() const override { return true; }
	virtual int mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) override {
		pitch_scale = p_rate_scale;
		for (int i = 0; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(1, 1);
		}
		return p_frames;
	}
};

class PitchStream : public AudioStream {
	GDCLASS(PitchStream, AudioStream);

public:
	Ref<PitchPlayback> playback;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override {
		playback.instantiate();
		return playback;
	}
	virtual String get_stream_name() const override { return "PitchStream"; }
};

TEST_CASE("[AudioStreamRandomizer] Random pitch and volume between a minimum and a maximum") {
	Ref<PitchStream> stream;
	stream.instantiate();
	Ref<AudioStreamRandomizer> randomizer;
	randomizer.instantiate();
	randomizer->add_stream(-1, stream);
	randomizer->set_random_pitch_semitones_min(-2);
	randomizer->set_random_pitch_semitones_max(5);
	randomizer->set_random_volume_offset_db_min(-6);
	randomizer->set_random_volume_offset_db_max(0);

	float lowest_pitch = 100;
	float highest_pitch = -100;
	float lowest_volume = 100;
	float highest_volume = -100;
	AudioFrame frames[4];
	for (int i = 0; i < 100; i++) {
		const Ref<AudioStreamPlayback> playback = randomizer->instantiate_playback();
		playback->start();
		playback->mix(frames, 1.0, 4);
		const float semitones = 12.0f * Math::log2(stream->playback->pitch_scale);
		const float volume_db = Math::linear_to_db(frames[0].left);
		lowest_pitch = MIN(lowest_pitch, semitones);
		highest_pitch = MAX(highest_pitch, semitones);
		lowest_volume = MIN(lowest_volume, volume_db);
		highest_volume = MAX(highest_volume, volume_db);
	}
	CHECK(lowest_pitch >= -2.001);
	CHECK(highest_pitch <= 5.001);
	CHECK_MESSAGE(lowest_pitch < 0, "Lower pitches are picked.");
	CHECK_MESSAGE(highest_pitch > 2.5, "Higher pitches are picked, also beyond the lowest distance below.");
	CHECK(lowest_volume >= -6.001);
	CHECK_MESSAGE(highest_volume <= 0.001, "Only quieter playbacks with a maximum of 0 dB.");
	CHECK(lowest_volume < -3);
}

TEST_CASE("[AudioStreamRandomizer] No variation by default") {
	Ref<PitchStream> stream;
	stream.instantiate();
	Ref<AudioStreamRandomizer> randomizer;
	randomizer.instantiate();
	randomizer->add_stream(-1, stream);

	AudioFrame frames[4];
	const Ref<AudioStreamPlayback> playback = randomizer->instantiate_playback();
	playback->start();
	playback->mix(frames, 1.0, 4);
	CHECK(stream->playback->pitch_scale == doctest::Approx(1.0));
	CHECK(frames[0].left == doctest::Approx(1.0));
}

#ifndef DISABLE_DEPRECATED
TEST_CASE("[AudioStreamRandomizer] Older properties set a range around zero") {
	Ref<AudioStreamRandomizer> randomizer;
	randomizer.instantiate();

	// As in resources saved by older versions.
	randomizer->set("random_pitch", 2.0);
	CHECK(randomizer->get_random_pitch_semitones_min() == doctest::Approx(-12.0));
	CHECK(randomizer->get_random_pitch_semitones_max() == doctest::Approx(12.0));
	randomizer->set("random_volume_offset_db", 3.0);
	CHECK(randomizer->get_random_volume_offset_db_min() == doctest::Approx(-3.0));
	CHECK(randomizer->get_random_volume_offset_db_max() == doctest::Approx(3.0));

	randomizer->set_random_pitch_semitones(4);
	CHECK(randomizer->get_random_pitch_semitones_min() == doctest::Approx(-4.0));
	CHECK(randomizer->get_random_pitch_semitones_max() == doctest::Approx(4.0));

	// The largest distance from zero.
	randomizer->set_random_pitch_semitones_min(-2);
	randomizer->set_random_pitch_semitones_max(5);
	CHECK(randomizer->get_random_pitch_semitones() == doctest::Approx(5.0));
	CHECK(randomizer->get_random_pitch() == doctest::Approx(Math::pow(2.0, 5.0 / 12.0)));
}
#endif // DISABLE_DEPRECATED

} // namespace TestAudioStreamRandomizer
