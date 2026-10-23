/**************************************************************************/
/*  test_translation_document.cpp                                         */
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

TEST_FORCE_LINK(test_translation_document)

#ifdef TOOLS_ENABLED

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "editor/translations/translation_document.h"
#include "tests/test_utils.h"

namespace TestTranslationDocument {

static void write_file(const String &p_path, const String &p_text) {
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	REQUIRE(file.is_valid());
	file->store_string(p_text);
}

static String read_file(const String &p_path) {
	return FileAccess::get_file_as_string(p_path);
}

TEST_CASE("[TranslationDocument] CSV texts") {
	const String path = TestUtils::get_temp_path("translation_document_texts.csv");
	write_file(path, String::utf8("keys,?context,_comments,en,ru\n"
								  "HELLO,,Greeting,Hello,\"Привет, мир\"\n"
								  "OPEN,menu,,Open,Открыть\n"
								  "MULTILINE,,,First\\nSecond,\n"));

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_CSV, path) == OK);
	CHECK(document->get_locales() == PackedStringArray{ "en", "ru" });
	CHECK(document->get_keys() == PackedStringArray{ "HELLO", "OPEN", "MULTILINE" });
	CHECK(document->get_text("HELLO", "ru") == String::utf8("Привет, мир"));
	CHECK(document->get_comment("HELLO") == "Greeting");
	CHECK(document->has_key("OPEN", "menu"));
	CHECK_FALSE(document->has_key("OPEN"));
	CHECK(document->get_text("OPEN", "en", "menu") == "Open");
	// Unescaped like the CSV importer does by default.
	CHECK(document->get_text("MULTILINE", "en") == "First\nSecond");
	CHECK(document->get_text("MULTILINE", "ru").is_empty());
}

TEST_CASE("[TranslationDocument] CSV rows that don't change are kept as written") {
	const String path = TestUtils::get_temp_path("translation_document_kept.csv");
	const String csv = String::utf8(
			"keys,_notes,en,ru\n"
			"HELLO,a note,Hello,Привет\n"
			"\"QUOTED\",,\"Quoted\",\"Цитата\"\n"
			"BYE,,Bye,Пока");
	write_file(path, csv);

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_CSV, path) == OK);
	CHECK(document->save() == OK);
	CHECK(read_file(path) == csv);

	document->set_text("HELLO", "en", "Hi");
	document->add_key("NEW");
	CHECK(document->save() == OK);
	// The other cells of a changed row are kept too.
	CHECK(read_file(path) == String::utf8("keys,_notes,en,ru\n"
										  "HELLO,a note,Hi,Привет\n"
										  "\"QUOTED\",,\"Quoted\",\"Цитата\"\n"
										  "BYE,,Bye,Пока\n"
										  "NEW,,,\n"));

	document->remove_key("NEW");
	document->add_locale("ja");
	document->set_text("BYE", "ja", String::utf8("さようなら"));
	CHECK(document->save() == OK);
	CHECK(read_file(path) == String::utf8("keys,_notes,en,ru,ja\n"
										  "HELLO,a note,Hi,Привет,\n"
										  "QUOTED,,Quoted,Цитата,\n"
										  "BYE,,Bye,Пока,さようなら\n"));

	document->remove_locale("ru");
	CHECK(document->save() == OK);
	CHECK(read_file(path) == String::utf8("keys,_notes,en,ja\n"
										  "HELLO,a note,Hi,\n"
										  "QUOTED,,Quoted,\n"
										  "BYE,,Bye,さようなら\n"));

	// Read again as saved.
	Ref<TranslationDocument> reloaded;
	reloaded.instantiate();
	REQUIRE(reloaded->load(TranslationDocument::FORMAT_CSV, path) == OK);
	CHECK(reloaded->get("entries") == document->get("entries"));

	// The columns follow the order of the locales.
	document->set_locales(PackedStringArray{ "ja", "en" });
	CHECK(document->save() == OK);
	CHECK(read_file(path) == String::utf8("keys,_notes,ja,en\n"
										  "HELLO,a note,,Hi\n"
										  "QUOTED,,,Quoted\n"
										  "BYE,,さようなら,Bye\n"));
}

TEST_CASE("[TranslationDocument] Changes made outside of the editor aren't overwritten") {
	const String path = TestUtils::get_temp_path("translation_document_outside.csv");
	write_file(path, "keys,en\nHELLO,Hello\n");

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_CSV, path) == OK);
	// Possibly in the same second, which the modification time doesn't tell.
	write_file(path, "keys,en\nHELLO,Hi\nBYE,Bye\n");
	document->set_text("HELLO", "en", "Hey");
	CHECK(document->save() != OK);
	CHECK_FALSE(document->get_last_error().is_empty());
	CHECK(read_file(path) == "keys,en\nHELLO,Hi\nBYE,Bye\n");

	// The changes are kept until the conflict is resolved, and the next saves are refused too.
	CHECK(document->has_save_conflict());
	CHECK_FALSE(document->reload_if_changed());
	CHECK(document->get_text("HELLO", "en") == "Hey");
	document->set_text("HELLO", "en", "Hey you");
	CHECK(document->save() != OK);
	CHECK(read_file(path) == "keys,en\nHELLO,Hi\nBYE,Bye\n");

	// Read again.
	CHECK(document->resolve_save_conflict(false) == OK);
	CHECK_FALSE(document->has_save_conflict());
	CHECK(document->get_text("HELLO", "en") == "Hi");
	CHECK(document->has_key("BYE"));
	document->set_text("HELLO", "en", "Hey");
	CHECK(document->save() == OK);
	CHECK(read_file(path) == "keys,en\nHELLO,Hey\nBYE,Bye\n");

	// Overwritten.
	write_file(path, "keys,en\nHELLO,Hello again\n");
	document->set_text("BYE", "en", "Goodbye");
	CHECK(document->save() != OK);
	CHECK(document->resolve_save_conflict(true) == OK);
	CHECK(read_file(path) == "keys,en\nHELLO,Hey\nBYE,Goodbye\n");

	// Read again without changes of the document, also within the same second.
	write_file(path, "keys,en\nHELLO,Hello\nBYE,Goodbye\n");
	CHECK(document->reload_if_changed());
	CHECK(document->get_text("HELLO", "en") == "Hello");
	CHECK_FALSE(document->reload_if_changed());
}

TEST_CASE("[TranslationDocument] Keys made plural have the forms of each locale") {
	const String folder = TestUtils::get_temp_path("translation_document_plural_key");
	DirAccess::make_dir_recursive_absolute(folder);
	const String en_path = folder.path_join("en.po");
	const String ru_path = folder.path_join("ru.po");
	write_file(en_path, "msgid \"\"\nmsgstr \"\"\n\"Language: en\\n\"\n\nmsgid \"APPLE\"\nmsgstr \"apple\"\n");
	write_file(ru_path, String::utf8("msgid \"\"\nmsgstr \"\"\n\"Language: ru\\n\"\n"
									 "\"Plural-Forms: nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);\\n\"\n"
									 "\nmsgid \"APPLE\"\nmsgstr \"яблоко\"\n"));
	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ en_path, ru_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);
	document->set_plural_key("APPLE", "APPLES");
	CHECK(document->save() == OK);
	CHECK(read_file(en_path).contains("msgstr[1] \"\"\n"));
	CHECK_FALSE(read_file(en_path).contains("msgstr[2]"));
	CHECK(read_file(ru_path).contains("msgstr[2] \"\"\n"));
	CHECK_FALSE(read_file(ru_path).contains("msgstr[3]"));

	// The files are read by the loader of the project.
	const Ref<Translation> en = ResourceLoader::load(en_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(en.is_valid());
	CHECK(en->get_plural_message("APPLE", "APPLES", 1) == "apple");
	const Ref<Translation> ru = ResourceLoader::load(ru_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(ru.is_valid());
	CHECK(ru->get_plural_message("APPLE", "APPLES", 1) == String::utf8("яблоко"));

	// Empty forms past the forms of the locale are left out, e.g. of a file written with too many forms.
	PackedStringArray forms = { "apple", "apples" };
	forms.resize(21);
	document->set_plural_texts("APPLE", "en", forms);
	CHECK(document->save() == OK);
	CHECK(read_file(en_path).contains("msgstr[1] \"apples\"\n"));
	CHECK_FALSE(read_file(en_path).contains("msgstr[2]"));

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

TEST_CASE("[TranslationDocument] Files saved before another file fails aren't taken for outside changes") {
	const String folder = TestUtils::get_temp_path("translation_document_partial");
	DirAccess::make_dir_recursive_absolute(folder);
	const String en_path = folder.path_join("en.po");
	const String fr_path = folder.path_join("fr.po");
	write_file(en_path, "msgid \"\"\nmsgstr \"\"\n\"Language: en\\n\"\n\nmsgid \"HELLO\"\nmsgstr \"Hello\"\n");
	write_file(fr_path, "msgid \"\"\nmsgstr \"\"\n\"Language: fr\\n\"\n\nmsgid \"HELLO\"\nmsgstr \"Salut\"\n");
	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ en_path, fr_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);
	document->set_text("HELLO", "en", "Hi");
	document->set_text("HELLO", "fr", "Bonjour");

	// The file is written to a temporary file first, which can't be created while a folder has its name.
	DirAccess::make_dir_absolute(fr_path + ".tmp");
	ERR_PRINT_OFF;
	CHECK(document->save() != OK);
	ERR_PRINT_ON;
	CHECK(read_file(en_path).contains("msgstr \"Hi\""));
	CHECK(read_file(fr_path).contains("msgstr \"Salut\""));
	CHECK_FALSE(document->has_save_conflict());

	DirAccess::remove_absolute(fr_path + ".tmp");
	CHECK(document->save() == OK);
	CHECK(read_file(fr_path).contains("msgstr \"Bonjour\""));
	CHECK_FALSE(document->reload_if_changed());

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

TEST_CASE("[TranslationDocument] CSV contexts, plurals and comments") {
	const String path = TestUtils::get_temp_path("translation_document_plurals.csv");
	const String csv = String::utf8(
			"keys,?plural,en,ru\n"
			"?pluralrule,,,nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);\n"
			"APPLE,APPLES,apple,яблоко\n"
			",,apples,яблока\n"
			",,,яблок\n"
			"PEAR,,pear,груша\n");
	write_file(path, csv);

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_CSV, path) == OK);
	CHECK(document->get_plural_key("APPLE") == "APPLES");
	CHECK(document->get_plural_texts("APPLE", "en") == PackedStringArray{ "apple", "apples" });
	CHECK(document->get_plural_texts("APPLE", "ru") == PackedStringArray{ String::utf8("яблоко"), String::utf8("яблока"), String::utf8("яблок") });
	CHECK(document->get_locale_plural_rules("ru").begins_with("nplurals=3;"));
	CHECK(document->get_locale_plural_rules("en").is_empty());
	CHECK(document->get_text("PEAR", "ru") == String::utf8("груша"));
	CHECK(document->save() == OK);
	CHECK(read_file(path) == csv);

	document->set_plural_texts("APPLE", "en", PackedStringArray{ "an apple", "apples" });
	document->set_text("BOOK", "en", "Book", "library");
	document->set_comment("PEAR", "A fruit");
	CHECK(document->save() == OK);
	CHECK(read_file(path) == String::utf8("keys,?plural,en,ru,?context,_comments\n"
										  "?pluralrule,,,nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);,,\n"
										  "APPLE,APPLES,an apple,яблоко,,\n"
										  ",,apples,яблока,,\n"
										  ",,,яблок,,\n"
										  "PEAR,,pear,груша,,A fruit\n"
										  "BOOK,,Book,,library,\n"));

	Ref<TranslationDocument> reloaded;
	reloaded.instantiate();
	REQUIRE(reloaded->load(TranslationDocument::FORMAT_CSV, path) == OK);
	CHECK(reloaded->get("entries") == document->get("entries"));
}

TEST_CASE("[TranslationDocument] PO files keep their comments") {
	const String folder = TestUtils::get_temp_path("translation_document_po");
	DirAccess::make_dir_recursive_absolute(folder);
	const String ru_path = folder.path_join("ru.po");
	const String en_path = folder.path_join("en.po");
	const String ru = String::utf8(
			"# Translators of the project.\n"
			"msgid \"\"\n"
			"msgstr \"\"\n"
			"\"Language: ru\\n\"\n"
			"\"Plural-Forms: nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);\\n\"\n"
			"\n"
			"#. Greeting\n"
			"#: main.tscn\n"
			"msgid \"HELLO\"\n"
			"msgstr \"Привет\"\n"
			"\n"
			"#: main.gd:10\n"
			"#, fuzzy\n"
			"msgctxt \"menu\"\n"
			"msgid \"OPEN\"\n"
			"msgstr \"Открыть\"\n"
			"\n"
			"msgid \"APPLE\"\n"
			"msgid_plural \"APPLES\"\n"
			"msgstr[0] \"яблоко\"\n"
			"msgstr[1] \"яблока\"\n"
			"msgstr[2] \"яблок\"\n"
			"\n"
			"#~ msgid \"OLD\"\n"
			"#~ msgstr \"Старое\"\n");
	const String en = String(
			"msgid \"\"\n"
			"msgstr \"\"\n"
			"\"Language: en\\n\"\n"
			"\n"
			"msgid \"HELLO\"\n"
			"msgstr \"Hello\"\n");
	write_file(ru_path, ru);
	write_file(en_path, en);

	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ en_path, ru_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);
	CHECK(document->get_locales() == PackedStringArray{ "en", "ru" });
	CHECK(document->get_text("HELLO", "ru") == String::utf8("Привет"));
	CHECK(document->get_text("HELLO", "en") == "Hello");
	CHECK(document->get_comment("HELLO") == "Greeting");
	CHECK(document->get_text("OPEN", "ru", "menu") == String::utf8("Открыть"));
	CHECK(document->get_plural_key("APPLE") == "APPLES");
	CHECK(document->get_plural_texts("APPLE", "ru").size() == 3);
	CHECK(document->get_locale_plural_rules("ru").begins_with("nplurals=3;"));
	CHECK(document->save() == OK);
	CHECK(read_file(ru_path) == ru);
	CHECK(read_file(en_path) == en);

	// Only the changed entry is written again, the keys missing in a file are only added with their texts.
	document->set_text("HELLO", "ru", String::utf8("Здравствуй"));
	CHECK(document->save() == OK);
	CHECK(read_file(en_path) == en);
	CHECK(read_file(ru_path) == ru.replace(String::utf8("msgstr \"Привет\""), String::utf8("msgstr \"Здравствуй\"")));

	document->set_comment("OPEN", "Menu item", "menu");
	document->set_text("OPEN", "en", "Open", "menu");
	document->add_key("NEW");
	CHECK(document->save() == OK);
	CHECK(read_file(ru_path).contains(String::utf8(
			"#. Menu item\n"
			"#: main.gd:10\n"
			"#, fuzzy\n"
			"msgctxt \"menu\"\n"
			"msgid \"OPEN\"\n"
			"msgstr \"Открыть\"\n")));
	CHECK(read_file(ru_path).ends_with(String::utf8(
			"msgid \"NEW\"\n"
			"msgstr \"\"\n"
			"\n"
			"#~ msgid \"OLD\"\n"
			"#~ msgstr \"Старое\"\n")));
	CHECK(read_file(en_path) == en + String("\n"
											"#. Menu item\n"
											"msgctxt \"menu\"\n"
											"msgid \"OPEN\"\n"
											"msgstr \"Open\"\n"
											"\n"
											"msgid \"NEW\"\n"
											"msgstr \"\"\n"));

	Ref<TranslationDocument> reloaded;
	reloaded.instantiate();
	REQUIRE(reloaded->load(TranslationDocument::FORMAT_PO, folder) == OK);
	CHECK(reloaded->get_text("HELLO", "ru") == String::utf8("Здравствуй"));
	CHECK(reloaded->get_comment("OPEN", "menu") == "Menu item");
	CHECK(reloaded->has_key("NEW"));

	// The entry keeps what was read from the files with another context.
	document->set_context("OPEN", "menu", "file");
	CHECK(document->save() == OK);
	CHECK(read_file(ru_path).contains(String::utf8(
			"#. Menu item\n"
			"#: main.gd:10\n"
			"#, fuzzy\n"
			"msgctxt \"file\"\n"
			"msgid \"OPEN\"\n"
			"msgstr \"Открыть\"\n")));
	CHECK_FALSE(document->has_key("OPEN", "menu"));

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

TEST_CASE("[TranslationDocument] Reloaded when the files change") {
	const String folder = TestUtils::get_temp_path("translation_document_reload");
	DirAccess::make_dir_recursive_absolute(folder);
	const String de_path = folder.path_join("de.po");
	const String fr_path = folder.path_join("fr.po");
	write_file(de_path, "msgid \"\"\nmsgstr \"\"\n\"Language: de\\n\"\n\nmsgid \"HELLO\"\nmsgstr \"Hallo\"\n");
	write_file(fr_path, "msgid \"\"\nmsgstr \"\"\n\"Language: fr\\n\"\n\nmsgid \"BYE\"\nmsgstr \"Salut\"\n\nmsgid \"HELLO\"\nmsgstr \"Bonjour\"\n");

	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ de_path, fr_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);
	CHECK(document->get_keys() == PackedStringArray{ "HELLO", "BYE" });
	CHECK_FALSE(document->reload_if_changed());

	// A file removed from the project settings.
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ fr_path });
	CHECK(document->reload_if_changed());
	CHECK(document->get_locales() == PackedStringArray{ "fr" });
	CHECK(document->get_keys() == PackedStringArray{ "BYE", "HELLO" });
	CHECK(document->get_text("HELLO", "fr") == "Bonjour");
	CHECK(document->get_text("HELLO", "de").is_empty());

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray());
	CHECK(document->reload_if_changed());
	CHECK(document->get_locales().is_empty());
	CHECK(document->get_keys().is_empty());
	CHECK_FALSE(document->has_key("HELLO"));

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

TEST_CASE("[TranslationDocument] PO entries without empty lines between them") {
	const String folder = TestUtils::get_temp_path("translation_document_po_compact");
	DirAccess::make_dir_recursive_absolute(folder);
	const String de_path = folder.path_join("de.po");
	const String de = String::utf8(
			"msgid \"\"\n"
			"msgstr \"\"\n"
			"\"Language: de\\n\"\n"
			"msgid \"APPLE\"\n"
			"msgstr \"Apfel\"\n"
			"#, fuzzy\n"
			"msgid \"PEAR\"\n"
			"msgstr \"Birne\"\n"
			"msgctxt \"menu\"\n"
			"msgid \"OPEN\"\n"
			"msgstr \"Öffnen\"\n");
	write_file(de_path, de);

	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ de_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);
	CHECK(document->get_keys() == PackedStringArray{ "APPLE", "PEAR", "OPEN" });
	CHECK(document->get_text("APPLE", "de") == "Apfel");
	CHECK(document->get_text("PEAR", "de") == "Birne");
	CHECK(document->get_text("OPEN", "de", "menu") == String::utf8("Öffnen"));
	// Saved with empty lines between the entries, and nothing lost.
	document->set_text("APPLE", "de", "Apfelbaum");
	CHECK(document->save() == OK);
	CHECK(read_file(de_path).contains("#, fuzzy\nmsgid \"PEAR\"\nmsgstr \"Birne\"\n"));
	Ref<TranslationDocument> reloaded;
	reloaded.instantiate();
	REQUIRE(reloaded->load(TranslationDocument::FORMAT_PO, folder) == OK);
	CHECK(reloaded->get_keys() == PackedStringArray{ "APPLE", "PEAR", "OPEN" });
	CHECK(reloaded->get_text("APPLE", "de") == "Apfelbaum");
	CHECK(reloaded->get_text("OPEN", "de", "menu") == String::utf8("Öffnen"));

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

TEST_CASE("[TranslationDocument] PO texts with control characters") {
	const String folder = TestUtils::get_temp_path("translation_document_po_control");
	DirAccess::make_dir_recursive_absolute(folder);
	const String de_path = folder.path_join("de.po");
	write_file(de_path, "msgid \"\"\nmsgstr \"\"\n\"Language: de\\n\"\n\nmsgid \"KEY\"\nmsgstr \"\"\n");

	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ de_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);
	const String text = String("A\rB\tC\"D\\E") + String::chr(7) + "F";
	document->set_text("KEY", "de", text);
	CHECK(document->save() == OK);
	CHECK_MESSAGE(!read_file(de_path).contains("\r"), "Control characters are escaped.");

	Ref<TranslationDocument> reloaded;
	reloaded.instantiate();
	REQUIRE(reloaded->load(TranslationDocument::FORMAT_PO, folder) == OK);
	CHECK(reloaded->get_text("KEY", "de") == text);

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

TEST_CASE("[TranslationDocument] Added locales don't overwrite PO files outside of the project") {
	const String folder = TestUtils::get_temp_path("translation_document_po_add");
	DirAccess::make_dir_recursive_absolute(folder);
	const String de_path = folder.path_join("de.po");
	const String fr_path = folder.path_join("fr.po");
	const String de = "msgid \"\"\nmsgstr \"\"\n\"Language: de\\n\"\n\nmsgid \"HELLO\"\nmsgstr \"Hallo\"\n";
	const String fr = "msgid \"\"\nmsgstr \"\"\n\"Language: fr\\n\"\n\nmsgid \"HELLO\"\nmsgstr \"Bonjour\"\n";
	write_file(de_path, de);
	write_file(fr_path, fr);

	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ de_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);
	document->add_locale("fr");
	CHECK(document->save() == OK);
	CHECK(read_file(fr_path) == fr);
	const String new_path = folder.path_join("fr_2.po");
	CHECK(FileAccess::exists(new_path));
	CHECK(PackedStringArray(GLOBAL_GET("internationalization/locale/translations")).has(new_path));
	CHECK_FALSE(PackedStringArray(GLOBAL_GET("internationalization/locale/translations")).has(fr_path));

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

TEST_CASE("[TranslationDocument] Snapshots restore removed keys and locales as they were read") {
	const String folder = TestUtils::get_temp_path("translation_document_po_snapshot");
	DirAccess::make_dir_recursive_absolute(folder);
	const String en_path = folder.path_join("en.po");
	const String ru_path = folder.path_join("russian.po");
	const String en = "msgid \"\"\nmsgstr \"\"\n\"Language: en\\n\"\n\nmsgid \"HELLO\"\nmsgstr \"Hello\"\n";
	const String ru = String::utf8(
			"# Translators of the project.\n"
			"msgid \"\"\n"
			"msgstr \"\"\n"
			"\"Language: ru\\n\"\n"
			"\n"
			"#: main.tscn\n"
			"#, fuzzy\n"
			"msgid \"HELLO\"\n"
			"msgstr \"Привет\"\n"
			"\n"
			"#~ msgid \"OLD\"\n"
			"#~ msgstr \"Старое\"\n");
	write_file(en_path, en);
	write_file(ru_path, ru);

	const PackedStringArray old_translations = GLOBAL_GET("internationalization/locale/translations");
	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", PackedStringArray{ en_path, ru_path });

	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_PO, folder) == OK);

	// The comments and flags of a removed key come back with it.
	Ref<TranslationDocument> snapshot = document->get_snapshot();
	document->remove_key("HELLO");
	CHECK(document->save() == OK);
	CHECK_FALSE(read_file(ru_path).contains("fuzzy"));
	document->set_snapshot(snapshot);
	CHECK(document->save() == OK);
	CHECK(read_file(ru_path) == ru);
	CHECK(read_file(en_path) == en);

	// The file of a removed locale comes back with its name, header and obsolete entries.
	snapshot = document->get_snapshot();
	document->remove_locale("ru");
	document->save();
	document->set_snapshot(snapshot);
	CHECK(document->save() == OK);
	CHECK(read_file(ru_path) == ru);
	CHECK(PackedStringArray(GLOBAL_GET("internationalization/locale/translations")).has(ru_path));

	// The file of an added locale is removed again, from the disk and from the project.
	snapshot = document->get_snapshot();
	document->add_locale("fr");
	CHECK(document->save() == OK);
	const String fr_path = folder.path_join("fr.po");
	CHECK(FileAccess::exists(fr_path));
	CHECK(PackedStringArray(GLOBAL_GET("internationalization/locale/translations")).has(fr_path));
	document->set_snapshot(snapshot);
	const Error err = document->save();
	CHECK(document->get_locales() == PackedStringArray{ "en", "ru" });
	if (err == OK) {
		CHECK_FALSE(FileAccess::exists(fr_path));
		CHECK_FALSE(PackedStringArray(GLOBAL_GET("internationalization/locale/translations")).has(fr_path));
	} else {
		// The trash isn't available everywhere, e.g. for temporary folders, then the file is removed at the next save.
		CHECK(document->get_last_error().contains("fr.po"));
	}

	ProjectSettings::get_singleton()->set_setting("internationalization/locale/translations", old_translations);
}

} // namespace TestTranslationDocument

#endif // TOOLS_ENABLED
