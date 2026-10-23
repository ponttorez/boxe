/**************************************************************************/
/*  localization_dock.cpp                                                 */
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

#include "localization_dock.h"

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/string/translation_server.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/translations/editor_locale_dialog.h"
#include "editor/translations/template_generator.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/tree.h"

static PackedStringArray _get_entry_forms(const Dictionary &p_entry, const String &p_locale) {
	const Dictionary texts = p_entry.get("texts", Dictionary());
	const Variant forms = texts.get(p_locale, Variant());
	if (forms.get_type() == Variant::PACKED_STRING_ARRAY) {
		return forms;
	}
	if (forms.get_type() == Variant::STRING) {
		return PackedStringArray{ forms };
	}
	return PackedStringArray();
}

Array LocalizationDock::_make_item_id(const StringName &p_key, const StringName &p_context, int p_form) {
	return Array{ p_key, p_context, p_form };
}

int LocalizationDock::_get_plural_count(const String &p_locale, const String &p_rules) {
	Ref<Translation> translation;
	translation.instantiate();
	translation->set_locale(p_locale);
	translation->set_plural_rules_override(p_rules);
	return MAX(1, translation->get_nplurals());
}

int LocalizationDock::_get_entry_index(const Array &p_entries, const StringName &p_key, const StringName &p_context) const {
	for (int i = 0; i < p_entries.size(); i++) {
		const Dictionary entry = p_entries[i];
		if (StringName(entry.get("key", String())) == p_key && StringName(entry.get("context", String())) == p_context) {
			return i;
		}
	}
	return -1;
}

String LocalizationDock::_make_unique_key(const String &p_key, const StringName &p_context) const {
	if (!document->has_key(p_key, p_context)) {
		return p_key;
	}
	int number = 2;
	while (document->has_key(p_key + "_" + itos(number), p_context)) {
		number++;
	}
	return p_key + "_" + itos(number);
}

bool LocalizationDock::_matches_search(const Dictionary &p_entry) const {
	const String filter = search->get_text().strip_edges();
	if (filter.is_empty()) {
		return true;
	}
	for (const char *field : { "key", "context", "plural_key", "comment" }) {
		if (String(p_entry.get(field, String())).containsn(filter)) {
			return true;
		}
	}
	for (const String &locale : document->get_locales()) {
		for (const String &text : _get_entry_forms(p_entry, locale)) {
			if (text.containsn(filter)) {
				return true;
			}
		}
	}
	return false;
}

// The translations of the project, the edited ones and opening others.
void LocalizationDock::_update_document_list() {
	document_select->clear();
	bool edited_listed = false;
	for (const Ref<TranslationDocument> &project_document : TranslationDocument::get_project_documents()) {
		document_select->add_icon_item(get_editor_theme_icon(project_document->is_read_only() ? SNAME("File") : SNAME("Translation")), project_document->get_display_name());
		document_select->set_item_metadata(-1, project_document);
		document_select->set_item_tooltip(-1, String("\n").join(project_document->get_translation_files()));
		if (project_document == document) {
			document_select->select(document_select->get_item_count() - 1);
			edited_listed = true;
		}
	}
	if (document.is_valid() && !edited_listed) {
		// A new CSV file has no imported translations to list until it has texts.
		document_select->add_icon_item(get_editor_theme_icon(SNAME("Translation")), document->get_display_name());
		document_select->set_item_metadata(-1, document);
		document_select->set_item_tooltip(-1, document->get_source_path() + "\n" + TTR("Not in the translations of the project yet, its texts are added to them once it has some."));
		document_select->select(document_select->get_item_count() - 1);
	}
	for (const String &path : TranslationDocument::get_missing_translations()) {
		// Removed outside of the editor, or not there for now, so the project settings still have it.
		document_select->add_icon_item(get_editor_theme_icon(SNAME("StatusWarning")), path.get_file());
		document_select->set_item_metadata(-1, path);
		document_select->set_item_tooltip(-1, path + "\n" + TTR("This translation file doesn't exist. Select it to remove it from the project."));
	}
	if (document_select->get_item_count() > 0) {
		document_select->add_separator();
	}
	document_select->add_icon_item(get_editor_theme_icon(SNAME("New")), TTR("New CSV File..."));
	document_select->set_item_metadata(-1, DOCUMENT_MENU_NEW_CSV);
	document_select->add_icon_item(get_editor_theme_icon(SNAME("Folder")), TTR("New PO Files..."));
	document_select->set_item_metadata(-1, DOCUMENT_MENU_NEW_PO);
	document_select->add_icon_item(get_editor_theme_icon(SNAME("Load")), TTR("Open..."));
	document_select->set_item_metadata(-1, DOCUMENT_MENU_OPEN);
	if (document.is_null()) {
		document_select->select(-1);
		document_select->set_text(TTR("Select Translations"));
	}
}

void LocalizationDock::_document_selected(int p_index) {
	const Variant item = document_select->get_item_metadata(p_index);
	switch (item.get_type()) {
		case Variant::INT: {
			switch (int(item)) {
				case DOCUMENT_MENU_NEW_CSV:
					new_csv_dialog->popup_file_dialog();
					break;
				case DOCUMENT_MENU_NEW_PO:
					new_po_dialog->popup_file_dialog();
					break;
				case DOCUMENT_MENU_OPEN:
					open_dialog->popup_file_dialog();
					break;
			}
			_update_document_list();
		} break;
		case Variant::STRING: {
			missing_path = item;
			remove_missing_dialog->set_text(vformat(TTR("The file \"%s\" doesn't exist.\nRemove it from the translations of the project?"), missing_path));
			remove_missing_dialog->popup_centered();
			_update_document_list();
		} break;
		default: {
			edit(Ref<TranslationDocument>(item));
		} break;
	}
}

void LocalizationDock::_remove_missing_translation() {
	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	PackedStringArray translations = old_translations;
	translations.erase(missing_path);

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Remove Translation from Project"));
	undo_redo->add_do_property(ProjectSettings::get_singleton(), "internationalization/locale/translations", translations);
	undo_redo->add_undo_property(ProjectSettings::get_singleton(), "internationalization/locale/translations", old_translations);
	undo_redo->add_do_method(ProjectSettings::get_singleton(), "save");
	undo_redo->add_undo_method(ProjectSettings::get_singleton(), "save");
	undo_redo->commit_action();
}

// The locale of the first texts of new translations.
String LocalizationDock::_get_new_locale() const {
	const String fallback = GLOBAL_GET("internationalization/locale/fallback");
	return fallback.is_empty() ? String("en") : fallback;
}

// A CSV file in the format of the CSV translations, imported as translations of the project.
void LocalizationDock::_create_csv(const String &p_path) {
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	if (file.is_null()) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("Can't write \"%s\"."), p_path));
		return;
	}
	file->store_csv_line({ "keys", _get_new_locale() });
	file.unref();
	const Ref<TranslationDocument> new_document = TranslationDocument::get_document_for_file(p_path);
	new_document->add_to_project();
	new_document->save();
	edit(new_document);
}

// A folder of PO files, with one file per locale added to the translations of the project.
void LocalizationDock::_create_po_folder(const String &p_path) {
	const Ref<TranslationDocument> new_document = TranslationDocument::get_document(TranslationDocument::FORMAT_PO, p_path);
	if (new_document->get_locales().is_empty()) {
		new_document->add_locale(_get_new_locale());
		new_document->save();
	}
	edit(new_document);
}

void LocalizationDock::_open_file(const String &p_path) {
	if (p_path.get_extension().to_lower() == "csv" && FileAccess::exists(p_path + ".import")) {
		Ref<ConfigFile> config;
		config.instantiate();
		if (config->load(p_path + ".import") == OK && String(config->get_value("remap", "importer", String())) != "csv_translation") {
			EditorNode::get_singleton()->show_warning(vformat(TTR("\"%s\" is not imported as translations.\nSelect \"CSV Translation\" in its import options to edit it here."), p_path));
			_update_document_list();
			return;
		}
	}
	if (p_path.get_extension().to_lower() == "po") {
		// The PO files of a folder are the ones in the translations of the project.
		PackedStringArray translations = GLOBAL_GET("internationalization/locale/translations");
		if (!translations.has(p_path)) {
			translations.push_back(p_path);
			ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", translations);
			ProjectSettings::get_singleton()->save();
		}
	}
	const Ref<TranslationDocument> new_document = TranslationDocument::get_document_for_file(p_path);
	if (new_document.is_null()) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("\"%s\" is not a translation file."), p_path));
		_update_document_list();
		return;
	}
	if (!new_document->get_last_error().is_empty()) {
		EditorNode::get_singleton()->show_warning(new_document->get_last_error());
	}
	new_document->add_to_project();
	edit(new_document);
}

void LocalizationDock::_queue_update() {
	if (update_queued) {
		return;
	}
	update_queued = true;
	callable_mp(this, &LocalizationDock::_update).call_deferred();
}

void LocalizationDock::_add_column(ColumnKind p_kind, const String &p_locale) {
	column_kinds.push_back(p_kind);
	column_locales.push_back(p_locale);
}

// One line per row like other tables, with the full texts in the tooltips and the cell editor.
void LocalizationDock::_set_cell(TreeItem *p_item, int p_column, const String &p_text, bool p_editable) {
	p_item->set_text(p_column, p_text.replace("\n", U" ↵ "));
	p_item->set_tooltip_text(p_column, p_text);
	p_item->set_selectable(p_column, p_editable);
}

void LocalizationDock::_update_columns() {
	column_kinds.clear();
	column_locales.clear();
	_add_column(COLUMN_KEY);
	if (show_context) {
		_add_column(COLUMN_CONTEXT);
	}
	if (show_plural_key) {
		_add_column(COLUMN_PLURAL_KEY);
	}
	if (show_comment) {
		_add_column(COLUMN_COMMENT);
	}
	for (const String &locale : document->get_locales()) {
		_add_column(COLUMN_LOCALE, locale);
	}

	tree->set_columns(column_kinds.size());
	for (int i = 0; i < column_kinds.size(); i++) {
		tree->set_column_expand(i, true);
		tree->set_column_clip_content(i, true);
		tree->set_column_custom_minimum_width(i, 120 * EDSCALE);
		tree->set_column_expand_ratio(i, column_kinds[i] == COLUMN_LOCALE ? 2 : 1);
		tree->set_column_title_alignment(i, HORIZONTAL_ALIGNMENT_LEFT);
		switch (column_kinds[i]) {
			case COLUMN_KEY: {
				tree->set_column_title(i, TTR("Key"));
				tree->set_column_title_tooltip_text(i, TTR("Key of the text, used as the text of nodes and in tr(). Double-click to rename."));
			} break;
			case COLUMN_CONTEXT: {
				tree->set_column_title(i, TTR("Context"));
				tree->set_column_title_tooltip_text(i, TTR("Context telling apart the texts of a same key, used in tr(). Double-click to edit."));
			} break;
			case COLUMN_PLURAL_KEY: {
				tree->set_column_title(i, TTR("Plural Key"));
				tree->set_column_title_tooltip_text(i, TTR("Plural key of the text, used in tr_n(). Double-click to edit."));
			} break;
			case COLUMN_COMMENT: {
				tree->set_column_title(i, TTR("Comment"));
				tree->set_column_title_tooltip_text(i, TTR("Comment for translators. Double-click to edit."));
			} break;
			case COLUMN_LOCALE: {
				// Set with the counts of translated keys.
			} break;
		}
	}
}

// Keys shown by translation status, in all the locales or in one.
void LocalizationDock::_update_filter_menu() {
	if (document.is_valid() && !filter_locale.is_empty() && !document->has_locale(filter_locale)) {
		filter_locale = String();
	}

	PopupMenu *popup = filter_menu->get_popup();
	popup->clear();
	popup->add_separator(TTR("Show"));
	popup->add_radio_check_item(TTR("All Keys"), FILTER_ALL);
	popup->add_radio_check_item(TTR("Not Translated"), FILTER_NOT_TRANSLATED);
	popup->add_radio_check_item(TTR("Translated"), FILTER_TRANSLATED);
	popup->set_item_checked(popup->get_item_index(filter_status), true);

	popup->add_separator(TTR("In"));
	popup->add_radio_check_item(TTR("All Locales"), FILTER_MENU_ALL_LOCALES);
	popup->set_item_checked(-1, filter_locale.is_empty());
	popup->set_item_disabled(-1, filter_status == FILTER_ALL);
	const PackedStringArray locales = document.is_valid() ? document->get_locales() : PackedStringArray();
	for (int i = 0; i < locales.size(); i++) {
		popup->add_radio_check_item(TranslationServer::get_singleton()->get_locale_name(locales[i]) + " [" + locales[i] + "]", FILTER_MENU_LOCALE + i);
		popup->set_item_metadata(-1, locales[i]);
		popup->set_item_checked(-1, locales[i] == filter_locale);
		popup->set_item_disabled(-1, filter_status == FILTER_ALL);
	}

	// The button tells why keys are hidden.
	if (filter_status == FILTER_ALL) {
		filter_menu->set_text(String());
		filter_menu->remove_theme_color_override(SNAME("icon_normal_color"));
		filter_menu->remove_theme_color_override(SceneStringName(font_color));
	} else {
		const String status = filter_status == FILTER_NOT_TRANSLATED ? TTR("Not Translated") : TTR("Translated");
		filter_menu->set_text(status + ": " + (filter_locale.is_empty() ? TTR("all") : filter_locale));
		const Color accent = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		filter_menu->add_theme_color_override(SNAME("icon_normal_color"), accent);
		filter_menu->add_theme_color_override(SceneStringName(font_color), accent);
	}
}

void LocalizationDock::_set_filter(FilterStatus p_status, const String &p_locale) {
	filter_status = p_status;
	filter_locale = p_locale;
	_queue_update();
}

void LocalizationDock::_filter_menu_pressed(int p_id) {
	if (p_id == FILTER_MENU_ALL_LOCALES) {
		_set_filter(filter_status, String());
	} else if (p_id >= FILTER_MENU_LOCALE) {
		PopupMenu *popup = filter_menu->get_popup();
		_set_filter(filter_status, popup->get_item_metadata(popup->get_item_index(p_id)));
	} else {
		_set_filter(FilterStatus(p_id), filter_locale);
	}
}

void LocalizationDock::_update() {
	update_queued = false;
	if (!is_inside_tree()) {
		return; // Updated with the theme once in the tree.
	}
	if (document.is_valid()) {
		// Removed outside of the editor, or changed there.
		const bool removed = document->get_format() == TranslationDocument::FORMAT_PO ? !DirAccess::exists(document->get_source_path()) : !FileAccess::exists(document->get_source_path());
		if (!removed) {
			document->reload_if_changed();
		}
		// Removed from the project settings too.
		if (removed || !document->is_in_project()) {
			edit(Ref<TranslationDocument>());
			return;
		}
	}
	if (document.is_null() && is_visible_in_tree()) {
		const Vector<Ref<TranslationDocument>> project_documents = TranslationDocument::get_project_documents();
		if (!project_documents.is_empty()) {
			edit(project_documents[0]);
			return;
		}
	}

	const bool editing = document.is_valid();
	const bool read_only = editing && document->is_read_only();
	document_tools->set_visible(editing);
	tree->set_visible(editing);
	read_only_label->set_visible(read_only);
	add_key_button->set_visible(!read_only);
	add_locale_button->set_visible(!read_only);
	template_button->set_visible(!read_only);
	_update_document_list();

	// Kept across updates, as the tree is rebuilt after each change.
	Array selected_id;
	int selected_column = 0;
	if (tree->get_selected()) {
		selected_id = tree->get_selected()->get_metadata(0);
		selected_column = MAX(0, tree->get_selected_column());
	}
	HashSet<String> collapsed_keys;
	if (tree->get_root()) {
		for (TreeItem *item = tree->get_root()->get_first_child(); item; item = item->get_next()) {
			if (item->is_collapsed()) {
				const Array id = item->get_metadata(0);
				collapsed_keys.insert(String(id[0]) + "\n" + String(id[1]));
			}
		}
	}
	const double scroll = tree->get_vscroll_bar()->get_value();
	tree->clear();

	if (!editing) {
		empty_message->hide();
		return;
	}
	_update_filter_menu();
	_update_columns();

	const PackedStringArray locales = document->get_locales();
	HashMap<String, int> plural_counts;
	HashMap<String, int> translated_counts;
	int max_plural_count = 1;
	for (const String &locale : locales) {
		plural_counts[locale] = _get_plural_count(locale, document->get_locale_plural_rules(locale));
		max_plural_count = MAX(max_plural_count, plural_counts[locale]);
		translated_counts[locale] = 0;
	}

	Color missing_color = get_theme_color(SNAME("warning_color"), EditorStringName(Editor));
	missing_color.a = 0.15;
	const Color form_color = get_theme_color(SNAME("font_disabled_color"), EditorStringName(Editor));

	TreeItem *root = tree->create_item();
	TreeItem *item_to_select = nullptr;
	TreeItem *item_to_edit = nullptr;
	const Array entries = document->get("entries");
	for (const Variant &v : entries) {
		const Dictionary entry = v;
		const StringName key = entry.get("key", String());
		const StringName context = entry.get("context", String());
		const String plural_key = entry.get("plural_key", String());
		const bool plural = !plural_key.is_empty();

		// The text of a locale is not translated when one of its forms is empty.
		bool all_translated = !locales.is_empty();
		bool filter_locale_translated = false;
		for (const String &locale : locales) {
			const PackedStringArray forms = _get_entry_forms(entry, locale);
			const int count = plural ? plural_counts[locale] : 1;
			bool complete = forms.size() >= count;
			for (int form = 0; complete && form < count; form++) {
				complete = !forms[form].is_empty();
			}
			if (complete) {
				translated_counts[locale]++;
			}
			all_translated = all_translated && complete;
			if (locale == filter_locale) {
				filter_locale_translated = complete;
			}
		}
		if (filter_status != FILTER_ALL) {
			const bool translated = filter_locale.is_empty() ? all_translated : filter_locale_translated;
			if (translated != (filter_status == FILTER_TRANSLATED)) {
				continue;
			}
		}
		if (!_matches_search(entry)) {
			continue;
		}

		TreeItem *parent = nullptr;
		for (int form = 0; form < (plural ? max_plural_count : 1); form++) {
			TreeItem *item = tree->create_item(form == 0 ? root : parent);
			// Only plural keys have a fold arrow, the others aren't indented for it.
			item->set_disable_folding(form > 0 || !plural);
			if (form == 0) {
				parent = item;
				item->set_collapsed(collapsed_keys.has(String(key) + "\n" + String(context)));
			}
			item->set_metadata(0, _make_item_id(key, context, form));

			for (int column = 0; column < column_kinds.size(); column++) {
				switch (column_kinds[column]) {
					case COLUMN_KEY: {
						if (form == 0) {
							_set_cell(item, column, key, true);
						} else {
							_set_cell(item, column, vformat(TTR("Form %d"), form + 1), false);
							item->set_custom_color(column, form_color);
						}
					} break;
					case COLUMN_CONTEXT: {
						_set_cell(item, column, form == 0 ? String(context) : String(), form == 0);
					} break;
					case COLUMN_PLURAL_KEY: {
						_set_cell(item, column, form == 0 ? plural_key : String(), form == 0);
					} break;
					case COLUMN_COMMENT: {
						_set_cell(item, column, form == 0 ? String(entry.get("comment", String())) : String(), form == 0);
					} break;
					case COLUMN_LOCALE: {
						const String &locale = column_locales[column];
						if (form >= (plural ? plural_counts[locale] : 1)) {
							_set_cell(item, column, String(), false);
							break;
						}
						const PackedStringArray forms = _get_entry_forms(entry, locale);
						const String text = form < forms.size() ? forms[form] : String();
						_set_cell(item, column, text, true);
						if (text.is_empty()) {
							item->set_custom_bg_color(column, missing_color);
						}
					} break;
				}
			}

			const Array id = item->get_metadata(0);
			if (id == selected_id) {
				item_to_select = item;
			}
			if (form == 0 && key == pending_edit_key) {
				item_to_edit = item;
			}
		}
	}

	for (int column = 0; column < column_kinds.size(); column++) {
		if (column_kinds[column] == COLUMN_LOCALE) {
			const String &locale = column_locales[column];
			tree->set_column_title(column, vformat("%s [%s]  %d/%d", TranslationServer::get_singleton()->get_locale_name(locale), locale, translated_counts[locale], entries.size()));
			tree->set_column_title_tooltip_text(column, TTR("Double-click a text to edit it, Shift+Enter starts a new line. Right-click for the options of this locale."));
		}
	}

	empty_message->set_visible(entries.is_empty() || locales.is_empty());
	if (locales.is_empty()) {
		empty_message->set_text(TTR("Add a locale to translate the keys."));
	} else if (entries.is_empty()) {
		empty_message->set_text(TTR("Add keys, or add the texts of the scenes and scripts listed in Project Settings > Localization > Template Generation."));
	}

	tree->get_vscroll_bar()->set_value(scroll);
	if (item_to_edit) {
		pending_edit_key = StringName();
		item_to_edit->select(0);
		tree->scroll_to_item(item_to_edit);
		callable_mp(this, &LocalizationDock::_edit_selected_cell).call_deferred();
	} else if (item_to_select) {
		item_to_select->select(MIN(selected_column, column_kinds.size() - 1));
	}
}

void LocalizationDock::_commit_entries(const String &p_action, const Array &p_entries) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(p_action);
	undo_redo->add_do_property(document.ptr(), "entries", p_entries);
	// The snapshot keeps what the entries had in their files, like comments and flags, so removed entries come back whole.
	undo_redo->add_undo_method(document.ptr(), "set_snapshot", document->get_snapshot());
	undo_redo->commit_action();
}

void LocalizationDock::_edit_text(const StringName &p_key, const StringName &p_context, const String &p_locale, int p_form, const String &p_text) {
	const PackedStringArray old_forms = document->get_plural_texts(p_key, p_locale, p_context);
	PackedStringArray forms = old_forms;
	if (forms.size() <= p_form) {
		forms.resize(p_form + 1);
	}
	forms.write[p_form] = p_text;
	while (!forms.is_empty() && forms[forms.size() - 1].is_empty()) {
		forms.resize(forms.size() - 1);
	}
	if (forms == old_forms) {
		return;
	}

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Edit Localization Text"));
	undo_redo->add_do_method(document.ptr(), "set_plural_texts", p_key, p_locale, forms, p_context);
	undo_redo->add_undo_method(document.ptr(), "set_plural_texts", p_key, p_locale, old_forms, p_context);
	undo_redo->commit_action();
}

String LocalizationDock::_get_cell_text(const Array &p_id, int p_column) const {
	const StringName key = p_id[0];
	const StringName context = p_id[1];
	switch (column_kinds[p_column]) {
		case COLUMN_KEY:
			return key;
		case COLUMN_CONTEXT:
			return context;
		case COLUMN_PLURAL_KEY:
			return document->get_plural_key(key, context);
		case COLUMN_COMMENT:
			return document->get_comment(key, context);
		case COLUMN_LOCALE: {
			const PackedStringArray forms = document->get_plural_texts(key, column_locales[p_column], context);
			const int form = p_id[2];
			return form < forms.size() ? forms[form] : String();
		}
	}
	return String();
}

void LocalizationDock::_edit_cell(TreeItem *p_item, int p_column) {
	if (document->is_read_only() || !p_item || p_column < 0 || p_column >= column_kinds.size() || !p_item->is_selectable(p_column)) {
		return;
	}
	cell_edit_id = p_item->get_metadata(0);
	cell_edit_column = p_column;
	cell_edit_done = false;
	const String text = _get_cell_text(cell_edit_id, p_column);

	// Texts and comments can have several lines.
	const bool multiline = column_kinds[p_column] == COLUMN_LOCALE || column_kinds[p_column] == COLUMN_COMMENT;
	Rect2 rect = tree->get_item_rect(p_item, p_column);
	rect.position += tree->get_screen_position();
	if (multiline) {
		rect.size.height = MAX(rect.size.height, 100 * EDSCALE);
		cell_line_edit->hide();
		cell_text_edit->show();
		cell_text_edit->set_text(text);
		cell_text_edit->select_all();
		cell_text_edit->set_accessibility_name(tree->get_column_title(p_column));
	} else {
		cell_text_edit->hide();
		cell_line_edit->show();
		cell_line_edit->set_text(text);
		cell_line_edit->select_all();
		cell_line_edit->set_accessibility_name(tree->get_column_title(p_column));
	}
	cell_popup->set_position(rect.position);
	cell_popup->set_size(rect.size);
	cell_popup->popup();
	if (multiline) {
		cell_text_edit->grab_focus();
	} else {
		cell_line_edit->grab_focus();
	}
}

void LocalizationDock::_edit_selected_cell() {
	_edit_cell(tree->get_selected(), tree->get_selected_column());
}

// Enter applies the edit, Shift+Enter starts a new line.
void LocalizationDock::_cell_text_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventKey> key = p_event;
	if (key.is_valid() && key->is_pressed() && (key->get_keycode() == Key::ENTER || key->get_keycode() == Key::KP_ENTER)) {
		if (key->is_shift_pressed()) {
			cell_text_edit->insert_text_at_caret("\n");
		} else {
			_finish_cell_edit(true);
		}
		cell_text_edit->accept_event();
	}
}

void LocalizationDock::_finish_cell_edit(bool p_apply) {
	if (cell_edit_done) {
		return;
	}
	cell_edit_done = true;
	cell_popup->hide();
	if (p_apply && document.is_valid() && cell_edit_column >= 0 && cell_edit_column < column_kinds.size()) {
		const String text = cell_line_edit->is_visible() ? cell_line_edit->get_text() : cell_text_edit->get_text();
		if (text != _get_cell_text(cell_edit_id, cell_edit_column)) {
			_apply_cell_edit(cell_edit_id, cell_edit_column, text);
		}
	}
	tree->grab_focus();
}

// Clicking outside applies the edit, Escape cancels it.
void LocalizationDock::_cell_popup_hidden() {
	_finish_cell_edit(cell_popup->get_hide_reason() != Popup::HIDE_REASON_CANCELED);
}

void LocalizationDock::_apply_cell_edit(const Array &p_id, int p_column, const String &p_text) {
	const int column = p_column;
	const StringName key = p_id[0];
	const StringName context = p_id[1];
	const int form = p_id[2];
	const String &text = p_text;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();

	switch (column_kinds[column]) {
		case COLUMN_KEY: {
			const StringName new_key = text.strip_edges();
			if (new_key == key) {
				_queue_update();
				break;
			}
			if (new_key == StringName() || document->has_key(new_key, context)) {
				EditorNode::get_singleton()->show_warning(new_key == StringName() ? TTR("The key can't be empty.") : vformat(TTR("The key \"%s\" already exists."), new_key));
				_queue_update();
				break;
			}
			undo_redo->create_action(TTR("Rename Localization Key"));
			undo_redo->add_do_method(document.ptr(), "rename_key", key, new_key, context);
			undo_redo->add_undo_method(document.ptr(), "rename_key", new_key, key, context);
			undo_redo->commit_action();
		} break;

		case COLUMN_CONTEXT: {
			const StringName new_context = text.strip_edges();
			if (new_context == context) {
				_queue_update();
				break;
			}
			if (document->has_key(key, new_context)) {
				EditorNode::get_singleton()->show_warning(vformat(TTR("The key \"%s\" already exists with this context."), key));
				_queue_update();
				break;
			}
			Array entries = document->get("entries");
			Dictionary entry = entries[_get_entry_index(entries, key, context)];
			if (new_context == StringName()) {
				entry.erase("context");
			} else {
				entry["context"] = String(new_context);
			}
			_commit_entries(TTR("Change Localization Key Context"), entries);
		} break;

		case COLUMN_PLURAL_KEY: {
			undo_redo->create_action(TTR("Change Localization Plural Key"));
			undo_redo->add_do_method(document.ptr(), "set_plural_key", key, text.strip_edges(), context);
			undo_redo->add_undo_method(document.ptr(), "set_plural_key", key, document->get_plural_key(key, context), context);
			undo_redo->commit_action();
		} break;

		case COLUMN_COMMENT: {
			undo_redo->create_action(TTR("Change Localization Comment"));
			undo_redo->add_do_method(document.ptr(), "set_comment", key, text, context);
			undo_redo->add_undo_method(document.ptr(), "set_comment", key, document->get_comment(key, context), context);
			undo_redo->commit_action();
		} break;

		case COLUMN_LOCALE: {
			_edit_text(key, context, column_locales[column], form, text);
		} break;
	}
}

void LocalizationDock::_add_key() {
	search->clear();
	filter_status = FILTER_ALL;
	const String key = _make_unique_key("new_key", StringName());
	pending_edit_key = key;

	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Add Localization Key"));
	undo_redo->add_do_method(document.ptr(), "add_key", key);
	undo_redo->add_undo_method(document.ptr(), "remove_key", key);
	undo_redo->commit_action();
}

// The texts of the scenes and scripts found by the template generation, like in a POT file.
void LocalizationDock::_add_template_keys() {
	const PackedStringArray sources = GLOBAL_GET("internationalization/locale/translations_pot_files");
	if (sources.is_empty()) {
		EditorNode::get_singleton()->show_warning(TTR("Add the scenes and scripts to translate in Project Settings > Localization > Template Generation first."));
		return;
	}
	Array entries = document->get("entries");
	int added = 0;
	for (const Vector<String> &message : TranslationTemplateGenerator::get_singleton()->get_messages()) {
		if (document->has_key(message[0], message[1])) {
			continue;
		}
		Dictionary entry;
		entry["key"] = message[0];
		if (!message[1].is_empty()) {
			entry["context"] = message[1];
		}
		if (!message[2].is_empty()) {
			entry["plural_key"] = message[2];
		}
		if (!message[3].is_empty()) {
			entry["comment"] = message[3];
		}
		entry["texts"] = Dictionary();
		entries.push_back(entry);
		added++;
	}
	if (added == 0) {
		EditorToaster::get_singleton()->popup_str(TTR("All the texts of the template are already there."));
		return;
	}
	search->clear();
	_commit_entries(TTR("Add Localization Keys from Template"), entries);
	EditorToaster::get_singleton()->popup_str(vformat(TTRN("Added %d key from the template.", "Added %d keys from the template.", added), added));
}

void LocalizationDock::_locale_selected(const String &p_locale) {
	if (document->has_locale(p_locale)) {
		return;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Add Localization Locale"));
	undo_redo->add_do_method(document.ptr(), "add_locale", p_locale);
	undo_redo->add_undo_method(document.ptr(), "set_snapshot", document->get_snapshot());
	undo_redo->commit_action();
}

void LocalizationDock::_view_option_pressed(int p_option) {
	PopupMenu *popup = view_menu->get_popup();
	const int index = popup->get_item_index(p_option);
	popup->set_item_checked(index, !popup->is_item_checked(index));
	show_context = popup->is_item_checked(popup->get_item_index(VIEW_CONTEXT));
	show_plural_key = popup->is_item_checked(popup->get_item_index(VIEW_PLURAL_KEY));
	show_comment = popup->is_item_checked(popup->get_item_index(VIEW_COMMENT));
	_queue_update();
}

void LocalizationDock::_item_mouse_selected(const Vector2 &p_position, MouseButton p_button) {
	TreeItem *item = tree->get_selected();
	if (p_button != MouseButton::RIGHT || !item || document->is_read_only()) {
		return;
	}
	menu_item_id = item->get_metadata(0);
	const bool plural = !document->get_plural_key(menu_item_id[0], menu_item_id[1]).is_empty();

	key_menu->clear();
	key_menu->add_icon_item(get_editor_theme_icon(SNAME("Duplicate")), TTR("Duplicate"), KEY_MENU_DUPLICATE);
	if (plural) {
		key_menu->add_item(TTR("Make Singular"), KEY_MENU_MAKE_SINGULAR);
	} else {
		key_menu->add_item(TTR("Make Plural"), KEY_MENU_MAKE_PLURAL);
	}
	key_menu->add_separator();
	key_menu->add_icon_item(get_editor_theme_icon(SNAME("Remove")), TTR("Delete"), KEY_MENU_DELETE);
	key_menu->set_position(tree->get_screen_position() + p_position);
	key_menu->reset_size();
	key_menu->popup();
}

void LocalizationDock::_key_menu_pressed(int p_option) {
	const StringName key = menu_item_id[0];
	const StringName context = menu_item_id[1];
	Array entries = document->get("entries");
	const int index = _get_entry_index(entries, key, context);
	ERR_FAIL_COND(index < 0);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();

	switch (p_option) {
		case KEY_MENU_DUPLICATE: {
			Dictionary copy = Dictionary(entries[index]).duplicate(true);
			copy["key"] = _make_unique_key(String(key) + "_copy", context);
			entries.insert(index + 1, copy);
			_commit_entries(TTR("Duplicate Localization Key"), entries);
		} break;

		case KEY_MENU_MAKE_PLURAL: {
			undo_redo->create_action(TTR("Make Localization Key Plural"));
			undo_redo->add_do_method(document.ptr(), "set_plural_key", key, String(key) + "_plural", context);
			undo_redo->add_undo_method(document.ptr(), "set_plural_key", key, String(), context);
			undo_redo->commit_action();
		} break;

		case KEY_MENU_MAKE_SINGULAR: {
			// Only the singular forms are kept.
			Dictionary entry = entries[index];
			entry.erase("plural_key");
			Dictionary texts = entry.get("texts", Dictionary());
			for (const String &locale : document->get_locales()) {
				const PackedStringArray forms = _get_entry_forms(entry, locale);
				if (!forms.is_empty()) {
					texts[locale] = forms[0];
				}
			}
			_commit_entries(TTR("Make Localization Key Singular"), entries);
		} break;

		case KEY_MENU_DELETE: {
			entries.remove_at(index);
			_commit_entries(TTR("Delete Localization Key"), entries);
		} break;
	}
}

void LocalizationDock::_column_title_clicked(int p_column, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT || document->is_read_only() || p_column >= column_kinds.size() || column_kinds[p_column] != COLUMN_LOCALE) {
		return;
	}
	menu_locale = column_locales[p_column];
	const PackedStringArray locales = document->get_locales();
	const int index = locales.find(menu_locale);
	locale_menu->set_item_disabled(locale_menu->get_item_index(LOCALE_MENU_MOVE_LEFT), index <= 0);
	locale_menu->set_item_disabled(locale_menu->get_item_index(LOCALE_MENU_MOVE_RIGHT), index >= locales.size() - 1);
	locale_menu->set_position(tree->get_screen_position() + tree->get_local_mouse_position());
	locale_menu->reset_size();
	locale_menu->popup();
}

void LocalizationDock::_locale_menu_pressed(int p_option) {
	const String locale_name = TranslationServer::get_singleton()->get_locale_name(menu_locale) + " [" + menu_locale + "]";
	switch (p_option) {
		case LOCALE_MENU_MOVE_LEFT:
		case LOCALE_MENU_MOVE_RIGHT: {
			const PackedStringArray old_locales = document->get_locales();
			PackedStringArray locales = old_locales;
			const int index = locales.find(menu_locale);
			const int other = index + (p_option == LOCALE_MENU_MOVE_LEFT ? -1 : 1);
			ERR_FAIL_INDEX(other, locales.size());
			SWAP(locales.write[index], locales.write[other]);

			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->create_action(TTR("Move Localization Locale"));
			undo_redo->add_do_property(document.ptr(), "locales", locales);
			undo_redo->add_undo_property(document.ptr(), "locales", old_locales);
			undo_redo->commit_action();
		} break;

		case LOCALE_MENU_SHOW_NOT_TRANSLATED: {
			_set_filter(FILTER_NOT_TRANSLATED, menu_locale);
		} break;

		case LOCALE_MENU_PLURAL_RULES: {
			plural_rules_dialog->set_title(vformat(TTR("Plural Rules of %s"), locale_name));
			plural_rules_edit->set_text(document->get_locale_plural_rules(menu_locale));
			plural_rules_dialog->popup_centered(Size2(500, 0) * EDSCALE);
			plural_rules_edit->grab_focus();
		} break;

		case LOCALE_MENU_REMOVE: {
			if (document->get_format() == TranslationDocument::FORMAT_PO) {
				remove_locale_dialog->set_text(vformat(TTR("Remove %s and all its texts?\nIts PO file is moved to the trash."), locale_name));
			} else {
				remove_locale_dialog->set_text(vformat(TTR("Remove %s and its column with all its texts from the CSV file?"), locale_name));
			}
			remove_locale_dialog->popup_centered();
		} break;
	}
}

void LocalizationDock::_remove_locale_confirmed() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Remove Localization Locale"));
	undo_redo->add_do_method(document.ptr(), "remove_locale", menu_locale);
	// The snapshot keeps the PO file of the locale, its name, header and obsolete entries, so undo writes it back as it was.
	undo_redo->add_undo_method(document.ptr(), "set_snapshot", document->get_snapshot());
	undo_redo->commit_action();
}

void LocalizationDock::_plural_rules_confirmed() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->create_action(TTR("Change Localization Plural Rules"));
	undo_redo->add_do_method(document.ptr(), "set_locale_plural_rules", menu_locale, plural_rules_edit->get_text().strip_edges());
	undo_redo->add_undo_method(document.ptr(), "set_locale_plural_rules", menu_locale, document->get_locale_plural_rules(menu_locale));
	undo_redo->commit_action();
}

void LocalizationDock::edit(const Ref<TranslationDocument> &p_document) {
	if (p_document == document) {
		return;
	}
	if (document.is_valid()) {
		document->disconnect_changed(callable_mp(this, &LocalizationDock::_queue_update));
	}
	document = p_document;
	if (document.is_valid()) {
		document->connect_changed(callable_mp(this, &LocalizationDock::_queue_update));

		// Show the optional columns the translations use.
		bool has_contexts = false;
		bool has_plurals = false;
		const Array entries = document->get("entries");
		for (const Variant &v : entries) {
			const Dictionary entry = v;
			has_contexts = has_contexts || entry.has("context");
			has_plurals = has_plurals || entry.has("plural_key");
		}
		PopupMenu *popup = view_menu->get_popup();
		if (has_contexts && !show_context) {
			_view_option_pressed(VIEW_CONTEXT);
		}
		if (has_plurals && !show_plural_key) {
			_view_option_pressed(VIEW_PLURAL_KEY);
		}
		popup->set_item_checked(popup->get_item_index(VIEW_COMMENT), show_comment);
	}
	search->clear();
	filter_status = FILTER_ALL;
	filter_locale = String();
	tree->clear();
	_update();
}

void LocalizationDock::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (document.is_null() && is_visible_in_tree()) {
				_queue_update();
			}
		} break;

		case NOTIFICATION_ENTER_TREE: {
			ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &LocalizationDock::_queue_update));
			EditorFileSystem::get_singleton()->connect("filesystem_changed", callable_mp(this, &LocalizationDock::_queue_update));
		} break;

		case NOTIFICATION_EXIT_TREE: {
			ProjectSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &LocalizationDock::_queue_update));
			EditorFileSystem::get_singleton()->disconnect("filesystem_changed", callable_mp(this, &LocalizationDock::_queue_update));
		} break;

		case NOTIFICATION_THEME_CHANGED: {
			search->set_right_icon(get_editor_theme_icon(SNAME("Search")));
			add_key_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
			add_locale_button->set_button_icon(get_editor_theme_icon(SNAME("Translation")));
			template_button->set_button_icon(get_editor_theme_icon(SNAME("Load")));
			view_menu->set_button_icon(get_editor_theme_icon(SNAME("GuiVisibilityVisible")));
			filter_menu->set_button_icon(get_editor_theme_icon(SNAME("AnimationFilter")));
			locale_menu->set_item_icon(locale_menu->get_item_index(LOCALE_MENU_SHOW_NOT_TRANSLATED), get_editor_theme_icon(SNAME("AnimationFilter")));
			locale_menu->set_item_icon(locale_menu->get_item_index(LOCALE_MENU_MOVE_LEFT), get_editor_theme_icon(SNAME("MoveLeft")));
			locale_menu->set_item_icon(locale_menu->get_item_index(LOCALE_MENU_MOVE_RIGHT), get_editor_theme_icon(SNAME("MoveRight")));
			locale_menu->set_item_icon(locale_menu->get_item_index(LOCALE_MENU_REMOVE), get_editor_theme_icon(SNAME("Remove")));
			_queue_update();
		} break;
	}
}

LocalizationDock::LocalizationDock() {
	singleton = this;
	set_name(TTRC("Localization"));
	set_icon_name("Translation");
	set_dock_shortcut(ED_SHORTCUT_AND_COMMAND("bottom_panels/toggle_localization_bottom_panel", TTRC("Toggle Localization Dock")));
	set_default_slot(EditorDock::DOCK_SLOT_BOTTOM);
	set_available_layouts(EditorDock::DOCK_LAYOUT_ALL);
	set_closable(true);
	set_custom_minimum_size(Size2(0, 250 * EDSCALE));

	VBoxContainer *vbox = memnew(VBoxContainer);
	add_child(vbox);

	HBoxContainer *toolbar = memnew(HBoxContainer);
	vbox->add_child(toolbar);

	document_select = memnew(OptionButton);
	document_select->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	document_select->set_accessibility_name(TTRC("Edited Translations"));
	document_select->set_fit_to_longest_item(false);
	document_select->set_custom_minimum_size(Size2(160 * EDSCALE, 0));
	document_select->set_tooltip_text(TTRC("The translations edited in the dock, from Project Settings > Localization > Translations."));
	document_select->connect(SceneStringName(item_selected), callable_mp(this, &LocalizationDock::_document_selected));
	toolbar->add_child(document_select);

	open_dialog = memnew(EditorFileDialog);
	open_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
	open_dialog->set_title(TTRC("Open Translations"));
	open_dialog->add_filter("*.csv", TTRC("CSV Translations"));
	List<String> extensions;
	ResourceLoader::get_recognized_extensions_for_type("Translation", &extensions);
	for (const String &extension : extensions) {
		open_dialog->add_filter("*." + extension);
	}
	open_dialog->connect("file_selected", callable_mp(this, &LocalizationDock::_open_file));
	add_child(open_dialog);

	remove_missing_dialog = memnew(ConfirmationDialog);
	remove_missing_dialog->set_title(TTRC("Missing Translation"));
	remove_missing_dialog->set_ok_button_text(TTRC("Remove"));
	remove_missing_dialog->connect(SceneStringName(confirmed), callable_mp(this, &LocalizationDock::_remove_missing_translation));
	add_child(remove_missing_dialog);

	new_csv_dialog = memnew(EditorFileDialog);
	new_csv_dialog->set_file_mode(FileDialog::FILE_MODE_SAVE_FILE);
	new_csv_dialog->set_title(TTRC("New CSV Translations"));
	new_csv_dialog->add_filter("*.csv", TTRC("CSV Translations"));
	new_csv_dialog->set_current_file("translations.csv");
	new_csv_dialog->connect("file_selected", callable_mp(this, &LocalizationDock::_create_csv));
	add_child(new_csv_dialog);

	new_po_dialog = memnew(EditorFileDialog);
	new_po_dialog->set_file_mode(FileDialog::FILE_MODE_OPEN_DIR);
	new_po_dialog->set_title(TTRC("Folder of the PO Files"));
	new_po_dialog->connect("dir_selected", callable_mp(this, &LocalizationDock::_create_po_folder));
	add_child(new_po_dialog);

	// The tools of the edited translations.
	document_tools = memnew(HBoxContainer);
	document_tools->set_h_size_flags(SIZE_EXPAND_FILL);
	toolbar->add_child(document_tools);

	search = memnew(LineEdit);
	search->set_placeholder(TTRC("Filter Keys and Texts"));
	search->set_accessibility_name(TTRC("Filter Keys and Texts"));
	search->set_clear_button_enabled(true);
	search->set_custom_minimum_size(Size2(200 * EDSCALE, 0));
	search->connect(SceneStringName(text_changed), callable_mp(this, &LocalizationDock::_queue_update).unbind(1));
	document_tools->add_child(search);

	filter_menu = memnew(MenuButton);
	filter_menu->set_flat(false);
	filter_menu->set_theme_type_variation("FlatMenuButton");
	filter_menu->set_tooltip_text(TTRC("Show only the keys translated or not in all the locales, or in one locale."));
	filter_menu->set_accessibility_name(TTRC("Filter Keys"));
	filter_menu->get_popup()->set_hide_on_checkable_item_selection(false);
	filter_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &LocalizationDock::_filter_menu_pressed));
	document_tools->add_child(filter_menu);

	Control *spacer = memnew(Control);
	spacer->set_h_size_flags(SIZE_EXPAND_FILL);
	document_tools->add_child(spacer);

	read_only_label = memnew(Label);
	read_only_label->set_text(TTRC("Read-only"));
	read_only_label->set_tooltip_text(TTRC("Only CSV and PO files can be edited here. Edit the files these translations were made from instead."));
	read_only_label->set_mouse_filter(MOUSE_FILTER_PASS);
	read_only_label->set_theme_type_variation("HeaderSmall");
	document_tools->add_child(read_only_label);

	add_key_button = memnew(Button);
	add_key_button->set_text(TTRC("Add Key"));
	add_key_button->set_theme_type_variation(SceneStringName(FlatButton));
	add_key_button->set_tooltip_text(TTRC("Add a key and rename it."));
	add_key_button->connect(SceneStringName(pressed), callable_mp(this, &LocalizationDock::_add_key));
	document_tools->add_child(add_key_button);

	add_locale_button = memnew(Button);
	add_locale_button->set_text(TTRC("Add Locale"));
	add_locale_button->set_theme_type_variation(SceneStringName(FlatButton));
	add_locale_button->set_tooltip_text(TTRC("Add a locale, as a column of the CSV file or as a PO file."));
	document_tools->add_child(add_locale_button);

	template_button = memnew(Button);
	template_button->set_text(TTRC("Add from Template"));
	template_button->set_theme_type_variation(SceneStringName(FlatButton));
	template_button->set_tooltip_text(TTRC("Add the texts of the scenes and scripts listed in Project Settings > Localization > Template Generation which are not there yet."));
	template_button->connect(SceneStringName(pressed), callable_mp(this, &LocalizationDock::_add_template_keys));
	document_tools->add_child(template_button);

	view_menu = memnew(MenuButton);
	view_menu->set_flat(false);
	view_menu->set_theme_type_variation("FlatMenuButton");
	view_menu->set_tooltip_text(TTRC("Shown Columns"));
	view_menu->set_accessibility_name(TTRC("Shown Columns"));
	view_menu->get_popup()->add_check_item(TTRC("Context"), VIEW_CONTEXT);
	view_menu->get_popup()->add_check_item(TTRC("Plural Key"), VIEW_PLURAL_KEY);
	view_menu->get_popup()->add_check_item(TTRC("Comment"), VIEW_COMMENT);
	view_menu->get_popup()->set_item_checked(view_menu->get_popup()->get_item_index(VIEW_COMMENT), show_comment);
	view_menu->get_popup()->set_hide_on_checkable_item_selection(false);
	view_menu->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &LocalizationDock::_view_option_pressed));
	document_tools->add_child(view_menu);

	empty_message = memnew(Label);
	empty_message->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	empty_message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	empty_message->set_theme_type_variation("HeaderSmall");
	vbox->add_child(empty_message);

	tree = memnew(Tree);
	tree->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	tree->set_hide_root(true);
	tree->set_column_titles_visible(true);
	tree->set_allow_rmb_select(true);
	tree->set_theme_type_variation("TreeTable");
	tree->set_v_size_flags(SIZE_EXPAND_FILL);
	tree->set_accessibility_name(TTRC("Localization Keys"));
	tree->connect("item_activated", callable_mp(this, &LocalizationDock::_edit_selected_cell));
	tree->connect("item_mouse_selected", callable_mp(this, &LocalizationDock::_item_mouse_selected));
	tree->connect("column_title_clicked", callable_mp(this, &LocalizationDock::_column_title_clicked));
	vbox->add_child(tree);

	cell_popup = memnew(Popup);
	cell_popup->connect("popup_hide", callable_mp(this, &LocalizationDock::_cell_popup_hidden));
	add_child(cell_popup);
	VBoxContainer *cell_vbox = memnew(VBoxContainer);
	cell_vbox->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	cell_vbox->add_theme_constant_override("separation", 0);
	cell_popup->add_child(cell_vbox);
	cell_line_edit = memnew(LineEdit);
	cell_line_edit->set_v_size_flags(SIZE_EXPAND_FILL);
	cell_line_edit->connect(SceneStringName(text_submitted), callable_mp(this, &LocalizationDock::_finish_cell_edit).bind(true).unbind(1));
	cell_vbox->add_child(cell_line_edit);
	cell_text_edit = memnew(TextEdit);
	cell_text_edit->set_v_size_flags(SIZE_EXPAND_FILL);
	cell_text_edit->set_line_wrapping_mode(TextEdit::LINE_WRAPPING_BOUNDARY);
	cell_text_edit->connect(SceneStringName(gui_input), callable_mp(this, &LocalizationDock::_cell_text_input));
	cell_vbox->add_child(cell_text_edit);

	key_menu = memnew(PopupMenu);
	key_menu->connect(SceneStringName(id_pressed), callable_mp(this, &LocalizationDock::_key_menu_pressed));
	add_child(key_menu);

	locale_menu = memnew(PopupMenu);
	locale_menu->add_item(TTRC("Move Left"), LOCALE_MENU_MOVE_LEFT);
	locale_menu->add_item(TTRC("Move Right"), LOCALE_MENU_MOVE_RIGHT);
	locale_menu->add_item(TTRC("Show Not Translated"), LOCALE_MENU_SHOW_NOT_TRANSLATED);
	locale_menu->add_item(TTRC("Plural Rules..."), LOCALE_MENU_PLURAL_RULES);
	locale_menu->add_separator();
	locale_menu->add_item(TTRC("Remove Locale"), LOCALE_MENU_REMOVE);
	locale_menu->connect(SceneStringName(id_pressed), callable_mp(this, &LocalizationDock::_locale_menu_pressed));
	add_child(locale_menu);

	locale_dialog = memnew(EditorLocaleDialog);
	locale_dialog->connect("locale_selected", callable_mp(this, &LocalizationDock::_locale_selected));
	add_child(locale_dialog);
	add_locale_button->connect(SceneStringName(pressed), callable_mp(locale_dialog, &EditorLocaleDialog::popup_locale_dialog));

	remove_locale_dialog = memnew(ConfirmationDialog);
	remove_locale_dialog->set_title(TTRC("Remove Locale"));
	remove_locale_dialog->set_ok_button_text(TTRC("Remove"));
	remove_locale_dialog->connect(SceneStringName(confirmed), callable_mp(this, &LocalizationDock::_remove_locale_confirmed));
	add_child(remove_locale_dialog);

	plural_rules_dialog = memnew(ConfirmationDialog);
	VBoxContainer *rules_vbox = memnew(VBoxContainer);
	plural_rules_dialog->add_child(rules_vbox);
	Label *rules_help = memnew(Label);
	rules_help->set_text(TTRC("Plural rules in the gettext format, empty to use the default rules of the locale."));
	rules_help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	rules_vbox->add_child(rules_help);
	plural_rules_edit = memnew(LineEdit);
	plural_rules_edit->set_placeholder("nplurals=2; plural=(n != 1);");
	plural_rules_edit->set_accessibility_name(TTRC("Plural Rules"));
	rules_vbox->add_child(plural_rules_edit);
	plural_rules_dialog->register_text_enter(plural_rules_edit);
	plural_rules_dialog->connect(SceneStringName(confirmed), callable_mp(this, &LocalizationDock::_plural_rules_confirmed));
	add_child(plural_rules_dialog);
}

LocalizationDock::~LocalizationDock() {
	singleton = nullptr;
	TranslationDocument::clear_cache();
}
