/**************************************************************************/
/*  localization_key_picker.cpp                                           */
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

#include "localization_key_picker.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/string/translation_server.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup.h"
#include "scene/gui/tree.h"
#include "scene/main/node.h"
#include "scene/resources/style_box_flat.h"

bool LocalizationKeyPicker::is_text_property(const String &p_path) {
	const String name = p_path.get_slicec('/', p_path.get_slice_count("/") - 1);
	return name == "text" || name == "title" || name.ends_with("_text");
}

// The preview locale of the editor, or else the fallback locale.
String LocalizationKeyPicker::_get_display_locale(const Ref<TranslationDocument> &p_document) const {
	const String preview_locale = EditorNode::get_singleton()->get_preview_locale();
	if (!preview_locale.is_empty() && p_document->has_locale(preview_locale)) {
		return TranslationServer::get_singleton()->standardize_locale(preview_locale);
	}
	const String fallback = GLOBAL_GET("internationalization/locale/fallback");
	if (p_document->has_locale(fallback)) {
		return TranslationServer::get_singleton()->standardize_locale(fallback);
	}
	const PackedStringArray locales = p_document->get_locales();
	return locales.is_empty() ? String() : locales[0];
}

String LocalizationKeyPicker::_get_value() const {
	Object *object = ObjectDB::get_instance(object_id);
	return object ? String(object->get(property)) : String();
}

// The arguments formatted in the text, for the text of text controls.
Variant LocalizationKeyPicker::_get_arguments() const {
	Object *object = ObjectDB::get_instance(object_id);
	return (object && property == SNAME("text")) ? object->get(SNAME("text_arguments")) : Variant();
}

void LocalizationKeyPicker::_update_preview() {
	shown_value = _get_value();
	shown_arguments = _get_arguments();
	shown_preview_locale = EditorNode::get_singleton()->get_preview_locale();

	const StringName key = shown_value.strip_edges();
	for (const Ref<TranslationDocument> &document : documents) {
		if (key == StringName() || !document->has_key(key)) {
			continue;
		}
		const String locale = _get_display_locale(document);
		String text = document->get_text(key, locale);
		if (shown_arguments.get_type() == Variant::DICTIONARY) {
			text = TranslationServer::get_singleton()->format_arguments(text, shown_arguments, locale);
		}
		if (text.is_empty()) {
			preview->set_text(vformat(TTR("[%s] No text in this locale."), locale));
			preview->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
		} else {
			preview->set_text(vformat("[%s] %s", locale, text.replace("\n", " ")));
			preview->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("success_color"), EditorStringName(Editor)));
		}
		preview->set_tooltip_text(vformat(TTR("Key of %s:"), document->get_display_name()) + "\n" + text);
		return;
	}
	preview->set_text(TTR("Not a localization key."));
	preview->set_tooltip_text(String());
	preview->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor)));
}

void LocalizationKeyPicker::_update_keys() {
	const String filter = search->get_text().strip_edges();
	const String value = _get_value().strip_edges();
	keys->clear();
	TreeItem *root = keys->create_item();
	TreeItem *selected = nullptr;
	for (const Ref<TranslationDocument> &document : documents) {
		if (!filter_source.is_empty() && document->get_source_path() != filter_source) {
			continue;
		}
		const String locale = _get_display_locale(document);
		for (const String &key : document->get_keys()) {
			if (!document->has_key(key)) {
				continue; // Only used with contexts, which the text of nodes don't have.
			}
			const String text = document->get_text(key, locale);
			if (!filter.is_empty() && !key.containsn(filter) && !text.containsn(filter)) {
				continue;
			}
			TreeItem *item = keys->create_item(root);
			item->set_disable_folding(true);
			item->set_text(0, key);
			item->set_text(1, text.replace("\n", " "));
			item->set_tooltip_text(1, text);
			if (key == value && !selected) {
				selected = item;
			}
		}
	}
	if (selected) {
		selected->select(0);
		keys->scroll_to_item(selected, true);
	} else if (root->get_first_child()) {
		root->get_first_child()->select(0);
	}
}

// The preview follows the edits of the translations, and the translations follow the project settings.
void LocalizationKeyPicker::_set_documents(const Vector<Ref<TranslationDocument>> &p_documents) {
	const Callable update = callable_mp(this, &LocalizationKeyPicker::_update_preview);
	for (const Ref<TranslationDocument> &document : documents) {
		document->disconnect_changed(update);
	}
	documents = p_documents;
	for (const Ref<TranslationDocument> &document : documents) {
		document->connect_changed(update);
	}
}

void LocalizationKeyPicker::_project_settings_changed() {
	_set_documents(TranslationDocument::get_project_documents());
	_update_preview();
}

// The translations to pick keys from, shown when the project has several.
void LocalizationKeyPicker::_update_file_filter() {
	file_filter->clear();
	file_filter->add_item(TTR("All Translations"));
	file_filter->set_item_metadata(-1, String());
	bool found = false;
	for (const Ref<TranslationDocument> &document : documents) {
		file_filter->add_item(document->get_display_name());
		file_filter->set_item_metadata(-1, document->get_source_path());
		file_filter->set_item_tooltip(-1, document->get_source_path());
		if (document->get_source_path() == filter_source) {
			file_filter->select(file_filter->get_item_count() - 1);
			found = true;
		}
	}
	if (!found) {
		filter_source = String();
		file_filter->select(0);
	}
	file_filter->set_visible(documents.size() > 1);
}

void LocalizationKeyPicker::_file_filter_selected(int p_index) {
	filter_source = file_filter->get_item_metadata(p_index);
	_update_keys();
}

void LocalizationKeyPicker::_popup_keys() {
	_set_documents(TranslationDocument::get_project_documents());
	_update_file_filter();
	search->clear();
	_update_keys();
	const Size2 size = Size2(600, 350) * EDSCALE;
	popup->popup(Rect2i(pick_button->get_screen_position() + Vector2(0, pick_button->get_size().y), size));
	search->grab_focus();
}

void LocalizationKeyPicker::_key_activated() {
	TreeItem *item = keys->get_selected();
	if (!item) {
		return;
	}
	popup->hide();
	Object *object = ObjectDB::get_instance(object_id);
	ERR_FAIL_NULL(object);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(vformat(TTR("Set %s"), property), UndoRedo::MERGE_DISABLE, object);
	undo_redo->add_do_property(object, property, item->get_text(0));
	undo_redo->add_undo_property(object, property, object->get(property));
	undo_redo->commit_action();
}

void LocalizationKeyPicker::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_process_internal(true);
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			pick_button->set_button_icon(get_editor_theme_icon(SNAME("Translation")));
			search->set_right_icon(get_editor_theme_icon(SNAME("Search")));
			{
				// Rounded like the controls of the editor, as the popup has a transparent background.
				Ref<StyleBoxFlat> style = get_theme_stylebox(SceneStringName(panel), SNAME("PopupPanel"));
				if (style.is_valid()) {
					style = style->duplicate();
					style->set_corner_radius_all(int(EDITOR_GET("interface/theme/corner_radius")) * EDSCALE);
					popup->add_theme_style_override(SceneStringName(panel), style);
				}
			}
			_update_preview();
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			const Node *node = Object::cast_to<Node>(ObjectDB::get_instance(object_id));
			set_visible(node && node->can_auto_translate());

			// The text is also changed by undo and the other editors of the property.
			if (_get_value() != shown_value || _get_arguments() != shown_arguments || EditorNode::get_singleton()->get_preview_locale() != shown_preview_locale) {
				_update_preview();
			}
		} break;
	}
}

LocalizationKeyPicker::LocalizationKeyPicker(Object *p_object, const StringName &p_property) {
	object_id = p_object->get_instance_id();
	property = p_property;
	_set_documents(TranslationDocument::get_project_documents());
	ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &LocalizationKeyPicker::_project_settings_changed));

	pick_button = memnew(Button);
	pick_button->set_flat(true);
	pick_button->set_tooltip_text(TTRC("Pick a key of the translations of the project."));
	pick_button->set_accessibility_name(TTRC("Pick Localization Key"));
	pick_button->connect(SceneStringName(pressed), callable_mp(this, &LocalizationKeyPicker::_popup_keys));
	add_child(pick_button);

	preview = memnew(Label);
	preview->set_h_size_flags(SIZE_EXPAND_FILL);
	preview->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	preview->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	preview->set_mouse_filter(MOUSE_FILTER_PASS);
	preview->set_focus_mode(FOCUS_ACCESSIBILITY);
	add_child(preview);

	popup = memnew(PopupPanel);
	add_child(popup);
	VBoxContainer *vbox = memnew(VBoxContainer);
	popup->add_child(vbox);

	search = memnew(LineEdit);
	search->set_placeholder(TTRC("Filter Keys and Texts"));
	search->set_accessibility_name(TTRC("Filter Keys and Texts"));
	search->set_clear_button_enabled(true);
	search->connect(SceneStringName(text_changed), callable_mp(this, &LocalizationKeyPicker::_update_keys).unbind(1));
	search->connect(SceneStringName(text_submitted), callable_mp(this, &LocalizationKeyPicker::_key_activated).unbind(1));
	search->set_h_size_flags(SIZE_EXPAND_FILL);
	HBoxContainer *filters = memnew(HBoxContainer);
	vbox->add_child(filters);
	filters->add_child(search);

	file_filter = memnew(OptionButton);
	file_filter->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	file_filter->set_tooltip_text(TTRC("Show the keys of one translation file of the project."));
	file_filter->set_accessibility_name(TTRC("Filter by File"));
	file_filter->set_fit_to_longest_item(false);
	file_filter->set_custom_minimum_size(Size2(150 * EDSCALE, 0));
	file_filter->connect(SceneStringName(item_selected), callable_mp(this, &LocalizationKeyPicker::_file_filter_selected));
	filters->add_child(file_filter);

	keys = memnew(Tree);
	keys->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	keys->set_hide_root(true);
	keys->set_columns(2);
	keys->set_column_titles_visible(true);
	keys->set_column_title(0, TTRC("Key"));
	keys->set_column_title(1, TTRC("Text"));
	keys->set_column_expand_ratio(0, 2);
	keys->set_column_expand_ratio(1, 3);
	// Like the table of the Localization dock.
	keys->set_theme_type_variation("TreeTable");
	for (int i = 0; i < 2; i++) {
		keys->set_column_clip_content(i, true);
		keys->set_column_title_alignment(i, HORIZONTAL_ALIGNMENT_LEFT);
	}
	keys->set_select_mode(Tree::SELECT_ROW);
	keys->set_v_size_flags(SIZE_EXPAND_FILL);
	keys->set_accessibility_name(TTRC("Localization Keys"));
	keys->connect("item_activated", callable_mp(this, &LocalizationKeyPicker::_key_activated));
	vbox->add_child(keys);
}

bool EditorInspectorPluginLocalizationKey::can_handle(Object *p_object) {
	// Only shown in projects with translations.
	const Vector<String> translations = GLOBAL_GET("internationalization/locale/translations");
	return !translations.is_empty() && Object::cast_to<Node>(p_object) != nullptr;
}

bool EditorInspectorPluginLocalizationKey::parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide) {
	if (p_type != Variant::STRING || !p_usage.has_flag(PROPERTY_USAGE_EDITOR) || p_usage.has_flag(PROPERTY_USAGE_READ_ONLY) || !LocalizationKeyPicker::is_text_property(p_path)) {
		return false;
	}
	// Added below the usual editor of the property.
	add_property_editor(p_path, memnew(LocalizationKeyPicker(p_object, p_path)), true);
	return false;
}
