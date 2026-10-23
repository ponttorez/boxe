/**************************************************************************/
/*  localization_dock.h                                                   */
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

#include "editor/docks/editor_dock.h"
#include "editor/translations/translation_document.h"

class Button;
class ConfirmationDialog;
class EditorFileDialog;
class EditorLocaleDialog;
class HBoxContainer;
class Label;
class LineEdit;
class MenuButton;
class OptionButton;
class Popup;
class PopupMenu;
class TextEdit;
class Tree;
class TreeItem;

// The keys of the translations of the project with their texts in each locale, in a dock.
// It edits CSV and PO files directly, the other translation files are only shown.
class LocalizationDock : public EditorDock {
	GDCLASS(LocalizationDock, EditorDock);

	enum ColumnKind {
		COLUMN_KEY,
		COLUMN_CONTEXT,
		COLUMN_PLURAL_KEY,
		COLUMN_COMMENT,
		COLUMN_LOCALE,
	};

	enum ViewOption {
		VIEW_CONTEXT,
		VIEW_PLURAL_KEY,
		VIEW_COMMENT,
	};

	enum KeyMenuOption {
		KEY_MENU_DUPLICATE,
		KEY_MENU_MAKE_PLURAL,
		KEY_MENU_MAKE_SINGULAR,
		KEY_MENU_DELETE,
	};

	enum FilterStatus {
		FILTER_ALL,
		FILTER_NOT_TRANSLATED,
		FILTER_TRANSLATED,
	};

	enum FilterMenuOption {
		FILTER_MENU_ALL_LOCALES = 100,
		FILTER_MENU_LOCALE, // Followed by one per locale.
	};

	enum DocumentMenuOption {
		DOCUMENT_MENU_NEW_CSV,
		DOCUMENT_MENU_NEW_PO,
		DOCUMENT_MENU_OPEN,
	};

	enum LocaleMenuOption {
		LOCALE_MENU_MOVE_LEFT,
		LOCALE_MENU_MOVE_RIGHT,
		LOCALE_MENU_SHOW_NOT_TRANSLATED,
		LOCALE_MENU_PLURAL_RULES,
		LOCALE_MENU_REMOVE,
	};

	static inline LocalizationDock *singleton = nullptr;

	Ref<TranslationDocument> document;

	OptionButton *document_select = nullptr;
	EditorFileDialog *open_dialog = nullptr;
	EditorFileDialog *new_csv_dialog = nullptr;
	EditorFileDialog *new_po_dialog = nullptr;
	ConfirmationDialog *remove_missing_dialog = nullptr;
	ConfirmationDialog *save_conflict_dialog = nullptr;
	String missing_path;
	HBoxContainer *document_tools = nullptr;
	Label *read_only_label = nullptr;
	LineEdit *search = nullptr;
	MenuButton *filter_menu = nullptr;
	FilterStatus filter_status = FILTER_ALL;
	String filter_locale; // All the locales when empty.
	MenuButton *view_menu = nullptr;
	Button *add_key_button = nullptr;
	Button *add_locale_button = nullptr;
	Button *template_button = nullptr;
	Tree *tree = nullptr;
	Label *empty_message = nullptr;

	// Edits a cell over it, showing the full text of the cells which only show one line.
	Popup *cell_popup = nullptr;
	LineEdit *cell_line_edit = nullptr;
	TextEdit *cell_text_edit = nullptr;
	Array cell_edit_id;
	int cell_edit_column = -1;
	bool cell_edit_done = false;

	PopupMenu *key_menu = nullptr;
	PopupMenu *locale_menu = nullptr;
	EditorLocaleDialog *locale_dialog = nullptr;
	ConfirmationDialog *remove_locale_dialog = nullptr;
	ConfirmationDialog *plural_rules_dialog = nullptr;
	LineEdit *plural_rules_edit = nullptr;

	bool show_context = false;
	bool show_plural_key = false;
	bool show_comment = true;

	Vector<ColumnKind> column_kinds;
	Vector<String> column_locales;
	Array menu_item_id;
	String menu_locale;
	StringName pending_edit_key;
	bool update_queued = false;

	static Array _make_item_id(const StringName &p_key, const StringName &p_context, int p_form);
	int _get_entry_index(const Array &p_entries, const StringName &p_key, const StringName &p_context) const;

	void _update_document_list();
	void _document_selected(int p_index);
	void _create_csv(const String &p_path);
	void _create_po_folder(const String &p_path);
	void _remove_missing_translation();
	void _resolve_save_conflict(bool p_overwrite);
	void _save_conflict_action(const StringName &p_action);
	void _open_file(const String &p_path);
	String _get_new_locale() const;

	void _queue_update();
	void _update();
	void _add_column(ColumnKind p_kind, const String &p_locale = String());
	static void _set_cell(TreeItem *p_item, int p_column, const String &p_text, bool p_editable);
	void _update_columns();
	void _update_filter_menu();
	void _set_filter(FilterStatus p_status, const String &p_locale);

	void _commit_entries(const String &p_action, const Array &p_entries);
	String _get_cell_text(const Array &p_id, int p_column) const;
	void _edit_cell(TreeItem *p_item, int p_column);
	void _edit_selected_cell();
	void _cell_text_input(const Ref<InputEvent> &p_event);
	void _finish_cell_edit(bool p_apply);
	void _cell_popup_hidden();
	void _apply_cell_edit(const Array &p_id, int p_column, const String &p_text);
	void _edit_text(const StringName &p_key, const StringName &p_context, const String &p_locale, int p_form, const String &p_text);

	void _add_key();
	void _add_template_keys();
	void _locale_selected(const String &p_locale);
	void _view_option_pressed(int p_option);
	void _filter_menu_pressed(int p_id);
	void _item_mouse_selected(const Vector2 &p_position, MouseButton p_button);
	void _key_menu_pressed(int p_option);
	void _column_title_clicked(int p_column, MouseButton p_button);
	void _locale_menu_pressed(int p_option);
	void _remove_locale_confirmed();
	void _plural_rules_confirmed();

protected:
	void _notification(int p_what);

public:
	// The logic of the dock, also used by tests. Entries are the dictionaries of TranslationDocument::get("entries").
	static int get_plural_count(const String &p_locale, const String &p_rules);
	// Whether all the forms of the text of the locale are written, p_plural_count of them for a plural key.
	static bool is_entry_translated(const Dictionary &p_entry, const String &p_locale, int p_plural_count);
	// Whether the key, context, plural key, comment or a text of the entry contains p_search, ignoring the case.
	static bool entry_matches_search(const Dictionary &p_entry, const PackedStringArray &p_locales, const String &p_search);
	// p_key, or p_key with a number when the document already has it.
	static String make_unique_key(const Ref<TranslationDocument> &p_document, const String &p_key, const StringName &p_context);

	static LocalizationDock *get_singleton() { return singleton; }

	void edit(const Ref<TranslationDocument> &p_document);
	Ref<TranslationDocument> get_edited_document() const { return document; }

	LocalizationDock();
	~LocalizationDock();
};
