/**************************************************************************/
/*  test_option_button.cpp                                                */
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

TEST_FORCE_LINK(test_option_button)

#ifndef ADVANCED_GUI_DISABLED

#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestOptionButton {

TEST_CASE("[SceneTree][OptionButton] Initialization") {
	OptionButton *test_opt = memnew(OptionButton);

	SUBCASE("There should be no options right after initialization") {
		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 0);
	}

	memdelete(test_opt);
}

TEST_CASE("[SceneTree][OptionButton] Single item") {
	OptionButton *test_opt = memnew(OptionButton);

	SUBCASE("There should a single item after after adding one") {
		test_opt->add_item("single", 1013);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 1);
		CHECK(test_opt->get_item_index(1013) == 0);
		CHECK(test_opt->get_item_id(0) == 1013);

		test_opt->remove_item(0);

		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 0);
	}

	SUBCASE("There should a single item after after adding an icon") {
		Ref<Texture2D> test_icon = memnew(Texture2D);
		test_opt->add_icon_item(test_icon, "icon", 345);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 1);
		CHECK(test_opt->get_item_index(345) == 0);
		CHECK(test_opt->get_item_id(0) == 345);

		test_opt->remove_item(0);

		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 0);
	}

	memdelete(test_opt);
}

TEST_CASE("[SceneTree][OptionButton] Many items") {
	OptionButton *test_opt = memnew(OptionButton);

	SUBCASE("Creating a complex structure and checking getters") {
		// Regular item at index 0.
		Ref<Texture2D> test_icon1 = memnew(Texture2D);
		Ref<Texture2D> test_icon2 = memnew(Texture2D);
		// Regular item at index 3.
		Ref<Texture2D> test_icon4 = memnew(Texture2D);

		test_opt->add_item("first", 100);
		test_opt->add_icon_item(test_icon1, "second_icon", 101);
		test_opt->add_icon_item(test_icon2, "third_icon", 102);
		test_opt->add_item("fourth", 104);
		test_opt->add_icon_item(test_icon4, "fifth_icon", 104);

		// Disable test_icon4.
		test_opt->set_item_disabled(4, true);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 5);

		// Check for test_icon2.
		CHECK(test_opt->get_item_index(102) == 2);
		CHECK(test_opt->get_item_id(2) == 102);

		// Remove the two regular items.
		test_opt->remove_item(3);
		test_opt->remove_item(0);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 3);

		// Check test_icon4.
		CHECK(test_opt->get_item_index(104) == 2);
		CHECK(test_opt->get_item_id(2) == 104);

		// Remove the two non-disabled icon items.
		test_opt->remove_item(1);
		test_opt->remove_item(0);

		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 1);
	}

	SUBCASE("Getters and setters not related to structure") {
		test_opt->add_item("regular", 2019);

		Ref<Texture2D> test_icon = memnew(Texture2D);
		test_opt->add_icon_item(test_icon, "icon", 3092);

		// item_text.
		test_opt->set_item_text(0, "example text");
		CHECK(test_opt->get_item_text(0) == "example text");

		// item_metadata.
		Dictionary m;
		m["bool"] = true;
		m["String"] = "yes";
		test_opt->set_item_metadata(1, m);
		CHECK(test_opt->get_item_metadata(1) == m);

		// item_tooltip.
		test_opt->set_item_tooltip(0, "tooltip guide");
		CHECK(test_opt->get_item_tooltip(0) == "tooltip guide");

		test_opt->remove_item(1);
		test_opt->remove_item(0);
	}

	memdelete(test_opt);
}

TEST_CASE("[SceneTree][OptionButton][MenuButton] Popups open below the button, apart by popup_separation") {
	OptionButton *option_button = memnew(OptionButton);
	MenuButton *menu_button = memnew(MenuButton);
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(option_button);
	root->add_child(menu_button);
	option_button->add_item("Item");
	menu_button->get_popup()->add_item("Item");
	option_button->set_position(Point2(10, 20));
	option_button->set_size(Size2(100, 30));
	menu_button->set_position(Point2(200, 20));
	menu_button->set_size(Size2(100, 30));

	option_button->show_popup();
	menu_button->show_popup();
	CHECK(option_button->get_popup()->get_position().y == 50);
	CHECK(menu_button->get_popup()->get_position().y == 50);
	option_button->get_popup()->hide();
	menu_button->get_popup()->hide();

	option_button->add_theme_constant_override("popup_separation", 6);
	menu_button->add_theme_constant_override("popup_separation", 6);
	option_button->show_popup();
	menu_button->show_popup();
	CHECK(option_button->get_popup()->get_position().y == 56);
	CHECK(menu_button->get_popup()->get_position().y == 56);

	memdelete(option_button);
	memdelete(menu_button);
}

TEST_CASE("[SceneTree][OptionButton][MenuButton] Popups show their items as BBCode with the button") {
	OptionButton *option_button = memnew(OptionButton);
	MenuButton *menu_button = memnew(MenuButton);
	CHECK_FALSE(option_button->get_popup()->is_bbcode_enabled());

	option_button->set_bbcode_enabled(true);
	menu_button->set_bbcode_enabled(true);
	CHECK(option_button->get_popup()->is_bbcode_enabled());
	CHECK(menu_button->get_popup()->is_bbcode_enabled());

	option_button->set_bbcode_enabled(false);
	CHECK_FALSE(option_button->get_popup()->is_bbcode_enabled());

	memdelete(option_button);
	memdelete(menu_button);
}

} // namespace TestOptionButton

#endif // ADVANCED_GUI_DISABLED
