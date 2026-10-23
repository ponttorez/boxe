/**************************************************************************/
/*  translation_document.h                                                */
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

#include "core/object/ref_counted.h"
#include "core/string/translation.h"
#include "core/templates/hash_map.h"

// The keys and texts of a translation source of the project, edited in the Localization dock.
// A source is a CSV file with a column per locale, a folder of PO files with one locale each,
// or another translation file (MO, compiled translations), which can only be read.
// Edits are saved to the files at once, in their own format: entries that didn't change are
// written back as they were read, with their comments, references and flags.
class TranslationDocument : public RefCounted {
	GDCLASS(TranslationDocument, RefCounted);

public:
	enum Format {
		FORMAT_CSV,
		FORMAT_PO,
		FORMAT_READ_ONLY,
	};

private:
	struct Texts {
		StringName key;
		StringName context;
		String plural_key;
		String comment;
		HashMap<String, Vector<String>> texts; // Locale to forms, the first one is the singular.

		bool operator==(const Texts &p_other) const;
	};

	struct Entry : Texts {
		// The entry as read from the files, so unchanged entries are written back as they were.
		bool loaded = false;
		Texts loaded_texts;
		Vector<Vector<String>> csv_rows;
		String csv_raw; // The rows as written in the file.
		HashMap<String, String> po_blocks; // Locale to the lines of the entry in its file.
		HashMap<String, String> po_comments; // Locale to the comment lines that aren't edited here.
		HashMap<String, String> po_extracted; // Locale to the comment of the entry in its file.
		HashMap<String, int> po_order; // Locale to the position of the entry in its file.
	};

	struct PoFile {
		String path;
		String header; // The lines of the header entry, with its comments.
		String obsolete; // Obsolete entries, kept at the end of the file.
	};

	Format format = FORMAT_READ_ONLY;
	String source_path; // The CSV file, the folder of the PO files, or the read-only file.
	// Checksums of the files as read or saved, to read them again when they're changed outside of the editor, and so
	// saving doesn't overwrite those changes.
	HashMap<String, String> source_checksums;
	bool save_conflict = false; // The files were changed outside of the editor while the document has changes.
	Vector<String> locales;
	Vector<Entry> entries;
	HashMap<Translation::MessageKey, int, Translation::MessageKey> entry_indices;
	HashMap<String, String> plural_rules;
	bool saving_queued = false;
	bool loading = false;
	String last_error;
	PackedStringArray saved_files; // The files changed by the last save.
	bool adding_to_project = false; // Until the translations of a new CSV file are in the project.

	// CSV.
	String csv_delimiter = ",";
	bool csv_unescape_keys = false;
	bool csv_unescape_translations = true;
	Vector<String> csv_header;
	String csv_header_raw;
	Vector<String> csv_plural_rules_row;
	String csv_plural_rules_raw;
	HashMap<String, String> loaded_plural_rules;

	// PO.
	HashMap<String, PoFile> po_files; // Locale to file.

	void _rebuild_indices();
	void _content_changed();
	void _save_queued();
	int _find_entry(const StringName &p_key, const StringName &p_context) const;
	Entry *_get_entry(const StringName &p_key, const StringName &p_context);

	Error _write_if_changed(const String &p_path, const String &p_text);

	Error _load_csv();
	Error _save_csv();
	String _encode_csv_row(const Vector<String> &p_row) const;
	String _encode_csv_text(const String &p_text) const;
	String _decode_csv_text(const String &p_text) const;

	Error _load_po();
	Error _load_po_file(const String &p_path);
	Error _save_po();
	Error _save_po_file(const String &p_locale);
	String _make_po_header(const String &p_locale) const;
	int _get_plural_count(const String &p_locale) const;

	Error _load_read_only();

	PackedStringArray _get_imported_files() const;

	PackedStringArray _get_source_files() const;
	bool _is_source_changed(bool p_listing) const;
	void _update_source_state();
	void _reload_imported();

	void _copy_content(const TranslationDocument &p_from);

	// For undo and redo.
	Array _get_entries() const;
	void _set_entries(const Array &p_entries);

protected:
	static void _bind_methods();

public:
	// The documents of the translations of the project, kept loaded while the editor runs.
	static Vector<Ref<TranslationDocument>> get_project_documents();
	// The document of a CSV file, a folder of PO files or a read-only translation file.
	static Ref<TranslationDocument> get_document(Format p_format, const String &p_source);
	// The document of a translation file of the project, or of a CSV or PO file.
	static Ref<TranslationDocument> get_document_for_file(const String &p_path);
	// Paths of the translations listed in the project that don't exist.
	static PackedStringArray get_missing_translations();
	static void clear_cache();

	// Emitted when the texts change, edited or loaded again.
	void emit_changed();
	void connect_changed(const Callable &p_callable);
	void disconnect_changed(const Callable &p_callable);

	Error load(Format p_format, const String &p_path);
	// Reloads the files when they were changed outside of the editor.
	bool reload_if_changed();
	Error save();
	String get_last_error() const { return last_error; }
	// Whether saving was refused, as the files were changed outside of the editor. The changes of the document are kept
	// until the conflict is resolved, by overwriting the files or by reading them again.
	bool has_save_conflict() const { return save_conflict; }
	Error resolve_save_conflict(bool p_overwrite);

	Format get_format() const { return format; }
	bool is_read_only() const { return format == FORMAT_READ_ONLY; }
	String get_source_path() const { return source_path; }
	String get_display_name() const;
	// The translation files of the document, as listed in the project settings.
	PackedStringArray get_translation_files() const;
	// Lists the translation files of the document in the project settings, so the project uses its texts.
	void add_to_project();
	// Whether the project settings list translation files of the document, or will once a new CSV file has texts.
	bool is_in_project();

	void set_locales(const PackedStringArray &p_locales);
	PackedStringArray get_locales() const;
	void add_locale(const String &p_locale);
	void remove_locale(const String &p_locale);
	bool has_locale(const String &p_locale) const;

	void set_locale_plural_rules(const String &p_locale, const String &p_rules);
	String get_locale_plural_rules(const String &p_locale) const;

	PackedStringArray get_keys() const;
	bool has_key(const StringName &p_key, const StringName &p_context = "") const;
	void add_key(const StringName &p_key, const StringName &p_context = "");
	void remove_key(const StringName &p_key, const StringName &p_context = "");
	void rename_key(const StringName &p_key, const StringName &p_new_key, const StringName &p_context = "");
	// Keeps what was read from the files for the entry, like its comments and flags.
	void set_context(const StringName &p_key, const StringName &p_context, const StringName &p_new_context);

	void set_text(const StringName &p_key, const String &p_locale, const String &p_text, const StringName &p_context = "");
	String get_text(const StringName &p_key, const String &p_locale, const StringName &p_context = "") const;

	void set_plural_key(const StringName &p_key, const String &p_plural_key, const StringName &p_context = "");
	String get_plural_key(const StringName &p_key, const StringName &p_context = "") const;
	void set_plural_texts(const StringName &p_key, const String &p_locale, const PackedStringArray &p_texts, const StringName &p_context = "");
	PackedStringArray get_plural_texts(const StringName &p_key, const String &p_locale, const StringName &p_context = "") const;

	void set_comment(const StringName &p_key, const String &p_comment, const StringName &p_context = "");
	String get_comment(const StringName &p_key, const StringName &p_context = "") const;

	// A copy of the texts with everything read from the files, like comments, flags and file headers,
	// so undo restores removed keys and locales as they were.
	Ref<TranslationDocument> get_snapshot() const;
	void set_snapshot(const Ref<TranslationDocument> &p_snapshot);
};

VARIANT_ENUM_CAST(TranslationDocument::Format);
