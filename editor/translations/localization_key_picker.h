/**************************************************************************/
/*  localization_key_picker.h                                             */
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

#include "editor/inspector/editor_inspector.h"
#include "editor/translations/translation_document.h"
#include "scene/gui/box_container.h"

class Button;
class Label;
class LineEdit;
class OptionButton;
class PopupPanel;
class Tree;

// Shown below the translatable texts of nodes: picks a key of the translations of the project and previews its text.
// Hidden while the node doesn't translate its texts.
class LocalizationKeyPicker : public HBoxContainer {
	GDCLASS(LocalizationKeyPicker, HBoxContainer);

	ObjectID object_id;
	StringName property;

	Vector<Ref<TranslationDocument>> documents;
	String shown_value;
	Variant shown_arguments;
	String shown_preview_locale;

	Button *pick_button = nullptr;
	Label *preview = nullptr;
	PopupPanel *popup = nullptr;
	LineEdit *search = nullptr;
	OptionButton *file_filter = nullptr;
	String filter_source; // The translations shown, all of them when empty.
	Tree *keys = nullptr;

	String _get_display_locale(const Ref<TranslationDocument> &p_document) const;
	String _get_value() const;
	Variant _get_arguments() const;
	void _set_documents(const Vector<Ref<TranslationDocument>> &p_documents);
	void _project_settings_changed();
	void _update_preview();
	void _update_file_filter();
	void _file_filter_selected(int p_index);
	void _update_keys();
	void _popup_keys();
	void _key_activated();

protected:
	void _notification(int p_what);

public:
	// Translatable texts of nodes, like the properties extracted to POT files.
	static bool is_text_property(const String &p_path);

	LocalizationKeyPicker(Object *p_object, const StringName &p_property);
};

class EditorInspectorPluginLocalizationKey : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginLocalizationKey, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual bool parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide = false) override;
};
