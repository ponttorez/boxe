/**************************************************************************/
/*  bbcode_text.h                                                         */
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

#include "core/object/ref_counted.h"
#include "scene/gui/control.h"
#include "scene/resources/label_settings.h"
#include "servers/text/text_server.h"

class Texture2D;
class Window;

// Lays out and draws BBCode text for controls that show text without RichTextLabel.
// Supports the RichTextLabel tags except tables and effects, plus [spacer].
class BBCodeText : public RefCounted {
	GDCLASS(BBCodeText, RefCounted);

public:
	enum FontKind {
		FONT_NORMAL,
		FONT_BOLD,
		FONT_ITALICS,
		FONT_BOLD_ITALICS,
		FONT_MONO,
		FONT_KIND_MAX,
	};

	enum MetaUnderline {
		META_UNDERLINE_NEVER,
		META_UNDERLINE_ALWAYS,
		META_UNDERLINE_ON_HOVER,
	};

	// Colors and effects used to draw the text, everything not set by tags comes from here.
	struct DrawStyle {
		Color font_color = Color(1, 1, 1);
		int outline_size = 0;
		Color outline_color;
		Color shadow_color = Color(0, 0, 0, 0);
		Vector2 shadow_offset;
		int shadow_outline_size = 0;
		Vector<LabelSettings::StackedOutlineData> stacked_outlines;
		Vector<LabelSettings::StackedShadowData> stacked_shadows;
		Color hr_color = Color(1, 1, 1);
		int underline_alpha = 50;
		int strikethrough_alpha = 50;
		int highlight_h_padding = 3;
		int highlight_v_padding = 3;
	};

private:
	enum ObjectType {
		OBJECT_IMAGE,
		OBJECT_SPACER,
	};

	enum ImageUnit {
		IMAGE_UNIT_PIXEL,
		IMAGE_UNIT_PERCENT,
		IMAGE_UNIT_EM,
	};

	enum ListType {
		LIST_NUMBERS,
		LIST_LETTERS,
		LIST_ROMAN,
		LIST_DOTS,
	};

	// Inline formatting of a text run, all unset values come from the owner.
	struct SpanStyle {
		FontKind font_kind = FONT_NORMAL;
		Ref<Font> custom_font;
		Vector<Ref<FontVariation>> variations; // Applied in order on top of the base font.
		int font_size = -1;
		bool has_color = false;
		Color color;
		int outline_size = -1;
		bool has_outline_color = false;
		Color outline_color;
		bool underline = false;
		Color underline_color = Color(0, 0, 0, 0);
		bool strikethrough = false;
		Color strikethrough_color = Color(0, 0, 0, 0);
		Color bgcolor = Color(0, 0, 0, 0);
		Color fgcolor = Color(0, 0, 0, 0);
		int meta = -1;
		int hint = -1;
		String language;

		mutable Ref<Font> resolved_font;
		mutable uint64_t resolved_version = 0;
	};

	struct InlineObject {
		ObjectType type = OBJECT_IMAGE;
		int style = 0;

		Ref<Texture2D> texture;
		Color color = Color(1, 1, 1);
		Rect2 region;
		float width = 0;
		float height = 0;
		ImageUnit width_unit = IMAGE_UNIT_PIXEL;
		ImageUnit height_unit = IMAGE_UNIT_PIXEL;
		InlineAlignment inline_align = INLINE_ALIGNMENT_CENTER;
		bool pad = false;
		String tooltip;

		float weight = 1.0; // Spacer share of the free line space.
	};

	struct Run {
		String text; // Empty for objects.
		int style = 0;
		int object = -1;
	};

	struct ListLevel {
		int id = 0;
		ListType type = LIST_DOTS;
		bool capitalize = false;
		String bullet = U"•";
	};

	// Paragraph formatting from [p], alignment, [indent] and list tags.
	struct ParagraphStyle {
		int alignment = -1; // Owner alignment when negative.
		int direction = -1; // Owner direction when negative.
		bool has_language = false;
		String language;
		int st_parser = -1; // Owner parser when negative.
		bool has_jst_flags = false;
		BitField<TextServer::JustificationFlag> jst_flags;
		bool has_tab_stops = false;
		PackedFloat32Array tab_stops;
		int indent = 0;
		Vector<ListLevel> lists; // Outermost first.
	};

	struct Meta {
		Variant meta;
		MetaUnderline underline = META_UNDERLINE_ALWAYS;
		String tooltip;
	};

	// Visibility limits of the drawn glyphs, see Label::draw_text().
	struct Trim {
		bool trim_chars = false;
		bool trim_glyphs_ltr = false;
		bool trim_glyphs_rtl = false;
		int visible_chars = -1;
		int visible_glyphs = 0;
		int total_glyphs = 0;
		int char_start = 0;

		bool is_skipped(const Glyph &p_glyph, int p_processed) const {
			return (trim_chars && p_glyph.end + char_start > visible_chars) || (trim_glyphs_ltr && p_processed >= visible_glyphs) || (trim_glyphs_rtl && p_processed < total_glyphs - visible_glyphs);
		}
	};

	// A glyph of a drawn line, at its pen position.
	struct PlacedGlyph {
		const Glyph *glyph = nullptr;
		int style = 0;
		Vector2 pos;
		bool skipped = false; // Hidden by the visible characters.
	};

	enum DecorationKind {
		DECORATION_UNDERLINE,
		DECORATION_STRIKETHROUGH,
		DECORATION_HINT,
		DECORATION_MAX,
	};

	struct Line {
		RID rid;
		bool dropcap_shift = false;
		bool has_spacers = false;
		float natural_width = 0; // Width with collapsed spacers, used for minimum sizes.
	};

	struct Paragraph {
		Vector<Run> runs;
		ParagraphStyle style;
		// The width of the paragraph on one line, without the free space given to spacers.
		float unwrapped_width = 0;

		bool has_dropcap = false;
		String dropcap_text;
		Ref<Font> dropcap_font;
		int dropcap_font_size = -1;
		Rect2 dropcap_margins;
		bool dropcap_has_color = false;
		Color dropcap_color;
		int dropcap_outline_size = -1;
		bool dropcap_has_outline_color = false;
		Color dropcap_outline_color;

		bool is_hr = false;
		int hr_width = 90;
		bool hr_width_percent = true;
		int hr_height = 2;
		bool hr_height_percent = false;
		bool hr_has_color = false;
		Color hr_color;
		HorizontalAlignment hr_alignment = HORIZONTAL_ALIGNMENT_CENTER;

		int char_start = 0;
		int char_count = 0;
		String prefix; // List marker.

		RID text_rid;
		RID dropcap_rid;
		RID prefix_rid;
		Vector<Line> lines;
		float margin = 0; // Indent and list marker width.

		bool is_empty() const { return runs.is_empty() && !is_hr; }
	};

	// The styles of the open tags while parsing.
	struct ParseState {
		int style = 0;
		ParagraphStyle para;
		bool bold = false;
		bool italics = false;
	};

	// Adds the parsed text and objects to the paragraphs, with the styles of the open tags.
	struct Builder {
		BBCodeText *bbcode = nullptr;
		Vector<ParseState> states;
		Vector<String> tag_stack;

		ParseState &state();
		Paragraph &current();
		void push_state(const String &p_name);
		void push_style(const SpanStyle &p_style);
		void new_paragraph();
		void add_text(const String &p_text);
		void add_object(const InlineObject &p_object);
		void open_paragraph_tag();
		void set_font_kind();

		Builder(BBCodeText *p_bbcode);
	};

	const Control *owner = nullptr;

	String text;
	mutable Vector<Paragraph> paragraphs;
	Vector<SpanStyle> styles;
	Vector<InlineObject> objects;
	Vector<Meta> metas;
	Vector<String> hints;
	String parsed_text;
	bool has_relative_objects = false;
	int list_count = 0;

	Ref<Font> base_fonts[FONT_KIND_MAX];
	mutable Ref<Font> synthesized_fonts[FONT_KIND_MAX];
	int font_size = 16;
	uint64_t font_version = 1;

	HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_LEFT;
	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
	BitField<TextServer::LineBreakFlag> autowrap_flags_trim = TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	BitField<TextServer::JustificationFlag> jst_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA | TextServer::JUSTIFICATION_SKIP_LAST_LINE | TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	TextServer::OverrunBehavior overrun_behavior = TextServer::OVERRUN_NO_TRIMMING;
	String el_char = U"…";
	bool uppercase = false;
	PackedFloat32Array tab_stops;
	TextServer::Direction direction = TextServer::DIRECTION_AUTO;
	bool layout_rtl = false;
	String language;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;
	Array st_args;
	int tab_size = 4;

	TextServer::VisibleCharactersBehavior visible_chars_behavior = TextServer::VC_CHARS_BEFORE_SHAPING;
	int visible_chars = -1;
	float visible_ratio = 1.0;
	int lines_skipped = 0;
	int max_lines_visible = -1;
	int line_spacing = 0;
	int paragraph_spacing = 0;

	int width = -1;
	float height = -1;

	mutable bool shape_dirty = true;
	mutable bool lines_dirty = true;
	mutable int total_line_count = 0;

	int hovered_meta = -1;

	// Parsing.
	void _parse();
	void _clear_shaped() const;

	// Shaping and line breaking.
	Ref<Font> _get_base_font(FontKind p_kind) const;
	Ref<Font> _resolve_font(const SpanStyle &p_style) const;
	int _resolve_font_size(const SpanStyle &p_style) const;
	Size2 _get_object_size(const InlineObject &p_object) const;
	float _get_tab_width() const;
	void _shape_paragraph(Paragraph &p_para) const;
	static RID _add_line(Paragraph &p_para, int p_start, int p_end, bool p_dropcap_shift, const PackedFloat32Array &p_tab_stops, PackedInt32Array &r_ranges);
	void _break_paragraph(Paragraph &p_para) const;
	void _fit_spacers(Paragraph &p_para, const PackedInt32Array &p_ranges) const;
	void _trim_lines() const;
	void _ensure_shaped() const;

	// Layout.
	int _get_font_height() const;
	void _get_line_metrics(RID p_rid, double &r_ascent, double &r_descent) const;
	float _get_line_available_width(const Paragraph &p_para, const Line &p_line) const;
	float _get_line_x(const Paragraph &p_para, int p_line, const Rect2 &p_rect) const;
	int _get_layout(const Rect2 &p_rect, VerticalAlignment p_valign, float &r_offset_y, int &r_last_line, float &r_line_spacing) const;
	HorizontalAlignment _get_alignment(const Paragraph &p_para) const;
	bool _is_paragraph_rtl(const Paragraph &p_para) const;

	// Drawing.
	void _draw_line(RID p_ci, const Paragraph &p_para, int p_line, const Vector2 &p_ofs, const DrawStyle &p_style, int &r_processed_glyphs, int p_total_glyphs) const;
	Trim _make_trim(const Paragraph &p_para, int p_total_glyphs) const;
	Vector<int> _get_glyph_styles(RID p_rid) const;
	bool _is_object_skipped(const Trim &p_trim, const Paragraph &p_para, RID p_rid, const Variant &p_key, int p_processed) const;
	static void _place_line_glyphs(RID p_rid, bool p_rtl, const Vector2 &p_ofs, const Vector<int> &p_styles, const Trim &p_trim, int p_processed, LocalVector<PlacedGlyph> &r_glyphs);
	bool _is_glyph_drawn(const PlacedGlyph &p_glyph, const DrawStyle &p_style) const;
	static void _draw_box(RID p_target, float p_start, float p_end, const Color &p_color, const Vector2 &p_ofs, double p_ascent, double p_line_height, const DrawStyle &p_style);
	void _draw_boxes(const LocalVector<PlacedGlyph> &p_glyphs, RID p_target, bool p_foreground, const Vector2 &p_ofs, double p_ascent, double p_line_height, const DrawStyle &p_style) const;
	void _draw_glyph_effect(const LocalVector<PlacedGlyph> &p_glyphs, RID p_ci, int p_outline_size, bool p_fill, const Color &p_color, const Vector2 &p_offset, const DrawStyle &p_style) const;
	static void _draw_decoration(RID p_target, DecorationKind p_kind, float p_start, float p_end, float p_y, const Color &p_color, float p_thickness);
	void _draw_decorations(const LocalVector<PlacedGlyph> &p_glyphs, RID p_target, const Vector2 &p_ofs, double p_ascent, double p_line_height, float p_underline_position, float p_underline_thickness, const DrawStyle &p_style) const;
	Color _get_style_color(int p_style, const DrawStyle &p_draw) const;
	int _get_glyph_style(RID p_line, const Glyph &p_glyph) const;

	// Hit testing.
	bool _find_style_at(const Rect2 &p_rect, VerticalAlignment p_valign, const Point2 &p_pos, int &r_style, int &r_object) const;

protected:
	static void _bind_methods() {}

public:
	// Binds the bold_font, italics_font, bold_italics_font and mono_font theme items of p_class.
	static void bind_theme_fonts(const StringName &p_class);
	// Uses the theme fonts of p_control, or synthesizes the missing ones from p_normal_font.
	void update_theme_fonts(const Control *p_control, const Ref<Font> &p_normal_font, int p_font_size);
	void update_theme_fonts(const Window *p_window, const Ref<Font> &p_normal_font, int p_font_size);

	void set_owner(const Control *p_owner);

	void set_text(const String &p_bbcode);
	String get_text() const;
	String get_parsed_text() const;

	void set_fonts(const Ref<Font> &p_normal, const Ref<Font> &p_bold, const Ref<Font> &p_italics, const Ref<Font> &p_bold_italics, const Ref<Font> &p_mono, int p_size);

	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	void set_autowrap_trim_flags(BitField<TextServer::LineBreakFlag> p_flags);
	void set_justification_flags(BitField<TextServer::JustificationFlag> p_flags);
	void set_text_overrun_behavior(TextServer::OverrunBehavior p_behavior);
	void set_ellipsis_char(const String &p_char);
	void set_uppercase(bool p_uppercase);
	void set_tab_stops(const PackedFloat32Array &p_tab_stops);
	void set_direction(TextServer::Direction p_direction, bool p_layout_rtl);
	// Sets the direction from the text direction of a control, which follows the layout when it's inherited.
	void set_text_direction(Control::TextDirection p_direction, bool p_layout_rtl);
	void set_language(const String &p_language);
	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser, const Array &p_args);
	void set_visible_characters_behavior(TextServer::VisibleCharactersBehavior p_behavior);
	void set_visible_characters(int p_amount);
	void set_visible_ratio(float p_ratio);
	void set_lines_skipped(int p_lines);
	void set_max_lines_visible(int p_lines);
	void set_line_spacing(int p_spacing);
	void set_paragraph_spacing(int p_spacing);

	// Width for wrapping, alignment and spacers, negative to never wrap.
	void set_width(int p_width);
	// Height used to count the visible lines, negative to show all lines.
	void set_height(float p_height);

	bool has_metas() const;
	bool has_hints() const;

	int get_line_count() const;
	int get_visible_line_count() const;
	int get_total_character_count() const;
	int get_line_height(int p_line = -1) const;

	// Width of the widest line, with collapsed spacers.
	float get_lines_width() const;
	// Width of the widest paragraph without wrapping.
	float get_unwrapped_width() const;
	// Height of the visible lines.
	float get_visible_height() const;
	// Size of the visible lines, with collapsed spacers.
	Size2 get_size() const;
	// The size of the text as drawn, after the lines are trimmed by the overrun behavior. get_size() is the size before
	// that, for the minimum size of the owner.
	Size2 get_drawn_size() const;

	Rect2 get_character_bounds(const Rect2 &p_rect, VerticalAlignment p_valign, int p_pos) const;

	void draw(RID p_ci, const Rect2 &p_rect, VerticalAlignment p_valign, const DrawStyle &p_style) const;
	// Draws at p_pos in the current width, as TextParagraph::draw() does.
	void draw_at(RID p_ci, const Vector2 &p_pos, const DrawStyle &p_style, Rect2 *r_rect = nullptr) const;

	// Returns the index of the [url] at p_pos in the text drawn in p_rect, or -1.
	int get_meta_index_at(const Rect2 &p_rect, VerticalAlignment p_valign, const Point2 &p_pos) const;
	Variant get_url_meta(int p_index) const;
	void set_hovered_meta(int p_index);
	int get_hovered_meta() const { return hovered_meta; }
	// Returns the [hint], [url] or image tooltip at p_pos, or an empty string.
	String get_tooltip_at(const Rect2 &p_rect, VerticalAlignment p_valign, const Point2 &p_pos) const;

	BBCodeText();
	~BBCodeText();
};
