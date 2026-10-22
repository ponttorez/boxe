/**************************************************************************/
/*  test_localization_key_picker.cpp                                      */
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

TEST_FORCE_LINK(test_localization_key_picker)

#ifdef TOOLS_ENABLED

#include "editor/translations/localization_key_picker.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"

namespace TestLocalizationKeyPicker {

TEST_CASE("[SceneTree][LocalizationKeyPicker] Translated texts") {
	Ref<PackedSceneEditorTranslationParserPlugin> rules;
	rules.instantiate();
	Label *label = memnew(Label);
	LineEdit *line_edit = memnew(LineEdit);
	SceneTree::get_singleton()->get_root()->add_child(label);
	SceneTree::get_singleton()->get_root()->add_child(line_edit);

	CHECK(LocalizationKeyPicker::is_translatable_property(rules, label, "text"));
	CHECK(LocalizationKeyPicker::is_translatable_property(rules, label, "tooltip_text"));
	CHECK(LocalizationKeyPicker::is_translatable_property(rules, line_edit, "placeholder_text"));
	CHECK_FALSE_MESSAGE(LocalizationKeyPicker::is_translatable_property(rules, line_edit, "text"), "What users type isn't translated.");
	CHECK_FALSE(LocalizationKeyPicker::is_translatable_property(rules, label, "name"));

	label->set_auto_translate_mode(Node::AUTO_TRANSLATE_MODE_DISABLED);
	CHECK_FALSE_MESSAGE(LocalizationKeyPicker::is_translatable_property(rules, label, "text"), "Nodes that aren't translated have no key to pick.");

	memdelete(line_edit);
	memdelete(label);
}

} // namespace TestLocalizationKeyPicker

#endif // TOOLS_ENABLED
