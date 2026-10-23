/**************************************************************************/
/*  test_localization_dock.cpp                                            */
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

TEST_FORCE_LINK(test_localization_dock)

#ifdef TOOLS_ENABLED

#include "core/io/file_access.h"
#include "editor/translations/localization_dock.h"
#include "editor/translations/translation_document.h"
#include "tests/test_utils.h"

namespace TestLocalizationDock {

static Dictionary make_entry(const String &p_key, const Dictionary &p_texts, const String &p_plural_key = String()) {
	Dictionary entry;
	entry["key"] = p_key;
	entry["texts"] = p_texts;
	if (!p_plural_key.is_empty()) {
		entry["plural_key"] = p_plural_key;
	}
	return entry;
}

TEST_CASE("[LocalizationDock] Translated texts") {
	Dictionary texts;
	texts["de"] = "Apfel";
	texts["fr"] = "";
	const Dictionary entry = make_entry("APPLE", texts);
	CHECK(LocalizationDock::is_entry_translated(entry, "de", 1));
	CHECK_FALSE_MESSAGE(LocalizationDock::is_entry_translated(entry, "fr", 1), "Empty texts aren't translated.");
	CHECK_FALSE(LocalizationDock::is_entry_translated(entry, "ru", 1));

	Dictionary plural_texts;
	plural_texts["ru"] = PackedStringArray{ String::utf8("яблоко"), String::utf8("яблока") };
	plural_texts["en"] = PackedStringArray{ "apple", "apples" };
	const Dictionary plural = make_entry("APPLES", plural_texts, "apples");
	CHECK(LocalizationDock::is_entry_translated(plural, "en", 2));
	CHECK_FALSE_MESSAGE(LocalizationDock::is_entry_translated(plural, "ru", 3), "All the forms of the locale are needed.");

	CHECK(LocalizationDock::get_plural_count("en", String()) == 2);
	CHECK(LocalizationDock::get_plural_count("ru", String()) == 3);
	CHECK(LocalizationDock::get_plural_count("ja", String()) == 1);
	CHECK(LocalizationDock::get_plural_count("en", "nplurals=3; plural=(n==1 ? 0 : n==2 ? 1 : 2);") == 3);
}

TEST_CASE("[LocalizationDock] Search") {
	Dictionary texts;
	texts["de"] = "Apfelbaum";
	Dictionary entry = make_entry("TREE", texts);
	entry["comment"] = "In the garden";
	const PackedStringArray locales{ "de" };
	CHECK(LocalizationDock::entry_matches_search(entry, locales, ""));
	CHECK(LocalizationDock::entry_matches_search(entry, locales, "tree"));
	CHECK_MESSAGE(LocalizationDock::entry_matches_search(entry, locales, "  APFEL "), "Texts are searched, ignoring the case and spaces around.");
	CHECK(LocalizationDock::entry_matches_search(entry, locales, "garden"));
	CHECK_FALSE(LocalizationDock::entry_matches_search(entry, locales, "pear"));
	CHECK_FALSE_MESSAGE(LocalizationDock::entry_matches_search(entry, PackedStringArray(), "apfel"), "Only the texts of the given locales are searched.");
}

TEST_CASE("[LocalizationDock] Unique keys") {
	const String path = TestUtils::get_temp_path("localization_dock_keys.csv");
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	REQUIRE(f.is_valid());
	f->store_string("keys,en\n");
	f.unref();
	Ref<TranslationDocument> document;
	document.instantiate();
	REQUIRE(document->load(TranslationDocument::FORMAT_CSV, path) == OK);
	CHECK(LocalizationDock::make_unique_key(document, "new_key", StringName()) == "new_key");
	document->add_key("new_key");
	CHECK(LocalizationDock::make_unique_key(document, "new_key", StringName()) == "new_key_2");
	document->add_key("new_key_2");
	CHECK(LocalizationDock::make_unique_key(document, "new_key", StringName()) == "new_key_3");
	CHECK_MESSAGE(LocalizationDock::make_unique_key(document, "new_key", "menu") == "new_key", "Keys are unique within their context.");
}

} // namespace TestLocalizationDock

#endif // TOOLS_ENABLED
