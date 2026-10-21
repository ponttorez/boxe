/**************************************************************************/
/*  theme_sound.h                                                         */
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

#include "core/object/object_id.h"
#include "servers/audio/audio_stream.h"

class Node;

namespace ThemeSound {

// The bus of the closest Control, Viewport or CanvasLayer with a theme sound bus among p_context and its ancestors,
// or the gui/theme/sound_bus project setting.
StringName get_bus(const Node *p_context);
// The names of the audio buses, for the hint of the theme_sound_bus properties.
String get_bus_hint();

// Plays a theme sound on the bus of p_context. Sounds started again in quick succession, e.g. while typing or moving
// over the items of a list, are skipped, so they don't pile up. Theme sounds play once: a looping one is stopped after
// its length. Returns whether the sound started.
bool play(const Ref<AudioStream> &p_sound, const Node *p_context);
// Like play(), but not when the mouse is emulated from a touch, as a tap would play the hover sound with the press.
bool play_hover(const Ref<AudioStream> &p_sound, const Node *p_context);

// How many times the sound is playing, e.g. for tests.
int get_voice_count(const Ref<AudioStream> &p_sound);

// How many sounds started, and the last one, e.g. for tests.
uint64_t get_played_count();
ObjectID get_last_played();

// Forgets the recently played sounds.
void clear();

} // namespace ThemeSound
