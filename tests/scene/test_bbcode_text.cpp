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

#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

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

} // namespace TestBBCodeText
