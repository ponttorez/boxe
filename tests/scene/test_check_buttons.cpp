/**************************************************************************/
/*  test_check_buttons.cpp                                                */
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

TEST_FORCE_LINK(test_check_buttons)

#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/slider.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"
#include "tests/display_server_mock.h"

namespace TestCheckButtons {

static Ref<StyleBoxFlat> make_style(const Size2 &p_size) {
	Ref<StyleBoxFlat> style;
	style.instantiate();
	style->set_content_margin(SIDE_LEFT, p_size.x * 0.5);
	style->set_content_margin(SIDE_RIGHT, p_size.x * 0.5);
	style->set_content_margin(SIDE_TOP, p_size.y * 0.5);
	style->set_content_margin(SIDE_BOTTOM, p_size.y * 0.5);
	return style;
}

TEST_CASE("[SceneTree][CheckBox] The check is drawn with style boxes when the theme has them") {
	CheckBox *check_box = memnew(CheckBox);
	SceneTree::get_singleton()->get_root()->add_child(check_box);

	// The default theme draws icons, which give the size of the check.
	const Size2 icon_size = check_box->get_theme_icon(SNAME("checked"))->get_size();
	CHECK(check_box->get_combined_minimum_size().height >= icon_size.height);

	check_box->add_theme_style_override(SNAME("checked"), make_style(Size2(40, 30)));
	check_box->add_theme_style_override(SNAME("unchecked"), make_style(Size2(40, 30)));
	CHECK(check_box->get_combined_minimum_size().width >= 40);
	CHECK(check_box->get_combined_minimum_size().height >= 30);
	CHECK(check_box->get_combined_minimum_size().width < 40 + icon_size.width);

	memdelete(check_box);
}

TEST_CASE("[SceneTree][CheckButton] The switch is drawn with style boxes when the theme has them") {
	CheckButton *check_button = memnew(CheckButton);
	SceneTree::get_singleton()->get_root()->add_child(check_button);

	check_button->add_theme_style_override(SNAME("checked"), make_style(Size2(60, 34)));
	check_button->add_theme_style_override(SNAME("unchecked"), make_style(Size2(60, 34)));
	CHECK(check_button->get_combined_minimum_size().width >= 60);
	CHECK(check_button->get_combined_minimum_size().height >= 34);

	memdelete(check_button);
}

TEST_CASE("[SceneTree][PopupMenu] The checks of items are drawn with style boxes when the theme has them") {
	PopupMenu *menu = memnew(PopupMenu);
	SceneTree::get_singleton()->get_root()->add_child(menu);
	menu->add_check_item("Check");
	menu->add_radio_check_item("Radio");

	const float icon_width = menu->get_contents_minimum_size().width;
	for (const char *name : { "checked", "unchecked", "radio_checked", "radio_unchecked" }) {
		menu->add_theme_style_override(name, make_style(Size2(60, 30)));
	}
	const float style_width = menu->get_contents_minimum_size().width;
	CHECK(style_width > icon_width);
	CHECK(style_width - icon_width <= 60);

	memdelete(menu);
}

TEST_CASE("[SceneTree][Tree] The checks of cells are drawn with style boxes when the theme has them") {
	Tree *tree = memnew(Tree);
	SceneTree::get_singleton()->get_root()->add_child(tree);
	TreeItem *item = tree->create_item();
	item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);

	const Size2 icon_size = tree->get_theme_icon(SNAME("checked"))->get_size();
	const Size2 icon_min_size = item->get_minimum_size(0);
	tree->add_theme_style_override(SNAME("checked"), make_style(Size2(50, 40)));
	tree->add_theme_style_override(SNAME("unchecked"), make_style(Size2(50, 40)));
	const Size2 style_min_size = item->get_minimum_size(0);
	CHECK(style_min_size.width == doctest::Approx(icon_min_size.width - icon_size.width + 50));
	CHECK(style_min_size.height >= 40);

	memdelete(tree);
}

TEST_CASE("[SceneTree][Slider] The grabber is drawn with style boxes when the theme has them") {
	HSlider *slider = memnew(HSlider);
	SceneTree::get_singleton()->get_root()->add_child(slider);
	slider->set_max(100);

	// The largest grabber style box gives the size of the grabber in every state.
	slider->add_theme_style_override(SNAME("grabber"), make_style(Size2(30, 30)));
	slider->add_theme_style_override(SNAME("grabber_highlight"), make_style(Size2(40, 40)));
	CHECK(slider->get_combined_minimum_size().height == doctest::Approx(40));

	// The grabber is 40 px wide, so the values go from its center at 20 px to 220 px.
	slider->set_position(Point2());
	slider->set_size(Size2(240, 40));
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2(120, 20), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(120, 20), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	CHECK(slider->get_value() == doctest::Approx(50));
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2(20, 20), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(20, 20), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	CHECK(slider->get_value() == doctest::Approx(0));

	memdelete(slider);
}

} // namespace TestCheckButtons
