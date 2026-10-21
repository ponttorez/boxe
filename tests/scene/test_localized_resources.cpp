/**************************************************************************/
/*  test_localized_resources.cpp                                          */
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

TEST_FORCE_LINK(test_localized_resources)

#include "core/string/translation_server.h"
#include "scene/resources/audio_stream_wav.h"
#include "scene/resources/font.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/localized_audio_stream.h"
#include "scene/resources/localized_font.h"
#include "scene/resources/localized_texture_2d.h"
#include "tests/signal_watcher.h"

namespace TestLocalizedResources {

static Ref<ImageTexture> make_texture(int p_size) {
	return ImageTexture::create_from_image(Image::create_empty(p_size, p_size, false, Image::FORMAT_RGBA8));
}

static Ref<AudioStreamWAV> make_stream(int p_samples) {
	Ref<AudioStreamWAV> stream;
	stream.instantiate();
	stream->set_format(AudioStreamWAV::FORMAT_8_BITS);
	stream->set_mix_rate(1000);
	Vector<uint8_t> data;
	data.resize(p_samples);
	stream->set_data(data);
	return stream;
}

TEST_CASE("[SceneTree][LocalizedTexture2D] Texture of the locale") {
	TranslationServer *ts = TranslationServer::get_singleton();
	const String old_locale = ts->get_locale();
	ts->set_locale("ru");

	Ref<LocalizedTexture2D> texture;
	texture.instantiate();
	texture->set_default_texture(make_texture(1));
	Dictionary textures;
	textures["en"] = make_texture(2);
	textures["ru"] = make_texture(4);
	texture->set_textures(textures);
	CHECK(texture->get_size() == Size2(4, 4));

	const RID rid = texture->get_rid();
	SIGNAL_WATCH(texture.ptr(), CoreStringName(changed));

	// A locale with a region uses the texture of its language.
	ts->set_locale("en_US");
	CHECK(texture->get_size() == Size2(2, 2));
	SIGNAL_CHECK(CoreStringName(changed), { {} });

	// A locale without texture uses the fallback locale, or else the default texture.
	ts->set_locale("fr");
	CHECK(texture->get_current_texture() == textures["en"]);
	textures.erase("en");
	texture->set_textures(textures);
	CHECK(texture->get_size() == Size2(1, 1));

	// Materials keep the same texture.
	CHECK(texture->get_rid() == rid);

	SIGNAL_UNWATCH(texture.ptr(), CoreStringName(changed));
	ts->set_locale(old_locale);
}

TEST_CASE("[SceneTree][LocalizedTexture2D] Cycles") {
	Ref<LocalizedTexture2D> first;
	first.instantiate();
	Ref<LocalizedTexture2D> second;
	second.instantiate();

	ERR_PRINT_OFF;
	first->set_default_texture(first);
	Dictionary itself;
	itself["en"] = first;
	first->set_textures(itself);
	ERR_PRINT_ON;
	CHECK(first->get_default_texture().is_null());
	CHECK(first->get_textures().is_empty());

	// Through another localized texture.
	Dictionary textures;
	textures["en"] = first;
	second->set_textures(textures);
	ERR_PRINT_OFF;
	first->set_default_texture(second);
	ERR_PRINT_ON;
	CHECK(first->get_default_texture().is_null());
	CHECK(first->get_size() == Size2(1, 1));
}

TEST_CASE("[Audio][LocalizedAudioStream] Stream of the locale") {
	TranslationServer *ts = TranslationServer::get_singleton();
	const String old_locale = ts->get_locale();
	ts->set_locale("en");

	Ref<LocalizedAudioStream> stream;
	stream.instantiate();
	Dictionary streams;
	streams["en"] = make_stream(1000);
	streams["ja"] = make_stream(3000);
	stream->set_streams(streams);
	CHECK(stream->get_length() == doctest::Approx(1.0));

	ts->set_locale("ja_JP");
	CHECK(stream->get_current_stream() == streams["ja"]);
	CHECK(stream->get_length() == doctest::Approx(3.0));

	// Players register and play the sample of the stream they have, like on the web.
	const Ref<AudioSample> sample = stream->generate_sample();
	REQUIRE(sample.is_valid());
	CHECK(sample->stream == stream);

	ts->set_locale(old_locale);
}

TEST_CASE("[SceneTree][LocalizedFont] Font of the locale") {
	TranslationServer *ts = TranslationServer::get_singleton();
	const String old_locale = ts->get_locale();
	ts->set_locale("en");

	Ref<FontFile> latin;
	latin.instantiate();
	Ref<FontFile> japanese;
	japanese.instantiate();

	Ref<LocalizedFont> font;
	font.instantiate();
	font->set_default_font(latin);
	Dictionary fonts;
	fonts["ja"] = japanese;
	font->set_fonts(fonts);
	CHECK(font->get_base_font() == latin);

	ts->set_locale("ja");
	CHECK(font->get_current_font() == japanese);
	CHECK(font->get_base_font() == japanese);

	ts->set_locale(old_locale);
}

} // namespace TestLocalizedResources
