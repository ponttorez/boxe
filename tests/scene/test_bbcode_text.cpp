/**************************************************************************/
/*  test_bbcode_text.cpp                                                  */
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

TEST_FORCE_LINK(test_bbcode_text)

#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/link_button.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"
#include "scene/theme/theme_db.h"
#include "tests/display_server_mock.h"
#include "tests/signal_watcher.h"

namespace TestBBCodeText {

TEST_CASE("[SceneTree][Label] BBCode hints of hidden characters") {
	Label *label = memnew(Label);
	SceneTree::get_singleton()->get_root()->add_child(label);
	label->set_bbcode_enabled(true);
	label->set_text("[hint=Shown]Text[/hint]");
	label->set_visible_characters_behavior(TextServer::VC_CHARS_AFTER_SHAPING);
	label->set_size(Size2(200, 40));
	const Point2 point(3, 8);
	CHECK(label->get_tooltip(point) == "Shown");

	// Characters hidden by visible_characters aren't drawn, so they have no hint.
	label->set_visible_characters(0);
	CHECK(label->get_tooltip(point).is_empty());
	label->set_visible_characters_behavior(TextServer::VC_GLYPHS_LTR);
	CHECK(label->get_tooltip(point).is_empty());

	memdelete(label);
}

TEST_CASE("[SceneTree][ItemList] Automatic tooltips of BBCode items have no tags") {
	ItemList *item_list = memnew(ItemList);
	SceneTree::get_singleton()->get_root()->add_child(item_list);
	item_list->set_size(Size2(200, 100));
	item_list->set_bbcode_enabled(true);
	item_list->add_item("[b]Bold[/b] text");
	item_list->force_update_list_size();

	CHECK(item_list->get_tooltip(item_list->get_item_rect(0).get_center()) == "Bold text");

	memdelete(item_list);
}

static Label *make_label(const String &p_text) {
	Label *label = memnew(Label);
	SceneTree::get_singleton()->get_root()->add_child(label);
	label->set_bbcode_enabled(true);
	label->set_text(p_text);
	label->set_position(Point2());
	label->set_size(Size2(300, 100));
	return label;
}

TEST_CASE("[SceneTree][Label] BBCode text") {
	Label *label = make_label("[b]Bold[/b] [color=red]and[/color] [url=x]link[/url]");
	CHECK(label->get_parsed_text() == "Bold and link");
	CHECK(label->get_text() == "[b]Bold[/b] [color=red]and[/color] [url=x]link[/url]");

	label->set_text("A");
	const Size2 small = label->get_minimum_size();
	label->set_text("[font_size=60]A[/font_size]");
	CHECK_MESSAGE(label->get_minimum_size().height > small.height, "Tags change the layout.");

	label->set_bbcode_enabled(false);
	CHECK(label->get_parsed_text() == "[font_size=60]A[/font_size]");

	memdelete(label);
}

TEST_CASE("[SceneTree][Label] BBCode links and hints") {
	Label *label = make_label("Go [url=target]there[/url] [hint=Tip]now[/hint]");
	label->set_mouse_filter(Control::MOUSE_FILTER_STOP);
	SIGNAL_WATCH(label, "meta_clicked");
	SIGNAL_WATCH(label, "meta_hover_started");

	const Point2 link = label->get_character_bounds(4).get_center();
	SEND_GUI_MOUSE_MOTION_EVENT(link, MouseButtonMask::NONE, Key::NONE);
	SIGNAL_CHECK("meta_hover_started", { { "target" } });
	CHECK(label->get_cursor_shape(link) == Control::CURSOR_POINTING_HAND);
	SEND_GUI_MOUSE_BUTTON_EVENT(link, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(link, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	SIGNAL_CHECK("meta_clicked", { { "target" } });

	CHECK(label->get_tooltip(label->get_character_bounds(10).get_center()) == "Tip");
	CHECK(label->get_cursor_shape(label->get_character_bounds(0).get_center()) != Control::CURSOR_POINTING_HAND);

	SIGNAL_UNWATCH(label, "meta_clicked");
	SIGNAL_UNWATCH(label, "meta_hover_started");
	memdelete(label);
}

TEST_CASE("[SceneTree][Label] Visible characters of BBCode") {
	Label *label = make_label("[b]Hello[/b]");
	label->set_visible_characters_behavior(TextServer::VC_CHARS_AFTER_SHAPING);
	CHECK(label->get_character_bounds(4).size.x > 0);
	label->set_visible_characters(2);
	CHECK(label->get_visible_characters() == 2);
	label->set_visible_ratio(1.0);
	CHECK(label->get_visible_characters() == -1);

	memdelete(label);
}

TEST_CASE("[SceneTree][Label] Bold text uses the font of the label when the theme has no bold font") {
	// The font of the label isn't the default font of the theme, which a missing bold font falls back to.
	Ref<FontVariation> label_font;
	label_font.instantiate();
	label_font->set_base_font(ThemeDB::get_singleton()->get_fallback_font());

	Label *label = make_label("WWWWWWWWWW");
	label->add_theme_font_override("font", label_font);
	label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	CHECK(label->get_theme_font("bold_font") == label->get_theme_default_font());
	const float plain_width = label->get_minimum_size().width;
	label->set_text("[b]WWWWWWWWWW[/b]");
	CHECK_MESSAGE(label->get_minimum_size().width > plain_width, "Bold is synthesized from the font of the label instead of using the default font as it is.");

	memdelete(label);
}

TEST_CASE("[SceneTree][Label] Spacers in BBCode") {
	Label *label = make_label("A[spacer]B");
	label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	CHECK_MESSAGE(label->get_character_bounds(2).position.x > 250, "The spacer pushes the text to the end of the line.");
	CHECK_MESSAGE(label->get_minimum_size().width < 100, "The free space taken by the spacer isn't part of the minimum size.");

	label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	label->set_custom_maximum_size(Size2(300, 0));
	label->set_size(Size2(300, 100));
	CHECK_MESSAGE(label->get_minimum_size().width < 100, "Wrapped labels with a maximum width can still shrink.");

	memdelete(label);
}

TEST_CASE("[SceneTree][Button][Tree] BBCode tags aren't drawn") {
	Button *button = memnew(Button);
	SceneTree::get_singleton()->get_root()->add_child(button);
	button->set_text("[b]OK[/b]");
	const float raw_width = button->get_minimum_size().width;
	button->set_bbcode_enabled(true);
	CHECK(button->get_minimum_size().width < raw_width);
	memdelete(button);

	Tree *tree = memnew(Tree);
	SceneTree::get_singleton()->get_root()->add_child(tree);
	TreeItem *item = tree->create_item();
	item->set_text(0, "[i]Item[/i]");
	item->set_text_overrun_behavior(0, TextServer::OVERRUN_NO_TRIMMING);
	const float raw_cell_width = item->get_minimum_size(0).width;
	tree->set_bbcode_enabled(true);
	CHECK(item->get_minimum_size(0).width < raw_cell_width);
	memdelete(tree);
}

TEST_CASE("[SceneTree][LinkButton] BBCode hints of trimmed text in RTL layouts") {
	LinkButton *link = memnew(LinkButton);
	SceneTree::get_singleton()->get_root()->add_child(link);
	link->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
	link->set_bbcode_enabled(true);
	link->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	link->set_text("[hint=Shown]A long text that doesn't fit in the button[/hint]");
	link->set_size(Size2(60, 40));

	// The trimmed text is drawn at the right edge of the button.
	CHECK(link->get_tooltip(Point2(55, 8)) == "Shown");
	CHECK(link->get_tooltip(Point2(-10, 8)).is_empty());

	memdelete(link);
}

} // namespace TestBBCodeText
