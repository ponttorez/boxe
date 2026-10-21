/**************************************************************************/
/*  bbcode_text.cpp                                                       */
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

#include "bbcode_text.h"

#include "core/io/resource_loader.h"
#include "core/string/translation_server.h"
#include "scene/gui/bbcode_parser.h"
#include "scene/main/window.h"
#include "scene/resources/texture.h"
#include "scene/scene_string_names.h"
#include "scene/theme/theme_db.h"
#include "servers/rendering/rendering_server.h"

using namespace BBCodeParser;

static const char *theme_font_names[] = { "bold_font", "italics_font", "bold_italics_font", "mono_font" };

static String _roman(int p_num, bool p_capitalize) {
	if (p_num > 3999) {
		return "ERR";
	}
	static const char *roman_m[] = { "", "m", "mm", "mmm" };
	static const char *roman_c[] = { "", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm" };
	static const char *roman_x[] = { "", "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc" };
	static const char *roman_i[] = { "", "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix" };
	String s = String(roman_m[p_num / 1000]) + roman_c[(p_num % 1000) / 100] + roman_x[(p_num % 100) / 10] + roman_i[p_num % 10];
	return p_capitalize ? s.to_upper() : s;
}

static String _letters(int p_num, bool p_capitalize) {
	String s;
	int64_t n = p_num;
	do {
		n = n - 1;
		s = String::chr((p_capitalize ? 'A' : 'a') + n % 26) + s;
		n /= 26;
	} while (n);
	return s;
}

// Parsing.

static bool _parse_color(const String &p_value, Color &r_color) {
	Color color = Color::from_string(p_value.unquote(), Color(0, 0, 0, -1));
	if (color.a < 0) {
		return false;
	}
	r_color = color;
	return true;
}

BBCodeText::ParseState &BBCodeText::Builder::state() {
	return states.write[states.size() - 1];
}

BBCodeText::Paragraph &BBCodeText::Builder::current() {
	return bbcode->paragraphs.write[bbcode->paragraphs.size() - 1];
}

void BBCodeText::Builder::push_state(const String &p_name) {
	states.push_back(states[states.size() - 1]);
	tag_stack.push_back(p_name);
}

void BBCodeText::Builder::push_style(const SpanStyle &p_style) {
	bbcode->styles.push_back(p_style);
	state().style = bbcode->styles.size() - 1;
}

void BBCodeText::Builder::new_paragraph() {
	Paragraph para;
	para.style = state().para;
	bbcode->paragraphs.push_back(para);
}

void BBCodeText::Builder::add_text(const String &p_text) {
	Vector<String> parts = p_text.split("\n");
	for (int i = 0; i < parts.size(); i++) {
		if (i > 0) {
			new_paragraph();
		}
		if (!parts[i].is_empty()) {
			Run run;
			run.text = parts[i];
			run.style = state().style;
			current().runs.push_back(run);
		}
	}
}

void BBCodeText::Builder::add_object(const InlineObject &p_object) {
	bbcode->objects.push_back(p_object);
	Run run;
	run.style = state().style;
	run.object = bbcode->objects.size() - 1;
	current().runs.push_back(run);
}

// Paragraph tags start a new paragraph unless the current one is still empty.
void BBCodeText::Builder::open_paragraph_tag() {
	if (current().is_empty()) {
		current().style = state().para;
	} else {
		new_paragraph();
	}
}

void BBCodeText::Builder::set_font_kind() {
	SpanStyle style = bbcode->styles[state().style];
	style.font_kind = (state().bold && state().italics) ? FONT_BOLD_ITALICS : (state().bold ? FONT_BOLD : FONT_ITALICS);
	style.custom_font = Ref<Font>();
	style.variations.clear();
	push_style(style);
}

BBCodeText::Builder::Builder(BBCodeText *p_bbcode) {
	bbcode = p_bbcode;
	states.push_back(ParseState());
}

void BBCodeText::_parse() {
	_clear_shaped();
	paragraphs.clear();
	styles.clear();
	objects.clear();
	metas.clear();
	hints.clear();
	has_relative_objects = false;
	list_count = 0;

	styles.push_back(SpanStyle());
	paragraphs.push_back(Paragraph());
	Builder builder(this);

	String bbcode = text.replace("\r\n", "\n");
	int pos = 0;
	bool after_list_open_tag = false;
	bool after_list_close_tag = false;
	bool after_hr_tag = false;
	int list_end_paragraph = -1;

	while (pos <= bbcode.length()) {
		int brk_pos = bbcode.find_char('[', pos);
		if (brk_pos < 0) {
			brk_pos = bbcode.length();
		}

		String txt = brk_pos > pos ? bbcode.substr(pos, brk_pos - pos) : "";

		// Trim the first newline character, as RichTextLabel does around lists.
		if (after_list_close_tag || after_list_open_tag || after_hr_tag) {
			txt = txt.trim_prefix("\n");
		}
		after_hr_tag = false;

		if (brk_pos == bbcode.length()) {
			builder.add_text(txt);
			break;
		}

		int brk_end = find_unquoted(bbcode, ']', brk_pos + 1);
		if (brk_end == -1) {
			builder.add_text(txt + bbcode.substr(brk_pos));
			break;
		}

		const String tag = bbcode.substr(brk_pos + 1, brk_end - brk_pos - 1);
		const Tag parsed = parse_tag(tag);
		const OptionMap &options = parsed.options;

		if (tag.begins_with("/") && !builder.tag_stack.is_empty()) {
			const String closing = tag.substr(1);
			if (builder.tag_stack[builder.tag_stack.size() - 1] != closing) {
				builder.add_text(txt + "[" + tag);
				after_list_open_tag = false;
				after_list_close_tag = false;
				pos = brk_end;
				continue;
			}

			after_list_open_tag = false;
			after_list_close_tag = (closing == "ol" || closing == "ul");
			builder.add_text(txt);

			builder.tag_stack.remove_at(builder.tag_stack.size() - 1);
			builder.states.remove_at(builder.states.size() - 1);
			pos = brk_end + 1;

			if (closing == "p" || closing == "left" || closing == "center" || closing == "right" || closing == "fill" || closing == "indent" || closing == "ol" || closing == "ul") {
				if (builder.current().is_empty()) {
					builder.current().style = builder.state().para;
				} else if (after_list_close_tag) {
					builder.new_paragraph();
					list_end_paragraph = paragraphs.size() - 1;
				}
			}
			continue;
		}

		after_list_open_tag = (tag == "ol" || tag.begins_with("ol ") || tag == "ul" || tag.begins_with("ul "));
		after_list_close_tag = false;
		builder.add_text(txt);
		pos = brk_end + 1;

		if (tag == "b" || tag == "i") {
			builder.push_state(tag);
			if (tag == "b") {
				builder.state().bold = true;
			} else {
				builder.state().italics = true;
			}
			builder.set_font_kind();
		} else if (tag == "code") {
			builder.push_state(tag);
			SpanStyle style = styles[builder.state().style];
			style.font_kind = FONT_MONO;
			style.custom_font = Ref<Font>();
			style.variations.clear();
			builder.push_style(style);
		} else if (tag == "u" || tag.begins_with("u ")) {
			builder.push_state("u");
			SpanStyle style = styles[builder.state().style];
			style.underline = true;
			style.underline_color = Color(0, 0, 0, 0);
			if (options.has("color")) {
				_parse_color(options["color"], style.underline_color);
			}
			builder.push_style(style);
		} else if (tag == "s" || tag.begins_with("s ")) {
			builder.push_state("s");
			SpanStyle style = styles[builder.state().style];
			style.strikethrough = true;
			style.strikethrough_color = Color(0, 0, 0, 0);
			if (options.has("color")) {
				_parse_color(options["color"], style.strikethrough_color);
			}
			builder.push_style(style);
		} else if (tag.begins_with("char=")) {
			builder.add_text(String::chr(get_tag_value(tag).hex_to_int()));
		} else if (tag == "lb") {
			builder.add_text("[");
		} else if (tag == "rb") {
			builder.add_text("]");
		} else if (tag == "lrm") {
			builder.add_text(String::chr(0x200E));
		} else if (tag == "rlm") {
			builder.add_text(String::chr(0x200F));
		} else if (tag == "lre") {
			builder.add_text(String::chr(0x202A));
		} else if (tag == "rle") {
			builder.add_text(String::chr(0x202B));
		} else if (tag == "lro") {
			builder.add_text(String::chr(0x202D));
		} else if (tag == "rlo") {
			builder.add_text(String::chr(0x202E));
		} else if (tag == "pdf") {
			builder.add_text(String::chr(0x202C));
		} else if (tag == "alm") {
			builder.add_text(String::chr(0x061C));
		} else if (tag == "lri") {
			builder.add_text(String::chr(0x2066));
		} else if (tag == "rli") {
			builder.add_text(String::chr(0x2067));
		} else if (tag == "fsi") {
			builder.add_text(String::chr(0x2068));
		} else if (tag == "pdi") {
			builder.add_text(String::chr(0x2069));
		} else if (tag == "zwj") {
			builder.add_text(String::chr(0x200D));
		} else if (tag == "zwnj") {
			builder.add_text(String::chr(0x200C));
		} else if (tag == "wj") {
			builder.add_text(String::chr(0x2060));
		} else if (tag == "shy") {
			builder.add_text(String::chr(0x00AD));
		} else if (tag == "br") {
			builder.add_text("\r");
		} else if (tag == "spacer" || tag.begins_with("spacer=")) {
			InlineObject spacer;
			spacer.type = OBJECT_SPACER;
			spacer.style = builder.state().style;
			if (!parsed.value.is_empty()) {
				spacer.weight = MAX(0.0f, parsed.value.to_float());
			}
			builder.add_object(spacer);
		} else if (tag == "left" || tag == "center" || tag == "right" || tag == "fill") {
			builder.push_state(tag);
			if (tag == "left") {
				builder.state().para.alignment = HORIZONTAL_ALIGNMENT_LEFT;
			} else if (tag == "center") {
				builder.state().para.alignment = HORIZONTAL_ALIGNMENT_CENTER;
			} else if (tag == "right") {
				builder.state().para.alignment = HORIZONTAL_ALIGNMENT_RIGHT;
			} else {
				builder.state().para.alignment = HORIZONTAL_ALIGNMENT_FILL;
			}
			builder.open_paragraph_tag();
		} else if (tag == "p" || tag.begins_with("p ")) {
			builder.push_state("p");
			ParagraphStyle &para = builder.state().para;
			ParagraphOptions opts = parse_paragraph_options(options);
			para.alignment = opts.alignment;
			para.direction = (opts.direction == Control::TEXT_DIRECTION_INHERITED) ? -1 : (int)opts.direction;
			para.has_language = opts.has_language;
			para.language = opts.language;
			para.st_parser = opts.st_parser;
			para.has_jst_flags = opts.has_jst_flags;
			para.jst_flags = opts.jst_flags;
			para.has_tab_stops = opts.has_tab_stops;
			para.tab_stops = opts.tab_stops;
			builder.open_paragraph_tag();
		} else if (tag == "indent") {
			builder.push_state(tag);
			builder.state().para.indent++;
			builder.open_paragraph_tag();
		} else if (after_list_open_tag) {
			ListLevel list;
			list.id = list_count++;
			if (tag.begins_with("ul")) {
				list.type = LIST_DOTS;
				if (tag.begins_with("ul bullet=")) {
					list.bullet = get_tag_value(tag);
				}
				builder.push_state("ul");
			} else {
				list.type = LIST_NUMBERS;
				if (tag == "ol type=a" || tag == "ol type=A") {
					list.type = LIST_LETTERS;
				} else if (tag == "ol type=i" || tag == "ol type=I") {
					list.type = LIST_ROMAN;
				}
				list.capitalize = (tag == "ol type=A" || tag == "ol type=I");
				builder.push_state("ol");
			}
			builder.state().para.lists.push_back(list);
			builder.open_paragraph_tag();
		} else if (tag.begins_with("lang=")) {
			builder.push_state("lang");
			SpanStyle style = styles[builder.state().style];
			style.language = get_tag_value(tag).unquote();
			builder.push_style(style);
		} else if (tag == "url" || tag.begins_with("url ") || tag.begins_with("url=")) {
			Meta meta;
			if (tag == "url") {
				int end = bbcode.find_char('[', brk_end);
				if (end == -1) {
					end = bbcode.length();
				}
				meta.meta = bbcode.substr(brk_end + 1, end - brk_end - 1).unquote();
			} else if (tag.begins_with("url=")) {
				meta.meta = get_tag_value(tag).unquote();
			} else {
				if (options.has("underline")) {
					const String &underline = options["underline"];
					if (underline == "never") {
						meta.underline = META_UNDERLINE_NEVER;
					} else if (underline == "hover") {
						meta.underline = META_UNDERLINE_ON_HOVER;
					}
				}
				if (options.has("tooltip")) {
					meta.tooltip = options["tooltip"];
				}
				meta.meta = options.has("href") ? options["href"] : String();
			}
			metas.push_back(meta);
			builder.push_state("url");
			SpanStyle style = styles[builder.state().style];
			style.meta = metas.size() - 1;
			builder.push_style(style);
		} else if (tag.begins_with("hint=")) {
			hints.push_back(get_tag_value(tag).unquote());
			builder.push_state("hint");
			SpanStyle style = styles[builder.state().style];
			style.hint = hints.size() - 1;
			builder.push_style(style);
		} else if (tag.begins_with("dropcap")) {
			Paragraph &para = builder.current();
			para.has_dropcap = true;
			para.dropcap_font = Ref<Font>();
			para.dropcap_font_size = -1;
			para.dropcap_margins = Rect2();
			para.dropcap_has_color = false;
			para.dropcap_outline_size = -1;
			para.dropcap_has_outline_color = false;

			if (options.has("font") || options.has("f")) {
				Ref<Font> font = ResourceLoader::load(options.has("font") ? options["font"] : options["f"], "Font");
				if (font.is_valid()) {
					para.dropcap_font = font;
				}
			}
			if (options.has("font_size")) {
				para.dropcap_font_size = options["font_size"].to_int();
			}
			if (options.has("margins")) {
				Vector<String> subtag = split_unquoted(options["margins"], U',');
				normalize_subtags(subtag);
				if (subtag.size() == 4) {
					para.dropcap_margins = Rect2(subtag[0].to_float(), subtag[1].to_float(), subtag[2].to_float(), subtag[3].to_float());
				}
			}
			if (options.has("outline_size")) {
				para.dropcap_outline_size = options["outline_size"].to_int();
			}
			if (options.has("color")) {
				para.dropcap_has_color = _parse_color(options["color"], para.dropcap_color);
			}
			if (options.has("outline_color")) {
				para.dropcap_has_outline_color = _parse_color(options["outline_color"], para.dropcap_outline_color);
			}

			int end = bbcode.find_char('[', brk_end);
			if (end == -1) {
				end = bbcode.length();
			}
			para.dropcap_text = bbcode.substr(brk_end + 1, end - brk_end - 1);
			pos = end;
			builder.push_state(parsed.name);
		} else if (tag.begins_with("hr")) {
			if (!builder.current().is_empty()) {
				builder.new_paragraph();
			}
			Paragraph &para = builder.current();
			para.is_hr = true;
			if (options.has("align")) {
				const String &align = options["align"];
				if (align == "l" || align == "left") {
					para.hr_alignment = HORIZONTAL_ALIGNMENT_LEFT;
				} else if (align == "r" || align == "right") {
					para.hr_alignment = HORIZONTAL_ALIGNMENT_RIGHT;
				}
			}
			if (options.has("color")) {
				para.hr_has_color = _parse_color(options["color"], para.hr_color);
			}
			if (options.has("width")) {
				para.hr_width = options["width"].to_int();
				para.hr_width_percent = options["width"].ends_with("%");
			}
			if (options.has("height")) {
				para.hr_height = options["height"].to_int();
				para.hr_height_percent = options["height"].ends_with("%");
			}
			builder.new_paragraph();
			after_hr_tag = true;
		} else if (tag.begins_with("img")) {
			InlineObject img;
			img.style = builder.state().style;
			if (tag.begins_with("img=")) {
				Vector<String> subtag = split_unquoted(get_tag_value(parsed.blocks[0]), U',');
				normalize_subtags(subtag);
				img.inline_align = parse_image_alignment(subtag);
			}

			int end = bbcode.find_char('[', brk_end);
			if (end == -1) {
				end = bbcode.length();
			}
			String image = bbcode.substr(brk_end + 1, end - brk_end - 1);
			img.texture = ResourceLoader::load(image, "Texture2D");

			if (img.texture.is_valid()) {
				if (options.has("region")) {
					Vector<String> region_values = split_unquoted(options["region"], U',');
					if (region_values.size() == 4) {
						img.region = Rect2(region_values[0].to_float(), region_values[1].to_float(), region_values[2].to_float(), region_values[3].to_float());
					}
				}
				if (options.has("color")) {
					_parse_color(options["color"], img.color);
				}

				if (!parsed.value.is_empty()) {
					bool width_percent = false;
					bool height_percent = false;
					parse_image_size(parsed.value, img.width, img.height, width_percent, height_percent);
					img.width_unit = width_percent ? IMAGE_UNIT_PERCENT : IMAGE_UNIT_PIXEL;
					img.height_unit = height_percent ? IMAGE_UNIT_PERCENT : IMAGE_UNIT_PIXEL;
				} else {
					if (options.has("align")) {
						Vector<String> subtag = split_unquoted(options["align"], U',');
						normalize_subtags(subtag);
						img.inline_align = parse_image_alignment(subtag);
					}
					for (int i = 0; i < 2; i++) {
						const String key = i == 0 ? "width" : "height";
						if (!options.has(key)) {
							continue;
						}
						const String &value = options[key];
						float &size = i == 0 ? img.width : img.height;
						ImageUnit &unit = i == 0 ? img.width_unit : img.height_unit;
						size = value.to_float();
						if (value.ends_with("%")) {
							size = value.trim_suffix("%").to_float();
							unit = IMAGE_UNIT_PERCENT;
						} else if (value.ends_with("em")) {
							size = value.trim_suffix("em").to_float();
							unit = IMAGE_UNIT_EM;
						}
					}
					if (options.has("tooltip")) {
						img.tooltip = options["tooltip"];
					}
					if (options.has("pad")) {
						img.pad = options["pad"] == "true";
					}
				}
				has_relative_objects = has_relative_objects || img.width_unit != IMAGE_UNIT_PIXEL || img.height_unit != IMAGE_UNIT_PIXEL;
				builder.add_object(img);
			}
			pos = end;
			builder.push_state(parsed.name);
		} else if (tag.begins_with("color=") || tag.begins_with("outline_color=") || tag.begins_with("bgcolor=") || tag.begins_with("fgcolor=")) {
			builder.push_state(parsed.name);
			SpanStyle style = styles[builder.state().style];
			Color color;
			bool valid = _parse_color(get_tag_value(tag), color);
			if (parsed.name == "color") {
				style.has_color = valid;
				style.color = color;
			} else if (parsed.name == "outline_color") {
				style.has_outline_color = valid;
				style.outline_color = color;
			} else if (parsed.name == "bgcolor") {
				style.bgcolor = valid ? color : Color(0, 0, 0, 0);
			} else {
				style.fgcolor = valid ? color : Color(0, 0, 0, 0);
			}
			builder.push_style(style);
		} else if (tag.begins_with("font_size=")) {
			builder.push_state("font_size");
			SpanStyle style = styles[builder.state().style];
			style.font_size = get_tag_value(tag).to_int();
			builder.push_style(style);
		} else if (tag.begins_with("outline_size=")) {
			builder.push_state("outline_size");
			SpanStyle style = styles[builder.state().style];
			style.outline_size = MAX(0, get_tag_value(tag).to_int());
			builder.push_style(style);
		} else if (tag.begins_with("opentype_features=") || tag.begins_with("otf=")) {
			builder.push_state(parsed.name);
			SpanStyle style = styles[builder.state().style];
			Ref<FontVariation> variation;
			variation.instantiate();
			variation->set_opentype_features(parse_opentype_features(get_tag_value(tag)));
			style.variations.push_back(variation);
			builder.push_style(style);
		} else if (tag.begins_with("font")) {
			builder.push_state("font");
			SpanStyle style = styles[builder.state().style];
			Ref<FontVariation> variation;
			variation.instantiate();
			Ref<Font> font;
			parse_font_options(parsed, variation, font, style.font_size);
			if (font.is_valid()) {
				style.custom_font = font;
				style.variations.clear();
			}
			style.variations.push_back(variation);
			builder.push_style(style);
		} else if (parsed.name == "fade" || parsed.name == "shake" || parsed.name == "wave" || parsed.name == "tornado" || parsed.name == "rainbow" || parsed.name == "pulse" || tag == "cell" || tag.begins_with("cell ") || tag.begins_with("cell=") || tag.begins_with("table=")) {
			// Effects and tables are left to RichTextLabel, their text is shown as is.
			builder.push_state(parsed.name.begins_with("cell") ? String("cell") : parsed.name);
		} else {
			// Not a tag, keep the bracket.
			builder.add_text("[");
			pos = brk_pos + 1;
		}
	}

	// A list at the end of the text doesn't leave an empty line behind.
	if (paragraphs.size() > 1 && list_end_paragraph == paragraphs.size() - 1 && builder.current().is_empty()) {
		paragraphs.remove_at(paragraphs.size() - 1);
	}

	// Character ranges, list markers and parsed text.
	HashMap<int, int> list_counters;
	int start = 0;
	parsed_text = String();
	for (int i = 0; i < paragraphs.size(); i++) {
		Paragraph &para = paragraphs.write[i];
		para.char_start = start;
		para.char_count = para.is_hr ? 1 : 0;
		for (const Run &run : para.runs) {
			para.char_count += run.object >= 0 ? 1 : run.text.length();
			if (run.object < 0) {
				parsed_text += run.text;
			} else if (objects[run.object].type == OBJECT_IMAGE) {
				parsed_text += String::chr(0xFFFC);
			} else {
				parsed_text += " ";
			}
		}
		if (para.is_hr) {
			parsed_text += String::chr(0xFFFC);
		}
		if (i < paragraphs.size() - 1) {
			parsed_text += "\n";
		}
		start += para.char_count + 1;

		const Vector<ListLevel> &lists = para.style.lists;
		if (!lists.is_empty()) {
			list_counters[lists[lists.size() - 1].id] += 1;
			String prefix;
			int segments = 0;
			for (int j = lists.size() - 1; j >= 0; j--) {
				const ListLevel &list = lists[j];
				if (list.type == LIST_DOTS) {
					if (segments == 0) {
						prefix = list.bullet;
					}
					break;
				}
				int number = list_counters.has(list.id) ? list_counters[list.id] : 1;
				String segment;
				if (list.type == LIST_NUMBERS) {
					segment = itos(number);
					if (owner && owner->is_localizing_numeral_system() && !language.is_empty()) {
						segment = TranslationServer::get_singleton()->format_number(segment, language);
					}
				} else if (list.type == LIST_LETTERS) {
					segment = _letters(number, list.capitalize);
				} else {
					segment = _roman(number, list.capitalize);
				}
				prefix = segment + "." + prefix;
				segments++;
			}
			para.prefix = prefix + " ";
		}
	}

	shape_dirty = true;
}

void BBCodeText::_clear_shaped() const {
	for (Paragraph &para : paragraphs) {
		for (const Line &line : para.lines) {
			TS->free_rid(line.rid);
		}
		para.lines.clear();
		if (para.text_rid.is_valid()) {
			TS->free_rid(para.text_rid);
			para.text_rid = RID();
		}
		if (para.dropcap_rid.is_valid()) {
			TS->free_rid(para.dropcap_rid);
			para.dropcap_rid = RID();
		}
		if (para.prefix_rid.is_valid()) {
			TS->free_rid(para.prefix_rid);
			para.prefix_rid = RID();
		}
	}
	total_line_count = 0;
}

// Shaping and line breaking.

Ref<Font> BBCodeText::_get_base_font(FontKind p_kind) const {
	if (base_fonts[p_kind].is_valid()) {
		return base_fonts[p_kind];
	}
	const Ref<Font> &normal = base_fonts[FONT_NORMAL];
	if (p_kind == FONT_NORMAL || p_kind == FONT_MONO || normal.is_null()) {
		return normal;
	}
	if (synthesized_fonts[p_kind].is_null()) {
		// Same as the default theme fonts of RichTextLabel.
		Ref<FontVariation> variation;
		variation.instantiate();
		variation->set_base_font(normal);
		if (p_kind == FONT_BOLD || p_kind == FONT_BOLD_ITALICS) {
			variation->set_variation_embolden(1.2);
		}
		if (p_kind == FONT_ITALICS || p_kind == FONT_BOLD_ITALICS) {
			variation->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
		}
		synthesized_fonts[p_kind] = variation;
	}
	return synthesized_fonts[p_kind];
}

Ref<Font> BBCodeText::_resolve_font(const SpanStyle &p_style) const {
	if (p_style.resolved_version == font_version && p_style.resolved_font.is_valid()) {
		return p_style.resolved_font;
	}
	Ref<Font> font = p_style.custom_font.is_valid() ? p_style.custom_font : _get_base_font(p_style.font_kind);
	for (const Ref<FontVariation> &variation : p_style.variations) {
		Ref<FontVariation> resolved = variation->duplicate();
		resolved->set_base_font(font);
		font = resolved;
	}
	p_style.resolved_font = font;
	p_style.resolved_version = font_version;
	return font;
}

int BBCodeText::_resolve_font_size(const SpanStyle &p_style) const {
	return p_style.font_size > 0 ? p_style.font_size : font_size;
}

Size2 BBCodeText::_get_object_size(const InlineObject &p_object) const {
	if (p_object.type == OBJECT_SPACER || p_object.texture.is_null()) {
		return Size2();
	}

	const int obj_font_size = _resolve_font_size(styles[p_object.style]);
	Size2 rq_size(p_object.width, p_object.height);
	if (p_object.width_unit == IMAGE_UNIT_PERCENT) {
		rq_size.width = MAX(0, width) * p_object.width / 100.f;
	} else if (p_object.width_unit == IMAGE_UNIT_EM) {
		rq_size.width = obj_font_size * p_object.width;
	}
	if (p_object.height_unit == IMAGE_UNIT_PERCENT) {
		rq_size.height = MAX(0, width) * p_object.height / 100.f;
	} else if (p_object.height_unit == IMAGE_UNIT_EM) {
		rq_size.height = obj_font_size * p_object.height;
	}

	// Keep the aspect ratio when only one side is set, see RichTextLabel::_get_image_size().
	Size2 source = p_object.region.has_area() ? p_object.region.size : p_object.texture->get_size();
	Size2 ret;
	if (rq_size.width > 0) {
		ret.width = rq_size.width;
		ret.height = rq_size.height > 0 ? rq_size.height : source.height * rq_size.width / source.width;
	} else if (rq_size.height > 0) {
		ret.height = rq_size.height;
		ret.width = source.width * rq_size.height / source.height;
	} else {
		ret = source;
	}
	return ret;
}

float BBCodeText::_get_tab_width() const {
	const Ref<Font> &font = base_fonts[FONT_NORMAL];
	if (font.is_null() || tab_size <= 0) {
		return 0;
	}
	return MAX(1, tab_size * (font->get_char_size(' ', font_size).width + font->get_spacing(TextServer::SPACING_SPACE)));
}

void BBCodeText::_shape_paragraph(Paragraph &p_para) const {
	const String &para_language = p_para.style.has_language ? p_para.style.language : language;
	TextServer::Direction para_direction = p_para.style.direction >= 0 ? (TextServer::Direction)p_para.style.direction : direction;

	p_para.text_rid = TS->create_shaped_text();
	TS->shaped_text_set_direction(p_para.text_rid, para_direction);

	int remaining = (visible_chars >= 0 && visible_chars_behavior == TextServer::VC_CHARS_BEFORE_SHAPING) ? visible_chars - p_para.char_start : INT_MAX;
	String full_text;
	int last_style = p_para.runs.is_empty() ? 0 : p_para.runs[0].style;
	if (!p_para.is_hr) {
		for (const Run &run : p_para.runs) {
			if (remaining <= 0) {
				break;
			}
			last_style = run.style;
			if (run.object >= 0) {
				const InlineObject &object = objects[run.object];
				TS->shaped_text_add_object(p_para.text_rid, run.object, _get_object_size(object), object.inline_align, 1);
				full_text += String::chr(0xFFFC);
				remaining--;
				continue;
			}
			const SpanStyle &style = styles[run.style];
			const String &run_language = style.language.is_empty() ? para_language : style.language;
			String run_text = uppercase ? TS->string_to_upper(run.text, run_language) : run.text;
			if (run_text.length() > remaining) {
				run_text = run_text.substr(0, remaining);
			}
			Ref<Font> font = _resolve_font(style);
			ERR_CONTINUE(font.is_null());
			TS->shaped_text_add_string(p_para.text_rid, run_text, font->get_rids(), _resolve_font_size(style), font->get_opentype_features(), run_language, run.style);
			full_text += run_text;
			remaining -= run_text.length();
		}
	}

	// Zero-width space keeps the line height of empty paragraphs, as in Label.
	{
		const SpanStyle &style = styles[last_style];
		Ref<Font> font = _resolve_font(style);
		if (font.is_valid()) {
			TS->shaped_text_add_string(p_para.text_rid, String::chr(0x200B), font->get_rids(), _resolve_font_size(style), font->get_opentype_features(), String(), last_style);
			full_text += String::chr(0x200B);
		}
	}

	TextServer::StructuredTextParser parser = p_para.style.st_parser >= 0 ? (TextServer::StructuredTextParser)p_para.style.st_parser : st_parser;
	if (owner) {
		TS->shaped_text_set_bidi_override(p_para.text_rid, owner->structured_text_parser(parser, st_args, full_text));
	}

	const PackedFloat32Array &para_tab_stops = p_para.style.has_tab_stops ? p_para.style.tab_stops : tab_stops;
	if (!para_tab_stops.is_empty()) {
		TS->shaped_text_tab_align(p_para.text_rid, para_tab_stops);
	}

	if (p_para.has_dropcap && !p_para.dropcap_text.is_empty()) {
		Ref<Font> font = p_para.dropcap_font.is_valid() ? p_para.dropcap_font : base_fonts[FONT_NORMAL];
		if (font.is_valid()) {
			p_para.dropcap_rid = TS->create_shaped_text();
			TS->shaped_text_set_direction(p_para.dropcap_rid, para_direction);
			TS->shaped_text_add_string(p_para.dropcap_rid, p_para.dropcap_text, font->get_rids(), p_para.dropcap_font_size > 0 ? p_para.dropcap_font_size : font_size * 3, font->get_opentype_features(), para_language);
		}
	}

	if (!p_para.prefix.is_empty()) {
		const SpanStyle &style = styles[p_para.runs.is_empty() ? 0 : p_para.runs[0].style];
		Ref<Font> font = _resolve_font(style);
		if (font.is_valid()) {
			p_para.prefix_rid = TS->create_shaped_text();
			TS->shaped_text_set_direction(p_para.prefix_rid, para_direction);
			TS->shaped_text_add_string(p_para.prefix_rid, p_para.prefix, font->get_rids(), _resolve_font_size(style), font->get_opentype_features(), para_language);
		}
	}
}

RID BBCodeText::_add_line(Paragraph &p_para, int p_start, int p_end, bool p_dropcap_shift, const PackedFloat32Array &p_tab_stops, PackedInt32Array &r_ranges) {
	Line line;
	line.rid = TS->shaped_text_substr(p_para.text_rid, p_start, p_end - p_start);
	line.dropcap_shift = p_dropcap_shift;
	if (!p_tab_stops.is_empty()) {
		TS->shaped_text_tab_align(line.rid, p_tab_stops);
	}
	p_para.lines.push_back(line);
	r_ranges.push_back(p_start);
	r_ranges.push_back(p_end);
	return line.rid;
}

void BBCodeText::_break_paragraph(Paragraph &p_para) const {
	for (const Line &line : p_para.lines) {
		TS->free_rid(line.rid);
	}
	p_para.lines.clear();

	// Collapse the spacers before breaking, they are expanded per line afterwards.
	for (const Run &run : p_para.runs) {
		if (run.object >= 0 && objects[run.object].type == OBJECT_SPACER && TS->shaped_text_has_object(p_para.text_rid, run.object)) {
			TS->shaped_text_resize_object(p_para.text_rid, run.object, Size2(), INLINE_ALIGNMENT_CENTER);
		}
	}
	p_para.unwrapped_width = TS->shaped_text_get_size(p_para.text_rid).x;

	BitField<TextServer::LineBreakFlag> autowrap_flags = TextServer::BREAK_MANDATORY;
	if (width >= 0) {
		switch (autowrap_mode) {
			case TextServer::AUTOWRAP_WORD_SMART:
				autowrap_flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_ADAPTIVE | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_WORD:
				autowrap_flags = TextServer::BREAK_WORD_BOUND | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_ARBITRARY:
				autowrap_flags = TextServer::BREAK_GRAPHEME_BOUND | TextServer::BREAK_MANDATORY;
				break;
			case TextServer::AUTOWRAP_OFF:
				break;
		}
	}
	autowrap_flags = autowrap_flags | autowrap_flags_trim;

	const float line_width = width >= 0 ? MAX(1.0f, width - p_para.margin) : 0.0f;
	const PackedFloat32Array &para_tab_stops = p_para.style.has_tab_stops ? p_para.style.tab_stops : tab_stops;

	PackedInt32Array ranges;
	int start = 0;
	Size2i range = TS->shaped_text_get_range(p_para.text_rid);
	if (p_para.dropcap_rid.is_valid()) {
		// Flow around the dropcap, see TextParagraph::_shape_lines().
		Size2 dc_size = TS->shaped_text_get_size(p_para.dropcap_rid);
		float h_offset = dc_size.x + p_para.dropcap_margins.size.x + p_para.dropcap_margins.position.x;
		float v_offset = dc_size.y + p_para.dropcap_margins.size.y + p_para.dropcap_margins.position.y;
		PackedInt32Array line_breaks = TS->shaped_text_get_line_breaks(p_para.text_rid, width >= 0 ? MAX(1.0f, line_width - h_offset) : 0.0f, 0, autowrap_flags);
		for (int i = 0; i < line_breaks.size(); i = i + 2) {
			RID line = _add_line(p_para, line_breaks[i], line_breaks[i + 1], true, para_tab_stops, ranges);
			float h = TS->shaped_text_get_size(line).y + line_spacing;
			start = (i < line_breaks.size() - 2) ? line_breaks[i + 2] : range.y;
			if (v_offset < h) {
				break;
			}
			v_offset -= h;
		}
	}
	if (start == 0 || start < range.y) {
		PackedInt32Array line_breaks = TS->shaped_text_get_line_breaks(p_para.text_rid, line_width, start, autowrap_flags);
		for (int i = 0; i < line_breaks.size(); i = i + 2) {
			_add_line(p_para, line_breaks[i], line_breaks[i + 1], false, para_tab_stops, ranges);
		}
	}

	_fit_spacers(p_para, ranges);
}

void BBCodeText::_fit_spacers(Paragraph &p_para, const PackedInt32Array &p_ranges) const {
	bool resized = false;
	for (int i = 0; i < p_para.lines.size(); i++) {
		Line &line = p_para.lines.write[i];
		line.natural_width = TS->shaped_text_get_size(line.rid).x;

		float total_weight = 0;
		Array keys = TS->shaped_text_get_objects(line.rid);
		for (const Variant &key : keys) {
			if (objects[(int)key].type == OBJECT_SPACER) {
				line.has_spacers = true;
				total_weight += objects[(int)key].weight;
			}
		}
		const float free_width = _get_line_available_width(p_para, line) - line.natural_width;
		if (!line.has_spacers || width < 0 || free_width <= 0 || total_weight <= 0) {
			continue;
		}
		for (const Variant &key : keys) {
			if (objects[(int)key].type == OBJECT_SPACER) {
				TS->shaped_text_resize_object(p_para.text_rid, key, Size2(Math::floor(free_width * objects[(int)key].weight / total_weight), 0), INLINE_ALIGNMENT_CENTER);
				resized = true;
			}
		}
	}
	if (!resized) {
		return;
	}

	// Cut the lines again at the same places, now with the expanded spacers.
	const PackedFloat32Array &para_tab_stops = p_para.style.has_tab_stops ? p_para.style.tab_stops : tab_stops;
	for (int i = 0; i < p_para.lines.size(); i++) {
		Line &line = p_para.lines.write[i];
		TS->free_rid(line.rid);
		line.rid = TS->shaped_text_substr(p_para.text_rid, p_ranges[i * 2], p_ranges[i * 2 + 1] - p_ranges[i * 2]);
		if (!para_tab_stops.is_empty()) {
			TS->shaped_text_tab_align(line.rid, para_tab_stops);
		}
	}
}

void BBCodeText::_trim_lines() const {
	const int visible_lines = get_visible_line_count();
	const bool lines_hidden = visible_lines > 0 && visible_lines < total_line_count;
	const char32_t ellipsis = (el_char.length() > 0) ? el_char[0] : 0x2026;

	int line_index = 0;
	for (Paragraph &para : paragraphs) {
		BitField<TextServer::TextOverrunFlag> overrun_flags = TextServer::get_overrun_flags_from_behavior(overrun_behavior);
		BitField<TextServer::JustificationFlag> line_jst_flags = para.style.has_jst_flags ? para.style.jst_flags : jst_flags;
		const PackedFloat32Array &para_tab_stops = para.style.has_tab_stops ? para.style.tab_stops : tab_stops;
		if (!para_tab_stops.is_empty()) {
			line_jst_flags.set_flag(TextServer::JUSTIFICATION_AFTER_LAST_TAB);
		}
		const bool fill = _get_alignment(para) == HORIZONTAL_ALIGNMENT_FILL;
		const int line_count = para.lines.size();

		int jst_to_line = line_count;
		if (fill && !(line_count == 1 && line_jst_flags.has_flag(TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE))) {
			if (line_jst_flags.has_flag(TextServer::JUSTIFICATION_SKIP_LAST_LINE)) {
				jst_to_line = line_count - 1;
			}
			if (line_jst_flags.has_flag(TextServer::JUSTIFICATION_SKIP_LAST_LINE_WITH_VISIBLE_CHARS)) {
				for (int i = line_count - 1; i >= 0; i--) {
					if (TS->shaped_text_has_visible_chars(para.lines[i].rid)) {
						jst_to_line = i;
						break;
					}
				}
			}
		}

		for (int i = 0; i < line_count && width >= 0; i++) {
			const Line &line = para.lines[i];
			const float line_w = _get_line_available_width(para, line);
			const bool can_fill = fill && i < jst_to_line && !line.has_spacers;
			if (autowrap_mode != TextServer::AUTOWRAP_OFF) {
				if (lines_hidden) {
					overrun_flags.set_flag(TextServer::OVERRUN_ENFORCE_ELLIPSIS);
				}
				if (can_fill) {
					TS->shaped_text_fit_to_width(line.rid, line_w, line_jst_flags);
				} else if (lines_hidden && i == visible_lines - line_index - 1) {
					TS->shaped_text_set_custom_ellipsis(line.rid, ellipsis);
					TS->shaped_text_overrun_trim_to_width(line.rid, line_w, overrun_flags);
				}
			} else if (can_fill) {
				TS->shaped_text_fit_to_width(line.rid, line_w, line_jst_flags);
				overrun_flags.set_flag(TextServer::OVERRUN_JUSTIFICATION_AWARE);
				TS->shaped_text_set_custom_ellipsis(line.rid, ellipsis);
				TS->shaped_text_overrun_trim_to_width(line.rid, line_w, overrun_flags);
				TS->shaped_text_fit_to_width(line.rid, line_w, line_jst_flags | TextServer::JUSTIFICATION_CONSTRAIN_ELLIPSIS);
			} else {
				TS->shaped_text_set_custom_ellipsis(line.rid, ellipsis);
				TS->shaped_text_overrun_trim_to_width(line.rid, line_w, overrun_flags);
			}
		}
		line_index += line_count;
	}
}

void BBCodeText::_ensure_shaped() const {
	// When a shaped text is invalidated by an external source, reshape it.
	for (const Paragraph &para : paragraphs) {
		if (para.text_rid.is_valid() && !TS->shaped_text_is_ready(para.text_rid)) {
			shape_dirty = true;
			break;
		}
		for (const Line &line : para.lines) {
			if (!TS->shaped_text_is_ready(line.rid)) {
				lines_dirty = true;
				break;
			}
		}
	}

	if (!shape_dirty && !lines_dirty) {
		return;
	}

	if (base_fonts[FONT_NORMAL].is_null()) {
		return;
	}

	if (shape_dirty) {
		_clear_shaped();
		for (Paragraph &para : paragraphs) {
			if (visible_chars >= 0 && visible_chars_behavior == TextServer::VC_CHARS_BEFORE_SHAPING && para.char_start > visible_chars) {
				continue;
			}
			_shape_paragraph(para);
		}

		// Indents and list markers, all items of a list share the widest marker.
		HashMap<int, float> marker_widths;
		for (const Paragraph &para : paragraphs) {
			if (para.prefix_rid.is_valid()) {
				int id = para.style.lists[para.style.lists.size() - 1].id;
				marker_widths[id] = MAX(marker_widths.has(id) ? marker_widths[id] : 0.0f, (float)TS->shaped_text_get_size(para.prefix_rid).x);
			}
		}
		const float tab_width = _get_tab_width();
		for (Paragraph &para : paragraphs) {
			para.margin = tab_width * (para.style.indent + para.style.lists.size());
			if (!para.style.lists.is_empty()) {
				int id = para.style.lists[para.style.lists.size() - 1].id;
				para.margin += marker_widths.has(id) ? marker_widths[id] : 0.0f;
			}
		}
		shape_dirty = false;
		lines_dirty = true;
	}

	total_line_count = 0;
	for (Paragraph &para : paragraphs) {
		if (para.text_rid.is_valid()) {
			_break_paragraph(para);
		}
		total_line_count += para.lines.size();
	}
	_trim_lines();
	lines_dirty = false;
}

// Layout.

int BBCodeText::_get_font_height() const {
	const Ref<Font> &font = base_fonts[FONT_NORMAL];
	return font.is_valid() ? font->get_height(font_size) : 0;
}

void BBCodeText::_get_line_metrics(RID p_rid, double &r_ascent, double &r_descent) const {
	// Lines are at least as high as the base font, as in Label.
	int font_h = _get_font_height();
	r_ascent = TS->shaped_text_get_ascent(p_rid);
	r_descent = TS->shaped_text_get_descent(p_rid);
	if (r_ascent + r_descent < font_h) {
		double diff = font_h - (r_ascent + r_descent);
		r_ascent += diff / 2;
		r_descent += diff - (diff / 2);
	}
}

float BBCodeText::_get_line_available_width(const Paragraph &p_para, const Line &p_line) const {
	float ret = width - p_para.margin;
	if (p_line.dropcap_shift && p_para.dropcap_rid.is_valid()) {
		ret -= TS->shaped_text_get_size(p_para.dropcap_rid).x + p_para.dropcap_margins.size.x + p_para.dropcap_margins.position.x;
	}
	return MAX(1.0f, ret);
}

HorizontalAlignment BBCodeText::_get_alignment(const Paragraph &p_para) const {
	return p_para.style.alignment >= 0 ? (HorizontalAlignment)p_para.style.alignment : horizontal_alignment;
}

bool BBCodeText::_is_paragraph_rtl(const Paragraph &p_para) const {
	return p_para.text_rid.is_valid() && TS->shaped_text_get_inferred_direction(p_para.text_rid) == TextServer::DIRECTION_RTL;
}

float BBCodeText::_get_line_x(const Paragraph &p_para, int p_line, const Rect2 &p_rect) const {
	const Line &line = p_para.lines[p_line];
	const bool rtl = _is_paragraph_rtl(p_para);
	float shift = 0;
	if (line.dropcap_shift && p_para.dropcap_rid.is_valid()) {
		shift = TS->shaped_text_get_size(p_para.dropcap_rid).x + p_para.dropcap_margins.size.x + p_para.dropcap_margins.position.x;
	}
	const float avail = p_rect.size.width - p_para.margin - shift;
	const float line_w = TS->shaped_text_get_size(line.rid).x;

	float x = p_rect.position.x + (rtl ? 0.0f : p_para.margin + shift);
	switch (_get_alignment(p_para)) {
		case HORIZONTAL_ALIGNMENT_FILL:
			if (rtl && autowrap_mode != TextServer::AUTOWRAP_OFF) {
				x += avail - line_w;
			}
			break;
		case HORIZONTAL_ALIGNMENT_LEFT:
			if (layout_rtl) {
				x += avail - line_w;
			}
			break;
		case HORIZONTAL_ALIGNMENT_CENTER:
			x += int(avail - line_w) / 2;
			break;
		case HORIZONTAL_ALIGNMENT_RIGHT:
			if (!layout_rtl) {
				x += avail - line_w;
			}
			break;
	}
	return x;
}

int BBCodeText::_get_layout(const Rect2 &p_rect, VerticalAlignment p_valign, float &r_offset_y, int &r_last_line, float &r_line_spacing) const {
	// Same as Label::get_layout_data(), on the content rect.
	float total_h = 0.0;
	int lines_visible = 0;

	int line_index = 0;
	for (const Paragraph &para : paragraphs) {
		if (line_index + para.lines.size() <= lines_skipped) {
			line_index += para.lines.size();
		} else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			for (int i = start; i < para.lines.size(); i++) {
				double asc, dsc;
				_get_line_metrics(para.lines[i].rid, asc, dsc);
				total_h += asc + dsc + line_spacing;
				if (total_h > Math::ceil(p_rect.size.height + line_spacing)) {
					break;
				}
				lines_visible++;
			}
			total_h += paragraph_spacing;
			line_index += para.lines.size();
		}
	}

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	r_last_line = MIN(total_line_count, lines_visible + lines_skipped);

	int total_glyphs = 0;
	total_h = 0;
	line_index = 0;
	for (const Paragraph &para : paragraphs) {
		if (line_index + para.lines.size() <= lines_skipped) {
			line_index += para.lines.size();
		} else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines.size() < r_last_line) ? para.lines.size() : r_last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				double asc, dsc;
				_get_line_metrics(para.lines[i].rid, asc, dsc);
				total_h += asc + dsc + line_spacing;
				total_glyphs += TS->shaped_text_get_glyph_count(para.lines[i].rid) + TS->shaped_text_get_ellipsis_glyph_count(para.lines[i].rid);
			}
			total_h += paragraph_spacing;
			line_index += para.lines.size();
		}
	}

	float vbegin = 0;
	float vsep = 0;
	if (lines_visible > 0) {
		const float text_h = total_h - line_spacing - paragraph_spacing;
		switch (p_valign) {
			case VERTICAL_ALIGNMENT_TOP:
				break;
			case VERTICAL_ALIGNMENT_CENTER:
				vbegin = int(p_rect.size.y - text_h) / 2;
				break;
			case VERTICAL_ALIGNMENT_BOTTOM:
				vbegin = int(p_rect.size.y - text_h);
				break;
			case VERTICAL_ALIGNMENT_FILL:
				if (lines_visible > 1) {
					vsep = int(p_rect.size.y - text_h) / (lines_visible - 1);
				}
				break;
		}
	}
	r_offset_y = p_rect.position.y + vbegin;
	r_line_spacing = line_spacing + vsep;
	return total_glyphs;
}

// Drawing.

Color BBCodeText::_get_style_color(int p_style, const DrawStyle &p_draw) const {
	const SpanStyle &style = styles[p_style];
	return style.has_color ? style.color : p_draw.font_color;
}

int BBCodeText::_get_glyph_style(RID p_line, const Glyph &p_glyph) const {
	if (p_glyph.span_index < 0) {
		return 0;
	}
	if ((p_glyph.flags & TextServer::GRAPHEME_IS_EMBEDDED_OBJECT) == TextServer::GRAPHEME_IS_EMBEDDED_OBJECT) {
		Variant key = TS->shaped_get_span_embedded_object(p_line, p_glyph.span_index);
		if (key.get_type() == Variant::INT && (int)key >= 0 && (int)key < objects.size()) {
			return objects[(int)key].style;
		}
		return 0;
	}
	Variant meta = TS->shaped_get_span_meta(p_line, p_glyph.span_index);
	if (meta.get_type() == Variant::INT && (int)meta >= 0 && (int)meta < styles.size()) {
		return meta;
	}
	return 0;
}

// The drawn glyphs of a line in visual order, with the ellipsis in place.
void BBCodeText::_place_line_glyphs(RID p_rid, bool p_rtl, const Vector2 &p_ofs, const Vector<int> &p_styles, const Trim &p_trim, int p_processed, LocalVector<PlacedGlyph> &r_glyphs) {
	const Glyph *glyphs = TS->shaped_text_get_glyphs(p_rid);
	const int gl_size = TS->shaped_text_get_glyph_count(p_rid);
	const int trim_pos = TS->shaped_text_get_trim_pos(p_rid);
	const int ellipsis_pos = TS->shaped_text_get_ellipsis_pos(p_rid);
	const Glyph *ellipsis_glyphs = TS->shaped_text_get_ellipsis_glyphs(p_rid);
	const int ellipsis_gl_size = TS->shaped_text_get_ellipsis_glyph_count(p_rid);

	int ellipsis_style = 0;
	for (int j = gl_size - 1; j >= 0; j--) {
		if (trim_pos < 0 || (p_rtl ? j >= trim_pos : j < trim_pos)) {
			ellipsis_style = p_styles[j];
			break;
		}
	}

	r_glyphs.clear();
	int processed = p_processed;
	Vector2 ofs = p_ofs;
	if (p_rtl && ellipsis_pos >= 0) {
		for (int i = ellipsis_gl_size - 1; i >= 0; i--) {
			for (int j = 0; j < ellipsis_glyphs[i].repeat; j++) {
				r_glyphs.push_back({ &ellipsis_glyphs[i], ellipsis_style, ofs, p_trim.is_skipped(ellipsis_glyphs[i], processed) });
				processed++;
				ofs.x += ellipsis_glyphs[i].advance;
			}
		}
	}
	for (int i = 0; i < gl_size; i++) {
		if (trim_pos >= 0) {
			if (p_rtl) {
				if (i < trim_pos) {
					continue;
				}
			} else if (i >= trim_pos) {
				break;
			}
		}
		for (int j = 0; j < glyphs[i].repeat; j++) {
			r_glyphs.push_back({ &glyphs[i], p_styles[i], ofs, p_trim.is_skipped(glyphs[i], processed) });
			processed++;
			ofs.x += glyphs[i].advance;
		}
	}
	if (!p_rtl && ellipsis_pos >= 0) {
		for (int i = 0; i < ellipsis_gl_size; i++) {
			for (int j = 0; j < ellipsis_glyphs[i].repeat; j++) {
				r_glyphs.push_back({ &ellipsis_glyphs[i], ellipsis_style, ofs, p_trim.is_skipped(ellipsis_glyphs[i], processed) });
				processed++;
				ofs.x += ellipsis_glyphs[i].advance;
			}
		}
	}
}

bool BBCodeText::_is_glyph_drawn(const PlacedGlyph &p_glyph, const DrawStyle &p_style) const {
	return !p_glyph.skipped && _get_style_color(p_glyph.style, p_style).a != 0;
}

void BBCodeText::_draw_box(RID p_target, float p_start, float p_end, const Color &p_color, const Vector2 &p_ofs, double p_ascent, double p_line_height, const DrawStyle &p_style) {
	if (p_color.a <= 0) {
		return;
	}
	const Vector2 pos(p_start - p_style.highlight_h_padding, p_ofs.y - p_ascent - p_style.highlight_v_padding);
	const Vector2 size(p_end - p_start + 2 * p_style.highlight_h_padding, p_line_height + 2 * p_style.highlight_v_padding);
	RenderingServer::get_singleton()->canvas_item_add_rect(p_target, Rect2(pos, size), p_color);
}

// Highlight boxes of [bgcolor] or [fgcolor], merged over glyphs of the same color.
void BBCodeText::_draw_boxes(const LocalVector<PlacedGlyph> &p_glyphs, RID p_target, bool p_foreground, const Vector2 &p_ofs, double p_ascent, double p_line_height, const DrawStyle &p_style) const {
	float box_start = 0;
	float box_end = p_ofs.x;
	Color box_color(0, 0, 0, 0);
	for (const PlacedGlyph &placed : p_glyphs) {
		const SpanStyle &style = styles[placed.style];
		const Color color = placed.skipped ? Color(0, 0, 0, 0) : (p_foreground ? style.fgcolor : style.bgcolor);
		if (color != box_color) {
			_draw_box(p_target, box_start, placed.pos.x, box_color, p_ofs, p_ascent, p_line_height, p_style);
			box_start = placed.pos.x;
			box_color = color;
		}
		box_end = placed.pos.x + placed.glyph->advance;
	}
	_draw_box(p_target, box_start, box_end, box_color, p_ofs, p_ascent, p_line_height, p_style);
}

static void _draw_dashed_line(RID p_ci, const Vector2 &p_from, const Vector2 &p_to, const Color &p_color, float p_width, float p_dash) {
	const float length = p_from.distance_to(p_to);
	if (length <= 0) {
		return;
	}
	const Vector2 step = (p_to - p_from) / length;
	for (float x = 0; x < length; x += p_dash * 2) {
		RenderingServer::get_singleton()->canvas_item_add_line(p_ci, p_from + step * x, p_from + step * MIN(x + p_dash, length), p_color, p_width);
	}
}

BBCodeText::Trim BBCodeText::_make_trim(const Paragraph &p_para, int p_total_glyphs) const {
	Trim trim;
	trim.trim_chars = (visible_chars >= 0) && (visible_chars_behavior == TextServer::VC_CHARS_AFTER_SHAPING);
	trim.trim_glyphs_ltr = (visible_chars >= 0) && ((visible_chars_behavior == TextServer::VC_GLYPHS_LTR) || ((visible_chars_behavior == TextServer::VC_GLYPHS_AUTO) && !layout_rtl));
	trim.trim_glyphs_rtl = (visible_chars >= 0) && ((visible_chars_behavior == TextServer::VC_GLYPHS_RTL) || ((visible_chars_behavior == TextServer::VC_GLYPHS_AUTO) && layout_rtl));
	trim.visible_chars = visible_chars;
	trim.total_glyphs = p_total_glyphs;
	trim.visible_glyphs = p_total_glyphs * visible_ratio;
	trim.char_start = p_para.char_start;
	return trim;
}

Vector<int> BBCodeText::_get_glyph_styles(RID p_rid) const {
	const Glyph *glyphs = TS->shaped_text_get_glyphs(p_rid);
	const int gl_size = TS->shaped_text_get_glyph_count(p_rid);
	Vector<int> gl_styles;
	gl_styles.resize(gl_size);
	for (int i = 0; i < gl_size; i++) {
		gl_styles.write[i] = _get_glyph_style(p_rid, glyphs[i]);
	}
	return gl_styles;
}

// Whether an inline object is hidden by the visible characters, like its glyphs.
bool BBCodeText::_is_object_skipped(const Trim &p_trim, const Paragraph &p_para, RID p_rid, const Variant &p_key, int p_processed) const {
	if (p_trim.trim_chars && p_para.char_start + TS->shaped_text_get_object_range(p_rid, p_key).y > p_trim.visible_chars) {
		return true;
	}
	if (p_trim.trim_glyphs_ltr || p_trim.trim_glyphs_rtl) {
		const int obj_glyph = p_processed + TS->shaped_text_get_object_glyph(p_rid, p_key);
		return (p_trim.trim_glyphs_ltr && obj_glyph >= p_trim.visible_glyphs) || (p_trim.trim_glyphs_rtl && obj_glyph < p_trim.total_glyphs - p_trim.visible_glyphs);
	}
	return false;
}

void BBCodeText::_draw_line(RID p_ci, const Paragraph &p_para, int p_line, const Vector2 &p_ofs, const DrawStyle &p_style, int &r_processed_glyphs, int p_total_glyphs) const {
	const RID rid = p_para.lines[p_line].rid;
	const bool rtl = _is_paragraph_rtl(p_para);
	const Trim trim = _make_trim(p_para, p_total_glyphs);
	const Vector<int> gl_styles = _get_glyph_styles(rid);

	const double ascent = TS->shaped_text_get_ascent(rid);
	const double line_h = TS->shaped_text_get_size(rid).y;
	const float upos = TS->shaped_text_get_underline_position(rid);
	const float uthk = MAX(1.0f, (float)TS->shaped_text_get_underline_thickness(rid));
	const int processed = r_processed_glyphs;
	LocalVector<PlacedGlyph> placed_glyphs;
	_place_line_glyphs(rid, rtl, p_ofs, gl_styles, trim, processed, placed_glyphs);

	_draw_boxes(placed_glyphs, p_ci, false, p_ofs, ascent, line_h, p_style);

	if (p_style.shadow_color.a > 0) {
		if (p_style.shadow_outline_size > 0) {
			_draw_glyph_effect(placed_glyphs, p_ci, p_style.shadow_outline_size, false, p_style.shadow_color, p_style.shadow_offset, p_style);
		}
		_draw_glyph_effect(placed_glyphs, p_ci, 0, true, p_style.shadow_color, p_style.shadow_offset, p_style);
	}

	for (int i = p_style.stacked_shadows.size() - 1; i >= 0; i--) {
		const LabelSettings::StackedShadowData &shadow = p_style.stacked_shadows[i];
		_draw_glyph_effect(placed_glyphs, p_ci, shadow.outline_size, true, shadow.color, shadow.offset, p_style);
	}

	if (!p_style.stacked_outlines.is_empty()) {
		int stacked_size = p_style.outline_size;
		for (const LabelSettings::StackedOutlineData &outline : p_style.stacked_outlines) {
			stacked_size += MAX(0, outline.size);
		}
		for (int i = p_style.stacked_outlines.size() - 1; i >= 0; i--) {
			const LabelSettings::StackedOutlineData &outline = p_style.stacked_outlines[i];
			if (outline.size <= 0) {
				continue;
			}
			_draw_glyph_effect(placed_glyphs, p_ci, stacked_size, false, outline.color, Vector2(), p_style);
			stacked_size -= outline.size;
		}
	}

	// The outline, which tags can change.
	for (const PlacedGlyph &placed : placed_glyphs) {
		const Glyph &glyph = *placed.glyph;
		const SpanStyle &style = styles[placed.style];
		const int size = style.outline_size >= 0 ? style.outline_size : p_style.outline_size;
		const Color color = style.has_outline_color ? style.outline_color : p_style.outline_color;
		if (_is_glyph_drawn(placed, p_style) && glyph.font_rid.is_valid() && size > 0 && color.a != 0) {
			TS->font_draw_glyph_outline(glyph.font_rid, p_ci, glyph.font_size, size, placed.pos + Vector2(glyph.x_off, glyph.y_off), glyph.index, color);
		}
	}

	// Inline images.
	Array keys = TS->shaped_text_get_objects(rid);
	for (const Variant &key : keys) {
		const InlineObject &object = objects[(int)key];
		if (object.type != OBJECT_IMAGE || object.texture.is_null()) {
			continue;
		}
		if (_is_object_skipped(trim, p_para, rid, key, processed)) {
			continue;
		}
		Rect2 rect = TS->shaped_text_get_object_rect(rid, key);
		rect.position += p_ofs;
		if (object.pad) {
			Size2 pad_size = rect.size.min(object.texture->get_size());
			rect.position += (rect.size - pad_size) / 2;
			rect.size = pad_size;
		}
		if (object.region.has_area()) {
			object.texture->draw_rect_region(p_ci, rect, object.region, object.color);
		} else {
			object.texture->draw_rect(p_ci, rect, false, object.color);
		}
	}

	for (const PlacedGlyph &placed : placed_glyphs) {
		if (!_is_glyph_drawn(placed, p_style)) {
			continue;
		}
		const Glyph &glyph = *placed.glyph;
		const Vector2 pos = placed.pos + Vector2(glyph.x_off, glyph.y_off);
		const Color color = _get_style_color(placed.style, p_style);
		if (glyph.font_rid.is_valid()) {
			TS->font_draw_glyph(glyph.font_rid, p_ci, glyph.font_size, pos, glyph.index, color);
		} else if (((glyph.flags & TextServer::GRAPHEME_IS_VIRTUAL) != TextServer::GRAPHEME_IS_VIRTUAL) && ((glyph.flags & TextServer::GRAPHEME_IS_EMBEDDED_OBJECT) != TextServer::GRAPHEME_IS_EMBEDDED_OBJECT)) {
			TS->draw_hex_code_box(p_ci, glyph.font_size, pos, glyph.index, color);
		}
	}

	_draw_decorations(placed_glyphs, p_ci, p_ofs, ascent, line_h, upos, uthk, p_style);
	_draw_boxes(placed_glyphs, p_ci, true, p_ofs, ascent, line_h, p_style);

	r_processed_glyphs = processed + placed_glyphs.size();
}

// Outlines and shadows: the outline of the glyphs, and the glyphs too when filled.
void BBCodeText::_draw_glyph_effect(const LocalVector<PlacedGlyph> &p_glyphs, RID p_ci, int p_outline_size, bool p_fill, const Color &p_color, const Vector2 &p_offset, const DrawStyle &p_style) const {
	for (const PlacedGlyph &placed : p_glyphs) {
		const Glyph &glyph = *placed.glyph;
		if (!_is_glyph_drawn(placed, p_style) || !glyph.font_rid.is_valid() || p_color.a == 0) {
			continue;
		}
		const Vector2 pos = placed.pos + Vector2(glyph.x_off, glyph.y_off) + p_offset;
		if (p_outline_size > 0) {
			TS->font_draw_glyph_outline(glyph.font_rid, p_ci, glyph.font_size, p_outline_size, pos, glyph.index, p_color);
		}
		if (p_fill) {
			TS->font_draw_glyph(glyph.font_rid, p_ci, glyph.font_size, pos, glyph.index, p_color);
		}
	}
}

void BBCodeText::_draw_decoration(RID p_target, DecorationKind p_kind, float p_start, float p_end, float p_y, const Color &p_color, float p_thickness) {
	if (p_kind == DECORATION_HINT) {
		_draw_dashed_line(p_target, Vector2(p_start, p_y), Vector2(p_end, p_y), p_color, p_thickness, MAX(2.0f, p_thickness * 2));
	} else {
		RenderingServer::get_singleton()->canvas_item_add_line(p_target, Vector2(p_start, p_y), Vector2(p_end, p_y), p_color, p_thickness);
	}
}

// Underlines, strikethroughs and hint underlines, merged over glyphs of the same color.
void BBCodeText::_draw_decorations(const LocalVector<PlacedGlyph> &p_glyphs, RID p_target, const Vector2 &p_ofs, double p_ascent, double p_line_height, float p_underline_position, float p_underline_thickness, const DrawStyle &p_style) const {
	for (int kind = 0; kind < DECORATION_MAX; kind++) {
		bool started = false;
		float start_x = 0;
		float end_x = 0;
		Color line_color;
		const float y = p_ofs.y + (kind == DECORATION_STRIKETHROUGH ? (-p_ascent + p_line_height / 2) : p_underline_position);
		for (const PlacedGlyph &placed : p_glyphs) {
			const SpanStyle &style = styles[placed.style];
			const Color font_color = _get_style_color(placed.style, p_style);
			bool has_line = false;
			Color color;
			if (kind == DECORATION_UNDERLINE) {
				has_line = style.underline;
				if (!has_line && style.meta >= 0) {
					const Meta &meta = metas[style.meta];
					has_line = meta.underline == META_UNDERLINE_ALWAYS || (meta.underline == META_UNDERLINE_ON_HOVER && style.meta == hovered_meta);
				}
				color = (style.underline && style.underline_color.a != 0) ? style.underline_color : Color(font_color.r, font_color.g, font_color.b, font_color.a * p_style.underline_alpha / 100.0);
			} else if (kind == DECORATION_STRIKETHROUGH) {
				has_line = style.strikethrough;
				color = style.strikethrough_color.a != 0 ? style.strikethrough_color : Color(font_color.r, font_color.g, font_color.b, font_color.a * p_style.strikethrough_alpha / 100.0);
			} else {
				has_line = style.hint >= 0;
				color = Color(font_color.r, font_color.g, font_color.b, font_color.a * p_style.underline_alpha / 100.0);
			}
			if (started && (placed.skipped || !has_line || color != line_color)) {
				_draw_decoration(p_target, DecorationKind(kind), start_x, end_x, y, line_color, p_underline_thickness);
				started = false;
			}
			if (!placed.skipped && has_line) {
				if (!started) {
					started = true;
					start_x = placed.pos.x;
					line_color = color;
				}
				end_x = placed.pos.x + placed.glyph->advance;
			}
		}
		if (started) {
			_draw_decoration(p_target, DecorationKind(kind), start_x, end_x, y, line_color, p_underline_thickness);
		}
	}
}

// Hit testing.

bool BBCodeText::_find_style_at(const Rect2 &p_rect, VerticalAlignment p_valign, const Point2 &p_pos, int &r_style, int &r_object) const {
	_ensure_shaped();
	r_style = -1;
	r_object = -1;

	float ofs_y;
	int last_line;
	float ls;
	const int total_glyphs = _get_layout(p_rect, p_valign, ofs_y, last_line, ls);
	// Only the glyphs that are drawn react, so hidden visible characters have no links or hints.
	int processed_glyphs = 0;
	LocalVector<PlacedGlyph> placed_glyphs;

	int line_index = 0;
	for (const Paragraph &para : paragraphs) {
		if (line_index + para.lines.size() <= lines_skipped) {
			line_index += para.lines.size();
			continue;
		}
		int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
		int end = (line_index + para.lines.size() < last_line) ? para.lines.size() : last_line - line_index;
		if (end <= 0) {
			break;
		}
		const Trim trim = _make_trim(para, total_glyphs);
		const bool rtl = _is_paragraph_rtl(para);
		for (int i = start; i < end; i++) {
			const RID rid = para.lines[i].rid;
			double asc, dsc;
			_get_line_metrics(rid, asc, dsc);
			const Vector2 ofs(_get_line_x(para, i, p_rect), ofs_y + asc);
			_place_line_glyphs(rid, rtl, ofs, _get_glyph_styles(rid), trim, processed_glyphs, placed_glyphs);

			if (p_pos.y >= ofs_y && p_pos.y < ofs_y + asc + dsc) {
				Array keys = TS->shaped_text_get_objects(rid);
				for (const Variant &key : keys) {
					Rect2 rect = TS->shaped_text_get_object_rect(rid, key);
					rect.position += ofs;
					if (objects[(int)key].type == OBJECT_IMAGE && rect.has_point(p_pos) && !_is_object_skipped(trim, para, rid, key, processed_glyphs)) {
						r_object = key;
						r_style = objects[(int)key].style;
						return true;
					}
				}

				for (const PlacedGlyph &placed : placed_glyphs) {
					if (!placed.skipped && p_pos.x >= placed.pos.x && p_pos.x < placed.pos.x + placed.glyph->advance) {
						r_style = placed.style;
						return true;
					}
				}
				return false;
			}
			processed_glyphs += placed_glyphs.size();
			ofs_y += asc + dsc + ls;
		}
		ofs_y += paragraph_spacing;
		line_index += para.lines.size();
	}
	return false;
}

// Public API.

void BBCodeText::bind_theme_fonts(const StringName &p_class) {
	for (const char *name : theme_font_names) {
		ThemeDB::get_singleton()->bind_class_item(Theme::DATA_TYPE_FONT, p_class, name, name, [](Node *, const StringName &, const StringName &) {});
	}
}

// Missing theme fonts fall back to the default font of the theme, they're synthesized from the normal font instead.
// The font of the control isn't compared, as it can differ from the default font.
template <typename T>
static void _get_theme_fonts(const T *p_node, Ref<Font> r_fonts[4]) {
	const Ref<Font> default_font = p_node->get_theme_default_font();
	for (int i = 0; i < 4; i++) {
		const Ref<Font> font = p_node->get_theme_font(theme_font_names[i]);
		if (font.is_valid() && font != default_font && font != ThemeDB::get_singleton()->get_fallback_font()) {
			r_fonts[i] = font;
		}
	}
}

void BBCodeText::update_theme_fonts(const Control *p_control, const Ref<Font> &p_normal_font, int p_font_size) {
	Ref<Font> fonts[4];
	_get_theme_fonts(p_control, fonts);
	set_fonts(p_normal_font, fonts[0], fonts[1], fonts[2], fonts[3], p_font_size);
}

void BBCodeText::update_theme_fonts(const Window *p_window, const Ref<Font> &p_normal_font, int p_font_size) {
	Ref<Font> fonts[4];
	_get_theme_fonts(p_window, fonts);
	set_fonts(p_normal_font, fonts[0], fonts[1], fonts[2], fonts[3], p_font_size);
}

void BBCodeText::set_owner(const Control *p_owner) {
	owner = p_owner;
}

void BBCodeText::set_text(const String &p_bbcode) {
	if (text == p_bbcode && !paragraphs.is_empty()) {
		return;
	}
	text = p_bbcode;
	hovered_meta = -1;
	_parse();
}

String BBCodeText::get_text() const {
	return text;
}

String BBCodeText::get_parsed_text() const {
	return parsed_text;
}

void BBCodeText::set_fonts(const Ref<Font> &p_normal, const Ref<Font> &p_bold, const Ref<Font> &p_italics, const Ref<Font> &p_bold_italics, const Ref<Font> &p_mono, int p_size) {
	const Ref<Font> fonts[FONT_KIND_MAX] = { p_normal, p_bold, p_italics, p_bold_italics, p_mono };
	bool changed = p_size != font_size;
	for (int i = 0; i < FONT_KIND_MAX; i++) {
		changed = changed || fonts[i] != base_fonts[i];
	}
	if (!changed) {
		return;
	}
	if (p_normal != base_fonts[FONT_NORMAL]) {
		for (Ref<Font> &font : synthesized_fonts) {
			font = Ref<Font>();
		}
	}
	for (int i = 0; i < FONT_KIND_MAX; i++) {
		base_fonts[i] = fonts[i];
	}
	font_size = p_size;
	font_version++;
	shape_dirty = true;
}

void BBCodeText::set_horizontal_alignment(HorizontalAlignment p_alignment) {
	if (horizontal_alignment != p_alignment) {
		if (horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL || p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			lines_dirty = true;
		}
		horizontal_alignment = p_alignment;
	}
}

void BBCodeText::set_autowrap_mode(TextServer::AutowrapMode p_mode) {
	if (autowrap_mode != p_mode) {
		autowrap_mode = p_mode;
		lines_dirty = true;
	}
}

void BBCodeText::set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags) {
	if (autowrap_flags_trim != p_flags) {
		autowrap_flags_trim = p_flags;
		lines_dirty = true;
	}
}

void BBCodeText::set_justification_flags(BitField<TextServer::JustificationFlag> p_flags) {
	if (jst_flags != p_flags) {
		jst_flags = p_flags;
		lines_dirty = true;
	}
}

void BBCodeText::set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior) {
	if (overrun_behavior != p_behavior) {
		overrun_behavior = p_behavior;
		lines_dirty = true;
	}
}

void BBCodeText::set_ellipsis_char(const String &p_char) {
	if (el_char != p_char) {
		el_char = p_char;
		lines_dirty = true;
	}
}

void BBCodeText::set_uppercase(bool p_uppercase) {
	if (uppercase != p_uppercase) {
		uppercase = p_uppercase;
		shape_dirty = true;
	}
}

void BBCodeText::set_tab_stops(const PackedFloat32Array &p_tab_stops) {
	if (tab_stops != p_tab_stops) {
		tab_stops = p_tab_stops;
		shape_dirty = true;
	}
}

void BBCodeText::set_direction(TextServer::Direction p_direction, bool p_layout_rtl) {
	if (direction != p_direction || layout_rtl != p_layout_rtl) {
		direction = p_direction;
		layout_rtl = p_layout_rtl;
		shape_dirty = true;
	}
}

void BBCodeText::set_text_direction(Control::TextDirection p_direction, bool p_layout_rtl) {
	if (p_direction == Control::TEXT_DIRECTION_INHERITED) {
		set_direction(p_layout_rtl ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR, p_layout_rtl);
	} else {
		set_direction((TextServer::Direction)p_direction, p_layout_rtl);
	}
}

void BBCodeText::set_language(const String &p_language) {
	if (language != p_language) {
		language = p_language;
		_parse(); // List numbers may be localized.
	}
}

void BBCodeText::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser, const Array &p_args) {
	if (st_parser != p_parser || st_args != p_args) {
		st_parser = p_parser;
		st_args = p_args;
		shape_dirty = true;
	}
}

void BBCodeText::set_visible_characters_behavior(TextServer::VisibleCharactersBehavior p_behavior) {
	if (visible_chars_behavior != p_behavior) {
		visible_chars_behavior = p_behavior;
		shape_dirty = true;
	}
}

void BBCodeText::set_visible_characters(int p_amount) {
	if (visible_chars != p_amount) {
		visible_chars = p_amount;
		if (visible_chars_behavior == TextServer::VC_CHARS_BEFORE_SHAPING) {
			shape_dirty = true;
		}
	}
}

void BBCodeText::set_visible_ratio(float p_ratio) {
	visible_ratio = p_ratio;
}

void BBCodeText::set_lines_skipped(int p_lines) {
	if (lines_skipped != p_lines) {
		lines_skipped = p_lines;
		lines_dirty = true;
	}
}

void BBCodeText::set_max_lines_visible(int p_lines) {
	if (max_lines_visible != p_lines) {
		max_lines_visible = p_lines;
		lines_dirty = true;
	}
}

void BBCodeText::set_line_spacing(int p_spacing) {
	if (line_spacing != p_spacing) {
		line_spacing = p_spacing;
		lines_dirty = true;
	}
}

void BBCodeText::set_paragraph_spacing(int p_spacing) {
	if (paragraph_spacing != p_spacing) {
		paragraph_spacing = p_spacing;
		lines_dirty = true;
	}
}

void BBCodeText::set_width(int p_width) {
	if (width != p_width) {
		width = p_width;
		if (has_relative_objects) {
			shape_dirty = true;
		}
		lines_dirty = true;
	}
}

void BBCodeText::set_height(float p_height) {
	if (height != p_height) {
		height = p_height;
		lines_dirty = true;
	}
}

bool BBCodeText::has_metas() const {
	return !metas.is_empty();
}

bool BBCodeText::has_hints() const {
	if (!hints.is_empty()) {
		return true;
	}
	for (const Meta &meta : metas) {
		if (!meta.tooltip.is_empty()) {
			return true;
		}
	}
	for (const InlineObject &object : objects) {
		if (!object.tooltip.is_empty()) {
			return true;
		}
	}
	return false;
}

int BBCodeText::get_line_count() const {
	_ensure_shaped();
	return total_line_count;
}

int BBCodeText::get_visible_line_count() const {
	// Same as Label::get_visible_line_count(), with the height given by the owner.
	int lines_visible = 0;
	float total_h = 0.0;

	int line_index = 0;
	for (const Paragraph &para : paragraphs) {
		if (line_index + para.lines.size() <= lines_skipped) {
			line_index += para.lines.size();
		} else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			for (int i = start; i < para.lines.size(); i++) {
				double asc, dsc;
				_get_line_metrics(para.lines[i].rid, asc, dsc);
				total_h += asc + dsc + line_spacing;
				if (height >= 0 && total_h > Math::ceil(height + line_spacing)) {
					break;
				}
				lines_visible++;
			}
			total_h += paragraph_spacing;
			line_index += para.lines.size();
		}
	}

	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}
	return lines_visible;
}

int BBCodeText::get_total_character_count() const {
	if (paragraphs.is_empty()) {
		return 0;
	}
	const Paragraph &last = paragraphs[paragraphs.size() - 1];
	return last.char_start + last.char_count;
}

int BBCodeText::get_line_height(int p_line) const {
	_ensure_shaped();
	int font_h = _get_font_height();
	if (p_line >= 0 && p_line < total_line_count) {
		int line_index = 0;
		for (const Paragraph &para : paragraphs) {
			if (p_line < line_index + para.lines.size()) {
				double asc, dsc;
				_get_line_metrics(para.lines[p_line - line_index].rid, asc, dsc);
				return asc + dsc;
			}
			line_index += para.lines.size();
		}
	} else if (total_line_count > 0) {
		int h = font_h;
		for (const Paragraph &para : paragraphs) {
			for (const Line &line : para.lines) {
				h = MAX(h, TS->shaped_text_get_size(line.rid).y);
			}
		}
		return h;
	}
	return font_h;
}

float BBCodeText::get_lines_width() const {
	_ensure_shaped();
	float ret = 0;
	for (const Paragraph &para : paragraphs) {
		for (const Line &line : para.lines) {
			float shift = 0;
			if (line.dropcap_shift && para.dropcap_rid.is_valid()) {
				shift = TS->shaped_text_get_size(para.dropcap_rid).x + para.dropcap_margins.size.x + para.dropcap_margins.position.x;
			}
			ret = MAX(ret, line.natural_width + para.margin + shift);
		}
	}
	return ret;
}

float BBCodeText::get_unwrapped_width() const {
	_ensure_shaped();
	float ret = 0;
	for (const Paragraph &para : paragraphs) {
		if (para.text_rid.is_valid()) {
			float dc = 0;
			if (para.dropcap_rid.is_valid()) {
				dc = TS->shaped_text_get_size(para.dropcap_rid).x + para.dropcap_margins.size.x + para.dropcap_margins.position.x;
			}
			ret = MAX(ret, para.unwrapped_width + para.margin + dc);
		}
	}
	return ret;
}

float BBCodeText::get_visible_height() const {
	// Same as Label::_update_visible().
	_ensure_shaped();
	int lines_visible = total_line_count;
	if (max_lines_visible >= 0 && lines_visible > max_lines_visible) {
		lines_visible = max_lines_visible;
	}

	float ret = 0;
	int last_line = MIN(total_line_count, lines_visible + lines_skipped);
	int line_index = 0;
	for (const Paragraph &para : paragraphs) {
		if (line_index + para.lines.size() <= lines_skipped) {
			line_index += para.lines.size();
		} else {
			int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
			int end = (line_index + para.lines.size() < last_line) ? para.lines.size() : last_line - line_index;
			if (end <= 0) {
				break;
			}
			for (int i = start; i < end; i++) {
				double asc, dsc;
				_get_line_metrics(para.lines[i].rid, asc, dsc);
				ret += asc + dsc + line_spacing;
			}
			ret += paragraph_spacing;
			line_index += para.lines.size();
		}
	}
	if (ret > 0) {
		ret -= line_spacing + paragraph_spacing;
	}
	return ret;
}

Size2 BBCodeText::get_size() const {
	return Size2(get_lines_width(), get_visible_height());
}

Size2 BBCodeText::get_drawn_size() const {
	_ensure_shaped();
	float width = 0;
	for (const Paragraph &para : paragraphs) {
		for (const Line &line : para.lines) {
			float shift = 0;
			if (line.dropcap_shift && para.dropcap_rid.is_valid()) {
				shift = TS->shaped_text_get_size(para.dropcap_rid).x + para.dropcap_margins.size.x + para.dropcap_margins.position.x;
			}
			width = MAX(width, (float)TS->shaped_text_get_size(line.rid).x + para.margin + shift);
		}
	}
	return Size2(width, get_visible_height());
}

Rect2 BBCodeText::get_character_bounds(const Rect2 &p_rect, VerticalAlignment p_valign, int p_pos) const {
	_ensure_shaped();

	float ofs_y;
	int last_line;
	float ls;
	_get_layout(p_rect, p_valign, ofs_y, last_line, ls);

	int line_index = 0;
	for (const Paragraph &para : paragraphs) {
		if (line_index + para.lines.size() <= lines_skipped) {
			line_index += para.lines.size();
			continue;
		}
		int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
		int end = (line_index + para.lines.size() < last_line) ? para.lines.size() : last_line - line_index;
		if (end <= 0) {
			break;
		}
		for (int i = start; i < end; i++) {
			const RID rid = para.lines[i].rid;
			double asc, dsc;
			_get_line_metrics(rid, asc, dsc);
			float x = _get_line_x(para, i, p_rect);

			const Glyph *glyphs = TS->shaped_text_get_glyphs(rid);
			int gl_size = TS->shaped_text_get_glyph_count(rid);
			float gl_off = 0.0f;
			for (int j = 0; j < gl_size; j++) {
				if ((glyphs[j].count > 0) && ((glyphs[j].index != 0) || ((glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE))) {
					if (p_pos >= glyphs[j].start + para.char_start && p_pos < glyphs[j].end + para.char_start) {
						float advance = 0.f;
						for (int k = 0; k < glyphs[j].count; k++) {
							advance += glyphs[j + k].advance;
						}
						return Rect2(Vector2(x + gl_off, ofs_y), Vector2(advance, asc + dsc));
					}
				}
				gl_off += glyphs[j].advance * glyphs[j].repeat;
			}
			ofs_y += asc + dsc + ls;
		}
		ofs_y += paragraph_spacing;
		line_index += para.lines.size();
	}
	return Rect2();
}

void BBCodeText::draw(RID p_ci, const Rect2 &p_rect, VerticalAlignment p_valign, const DrawStyle &p_style) const {
	_ensure_shaped();
	RenderingServer *rs = RenderingServer::get_singleton();

	float ofs_y;
	int last_line;
	float ls;
	const int total_glyphs = _get_layout(p_rect, p_valign, ofs_y, last_line, ls);
	const bool trim_chars = (visible_chars >= 0) && (visible_chars_behavior == TextServer::VC_CHARS_AFTER_SHAPING);
	const bool trim_glyphs = (visible_chars >= 0) && (visible_chars_behavior != TextServer::VC_CHARS_BEFORE_SHAPING) && !trim_chars;
	int processed_glyphs = 0;

	int line_index = 0;
	for (const Paragraph &para : paragraphs) {
		if (line_index + para.lines.size() <= lines_skipped) {
			line_index += para.lines.size();
			continue;
		}
		int start = (line_index < lines_skipped) ? lines_skipped - line_index : 0;
		int end = (line_index + para.lines.size() < last_line) ? para.lines.size() : last_line - line_index;
		if (end <= 0) {
			break;
		}
		const bool rtl = _is_paragraph_rtl(para);
		const bool para_hidden = (trim_chars && para.char_start > visible_chars) || (trim_glyphs && visible_ratio <= 0);

		for (int i = start; i < end; i++) {
			const RID rid = para.lines[i].rid;
			double asc, dsc;
			_get_line_metrics(rid, asc, dsc);
			const float x = _get_line_x(para, i, p_rect);

			if (i == 0 && !para_hidden) {
				// List marker, next to the text start.
				if (para.prefix_rid.is_valid()) {
					const SpanStyle &style = styles[para.runs.is_empty() ? 0 : para.runs[0].style];
					const Color color = style.has_color ? style.color : p_style.font_color;
					const int outline_size = style.outline_size >= 0 ? style.outline_size : p_style.outline_size;
					const Color outline_color = style.has_outline_color ? style.outline_color : p_style.outline_color;
					const float prefix_w = TS->shaped_text_get_size(para.prefix_rid).x;
					const float line_w = TS->shaped_text_get_size(rid).x;
					Vector2 pos(rtl ? x + line_w : x - prefix_w, ofs_y + asc);
					if (outline_size > 0 && outline_color.a != 0) {
						TS->shaped_text_draw_outline(para.prefix_rid, p_ci, pos, -1, -1, outline_size, outline_color);
					}
					TS->shaped_text_draw(para.prefix_rid, p_ci, pos, -1, -1, color);
				}
				// Dropcap, at the paragraph start.
				if (para.dropcap_rid.is_valid()) {
					const Size2 dc_size = TS->shaped_text_get_size(para.dropcap_rid);
					const float h_offset = dc_size.x + para.dropcap_margins.size.x + para.dropcap_margins.position.x;
					float dc_x = rtl ? p_rect.position.x + p_rect.size.width - para.margin - h_offset : p_rect.position.x + para.margin;
					Vector2 pos(dc_x + para.dropcap_margins.position.x, ofs_y + para.dropcap_margins.position.y + TS->shaped_text_get_ascent(para.dropcap_rid));
					const int outline_size = para.dropcap_outline_size >= 0 ? para.dropcap_outline_size : p_style.outline_size;
					const Color outline_color = para.dropcap_has_outline_color ? para.dropcap_outline_color : p_style.outline_color;
					if (outline_size > 0 && outline_color.a != 0) {
						TS->shaped_text_draw_outline(para.dropcap_rid, p_ci, pos, -1, -1, outline_size, outline_color);
					}
					TS->shaped_text_draw(para.dropcap_rid, p_ci, pos, -1, -1, para.dropcap_has_color ? para.dropcap_color : p_style.font_color);
				}
				// Horizontal rule, centered in the line.
				if (para.is_hr) {
					const float avail = MAX(0.0f, p_rect.size.width - para.margin);
					const float line_height = asc + dsc;
					const float hr_w = para.hr_width_percent ? avail * para.hr_width / 100.0f : MIN((float)para.hr_width, avail);
					const float hr_h = para.hr_height_percent ? line_height * para.hr_height / 100.0f : (float)para.hr_height;
					float hr_x = p_rect.position.x + para.margin;
					if (para.hr_alignment == HORIZONTAL_ALIGNMENT_CENTER) {
						hr_x += Math::floor((avail - hr_w) / 2);
					} else if ((para.hr_alignment == HORIZONTAL_ALIGNMENT_RIGHT) != layout_rtl) {
						hr_x += avail - hr_w;
					}
					rs->canvas_item_add_rect(p_ci, Rect2(hr_x, ofs_y + Math::floor((line_height - hr_h) / 2), hr_w, hr_h), para.hr_has_color ? para.hr_color : p_style.hr_color);
				}
			}

			_draw_line(p_ci, para, i, Vector2(x, ofs_y + asc), p_style, processed_glyphs, total_glyphs);
			ofs_y += asc + dsc + ls;
		}
		ofs_y += paragraph_spacing;
		line_index += para.lines.size();
	}
}

void BBCodeText::draw_at(RID p_ci, const Vector2 &p_pos, const DrawStyle &p_style, Rect2 *r_rect) const {
	Rect2 rect(p_pos, Size2(width >= 0 ? (float)width : get_lines_width(), get_visible_height()));
	draw(p_ci, rect, VERTICAL_ALIGNMENT_TOP, p_style);
	if (r_rect) {
		*r_rect = rect;
	}
}

int BBCodeText::get_meta_index_at(const Rect2 &p_rect, VerticalAlignment p_valign, const Point2 &p_pos) const {
	int style = -1;
	int object = -1;
	if (!_find_style_at(p_rect, p_valign, p_pos, style, object) || style < 0) {
		return -1;
	}
	return styles[style].meta;
}

Variant BBCodeText::get_url_meta(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, metas.size(), Variant());
	return metas[p_index].meta;
}

void BBCodeText::set_hovered_meta(int p_index) {
	hovered_meta = p_index;
}

String BBCodeText::get_tooltip_at(const Rect2 &p_rect, VerticalAlignment p_valign, const Point2 &p_pos) const {
	int style = -1;
	int object = -1;
	if (!_find_style_at(p_rect, p_valign, p_pos, style, object)) {
		return String();
	}
	if (object >= 0 && !objects[object].tooltip.is_empty()) {
		return objects[object].tooltip;
	}
	if (style >= 0) {
		if (styles[style].hint >= 0) {
			return hints[styles[style].hint];
		}
		if (styles[style].meta >= 0) {
			return metas[styles[style].meta].tooltip;
		}
	}
	return String();
}

BBCodeText::BBCodeText() {
	styles.push_back(SpanStyle());
}

BBCodeText::~BBCodeText() {
	_clear_shaped();
}
