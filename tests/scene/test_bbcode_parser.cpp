/**************************************************************************/
/*  test_bbcode_parser.cpp                                                */
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

TEST_FORCE_LINK(test_bbcode_parser)

#include "scene/gui/bbcode_parser.h"

namespace TestBBCodeParser {

TEST_CASE("[BBCodeParser] Quotes") {
	CHECK(BBCodeParser::find_unquoted("a \"b c\" d", ' ', 0) == 1);
	CHECK_MESSAGE(BBCodeParser::find_unquoted("a \"b c\" d", ' ', 2) == 7, "Characters in double quotes are skipped.");
	CHECK_MESSAGE(BBCodeParser::find_unquoted("'b c' d", ' ', 0) == 5, "Characters in single quotes are skipped.");
	CHECK(BBCodeParser::find_unquoted("\"unclosed quote", ' ', 0) == -1);
	CHECK(BBCodeParser::find_unquoted("abc", 'b', -1) == -1);

	const Vector<String> parts = BBCodeParser::split_unquoted("a  'b c' d", ' ');
	REQUIRE(parts.size() == 3);
	CHECK(parts[0] == "a");
	CHECK_MESSAGE(parts[1] == "'b c'", "Quotes are kept, empty parts are dropped.");
	CHECK(parts[2] == "d");
	CHECK(BBCodeParser::split_unquoted("", ' ').is_empty());
}

TEST_CASE("[BBCodeParser] Tags") {
	BBCodeParser::Tag tag = BBCodeParser::parse_tag("url=res://a.tscn tooltip=\"Open it\" n=1");
	CHECK(tag.name == "url");
	CHECK(tag.value == "res://a.tscn");
	CHECK_MESSAGE(tag.options["tooltip"] == "Open it", "Option values are unquoted.");
	CHECK(tag.options["n"] == "1");
	CHECK(tag.blocks.size() == 3);

	tag = BBCodeParser::parse_tag("b");
	CHECK(tag.name == "b");
	CHECK(tag.value.is_empty());
	CHECK(tag.options.is_empty());

	CHECK(BBCodeParser::get_tag_value("color=red") == "red");
}

TEST_CASE("[BBCodeParser] Paragraph options") {
	BBCodeParser::Tag tag = BBCodeParser::parse_tag("p align=c dir=rtl lang=ar jst=w,k tab_stops=10,20 st=email");
	const BBCodeParser::ParagraphOptions options = BBCodeParser::parse_paragraph_options(tag.options);
	CHECK(options.alignment == HORIZONTAL_ALIGNMENT_CENTER);
	CHECK(options.direction == Control::TEXT_DIRECTION_RTL);
	CHECK(options.has_language);
	CHECK(options.language == "ar");
	CHECK(options.has_jst_flags);
	CHECK(options.jst_flags.has_flag(TextServer::JUSTIFICATION_WORD_BOUND));
	CHECK(options.jst_flags.has_flag(TextServer::JUSTIFICATION_KASHIDA));
	CHECK_FALSE(options.jst_flags.has_flag(TextServer::JUSTIFICATION_TRIM_EDGE_SPACES));
	CHECK(options.has_tab_stops);
	REQUIRE(options.tab_stops.size() == 2);
	CHECK(options.tab_stops[1] == doctest::Approx(20));
	CHECK(options.st_parser == TextServer::STRUCTURED_TEXT_EMAIL);

	const BBCodeParser::ParagraphOptions defaults = BBCodeParser::parse_paragraph_options(BBCodeParser::OptionMap());
	CHECK(defaults.alignment == HORIZONTAL_ALIGNMENT_LEFT);
	CHECK(defaults.direction == Control::TEXT_DIRECTION_INHERITED);
	CHECK_FALSE(defaults.has_language);
	CHECK_FALSE(defaults.has_jst_flags);
	CHECK_FALSE(defaults.has_tab_stops);
}

TEST_CASE("[BBCodeParser] Font options") {
	Ref<FontVariation> variation;
	variation.instantiate();
	Ref<Font> font;
	int font_size = 0;
	BBCodeParser::parse_font_options(BBCodeParser::parse_tag("font s=24 gl=2 emb=0.5"), variation, font, font_size);
	CHECK(font_size == 24);
	CHECK(variation->get_spacing(TextServer::SPACING_GLYPH) == 2);
	CHECK(variation->get_variation_embolden() == doctest::Approx(0.5));
	CHECK_MESSAGE(font.is_null(), "No font is loaded without a path.");

	const Dictionary features = BBCodeParser::parse_opentype_features("liga=0,calt");
	CHECK(int(features[TS->name_to_tag("liga")]) == 0);
	CHECK(int(features[TS->name_to_tag("calt")]) == 1);
}

TEST_CASE("[BBCodeParser] Images") {
	float width = 0;
	float height = 0;
	bool width_percent = false;
	bool height_percent = false;
	BBCodeParser::parse_image_size("50%x20", width, height, width_percent, height_percent);
	CHECK(width == doctest::Approx(50));
	CHECK(width_percent);
	CHECK(height == doctest::Approx(20));
	CHECK_FALSE(height_percent);

	width = 0;
	height = 7;
	BBCodeParser::parse_image_size("32", width, height, width_percent, height_percent);
	CHECK(width == doctest::Approx(32));
	CHECK_FALSE(width_percent);
	CHECK_MESSAGE(height == doctest::Approx(7), "The height isn't changed without one.");

	Vector<String> subtag;
	subtag.push_back("top");
	CHECK(BBCodeParser::parse_image_alignment(subtag) == INLINE_ALIGNMENT_TOP);
	subtag.push_back("baseline");
	CHECK(BBCodeParser::parse_image_alignment(subtag) == (INLINE_ALIGNMENT_TOP_TO | INLINE_ALIGNMENT_TO_BASELINE));
	CHECK(BBCodeParser::parse_image_alignment(Vector<String>(), INLINE_ALIGNMENT_BOTTOM) == INLINE_ALIGNMENT_BOTTOM);
}

} // namespace TestBBCodeParser
