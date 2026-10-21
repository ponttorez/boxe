/**************************************************************************/
/*  test_theme_sound.cpp                                                  */
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

TEST_FORCE_LINK(test_theme_sound)

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"
#include "scene/gui/theme_sound.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/audio_stream_wav.h"
#include "servers/audio/audio_server.h"

namespace TestThemeSound {

static Ref<AudioStream> make_sound() {
	Ref<AudioStreamWAV> sound;
	sound.instantiate();
	return sound;
}

TEST_CASE("[SceneTree][ThemeSound] The bus is taken from the closest control, layer or viewport") {
	Window *root = SceneTree::get_singleton()->get_root();
	CanvasLayer *layer = memnew(CanvasLayer);
	root->add_child(layer);
	Node2D *node_2d = memnew(Node2D);
	layer->add_child(node_2d);
	Control *control = memnew(Control);
	node_2d->add_child(control);

	CHECK(ThemeSound::get_bus(control) == StringName(GLOBAL_GET("gui/theme/sound_bus")));
	root->set_theme_sound_bus("ViewportBus");
	CHECK(ThemeSound::get_bus(control) == StringName("ViewportBus"));
	layer->set_theme_sound_bus("LayerBus");
	CHECK_MESSAGE(ThemeSound::get_bus(control) == StringName("LayerBus"), "Nodes that have no bus don't stop the search.");
	control->set_theme_sound_bus("ControlBus");
	CHECK(ThemeSound::get_bus(control) == StringName("ControlBus"));

	root->set_theme_sound_bus(StringName());
	memdelete(layer);
}

TEST_CASE("[SceneTree][ThemeSound] Sounds started in quick succession are skipped") {
	ThemeSound::clear();
	Control *control = memnew(Control);
	SceneTree::get_singleton()->get_root()->add_child(control);
	const Ref<AudioStream> sound = make_sound();
	const Ref<AudioStream> other_sound = make_sound();

	const uint64_t played = ThemeSound::get_played_count();
	CHECK(ThemeSound::play(sound, control));
	CHECK(ThemeSound::get_last_played() == sound->get_instance_id());
	CHECK_FALSE_MESSAGE(ThemeSound::play(sound, control), "The same sound doesn't start again at once.");
	CHECK_MESSAGE(ThemeSound::play(other_sound, control), "Other sounds still start.");
	CHECK(ThemeSound::get_played_count() == played + 2);

	ThemeSound::clear();
	CHECK(ThemeSound::play(sound, control));
	CHECK_FALSE(ThemeSound::play(Ref<AudioStream>(), control));

	memdelete(control);
}

// A sound of 50 ms. Without an audio server, as in tests, the voices of sounds end by their length.
class ShortSound : public AudioStream {
	GDCLASS(ShortSound, AudioStream);

public:
	virtual double get_length() const override { return 0.05; }
};

TEST_CASE("[SceneTree][ThemeSound] Voices end with their sound") {
	ThemeSound::clear();
	Control *control = memnew(Control);
	SceneTree::get_singleton()->get_root()->add_child(control);
	Ref<ShortSound> sound;
	sound.instantiate();

	CHECK(ThemeSound::play(sound, control));
	CHECK(ThemeSound::get_voice_count(sound) == 1);
	OS::get_singleton()->delay_usec(250000);
	CHECK_MESSAGE(ThemeSound::get_voice_count(sound) == 0, "The voice ends after the length of the sound.");
	CHECK(ThemeSound::play(sound, control));

	memdelete(control);
	ThemeSound::clear();
}

// A sound of p_seconds, with the audio server playing it.
static Ref<AudioStreamWAV> make_playing_sound(double p_seconds, bool p_loop = false) {
	Ref<AudioStreamWAV> sound;
	sound.instantiate();
	sound->set_format(AudioStreamWAV::FORMAT_8_BITS);
	sound->set_mix_rate(1000);
	Vector<uint8_t> data;
	data.resize(int(p_seconds * 1000));
	data.fill(0);
	sound->set_data(data);
	if (p_loop) {
		sound->set_loop_mode(AudioStreamWAV::LOOP_FORWARD);
		sound->set_loop_end(data.size());
	}
	return sound;
}

TEST_CASE("[Audio][ThemeSound] Voices of the audio server are limited") {
	ThemeSound::clear();
	Control *control = memnew(Control);
	const Ref<AudioStreamWAV> sound = make_playing_sound(1.0);

	int played = 0;
	for (int i = 0; i < 12; i++) {
		played += ThemeSound::play(sound, control) ? 1 : 0;
	}
	CHECK_MESSAGE(played == 1, "The same sound doesn't start again at once.");
	CHECK(ThemeSound::get_voice_count(sound) == 1);

	for (int i = 0; i < 6; i++) {
		OS::get_singleton()->delay_usec(60000);
		ThemeSound::play(sound, control);
	}
	CHECK_MESSAGE(ThemeSound::get_voice_count(sound) == 4, "At most four voices of a sound play at once.");

	memdelete(control);
	ThemeSound::clear();
}

TEST_CASE("[Audio][ThemeSound] Limits from the project settings") {
	ThemeSound::clear();
	Control *control = memnew(Control);
	const Ref<AudioStreamWAV> sound = make_playing_sound(1.0);
	ProjectSettings *settings = ProjectSettings::get_singleton();
	const Variant old_interval = settings->get_setting("gui/theme/sound_min_interval_msec");
	const Variant old_voices = settings->get_setting("gui/theme/sound_max_voices");

	settings->set_setting("gui/theme/sound_min_interval_msec", 0);
	settings->set_setting("gui/theme/sound_max_voices", 6);
	int played = 0;
	for (int i = 0; i < 10; i++) {
		played += ThemeSound::play(sound, control) ? 1 : 0;
	}
	CHECK_MESSAGE(played == 6, "Without an interval, every start plays up to the voice limit.");

	ThemeSound::clear();
	settings->set_setting("gui/theme/sound_max_voices", 0);
	played = 0;
	for (int i = 0; i < 10; i++) {
		played += ThemeSound::play(sound, control) ? 1 : 0;
	}
	CHECK_MESSAGE(played == 10, "0 voices doesn't limit them.");

	settings->set_setting("gui/theme/sound_min_interval_msec", old_interval);
	settings->set_setting("gui/theme/sound_max_voices", old_voices);
	memdelete(control);
	ThemeSound::clear();
}

TEST_CASE("[Audio][ThemeSound] Looping sounds are stopped after their length") {
	ThemeSound::clear();
	Control *control = memnew(Control);
	const Ref<AudioStreamWAV> sound = make_playing_sound(0.05, true);
	const int references = sound->get_reference_count();

	CHECK(ThemeSound::play(sound, control));
	CHECK(ThemeSound::get_voice_count(sound) == 1);
	memdelete(control);
	OS::get_singleton()->delay_usec(250000);
	CHECK(ThemeSound::get_voice_count(sound) == 0);
	// The audio server lets the playback go once it's stopped, on the main thread.
	for (int i = 0; i < 100 && sound->get_reference_count() > references; i++) {
		OS::get_singleton()->delay_usec(10000);
		AudioServer::get_singleton()->update();
	}
	CHECK(sound->get_reference_count() == references);
	ThemeSound::clear();
}

TEST_CASE("[SceneTree][ThemeSound] Touches don't play hover sounds") {
	ThemeSound::clear();
	Window *root = SceneTree::get_singleton()->get_root();
	Control *control = memnew(Control);
	root->add_child(control);

	Ref<InputEventMouseMotion> motion;
	motion.instantiate();
	motion->set_device(InputEvent::DEVICE_ID_EMULATION);
	root->push_input(motion);
	CHECK(root->gui_is_mouse_emulated_from_touch());
	CHECK_FALSE(ThemeSound::play_hover(make_sound(), control));

	motion->set_device(0);
	root->push_input(motion);
	CHECK_FALSE(root->gui_is_mouse_emulated_from_touch());
	CHECK(ThemeSound::play_hover(make_sound(), control));

	memdelete(control);
}

} // namespace TestThemeSound
