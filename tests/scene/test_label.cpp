/**************************************************************************/
/*  test_label.cpp                                                        */
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

TEST_FORCE_LINK(test_label)

#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. Needed for MODULE_TEXT_SERVER_FB_ENABLED and MODULE_TEXT_SERVER_ADV_ENABLED definitions.

#if defined(MODULE_TEXT_SERVER_FB_ENABLED) || defined(MODULE_TEXT_SERVER_ADV_ENABLED)

#include "core/string/translation_server.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestLabel {

TEST_CASE("[SceneTree][Label] Text arguments") {
	Label *test_label = memnew(Label);
	SceneTree::get_singleton()->get_root()->add_child(test_label);

	test_label->set_text("{count} {count|item|items} for {name}");
	CHECK(test_label->get_parsed_text() == "{count} {count|item|items} for {name}");

	test_label->set_text_argument("count", 1);
	test_label->set_text_argument("name", "Godot");
	CHECK(test_label->get_parsed_text() == "1 item for Godot");

	Dictionary arguments;
	arguments["count"] = 3;
	arguments["name"] = "Godot";
	test_label->set_text_arguments(arguments);
	CHECK(test_label->get_parsed_text() == "3 items for Godot");

	// No arguments, no formatting.
	test_label->set_text_arguments(Dictionary());
	CHECK(test_label->get_parsed_text() == "{count} {count|item|items} for {name}");

	memdelete(test_label);
}

TEST_CASE("[SceneTree][Label] Text arguments of a fallback translation") {
	TranslationServer *ts = TranslationServer::get_singleton();
	const String old_locale = ts->get_locale();
	const String old_fallback = ts->get_fallback_locale();
	Ref<Translation> english;
	english.instantiate();
	english->set_locale("en");
	english->add_message("apples", "{count} {count|apple|apples}");
	ts->add_translation(english);
	ts->set_fallback_locale("en");
	ts->set_locale("ru");

	Label *test_label = memnew(Label);
	SceneTree::get_singleton()->get_root()->add_child(test_label);
	test_label->set_text("apples");
	test_label->set_text_argument("count", 21);
	// The English plural rules, as the text is English.
	CHECK(test_label->get_parsed_text() == "21 apples");

	memdelete(test_label);
	ts->remove_translation(english);
	ts->set_fallback_locale(old_fallback);
	ts->set_locale(old_locale);
}

TEST_CASE("[SceneTree][Label] Text arguments of a regional translation") {
	TranslationServer *ts = TranslationServer::get_singleton();
	const String old_locale = ts->get_locale();
	Ref<Translation> brazilian;
	brazilian.instantiate();
	brazilian->set_locale("pt_BR");
	brazilian->add_message("apples", String::utf8("{count} {count|maçã|maçãs}"));
	ts->add_translation(brazilian);
	ts->set_locale("pt_PT");

	Label *test_label = memnew(Label);
	SceneTree::get_singleton()->get_root()->add_child(test_label);
	test_label->set_text("apples");
	test_label->set_text_argument("count", 0);
	// The rules of Brazilian Portuguese, which use the singular for 0, as the text is from the pt_BR translation.
	CHECK(test_label->get_parsed_text() == String::utf8("0 maçã"));

	memdelete(test_label);
	ts->remove_translation(brazilian);
	ts->set_locale(old_locale);
}

TEST_CASE("[SceneTree][Label] Custom minimum size") {
	// This is an anti-regression test case introduced in GH-116640. When the old minimum size behavior is removed, this test case should be removed too.
	Label *test_label = memnew(Label);
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(test_label);

	real_t min_width = 50;

	test_label->set_custom_minimum_size(Size2(min_width, 0));
	test_label->set_text("This is a long text that should be wrapped and exceeds minimum size.");
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase to fit the text with AUTOWRAP_OFF.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_ARBITRARY);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_ARBITRARY.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_WORD.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_WORD_SMART.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	test_label->set_clip_text(true);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with clip_text.");

	test_label->set_clip_text(false);
	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_CHAR);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_CHAR.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_WORD.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS_FORCE);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_ELLIPSIS_FORCE.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS_FORCE);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS_FORCE.");

	memdelete(test_label);
}

TEST_CASE("[SceneTree][Label] Sizing") {
	Label *test_label = memnew(Label);
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(test_label);

	real_t max_width = 50;
	real_t min_width = 25;

	test_label->set_custom_maximum_size(Size2(max_width, -1));
	test_label->set_custom_minimum_size(Size2(min_width, 0));
	test_label->set_text("This is a long text that should be wrapped and exceeds minimum size.");
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_OFF.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_OFF.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_ARBITRARY);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_ARBITRARY.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_ARBITRARY.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_WORD.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_WORD.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_WORD_SMART.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_WORD_SMART.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	test_label->set_clip_text(true);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with clip_text.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with clip_text.");

	test_label->set_clip_text(false);
	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_CHAR);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_CHAR.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_CHAR.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_WORD.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_WORD.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_ELLIPSIS.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_WORD_ELLIPSIS.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS_FORCE);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_ELLIPSIS_FORCE.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_ELLIPSIS_FORCE.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS_FORCE);
	SceneTree::get_singleton()->process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_WORD_ELLIPSIS_FORCE.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS_FORCE.");

	memdelete(test_label);
}

TEST_CASE("[SceneTree][Label] Text arguments don't make BBCode tags") {
	Label *label = memnew(Label);
	SceneTree::get_singleton()->get_root()->add_child(label);
	label->set_text("Hi {name}");
	label->set_text_argument("name", "[b]Player[/b]");
	CHECK(label->get_parsed_text() == "Hi [b]Player[/b]");

	label->set_bbcode_enabled(true);
	CHECK_MESSAGE(label->get_parsed_text() == "Hi [b]Player[/b]", "The brackets of the argument are shown as they are.");
	label->set_text("[i]Hi[/i] {name}");
	CHECK_MESSAGE(label->get_parsed_text() == "Hi [b]Player[/b]", "The tags of the text still apply.");

	// A resource gives its path, e.g. for [img]{icon}[/img].
	Ref<Resource> icon;
	icon.instantiate();
	icon->set_path_cache("res://icons/jump.png");
	label->set_text("Press {icon}");
	label->set_text_argument("icon", icon);
	CHECK(label->get_parsed_text() == "Press res://icons/jump.png");

	memdelete(label);
}

} // namespace TestLabel

#endif // MODULE_TEXT_SERVER_FB_ENABLED || MODULE_TEXT_SERVER_ADV_ENABLED
