/**************************************************************************/
/*  bbcode_parser.cpp                                                     */
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

#include "bbcode_parser.h"

#include "core/io/resource_loader.h"

namespace BBCodeParser {

int find_unquoted(const String &p_src, char32_t p_chr, int p_from) {
	if (p_from < 0) {
		return -1;
	}

	const int len = p_src.length();
	if (len == 0) {
		return -1;
	}

	const char32_t *src = p_src.get_data();
	bool in_single_quote = false;
	bool in_double_quote = false;
	for (int i = p_from; i < len; i++) {
		if (in_double_quote) {
			if (src[i] == '"') {
				in_double_quote = false;
			}
		} else if (in_single_quote) {
			if (src[i] == '\'') {
				in_single_quote = false;
			}
		} else {
			if (src[i] == '"') {
				in_double_quote = true;
			} else if (src[i] == '\'') {
				in_single_quote = true;
			} else if (src[i] == p_chr) {
				return i;
			}
		}
	}

	return -1;
}

Vector<String> split_unquoted(const String &p_src, char32_t p_splitter) {
	Vector<String> ret;

	if (p_src.is_empty()) {
		return ret;
	}

	int from = 0;
	int len = p_src.length();

	while (true) {
		int end = find_unquoted(p_src, p_splitter, from);
		if (end < 0) {
			end = len;
		}
		if (end > from) {
			ret.push_back(p_src.substr(from, end - from));
		}
		if (end == len) {
			break;
		}

		from = end + 1;
	}

	return ret;
}

String get_tag_value(const String &p_tag) {
	return p_tag.substr(p_tag.find_char('=') + 1);
}

void normalize_subtags(Vector<String> &r_subtags) {
	for (String &subtag : r_subtags) {
		subtag = subtag.unquote();
	}
}

Tag parse_tag(const String &p_raw) {
	Tag tag;
	tag.raw = p_raw;
	tag.blocks = split_unquoted(p_raw, ' ');

	if (!tag.blocks.is_empty()) {
		tag.name = tag.blocks[0];
		for (int i = 1; i < tag.blocks.size(); i++) {
			const String &expr = tag.blocks[i];
			int value_pos = expr.find_char('=');
			if (value_pos > -1) {
				tag.options[expr.substr(0, value_pos)] = expr.substr(value_pos + 1).unquote();
			}
		}
	} else {
		tag.name = p_raw;
	}

	int main_value_pos = tag.name.find_char('=');
	if (main_value_pos > -1) {
		tag.value = tag.name.substr(main_value_pos + 1);
		tag.name = tag.name.substr(0, main_value_pos);
	}
	return tag;
}

InlineAlignment parse_image_alignment(const Vector<String> &p_subtag, InlineAlignment p_default) {
	int alignment = p_default;
	if (p_subtag.size() > 1) {
		if (p_subtag[0] == "top" || p_subtag[0] == "t") {
			alignment = INLINE_ALIGNMENT_TOP_TO;
		} else if (p_subtag[0] == "center" || p_subtag[0] == "c") {
			alignment = INLINE_ALIGNMENT_CENTER_TO;
		} else if (p_subtag[0] == "bottom" || p_subtag[0] == "b") {
			alignment = INLINE_ALIGNMENT_BOTTOM_TO;
		}
		if (p_subtag[1] == "top" || p_subtag[1] == "t") {
			alignment |= INLINE_ALIGNMENT_TO_TOP;
		} else if (p_subtag[1] == "center" || p_subtag[1] == "c") {
			alignment |= INLINE_ALIGNMENT_TO_CENTER;
		} else if (p_subtag[1] == "baseline" || p_subtag[1] == "l") {
			alignment |= INLINE_ALIGNMENT_TO_BASELINE;
		} else if (p_subtag[1] == "bottom" || p_subtag[1] == "b") {
			alignment |= INLINE_ALIGNMENT_TO_BOTTOM;
		}
	} else if (!p_subtag.is_empty()) {
		if (p_subtag[0] == "top" || p_subtag[0] == "t") {
			alignment = INLINE_ALIGNMENT_TOP;
		} else if (p_subtag[0] == "center" || p_subtag[0] == "c") {
			alignment = INLINE_ALIGNMENT_CENTER;
		} else if (p_subtag[0] == "bottom" || p_subtag[0] == "b") {
			alignment = INLINE_ALIGNMENT_BOTTOM;
		}
	}
	return (InlineAlignment)alignment;
}

static const String *_find_option(const OptionMap &p_options, const String &p_name, const String &p_short = String()) {
	OptionMap::ConstIterator E = p_options.find(p_name);
	if (!E && !p_short.is_empty()) {
		E = p_options.find(p_short);
	}
	return E ? &E->value : nullptr;
}

ParagraphOptions parse_paragraph_options(const OptionMap &p_options) {
	ParagraphOptions ret;

	if (const String *value = _find_option(p_options, "justification_flags", "jst")) {
		Vector<String> subtag_b = split_unquoted(*value, U',');
		ret.has_jst_flags = true;
		ret.jst_flags = 0;
		for (const String &E : subtag_b) {
			if (E == "kashida" || E == "k") {
				ret.jst_flags.set_flag(TextServer::JUSTIFICATION_KASHIDA);
			} else if (E == "word" || E == "w") {
				ret.jst_flags.set_flag(TextServer::JUSTIFICATION_WORD_BOUND);
			} else if (E == "trim" || E == "tr") {
				ret.jst_flags.set_flag(TextServer::JUSTIFICATION_TRIM_EDGE_SPACES);
			} else if (E == "after_last_tab" || E == "lt") {
				ret.jst_flags.set_flag(TextServer::JUSTIFICATION_AFTER_LAST_TAB);
			} else if (E == "skip_last" || E == "sl") {
				ret.jst_flags.set_flag(TextServer::JUSTIFICATION_SKIP_LAST_LINE);
			} else if (E == "skip_last_with_chars" || E == "sv") {
				ret.jst_flags.set_flag(TextServer::JUSTIFICATION_SKIP_LAST_LINE_WITH_VISIBLE_CHARS);
			} else if (E == "do_not_skip_single" || E == "ns") {
				ret.jst_flags.set_flag(TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE);
			}
		}
	}
	if (const String *value = _find_option(p_options, "tab_stops")) {
		Vector<String> splitters;
		splitters.push_back(",");
		splitters.push_back(";");
		ret.has_tab_stops = true;
		ret.tab_stops = value->split_floats_mk(splitters);
	}
	if (const String *value = _find_option(p_options, "align")) {
		if (*value == "l" || *value == "left") {
			ret.alignment = HORIZONTAL_ALIGNMENT_LEFT;
		} else if (*value == "c" || *value == "center") {
			ret.alignment = HORIZONTAL_ALIGNMENT_CENTER;
		} else if (*value == "r" || *value == "right") {
			ret.alignment = HORIZONTAL_ALIGNMENT_RIGHT;
		} else if (*value == "f" || *value == "fill") {
			ret.alignment = HORIZONTAL_ALIGNMENT_FILL;
		}
	}
	if (const String *value = _find_option(p_options, "direction", "dir")) {
		if (*value == "a" || *value == "auto") {
			ret.direction = Control::TEXT_DIRECTION_AUTO;
		} else if (*value == "l" || *value == "ltr") {
			ret.direction = Control::TEXT_DIRECTION_LTR;
		} else if (*value == "r" || *value == "rtl") {
			ret.direction = Control::TEXT_DIRECTION_RTL;
		}
	}
	if (const String *value = _find_option(p_options, "language", "lang")) {
		ret.has_language = true;
		ret.language = *value;
	}
	if (const String *value = _find_option(p_options, "bidi_override", "st")) {
		if (*value == "d" || *value == "default") {
			ret.st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
		} else if (*value == "u" || *value == "uri") {
			ret.st_parser = TextServer::STRUCTURED_TEXT_URI;
		} else if (*value == "f" || *value == "file") {
			ret.st_parser = TextServer::STRUCTURED_TEXT_FILE;
		} else if (*value == "e" || *value == "email") {
			ret.st_parser = TextServer::STRUCTURED_TEXT_EMAIL;
		} else if (*value == "l" || *value == "list") {
			ret.st_parser = TextServer::STRUCTURED_TEXT_LIST;
		} else if (*value == "n" || *value == "gdscript") {
			ret.st_parser = TextServer::STRUCTURED_TEXT_GDSCRIPT;
		} else if (*value == "c" || *value == "custom") {
			ret.st_parser = TextServer::STRUCTURED_TEXT_CUSTOM;
		}
	}
	return ret;
}

void parse_font_options(const Tag &p_tag, const Ref<FontVariation> &r_variation, Ref<Font> &r_font, int &r_font_size) {
	if (p_tag.raw.begins_with("font=")) {
		if (!p_tag.blocks.is_empty()) {
			const String &fnt = get_tag_value(p_tag.blocks[0]).unquote();
			Ref<Font> font_data = ResourceLoader::load(fnt, "Font");
			if (font_data.is_valid()) {
				r_font = font_data;
			}
		}
	}

	const OptionMap &options = p_tag.options;
	if (const String *value = _find_option(options, "name", "n")) {
		Ref<Font> font_data = ResourceLoader::load(*value, "Font");
		if (font_data.is_valid()) {
			r_font = font_data;
		}
	}
	if (const String *value = _find_option(options, "size", "s")) {
		r_font_size = value->to_int();
	}
	if (const String *value = _find_option(options, "glyph_spacing", "gl")) {
		r_variation->set_spacing(TextServer::SPACING_GLYPH, value->to_int());
	}
	if (const String *value = _find_option(options, "space_spacing", "sp")) {
		r_variation->set_spacing(TextServer::SPACING_SPACE, value->to_int());
	}
	if (const String *value = _find_option(options, "top_spacing", "top")) {
		r_variation->set_spacing(TextServer::SPACING_TOP, value->to_int());
	}
	if (const String *value = _find_option(options, "bottom_spacing", "bt")) {
		r_variation->set_spacing(TextServer::SPACING_BOTTOM, value->to_int());
	}
	if (const String *value = _find_option(options, "embolden", "emb")) {
		r_variation->set_variation_embolden(value->to_float());
	}
	if (const String *value = _find_option(options, "face_index", "fi")) {
		r_variation->set_variation_face_index(value->to_int());
	}
	if (const String *value = _find_option(options, "slant", "sln")) {
		r_variation->set_variation_transform(Transform2D(1.0, value->to_float(), 0.0, 1.0, 0.0, 0.0));
	}
	if (const String *value = _find_option(options, "opentype_variation", "otv")) {
		Dictionary variations;
		if (!value->is_empty()) {
			Vector<String> variation_tags = value->split(",");
			for (int j = 0; j < variation_tags.size(); j++) {
				Vector<String> subtag_b = variation_tags[j].split("=");
				normalize_subtags(subtag_b);

				if (subtag_b.size() == 2) {
					variations[TS->name_to_tag(subtag_b[0])] = subtag_b[1].to_float();
				}
			}
			r_variation->set_variation_opentype(variations);
		}
	}
	if (const String *value = _find_option(options, "opentype_features", "otf")) {
		Dictionary features;
		if (!value->is_empty()) {
			Vector<String> feature_tags = value->split(",");
			for (int j = 0; j < feature_tags.size(); j++) {
				Vector<String> subtag_b = feature_tags[j].split("=");
				normalize_subtags(subtag_b);

				if (subtag_b.size() == 2) {
					features[TS->name_to_tag(subtag_b[0])] = subtag_b[1].to_float();
				} else if (subtag_b.size() == 1) {
					features[TS->name_to_tag(subtag_b[0])] = 1;
				}
			}
			r_variation->set_opentype_features(features);
		}
	}
}

Dictionary parse_opentype_features(const String &p_value) {
	Vector<String> subtag = p_value.split(",");
	normalize_subtags(subtag);

	Dictionary features;
	for (int i = 0; i < subtag.size(); i++) {
		Vector<String> subtag_a = subtag[i].split("=");
		normalize_subtags(subtag_a);

		if (subtag_a.size() == 2) {
			features[TS->name_to_tag(subtag_a[0])] = subtag_a[1].to_int();
		} else if (subtag_a.size() == 1) {
			features[TS->name_to_tag(subtag_a[0])] = 1;
		}
	}
	return features;
}

void parse_image_size(const String &p_value, float &r_width, float &r_height, bool &r_width_percent, bool &r_height_percent) {
	int sep = p_value.find_char('x');
	if (sep == -1) {
		r_width_percent = p_value.ends_with("%");
		r_width = p_value.to_float();
	} else {
		r_width_percent = p_value.substr(0, sep).ends_with("%");
		r_width = p_value.substr(0, sep).to_float();
		r_height_percent = p_value.substr(sep + 1).ends_with("%");
		r_height = p_value.substr(sep + 1).to_float();
	}
}

} // namespace BBCodeParser
