/**************************************************************************/
/*  translation_document.cpp                                              */
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

#include "translation_document.h"

#include "core/config/project_settings.h"
#include "core/core_string_names.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "core/templates/hash_set.h"
#include "editor/file_system/editor_file_system.h"

static const char *TRANSLATIONS_SETTING = "internationalization/locale/translations";

// Documents of the project.

static HashMap<String, Ref<TranslationDocument>> documents;

// The CSV file a translation was imported from, e.g. "texts.csv" for "texts.en.translation".
static String _find_csv_source(const String &p_translation) {
	const String base = p_translation.get_basename();
	const String csv = base.get_basename() + ".csv";
	if (!FileAccess::exists(csv) || !FileAccess::exists(csv + ".import")) {
		return String();
	}
	Ref<ConfigFile> config;
	config.instantiate();
	if (config->load(csv + ".import") != OK || String(config->get_value("remap", "importer", String())) != "csv_translation") {
		return String();
	}
	return csv;
}

static void _add_translation_files(const PackedStringArray &p_add, const PackedStringArray &p_remove = PackedStringArray()) {
	PackedStringArray translations = GLOBAL_GET(TRANSLATIONS_SETTING);
	bool changed = false;
	for (const String &path : p_add) {
		if (!translations.has(path)) {
			translations.push_back(path);
			changed = true;
		}
	}
	for (const String &path : p_remove) {
		if (translations.has(path)) {
			translations.erase(path);
			changed = true;
		}
	}
	if (changed) {
		ProjectSettings::get_singleton()->set_setting(TRANSLATIONS_SETTING, translations);
		if (EditorFileSystem::get_singleton()) {
			ProjectSettings::get_singleton()->save();
		}
	}
}

Vector<Ref<TranslationDocument>> TranslationDocument::get_project_documents() {
	Vector<Ref<TranslationDocument>> ret;
	const PackedStringArray paths = GLOBAL_GET(TRANSLATIONS_SETTING);
	for (const String &path : paths) {
		if (!FileAccess::exists(path)) {
			continue;
		}
		const Ref<TranslationDocument> document = get_document_for_file(path);
		if (document.is_valid() && !ret.has(document)) {
			ret.push_back(document);
		}
	}
	return ret;
}

Ref<TranslationDocument> TranslationDocument::get_document_for_file(const String &p_path) {
	const String extension = p_path.get_extension().to_lower();
	Format format = FORMAT_READ_ONLY;
	String source = p_path;
	if (extension == "po") {
		format = FORMAT_PO;
		source = p_path.get_base_dir();
	} else if (extension == "csv") {
		format = FORMAT_CSV;
	} else if (extension == "translation") {
		const String csv = _find_csv_source(p_path);
		if (!csv.is_empty()) {
			format = FORMAT_CSV;
			source = csv;
		}
	}

	return get_document(format, source);
}

Ref<TranslationDocument> TranslationDocument::get_document(Format p_format, const String &p_source) {
	const String id = itos(p_format) + ":" + p_source;
	Ref<TranslationDocument> *existing = documents.getptr(id);
	if (existing) {
		(*existing)->reload_if_changed();
		return *existing;
	}
	Ref<TranslationDocument> document;
	document.instantiate();
	if (document->load(p_format, p_source) != OK && p_format == FORMAT_READ_ONLY) {
		return Ref<TranslationDocument>();
	}
	documents[id] = document;
	return document;
}

PackedStringArray TranslationDocument::get_missing_translations() {
	PackedStringArray ret;
	const PackedStringArray paths = GLOBAL_GET(TRANSLATIONS_SETTING);
	for (const String &path : paths) {
		if (!FileAccess::exists(path)) {
			ret.push_back(path);
		}
	}
	return ret;
}

void TranslationDocument::clear_cache() {
	documents.clear();
}

// Texts.

bool TranslationDocument::Texts::operator==(const Texts &p_other) const {
	if (key != p_other.key || context != p_other.context || plural_key != p_other.plural_key || comment != p_other.comment || texts.size() != p_other.texts.size()) {
		return false;
	}
	for (const KeyValue<String, Vector<String>> &E : texts) {
		const Vector<String> *other = p_other.texts.getptr(E.key);
		if (!other || *other != E.value) {
			return false;
		}
	}
	return true;
}

// Model.

void TranslationDocument::emit_changed() {
	emit_signal(CoreStringName(changed));
}

void TranslationDocument::connect_changed(const Callable &p_callable) {
	if (!is_connected(CoreStringName(changed), p_callable)) {
		connect(CoreStringName(changed), p_callable);
	}
}

void TranslationDocument::disconnect_changed(const Callable &p_callable) {
	if (is_connected(CoreStringName(changed), p_callable)) {
		disconnect(CoreStringName(changed), p_callable);
	}
}

static bool _same_plural_rules(const HashMap<String, String> &p_a, const HashMap<String, String> &p_b) {
	if (p_a.size() != p_b.size()) {
		return false;
	}
	for (const KeyValue<String, String> &E : p_a) {
		const String *other = p_b.getptr(E.key);
		if (!other || *other != E.value) {
			return false;
		}
	}
	return true;
}

void TranslationDocument::_rebuild_indices() {
	entry_indices.clear();
	for (int i = 0; i < entries.size(); i++) {
		entry_indices[{ entries[i].context, entries[i].key }] = i;
	}
}

void TranslationDocument::_content_changed() {
	emit_changed();
	// Saved at once in the editor, elsewhere save() is called when needed.
	if (loading || is_read_only() || saving_queued || !EditorFileSystem::get_singleton()) {
		return;
	}
	// Several changes of one action are saved together.
	saving_queued = true;
	callable_mp(this, &TranslationDocument::_save_queued).call_deferred();
}

void TranslationDocument::_save_queued() {
	if (!saving_queued) {
		return; // Already saved.
	}
	// A conflict is resolved by the user in the Localization dock.
	if (save() != OK && !save_conflict) {
		ERR_PRINT(vformat("Can't save the translations of '%s': %s", source_path, last_error));
	}
}

int TranslationDocument::_find_entry(const StringName &p_key, const StringName &p_context) const {
	const int *index = entry_indices.getptr({ p_context, p_key });
	return index ? *index : -1;
}

TranslationDocument::Entry *TranslationDocument::_get_entry(const StringName &p_key, const StringName &p_context) {
	const int index = _find_entry(p_key, p_context);
	return index >= 0 ? &entries.write[index] : nullptr;
}

Array TranslationDocument::_get_entries() const {
	Array ret;
	for (const Entry &entry : entries) {
		Dictionary dict;
		dict["key"] = String(entry.key);
		if (entry.context != StringName()) {
			dict["context"] = String(entry.context);
		}
		if (!entry.plural_key.is_empty()) {
			dict["plural_key"] = entry.plural_key;
		}
		if (!entry.comment.is_empty()) {
			dict["comment"] = entry.comment;
		}
		Dictionary texts;
		for (const String &locale : locales) {
			const Vector<String> *forms = entry.texts.getptr(locale);
			if (!forms || forms->is_empty()) {
				continue;
			}
			if (forms->size() == 1) {
				texts[locale] = (*forms)[0];
			} else {
				texts[locale] = PackedStringArray(*forms);
			}
		}
		dict["texts"] = texts;
		ret.push_back(dict);
	}
	return ret;
}

void TranslationDocument::_set_entries(const Array &p_entries) {
	// Entries keep what was read from the files, so the unchanged ones are written back as they were.
	HashMap<Translation::MessageKey, Entry, Translation::MessageKey> previous;
	for (const Entry &entry : entries) {
		previous[{ entry.context, entry.key }] = entry;
	}
	entries.clear();
	for (const Variant &v : p_entries) {
		const Dictionary dict = v;
		Entry entry;
		const StringName key = dict.get("key", String());
		const StringName context = dict.get("context", String());
		const Entry *old = previous.getptr({ context, key });
		if (old) {
			entry = *old;
			entry.texts.clear();
		}
		entry.key = key;
		entry.context = context;
		entry.plural_key = dict.get("plural_key", String());
		entry.comment = dict.get("comment", String());
		const Dictionary texts = dict.get("texts", Dictionary());
		for (const KeyValue<Variant, Variant> &E : texts) {
			if (E.value.get_type() == Variant::PACKED_STRING_ARRAY) {
				entry.texts[String(E.key)] = PackedStringArray(E.value);
			} else {
				entry.texts[String(E.key)] = Vector<String>{ String(E.value) };
			}
		}
		entries.push_back(entry);
	}
	_rebuild_indices();
	_content_changed();
}

void TranslationDocument::_copy_content(const TranslationDocument &p_from) {
	locales = p_from.locales;
	entries = p_from.entries;
	plural_rules = p_from.plural_rules;
	loaded_plural_rules = p_from.loaded_plural_rules;
	csv_header = p_from.csv_header;
	csv_header_raw = p_from.csv_header_raw;
	csv_plural_rules_row = p_from.csv_plural_rules_row;
	csv_plural_rules_raw = p_from.csv_plural_rules_raw;
	po_files = p_from.po_files;
}

Ref<TranslationDocument> TranslationDocument::get_snapshot() const {
	Ref<TranslationDocument> snapshot;
	snapshot.instantiate();
	snapshot->_copy_content(*this);
	return snapshot;
}

void TranslationDocument::set_snapshot(const Ref<TranslationDocument> &p_snapshot) {
	ERR_FAIL_COND(p_snapshot.is_null());
	// The PO files of the locales that the snapshot doesn't have, e.g. when undoing the addition of a locale, are
	// kept, so saving removes them from the disk and the project like the files of removed locales.
	HashMap<String, PoFile> kept_files;
	for (const KeyValue<String, PoFile> &E : po_files) {
		if (!p_snapshot->po_files.has(E.key) && !p_snapshot->locales.has(E.key)) {
			kept_files[E.key] = E.value;
		}
	}
	_copy_content(*p_snapshot.ptr());
	for (const KeyValue<String, PoFile> &E : kept_files) {
		po_files[E.key] = E.value;
	}
	_rebuild_indices();
	_content_changed();
}

String TranslationDocument::get_display_name() const {
	switch (format) {
		case FORMAT_CSV:
			return source_path.get_file();
		case FORMAT_PO:
			return source_path.trim_prefix("res://").path_join("*.po");
		case FORMAT_READ_ONLY:
			return source_path.get_file();
	}
	return source_path;
}

PackedStringArray TranslationDocument::get_translation_files() const {
	PackedStringArray ret;
	const PackedStringArray paths = GLOBAL_GET(TRANSLATIONS_SETTING);
	for (const String &path : paths) {
		const bool belongs = (format == FORMAT_PO && path.get_extension().to_lower() == "po" && path.get_base_dir() == source_path) ||
				(format == FORMAT_CSV && path.get_extension().to_lower() == "translation" && _find_csv_source(path) == source_path) ||
				(format == FORMAT_READ_ONLY && path == source_path);
		if (belongs) {
			ret.push_back(path);
		}
	}
	return ret;
}

void TranslationDocument::set_locales(const PackedStringArray &p_locales) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	locales.clear();
	for (const String &locale : p_locales) {
		const String standardized = TranslationServer::get_singleton()->standardize_locale(locale);
		if (!standardized.is_empty() && !locales.has(standardized)) {
			locales.push_back(standardized);
		}
	}
	_content_changed();
}

PackedStringArray TranslationDocument::get_locales() const {
	return locales;
}

void TranslationDocument::add_locale(const String &p_locale) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	const String locale = TranslationServer::get_singleton()->standardize_locale(p_locale);
	ERR_FAIL_COND_MSG(locale.is_empty(), vformat("Invalid locale: '%s'.", p_locale));
	if (!locales.has(locale)) {
		locales.push_back(locale);
		_content_changed();
	}
}

void TranslationDocument::remove_locale(const String &p_locale) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	const String locale = TranslationServer::get_singleton()->standardize_locale(p_locale);
	if (!locales.has(locale)) {
		return;
	}
	locales.erase(locale);
	for (Entry &entry : entries) {
		entry.texts.erase(locale);
	}
	plural_rules.erase(locale);
	_content_changed();
}

bool TranslationDocument::has_locale(const String &p_locale) const {
	return locales.has(TranslationServer::get_singleton()->standardize_locale(p_locale));
}

void TranslationDocument::set_locale_plural_rules(const String &p_locale, const String &p_rules) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	const String locale = TranslationServer::get_singleton()->standardize_locale(p_locale);
	if (p_rules.is_empty()) {
		plural_rules.erase(locale);
	} else {
		plural_rules[locale] = p_rules;
	}
	_content_changed();
}

String TranslationDocument::get_locale_plural_rules(const String &p_locale) const {
	const String *rules = plural_rules.getptr(TranslationServer::get_singleton()->standardize_locale(p_locale));
	return rules ? *rules : String();
}

PackedStringArray TranslationDocument::get_keys() const {
	PackedStringArray ret;
	HashSet<StringName> added; // Keys with several contexts are listed once.
	for (const Entry &entry : entries) {
		if (!added.has(entry.key)) {
			added.insert(entry.key);
			ret.push_back(entry.key);
		}
	}
	return ret;
}

bool TranslationDocument::has_key(const StringName &p_key, const StringName &p_context) const {
	return _find_entry(p_key, p_context) >= 0;
}

void TranslationDocument::add_key(const StringName &p_key, const StringName &p_context) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	ERR_FAIL_COND_MSG(p_key == StringName(), "Key can't be empty.");
	if (has_key(p_key, p_context)) {
		return;
	}
	Entry entry;
	entry.key = p_key;
	entry.context = p_context;
	entries.push_back(entry);
	entry_indices[{ p_context, p_key }] = entries.size() - 1;
	_content_changed();
}

void TranslationDocument::remove_key(const StringName &p_key, const StringName &p_context) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	const int index = _find_entry(p_key, p_context);
	if (index < 0) {
		return;
	}
	entries.remove_at(index);
	_rebuild_indices();
	_content_changed();
}

void TranslationDocument::rename_key(const StringName &p_key, const StringName &p_new_key, const StringName &p_context) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	ERR_FAIL_COND_MSG(p_new_key == StringName(), "Key can't be empty.");
	ERR_FAIL_COND_MSG(has_key(p_new_key, p_context), vformat("Key '%s' already exists.", p_new_key));
	Entry *entry = _get_entry(p_key, p_context);
	ERR_FAIL_NULL_MSG(entry, vformat("Key '%s' doesn't exist.", p_key));
	entry->key = p_new_key;
	_rebuild_indices();
	_content_changed();
}

void TranslationDocument::set_context(const StringName &p_key, const StringName &p_context, const StringName &p_new_context) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	ERR_FAIL_COND_MSG(has_key(p_key, p_new_context), vformat("Key '%s' already exists with the context '%s'.", p_key, p_new_context));
	Entry *entry = _get_entry(p_key, p_context);
	ERR_FAIL_NULL_MSG(entry, vformat("Key '%s' doesn't exist.", p_key));
	entry->context = p_new_context;
	_rebuild_indices();
	_content_changed();
}

void TranslationDocument::set_text(const StringName &p_key, const String &p_locale, const String &p_text, const StringName &p_context) {
	Vector<String> texts = get_plural_texts(p_key, p_locale, p_context);
	if (texts.is_empty()) {
		texts.push_back(p_text);
	} else {
		texts.write[0] = p_text;
	}
	set_plural_texts(p_key, p_locale, texts, p_context);
}

String TranslationDocument::get_text(const StringName &p_key, const String &p_locale, const StringName &p_context) const {
	const PackedStringArray texts = get_plural_texts(p_key, p_locale, p_context);
	return texts.is_empty() ? String() : texts[0];
}

void TranslationDocument::set_plural_key(const StringName &p_key, const String &p_plural_key, const StringName &p_context) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	ERR_FAIL_COND_MSG(p_key == StringName(), "Key can't be empty.");
	add_key(p_key, p_context);
	_get_entry(p_key, p_context)->plural_key = p_plural_key;
	_content_changed();
}

String TranslationDocument::get_plural_key(const StringName &p_key, const StringName &p_context) const {
	const int index = _find_entry(p_key, p_context);
	return index >= 0 ? entries[index].plural_key : String();
}

void TranslationDocument::set_plural_texts(const StringName &p_key, const String &p_locale, const PackedStringArray &p_texts, const StringName &p_context) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	ERR_FAIL_COND_MSG(p_key == StringName(), "Key can't be empty.");
	const String locale = TranslationServer::get_singleton()->standardize_locale(p_locale);
	ERR_FAIL_COND_MSG(locale.is_empty(), vformat("Invalid locale: '%s'.", p_locale));
	add_key(p_key, p_context);
	if (!locales.has(locale)) {
		locales.push_back(locale);
	}
	Entry *entry = _get_entry(p_key, p_context);
	if (p_texts.is_empty()) {
		entry->texts.erase(locale);
	} else {
		entry->texts[locale] = p_texts;
	}
	_content_changed();
}

PackedStringArray TranslationDocument::get_plural_texts(const StringName &p_key, const String &p_locale, const StringName &p_context) const {
	const int index = _find_entry(p_key, p_context);
	if (index < 0) {
		return PackedStringArray();
	}
	const Vector<String> *texts = entries[index].texts.getptr(TranslationServer::get_singleton()->standardize_locale(p_locale));
	return texts ? PackedStringArray(*texts) : PackedStringArray();
}

void TranslationDocument::set_comment(const StringName &p_key, const String &p_comment, const StringName &p_context) {
	ERR_FAIL_COND_MSG(is_read_only(), "This translation file can't be edited.");
	ERR_FAIL_COND_MSG(p_key == StringName(), "Key can't be empty.");
	add_key(p_key, p_context);
	_get_entry(p_key, p_context)->comment = p_comment;
	_content_changed();
}

String TranslationDocument::get_comment(const StringName &p_key, const StringName &p_context) const {
	const int index = _find_entry(p_key, p_context);
	return index >= 0 ? entries[index].comment : String();
}

// Loading and saving.

Error TranslationDocument::load(Format p_format, const String &p_path) {
	format = p_format;
	source_path = p_path;
	save_conflict = false;
	locales.clear();
	entries.clear();
	entry_indices.clear();
	plural_rules.clear();
	loaded_plural_rules.clear();
	po_files.clear();
	csv_header.clear();
	csv_header_raw = String();
	csv_plural_rules_row.clear();
	csv_plural_rules_raw = String();
	last_error = String();

	loading = true;
	Error err = OK;
	switch (format) {
		case FORMAT_CSV:
			err = _load_csv();
			break;
		case FORMAT_PO:
			err = _load_po();
			break;
		case FORMAT_READ_ONLY:
			err = _load_read_only();
			break;
	}
	_rebuild_indices();
	loaded_plural_rules = plural_rules;
	_update_source_state();
	emit_changed();
	loading = false;
	return err;
}

bool TranslationDocument::reload_if_changed() {
	if (saving_queued || save_conflict || !_is_source_changed(true)) {
		return false;
	}
	load(format, source_path);
	return true;
}

Error TranslationDocument::resolve_save_conflict(bool p_overwrite) {
	ERR_FAIL_COND_V(!save_conflict, OK);
	if (!p_overwrite) {
		return load(format, source_path);
	}
	_update_source_state();
	save_conflict = false;
	return save();
}

Error TranslationDocument::save() {
	ERR_FAIL_COND_V_MSG(is_read_only(), ERR_UNAVAILABLE, "This translation file can't be edited.");
	saving_queued = false;
	last_error = String();
	saved_files.clear();
	// The files were changed since they were read, e.g. by a translator.
	if (save_conflict || _is_source_changed(false)) {
		last_error = TTR("The translation files were changed outside of the editor, so the changes aren't saved over them.");
		if (!save_conflict) {
			save_conflict = true;
			emit_changed();
		}
		return ERR_FILE_CANT_WRITE;
	}
	const Error err = format == FORMAT_CSV ? _save_csv() : _save_po();
	if (err != OK) {
		return err;
	}
	for (Entry &entry : entries) {
		entry.loaded = true;
		entry.loaded_texts = entry;
	}
	loaded_plural_rules = plural_rules;
	_reload_imported();
	_update_source_state();
	// Saved, but a file couldn't be removed.
	return last_error.is_empty() ? OK : ERR_CANT_OPEN;
}

PackedStringArray TranslationDocument::_get_source_files() const {
	if (format != FORMAT_PO) {
		return PackedStringArray{ source_path };
	}
	// The PO files of the folder listed in the project, which can be added and removed in the project settings.
	PackedStringArray ret;
	const PackedStringArray paths = GLOBAL_GET(TRANSLATIONS_SETTING);
	for (const String &path : paths) {
		if (path.get_extension().to_lower() == "po" && path.get_base_dir() == source_path) {
			ret.push_back(path);
		}
	}
	return ret;
}

// Whether the files changed since they were read or saved, also within the same second, unlike their modification
// times. With p_listing, the files added to or removed from the project settings count too.
bool TranslationDocument::_is_source_changed(bool p_listing) const {
	const PackedStringArray files = _get_source_files();
	for (const String &path : files) {
		const String *checksum = source_checksums.getptr(path);
		if (checksum ? *checksum != FileAccess::get_md5(path) : (p_listing || FileAccess::exists(path))) {
			return true;
		}
	}
	if (p_listing) {
		for (const KeyValue<String, String> &E : source_checksums) {
			if (!files.has(E.key)) {
				return true;
			}
		}
	}
	return false;
}

void TranslationDocument::_update_source_state() {
	source_checksums.clear();
	for (const String &path : _get_source_files()) {
		source_checksums[path] = FileAccess::get_md5(path);
	}
}

// The translations imported from the CSV file for each locale.
PackedStringArray TranslationDocument::_get_imported_files() const {
	PackedStringArray ret;
	const String base = source_path.get_basename();
	for (const String &locale : locales) {
		const String path = base + "." + locale + ".translation";
		if (FileAccess::exists(path)) {
			ret.push_back(path);
		}
	}
	return ret;
}

void TranslationDocument::add_to_project() {
	PackedStringArray paths;
	switch (format) {
		case FORMAT_CSV:
			// The translations of a new CSV file are added once it has texts.
			paths = _get_imported_files();
			adding_to_project = true;
			break;
		case FORMAT_PO:
			for (const KeyValue<String, PoFile> &E : po_files) {
				paths.push_back(E.value.path);
			}
			break;
		case FORMAT_READ_ONLY:
			paths.push_back(source_path);
			break;
	}
	_add_translation_files(paths);
	_update_source_state();
}

bool TranslationDocument::is_in_project() {
	if (!get_translation_files().is_empty()) {
		adding_to_project = false;
		return true;
	}
	return adding_to_project;
}

// The editor shows the new texts once the files are imported or reloaded, like when they change outside of the editor.
void TranslationDocument::_reload_imported() {
	EditorFileSystem *file_system = EditorFileSystem::get_singleton();
	PackedStringArray translations;
	if (format == FORMAT_CSV) {
		if (!saved_files.has(source_path)) {
			return;
		}
		const PackedStringArray imported = _get_imported_files();
		if (file_system) {
			file_system->update_file(source_path);
			// Without the progress dialog of the other imports, as the file is imported after each edit.
			const String importer = FileAccess::exists(source_path + ".import") ? String() : String("csv_translation");
			file_system->reimport_append(source_path, HashMap<StringName, Variant>(), importer, Variant());
		}
		// The translations of new locales are added to the project, the ones of removed locales are removed.
		// The ones removed from the project settings stay out.
		if (is_in_project()) {
			PackedStringArray add;
			for (const String &path : _get_imported_files()) {
				if (!imported.has(path) || adding_to_project) {
					add.push_back(path);
				}
			}
			PackedStringArray remove;
			for (const String &path : get_translation_files()) {
				if (!locales.has(path.get_basename().get_extension())) {
					remove.push_back(path);
				}
			}
			_add_translation_files(add, remove);
		}
		const String base = source_path.get_basename();
		for (const String &locale : locales) {
			translations.push_back(base + "." + locale + ".translation");
		}
	} else {
		for (const String &path : saved_files) {
			if (file_system) {
				file_system->update_file(path);
			}
			translations.push_back(path);
		}
	}

	bool replaced = false;
	for (const String &path : translations) {
		Ref<Resource> resource = ResourceCache::get_ref(path);
		if (resource.is_null()) {
			continue;
		}
		if (ResourceLoader::get_resource_type(path) != resource->get_class()) {
			// Compressed or not depending on the texts, so loaded again with the project translations.
			resource->set_path(String());
			replaced = true;
			continue;
		}
		resource->reload_from_file();
		resource->emit_changed();
	}
	if (replaced) {
		ProjectSettings::get_singleton()->emit_signal(SNAME("settings_changed"));
	}
}

int TranslationDocument::_get_plural_count(const String &p_locale) const {
	String rules = get_locale_plural_rules(p_locale);
	if (rules.is_empty()) {
		rules = TranslationServer::get_singleton()->get_plural_rules(p_locale);
	}
	const int start = rules.find("nplurals=");
	if (start < 0) {
		return 2;
	}
	// Only the number: to_int() would also take the digits of the rest of the rules.
	return MAX(1, rules.substr(start + 9).get_slicec(';', 0).strip_edges().to_int());
}

// Files are only written when their text changes, so saving doesn't touch the other files.
// The checksum of a file that's written is updated at once, so when another file can't be written, the files that were
// saved aren't taken for changes made outside of the editor.
Error TranslationDocument::_write_if_changed(const String &p_path, const String &p_text) {
	if (FileAccess::exists(p_path) && FileAccess::get_file_as_string(p_path) == p_text) {
		if (source_checksums.has(p_path)) {
			source_checksums[p_path] = FileAccess::get_md5(p_path);
		}
		return OK;
	}
	// Written to another file first, so a failed write doesn't leave the file cut short.
	const String temp_path = p_path + ".tmp";
	Error err;
	Ref<FileAccess> f = FileAccess::open(temp_path, FileAccess::WRITE, &err);
	if (f.is_null()) {
		last_error = vformat(TTR("Can't write \"%s\"."), p_path);
		return err;
	}
	const bool stored = f->store_string(p_text);
	f->flush();
	err = f->get_error();
	f.unref();
	if (!stored || err != OK) {
		DirAccess::remove_absolute(temp_path);
		last_error = vformat(TTR("Can't write \"%s\"."), p_path);
		return err != OK ? err : ERR_FILE_CANT_WRITE;
	}
	Ref<DirAccess> dir = DirAccess::create_for_path(p_path);
	err = dir->rename(temp_path, p_path);
	if (err != OK) {
		DirAccess::remove_absolute(temp_path);
		last_error = vformat(TTR("Can't write \"%s\"."), p_path);
		return err;
	}
	saved_files.push_back(p_path);
	if (source_checksums.has(p_path)) {
		source_checksums[p_path] = FileAccess::get_md5(p_path);
	}
	return OK;
}

// CSV, in the format of the CSV translations imported by the editor.

struct CsvRecord {
	String raw; // With its line breaks.
	Vector<String> cells;
};

// Records as read by FileAccess::get_csv_line(), which can have several lines in quotes.
static Vector<CsvRecord> _parse_csv(const String &p_text, char32_t p_delimiter) {
	Vector<CsvRecord> records;
	int from = 0;
	const int length = p_text.length();
	while (from < length) {
		int end = from;
		int quotes = 0;
		while (end < length) {
			const char32_t c = p_text[end];
			end++;
			if (c == '"') {
				quotes++;
			} else if (c == '\n' && quotes % 2 == 0) {
				break;
			}
		}
		CsvRecord record;
		record.raw = p_text.substr(from, end - from);
		from = end;

		const String line = record.raw.replace("\r\n", "\n").trim_suffix("\n");
		bool in_quote = false;
		String current;
		for (int i = 0; i < line.length(); i++) {
			const char32_t c = line[i];
			if (!in_quote && c == p_delimiter) {
				record.cells.push_back(current);
				current = String();
			} else if (c == '"') {
				if (in_quote && i + 1 < line.length() && line[i + 1] == '"') {
					current += '"';
					i++;
				} else {
					in_quote = !in_quote;
				}
			} else {
				current += c;
			}
		}
		record.cells.push_back(current);
		records.push_back(record);
	}
	return records;
}

static String _get_csv_cell(const Vector<String> &p_line, int p_column) {
	return (p_column >= 0 && p_column < p_line.size()) ? p_line[p_column] : String();
}

// Like FileAccess::store_csv_line().
String TranslationDocument::_encode_csv_row(const Vector<String> &p_row) const {
	String line;
	for (int i = 0; i < p_row.size(); i++) {
		String value = p_row[i];
		if (value.contains_char('"') || value.contains(csv_delimiter) || value.contains_char('\n')) {
			value = "\"" + value.replace("\"", "\"\"") + "\"";
		}
		line += (i > 0 ? csv_delimiter : String()) + value;
	}
	return line + "\n";
}

String TranslationDocument::_decode_csv_text(const String &p_text) const {
	return csv_unescape_translations ? p_text.c_unescape() : p_text;
}

// Only what can't be written as it is, as String::c_escape() would escape quotes too.
static String _escape_csv_text(const String &p_text) {
	return p_text.replace("\\", "\\\\").replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t");
}

String TranslationDocument::_encode_csv_text(const String &p_text) const {
	return csv_unescape_translations ? _escape_csv_text(p_text) : p_text;
}

Error TranslationDocument::_load_csv() {
	csv_delimiter = ",";
	csv_unescape_keys = false;
	csv_unescape_translations = true;
	Ref<ConfigFile> config;
	config.instantiate();
	if (config->load(source_path + ".import") == OK) {
		const int delimiter = config->get_value("params", "delimiter", 0);
		csv_delimiter = delimiter == 1 ? ";" : (delimiter == 2 ? "\t" : ",");
		csv_unescape_keys = config->get_value("params", "unescape_keys", false);
		csv_unescape_translations = config->get_value("params", "unescape_translations", true);
	}

	Error err;
	const String text = FileAccess::get_file_as_string(source_path, &err);
	if (err != OK) {
		last_error = vformat(TTR("Can't open \"%s\"."), source_path);
		return err;
	}
	const Vector<CsvRecord> records = _parse_csv(text, csv_delimiter[0]);
	if (records.is_empty()) {
		return OK;
	}

	csv_header = records[0].cells;
	csv_header_raw = records[0].raw;
	HashMap<int, String> locale_columns;
	int context_column = -1;
	int plural_column = -1;
	int comment_column = -1;
	for (int i = 1; i < csv_header.size(); i++) {
		const String column = csv_header[i].to_lower();
		if (column == "?context") {
			context_column = i;
		} else if (column == "?plural") {
			plural_column = i;
		} else if (column == "_comments" || column == "_comment") {
			comment_column = comment_column < 0 ? i : comment_column;
		} else if (!column.begins_with("_")) {
			const String locale = TranslationServer::get_singleton()->standardize_locale(csv_header[i]);
			if (!locale.is_empty()) {
				locale_columns[i] = locale;
				if (!locales.has(locale)) {
					locales.push_back(locale);
				}
			}
		}
	}

	int plural_entry = -1;
	for (int r = 1; r < records.size(); r++) {
		const Vector<String> &line = records[r].cells;
		if (line[0].to_lower() == "?pluralrule") {
			csv_plural_rules_row = line;
			csv_plural_rules_raw = records[r].raw;
			for (const KeyValue<int, String> &E : locale_columns) {
				if (!_get_csv_cell(line, E.key).is_empty()) {
					plural_rules[E.value] = _get_csv_cell(line, E.key);
				}
			}
			continue;
		}

		const String key = csv_unescape_keys ? line[0].c_unescape() : line[0];
		const String context = _get_csv_cell(line, context_column);
		const String plural_key = _get_csv_cell(line, plural_column);
		if (key.is_empty() && context.is_empty() && plural_key.is_empty()) {
			// The next plural forms of the previous key, or an empty line kept with the previous key.
			const int previous = plural_entry >= 0 ? plural_entry : entries.size() - 1;
			if (previous >= 0) {
				Entry &entry = entries.write[previous];
				if (plural_entry >= 0) {
					for (const KeyValue<int, String> &E : locale_columns) {
						entry.texts[E.value].push_back(_decode_csv_text(_get_csv_cell(line, E.key)));
					}
				}
				entry.csv_rows.push_back(line);
				entry.csv_raw += records[r].raw;
			}
			continue;
		}
		if (key.is_empty()) {
			// Not imported, kept with the previous key.
			if (!entries.is_empty()) {
				entries.write[entries.size() - 1].csv_rows.push_back(line);
				entries.write[entries.size() - 1].csv_raw += records[r].raw;
			}
			plural_entry = -1;
			continue;
		}

		Entry entry;
		entry.key = key;
		entry.context = context;
		entry.plural_key = plural_key;
		entry.comment = _get_csv_cell(line, comment_column);
		for (const KeyValue<int, String> &E : locale_columns) {
			entry.texts[E.value].push_back(_decode_csv_text(_get_csv_cell(line, E.key)));
		}
		entry.csv_rows.push_back(line);
		entry.csv_raw = records[r].raw;
		entries.push_back(entry);
		entry_indices[{ entry.context, entry.key }] = entries.size() - 1;
		plural_entry = plural_key.is_empty() ? -1 : entries.size() - 1;
	}

	// Without the empty forms at the end, and the locales with no text.
	for (Entry &entry : entries) {
		LocalVector<String> empty_locales;
		for (KeyValue<String, Vector<String>> &E : entry.texts) {
			while (!E.value.is_empty() && E.value[E.value.size() - 1].is_empty()) {
				E.value.resize(E.value.size() - 1);
			}
			if (E.value.is_empty()) {
				empty_locales.push_back(E.key);
			}
		}
		for (const String &locale : empty_locales) {
			entry.texts.erase(locale);
		}
		entry.loaded = true;
		entry.loaded_texts = entry;
	}
	return OK;
}

// A row of the read file, with its cells moved to the new columns.
static Vector<String> _remap_csv_row(const Vector<String> &p_row, const Vector<int> &p_old_columns) {
	Vector<String> row;
	row.resize(p_old_columns.size());
	for (int i = 0; i < p_old_columns.size(); i++) {
		row.write[i] = _get_csv_cell(p_row, p_old_columns[i]);
	}
	return row;
}

// Rows are written as they were read, the last one may have no line break.
static void _add_csv_raw(String &r_text, const String &p_raw) {
	if (!r_text.is_empty() && !r_text.ends_with("\n")) {
		r_text += "\n";
	}
	r_text += p_raw;
}

Error TranslationDocument::_save_csv() {
	// The columns of the file, with the columns needed by the texts added and the removed locales left out.
	bool has_contexts = false;
	bool has_plurals = false;
	bool has_comments = false;
	for (const Entry &entry : entries) {
		has_contexts = has_contexts || entry.context != StringName();
		has_plurals = has_plurals || !entry.plural_key.is_empty();
		has_comments = has_comments || !entry.comment.is_empty();
	}

	Vector<String> header;
	Vector<int> old_columns; // Column of the read file for each column, or -1.
	int context_column = -1;
	int plural_column = -1;
	int comment_column = -1;
	HashMap<String, int> locale_columns;
	HashMap<String, int> old_locale_columns;
	Vector<int> locale_slots; // The columns of the locales of the read file.
	for (int i = 0; i < csv_header.size(); i++) {
		const String column = csv_header[i].to_lower();
		if (i > 0 && !column.begins_with("_") && !column.begins_with("?")) {
			const String locale = TranslationServer::get_singleton()->standardize_locale(csv_header[i]);
			if (!locale.is_empty() && !locales.has(locale)) {
				continue;
			}
			if (!locale.is_empty() && !old_locale_columns.has(locale)) {
				old_locale_columns[locale] = i;
				locale_slots.push_back(header.size());
			}
		} else if (column == "?context") {
			context_column = header.size();
		} else if (column == "?plural") {
			plural_column = header.size();
		} else if ((column == "_comments" || column == "_comment") && comment_column < 0) {
			comment_column = header.size();
		}
		header.push_back(csv_header[i]);
		old_columns.push_back(i);
	}
	// The columns of the locales are in the order of the locales, which can be changed.
	int slot = 0;
	for (const String &locale : locales) {
		const int *old_column = old_locale_columns.getptr(locale);
		if (old_column) {
			const int column = locale_slots[slot++];
			header.write[column] = csv_header[*old_column];
			old_columns.write[column] = *old_column;
			locale_columns[locale] = column;
		}
	}
	if (header.is_empty()) {
		header.push_back("keys");
		old_columns.push_back(-1);
	}
	if (has_contexts && context_column < 0) {
		context_column = header.size();
		header.push_back("?context");
		old_columns.push_back(-1);
	}
	if (has_plurals && plural_column < 0) {
		plural_column = header.size();
		header.push_back("?plural");
		old_columns.push_back(-1);
	}
	if (has_comments && comment_column < 0) {
		comment_column = header.size();
		header.push_back("_comments");
		old_columns.push_back(-1);
	}
	for (const String &locale : locales) {
		if (!locale_columns.has(locale)) {
			locale_columns[locale] = header.size();
			header.push_back(locale);
			old_columns.push_back(-1);
		}
	}
	// The rows that didn't change are written as they were read while the columns are the same.
	const bool same_columns = header == csv_header && !csv_header_raw.is_empty();

	String text;
	_add_csv_raw(text, same_columns ? csv_header_raw : _encode_csv_row(header));
	if (plural_rules.is_empty()) {
		csv_plural_rules_row.clear();
		csv_plural_rules_raw = String();
	} else if (_same_plural_rules(plural_rules, loaded_plural_rules) && !csv_plural_rules_row.is_empty()) {
		if (!same_columns) {
			csv_plural_rules_row = _remap_csv_row(csv_plural_rules_row, old_columns);
			csv_plural_rules_raw = _encode_csv_row(csv_plural_rules_row);
		}
		_add_csv_raw(text, csv_plural_rules_raw);
	} else {
		Vector<String> row;
		row.resize(header.size());
		row.write[0] = "?pluralrule";
		for (const KeyValue<String, int> &E : locale_columns) {
			row.write[E.value] = get_locale_plural_rules(E.key);
		}
		csv_plural_rules_row = row;
		csv_plural_rules_raw = _encode_csv_row(row);
		_add_csv_raw(text, csv_plural_rules_raw);
	}

	for (Entry &entry : entries) {
		const bool unchanged = entry.loaded && entry.loaded_texts == entry && !entry.csv_rows.is_empty();
		if (unchanged && same_columns) {
			_add_csv_raw(text, entry.csv_raw);
			continue;
		}
		Vector<Vector<String>> rows;
		if (unchanged) {
			for (const Vector<String> &row : entry.csv_rows) {
				rows.push_back(_remap_csv_row(row, old_columns));
			}
		} else {
			// The cells of the other columns are kept.
			Vector<String> row = entry.csv_rows.is_empty() ? Vector<String>() : _remap_csv_row(entry.csv_rows[0], old_columns);
			row.resize(header.size());
			row.write[0] = csv_unescape_keys ? _escape_csv_text(entry.key) : String(entry.key);
			if (context_column >= 0) {
				row.write[context_column] = entry.context;
			}
			if (plural_column >= 0) {
				row.write[plural_column] = entry.plural_key;
			}
			if (comment_column >= 0) {
				row.write[comment_column] = entry.comment;
			}
			int forms = 1;
			for (const KeyValue<String, int> &E : locale_columns) {
				const Vector<String> *texts = entry.texts.getptr(E.key);
				row.write[E.value] = (texts && !texts->is_empty()) ? _encode_csv_text((*texts)[0]) : String();
				if (texts) {
					forms = MAX(forms, texts->size());
				}
			}
			rows.push_back(row);
			// The next plural forms, one row each.
			for (int form = 1; form < forms && !entry.plural_key.is_empty(); form++) {
				Vector<String> form_row;
				form_row.resize(header.size());
				for (const KeyValue<String, int> &E : locale_columns) {
					const Vector<String> *texts = entry.texts.getptr(E.key);
					if (texts && form < texts->size()) {
						form_row.write[E.value] = _encode_csv_text((*texts)[form]);
					}
				}
				rows.push_back(form_row);
			}
		}
		entry.csv_raw = String();
		for (const Vector<String> &row : rows) {
			entry.csv_raw += _encode_csv_row(row);
		}
		entry.csv_rows = rows;
		_add_csv_raw(text, entry.csv_raw);
	}

	const Error err = _write_if_changed(source_path, text);
	if (err != OK) {
		return err;
	}
	if (!same_columns) {
		csv_header = header;
		csv_header_raw = _encode_csv_row(header);
	}
	return OK;
}

// PO, one file per locale in a folder.

static String _po_unquote(const String &p_line) {
	const String line = p_line.strip_edges();
	if (line.length() < 2 || !line.begins_with("\"") || !line.ends_with("\"")) {
		return String();
	}
	return line.substr(1, line.length() - 2).c_unescape();
}

// Control characters are escaped too, as gettext reads a PO string on one line.
static String _po_quote(const String &p_text) {
	String escaped = p_text.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\t", "\\t").replace("\r", "\\r");
	escaped = escaped.replace("\a", "\\a").replace("\b", "\\b").replace("\f", "\\f").replace("\v", "\\v");
	return "\"" + escaped + "\"";
}

// A field split into lines at the line breaks, like the template generator writes them.
static String _po_field(const String &p_name, const String &p_value) {
	const Vector<String> lines = p_value.split("\n");
	if (lines.size() <= 1 || (lines.size() == 2 && lines[1].is_empty())) {
		return p_name + " " + _po_quote(p_value) + "\n";
	}
	String ret = p_name + " \"\"\n";
	for (int i = 0; i < lines.size(); i++) {
		const bool last = i == lines.size() - 1;
		if (last && lines[i].is_empty()) {
			break;
		}
		ret += _po_quote(last ? lines[i] : lines[i] + "\n") + "\n";
	}
	return ret;
}

struct PoBlock {
	String raw;
	String comments; // Lines kept as written, like references and flags.
	String extracted; // The "#." comments.
	String msgctxt;
	String msgid;
	String msgid_plural;
	Vector<String> msgstr;
	bool has_msgid = false;
	bool obsolete = false;
};

// Reads the entries of a PO file. An entry ends at an empty line, or where the comments or the
// msgctxt or msgid of the next entry start, like in the files read by the translation loader.
class PoParser {
	Vector<PoBlock> blocks;
	PoBlock block;
	String *field = nullptr;
	int msgstr_index = -1;

	void _finish() {
		if (!block.raw.is_empty()) {
			blocks.push_back(block);
		}
		block = PoBlock();
		field = nullptr;
		msgstr_index = -1;
	}

	bool _starts_next_entry(const String &p_stripped, const String &p_keyword) const {
		if (block.obsolete) {
			return !p_stripped.begins_with("#~");
		}
		if (!block.msgstr.is_empty()) {
			return p_stripped.begins_with("#") || p_keyword == "msgctxt" || p_keyword == "msgid";
		}
		return (p_keyword == "msgctxt" && (block.has_msgid || !block.msgctxt.is_empty())) || (p_keyword == "msgid" && block.has_msgid);
	}

	void _read_line(const String &p_line) {
		const String stripped = p_line.strip_edges();
		if (stripped.is_empty()) {
			_finish();
			return;
		}
		const String keyword = stripped.begins_with("\"") || stripped.begins_with("#") ? String() : stripped.get_slicec(' ', 0);
		if (_starts_next_entry(stripped, keyword)) {
			_finish();
		}
		block.raw += p_line + "\n";
		if (stripped.begins_with("#~")) {
			block.obsolete = true;
			return;
		}
		if (stripped.begins_with("#.")) {
			const String text = stripped.substr(2).strip_edges(true, false);
			block.extracted += (block.extracted.is_empty() ? "" : "\n") + text;
			return;
		}
		if (stripped.begins_with("#")) {
			block.comments += p_line + "\n";
			return;
		}
		if (stripped.begins_with("\"")) {
			if (field) {
				*field += _po_unquote(stripped);
			} else if (msgstr_index >= 0) {
				block.msgstr.write[msgstr_index] += _po_unquote(stripped);
			}
			return;
		}
		const String value = _po_unquote(stripped.substr(keyword.length()));
		field = nullptr;
		msgstr_index = -1;
		if (keyword == "msgctxt") {
			block.msgctxt = value;
			field = &block.msgctxt;
		} else if (keyword == "msgid") {
			block.msgid = value;
			block.has_msgid = true;
			field = &block.msgid;
		} else if (keyword == "msgid_plural") {
			block.msgid_plural = value;
			field = &block.msgid_plural;
		} else if (keyword.begins_with("msgstr")) {
			const int index = keyword.contains_char('[') ? keyword.get_slicec('[', 1).to_int() : 0;
			if (index >= 0) {
				if (block.msgstr.size() <= index) {
					block.msgstr.resize(index + 1);
				}
				block.msgstr.write[index] = value;
				msgstr_index = index;
			}
		}
	}

public:
	Vector<PoBlock> parse(const String &p_text) {
		for (const String &line : p_text.replace("\r\n", "\n").split("\n")) {
			_read_line(line);
		}
		_finish();
		return blocks;
	}
};

Error TranslationDocument::_load_po() {
	Error err = OK;
	const PackedStringArray paths = GLOBAL_GET(TRANSLATIONS_SETTING);
	for (const String &path : paths) {
		if (path.get_extension().to_lower() == "po" && path.get_base_dir() == source_path && FileAccess::exists(path)) {
			// The other files are still edited, the error is shown.
			const Error file_err = _load_po_file(path);
			err = err == OK ? file_err : err;
		}
	}
	for (Entry &entry : entries) {
		entry.loaded = true;
		entry.loaded_texts = entry;
	}
	return err;
}

Error TranslationDocument::_load_po_file(const String &p_path) {
	Error err;
	const String text = FileAccess::get_file_as_string(p_path, &err);
	if (err != OK) {
		last_error = vformat(TTR("Can't open \"%s\"."), p_path);
		return err;
	}

	PoFile file;
	file.path = p_path;
	String locale;
	String rules;
	const Vector<PoBlock> blocks = PoParser().parse(text);
	int header_index = -1;
	for (int i = 0; i < blocks.size(); i++) {
		const PoBlock &block = blocks[i];
		if (!block.obsolete && block.has_msgid && block.msgid.is_empty() && block.msgctxt.is_empty()) {
			header_index = i;
			file.header = block.raw;
			const String header = block.msgstr.is_empty() ? String() : block.msgstr[0];
			for (const String &line : header.split("\n")) {
				if (line.begins_with("Language:")) {
					locale = TranslationServer::get_singleton()->standardize_locale(line.trim_prefix("Language:").strip_edges());
				} else if (line.begins_with("Plural-Forms:")) {
					rules = line.trim_prefix("Plural-Forms:").strip_edges();
				}
			}
			break;
		}
	}
	if (locale.is_empty()) {
		locale = TranslationServer::get_singleton()->standardize_locale(p_path.get_file().get_basename());
	}
	if (locale.is_empty() || po_files.has(locale)) {
		last_error = vformat(TTR("The PO file \"%s\" has no language, or the language of another file."), p_path);
		return ERR_INVALID_DATA;
	}
	if (!rules.is_empty()) {
		plural_rules[locale] = rules;
	}
	if (!locales.has(locale)) {
		locales.push_back(locale);
	}

	int order = 0;
	for (int i = 0; i < blocks.size(); i++) {
		const PoBlock &block = blocks[i];
		if (block.obsolete) {
			file.obsolete += (file.obsolete.is_empty() ? "" : "\n") + block.raw;
			continue;
		}
		if (!block.has_msgid || i == header_index) {
			continue;
		}
		int index = _find_entry(block.msgid, block.msgctxt);
		if (index < 0) {
			Entry entry;
			entry.key = block.msgid;
			entry.context = block.msgctxt;
			entries.push_back(entry);
			index = entries.size() - 1;
			entry_indices[{ entry.context, entry.key }] = index;
		}
		Entry &entry = entries.write[index];
		if (entry.plural_key.is_empty()) {
			entry.plural_key = block.msgid_plural;
		}
		if (entry.comment.is_empty()) {
			entry.comment = block.extracted;
		}
		Vector<String> forms = block.msgstr;
		while (!forms.is_empty() && forms[forms.size() - 1].is_empty()) {
			forms.resize(forms.size() - 1);
		}
		if (!forms.is_empty()) {
			entry.texts[locale] = forms;
		}
		entry.po_blocks[locale] = block.raw;
		entry.po_comments[locale] = block.comments;
		entry.po_extracted[locale] = block.extracted;
		entry.po_order[locale] = order++;
	}
	po_files[locale] = file;
	return OK;
}

String TranslationDocument::_make_po_header(const String &p_locale) const {
	String rules = get_locale_plural_rules(p_locale);
	if (rules.is_empty()) {
		rules = TranslationServer::get_singleton()->get_plural_rules(p_locale);
	}
	String header = "msgid \"\"\nmsgstr \"\"\n";
	header += _po_quote("Project-Id-Version: " + String(GLOBAL_GET("application/config/name")) + "\n") + "\n";
	header += _po_quote("MIME-Version: 1.0\n") + "\n";
	header += _po_quote("Content-Type: text/plain; charset=UTF-8\n") + "\n";
	header += _po_quote("Content-Transfer-Encoding: 8-bit\n") + "\n";
	header += _po_quote("Language: " + p_locale + "\n") + "\n";
	if (!rules.is_empty()) {
		header += _po_quote("Plural-Forms: " + rules + "\n") + "\n";
	}
	return header;
}

Error TranslationDocument::_save_po() {
	// Files of removed locales go to the trash, and out of the project.
	PackedStringArray removed;
	LocalVector<String> removed_locales;
	for (const KeyValue<String, PoFile> &E : po_files) {
		if (!locales.has(E.key)) {
			removed_locales.push_back(E.key);
		}
	}
	for (const String &locale : removed_locales) {
		const String path = po_files[locale].path;
		if (FileAccess::exists(path) && OS::get_singleton()->move_to_trash(ProjectSettings::get_singleton()->globalize_path(path)) != OK) {
			// The file stays in the project, the next save tries again, and save() reports it.
			last_error = vformat(TTR("Can't move \"%s\" to the trash."), path);
			continue;
		}
		removed.push_back(path);
		po_files.erase(locale);
		source_checksums.erase(path);
	}

	PackedStringArray files;
	for (const String &locale : locales) {
		if (!po_files.has(locale)) {
			// A file that isn't part of the project yet is never overwritten.
			PoFile file;
			file.path = source_path.path_join(locale + ".po");
			for (int i = 2; FileAccess::exists(file.path); i++) {
				file.path = source_path.path_join(vformat("%s_%d.po", locale, i));
			}
			file.header = _make_po_header(locale);
			po_files[locale] = file;
		}
		// Files restored by undo are listed in the project again.
		files.push_back(po_files[locale].path);
		const Error file_err = _save_po_file(locale);
		if (file_err != OK) {
			return file_err;
		}
	}
	_add_translation_files(files, removed);
	if (EditorFileSystem::get_singleton() && !removed.is_empty()) {
		EditorFileSystem::get_singleton()->scan_changes();
	}
	return OK;
}

Error TranslationDocument::_save_po_file(const String &p_locale) {
	PoFile &file = po_files[p_locale];

	// The entries of the file in their order, then the new ones. New keys are added to all the files.
	struct Item {
		int order;
		int index;
		bool operator<(const Item &p_other) const {
			return order < p_other.order || (order == p_other.order && index < p_other.index);
		}
	};
	Vector<Item> items;
	for (int i = 0; i < entries.size(); i++) {
		const Entry &entry = entries[i];
		const int *order = entry.po_order.getptr(p_locale);
		if (order) {
			items.push_back({ *order, i });
		} else if (entry.texts.has(p_locale) || !entry.loaded) {
			items.push_back({ INT_MAX, i });
		}
	}
	items.sort();

	const String *loaded_rules = loaded_plural_rules.getptr(p_locale);
	const String rules = get_locale_plural_rules(p_locale);
	const bool rules_changed = rules != (loaded_rules ? *loaded_rules : String());
	bool has_plurals = false;
	for (const Item &item : items) {
		if (!entries[item.index].plural_key.is_empty()) {
			has_plurals = true;
			break;
		}
	}
	// The PO loader needs the plural rules once the file has plural keys, the rules of the locale are written then.
	String header_rules = rules;
	if (header_rules.is_empty() && has_plurals) {
		header_rules = TranslationServer::get_singleton()->get_plural_rules(p_locale);
	}
	Vector<String> lines = file.header.trim_suffix("\n").split("\n");
	int found = -1;
	for (int i = 0; i < lines.size(); i++) {
		if (lines[i].strip_edges().begins_with("\"Plural-Forms:")) {
			found = i;
		}
	}
	if (found >= 0 ? rules_changed : !header_rules.is_empty()) {
		// Only the plural rules line of the header changes.
		const String line = _po_quote("Plural-Forms: " + header_rules + "\n");
		if (found < 0) {
			lines.push_back(line);
		} else if (header_rules.is_empty()) {
			lines.remove_at(found);
		} else {
			lines.write[found] = line;
		}
		file.header = String("\n").join(lines) + "\n";
	}

	String text = file.header;
	for (int i = 0; i < items.size(); i++) {
		Entry &entry = entries.write[items[i].index];
		entry.po_order[p_locale] = i;
		text += "\n";
		// The block of the file is kept while what it has of the entry didn't change.
		const String *raw = entry.po_blocks.getptr(p_locale);
		const Texts &loaded = entry.loaded_texts;
		const bool comment_changed = !entry.loaded || entry.comment != loaded.comment;
		if (raw && entry.loaded && !comment_changed && entry.key == loaded.key && entry.context == loaded.context && entry.plural_key == loaded.plural_key) {
			const Vector<String> *texts = entry.texts.getptr(p_locale);
			const Vector<String> *loaded_texts = loaded.texts.getptr(p_locale);
			if ((texts ? *texts : Vector<String>()) == (loaded_texts ? *loaded_texts : Vector<String>())) {
				text += *raw;
				continue;
			}
		}
		// In the usual order: comments of the translators, extracted comments, then references and flags.
		String translator_comments;
		String other_comments;
		const String *comments = entry.po_comments.getptr(p_locale);
		if (comments) {
			for (const String &line : comments->split("\n", false)) {
				const String start = line.strip_edges().left(2);
				if (start == "#:" || start == "#," || start == "#|") {
					other_comments += line + "\n";
				} else {
					translator_comments += line + "\n";
				}
			}
		}
		// The comment of the file is kept, the comments of the files can differ.
		const String *extracted = entry.po_extracted.getptr(p_locale);
		const String comment = (extracted && !comment_changed) ? *extracted : entry.comment;
		String block = translator_comments;
		if (!comment.is_empty()) {
			block += "#. " + comment.replace("\n", "\n#. ") + "\n";
		}
		block += other_comments;
		if (entry.context != StringName()) {
			block += _po_field("msgctxt", entry.context);
		}
		block += _po_field("msgid", entry.key);
		const Vector<String> *texts = entry.texts.getptr(p_locale);
		if (entry.plural_key.is_empty()) {
			block += _po_field("msgstr", (texts && !texts->is_empty()) ? (*texts)[0] : String());
		} else {
			block += _po_field("msgid_plural", entry.plural_key);
			// The forms of the locale, and the forms past them that have texts. Empty ones past them are left out, e.g. the
			// ones of files written with wrong plural rules.
			int count = _get_plural_count(p_locale);
			if (texts) {
				for (int form = texts->size() - 1; form >= count; form--) {
					if (!(*texts)[form].is_empty()) {
						count = form + 1;
						break;
					}
				}
			}
			for (int form = 0; form < count; form++) {
				block += _po_field("msgstr[" + itos(form) + "]", (texts && form < texts->size()) ? (*texts)[form] : String());
			}
		}
		entry.po_blocks[p_locale] = block;
		entry.po_extracted[p_locale] = comment;
		text += block;
	}
	if (!file.obsolete.is_empty()) {
		text += "\n" + file.obsolete;
	}
	return _write_if_changed(file.path, text);
}

// Other translation files, which can only be read.

Error TranslationDocument::_load_read_only() {
	const Ref<Translation> translation = ResourceLoader::load(source_path);
	if (translation.is_null()) {
		last_error = vformat(TTR("\"%s\" is not a translation."), source_path);
		return ERR_INVALID_DATA;
	}
	const String locale = translation->get_locale();
	locales.push_back(locale);
	List<StringName> messages;
	translation->get_message_list(&messages);
	for (const StringName &message : messages) {
		Entry entry;
		entry.key = message;
		const String text = translation->get_message(message);
		if (!text.is_empty()) {
			entry.texts[locale] = Vector<String>{ text };
		}
		entries.push_back(entry);
	}
	return OK;
}

void TranslationDocument::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_locales", "locales"), &TranslationDocument::set_locales);
	ClassDB::bind_method(D_METHOD("get_locales"), &TranslationDocument::get_locales);
	ClassDB::bind_method(D_METHOD("add_locale", "locale"), &TranslationDocument::add_locale);
	ClassDB::bind_method(D_METHOD("remove_locale", "locale"), &TranslationDocument::remove_locale);
	ClassDB::bind_method(D_METHOD("set_locale_plural_rules", "locale", "rules"), &TranslationDocument::set_locale_plural_rules);
	ClassDB::bind_method(D_METHOD("add_key", "key", "context"), &TranslationDocument::add_key, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("remove_key", "key", "context"), &TranslationDocument::remove_key, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("rename_key", "key", "new_key", "context"), &TranslationDocument::rename_key, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("set_context", "key", "context", "new_context"), &TranslationDocument::set_context);
	ClassDB::bind_method(D_METHOD("set_plural_key", "key", "plural_key", "context"), &TranslationDocument::set_plural_key, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("set_plural_texts", "key", "locale", "texts", "context"), &TranslationDocument::set_plural_texts, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("set_comment", "key", "comment", "context"), &TranslationDocument::set_comment, DEFVAL(StringName()));
	ClassDB::bind_method(D_METHOD("_set_entries", "entries"), &TranslationDocument::_set_entries);
	ClassDB::bind_method(D_METHOD("_get_entries"), &TranslationDocument::_get_entries);
	ClassDB::bind_method(D_METHOD("set_snapshot", "snapshot"), &TranslationDocument::set_snapshot);

	ADD_SIGNAL(MethodInfo("changed"));

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "locales", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_locales", "get_locales");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "_set_entries", "_get_entries");
}
