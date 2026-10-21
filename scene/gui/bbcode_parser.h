/**************************************************************************/
/*  bbcode_parser.h                                                       */
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

#include "core/math/rect2.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "scene/gui/control.h"
#include "scene/resources/font.h"
#include "servers/text/text_server.h"

// Tag syntax shared by RichTextLabel and BBCodeText.
namespace BBCodeParser {

typedef HashMap<String, String> OptionMap;

// A tag such as [name=value key=value ...].
struct Tag {
	String raw; // Text between the brackets.
	String name;
	String value;
	OptionMap options;
	Vector<String> blocks; // Space separated parts of the tag, quotes kept.
};

int find_unquoted(const String &p_src, char32_t p_chr, int p_from);
Vector<String> split_unquoted(const String &p_src, char32_t p_splitter);
String get_tag_value(const String &p_tag);
void normalize_subtags(Vector<String> &r_subtags);
Tag parse_tag(const String &p_raw);

// Alignment of [img], from "top,center" style values.
InlineAlignment parse_image_alignment(const Vector<String> &p_subtag, InlineAlignment p_default = INLINE_ALIGNMENT_CENTER);

// Options of [p].
struct ParagraphOptions {
	HorizontalAlignment alignment = HORIZONTAL_ALIGNMENT_LEFT;
	Control::TextDirection direction = Control::TEXT_DIRECTION_INHERITED;
	bool has_language = false;
	String language;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
	bool has_jst_flags = false;
	BitField<TextServer::JustificationFlag> jst_flags;
	bool has_tab_stops = false;
	PackedFloat32Array tab_stops;
};
ParagraphOptions parse_paragraph_options(const OptionMap &p_options);

// Options of [font], applied to r_variation. Sets r_font when a font is loaded.
void parse_font_options(const Tag &p_tag, const Ref<FontVariation> &r_variation, Ref<Font> &r_font, int &r_font_size);
// Features of [opentype_features] and [otf].
Dictionary parse_opentype_features(const String &p_value);

// Size part of [img=WxH].
void parse_image_size(const String &p_value, float &r_width, float &r_height, bool &r_width_percent, bool &r_height_percent);

} // namespace BBCodeParser
