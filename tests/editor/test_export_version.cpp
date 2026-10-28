/**************************************************************************/
/*  test_export_version.cpp                                               */
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

TEST_FORCE_LINK(test_export_version)

#ifdef TOOLS_ENABLED

#include "core/config/project_settings.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_export_platform_extension.h"
#include "editor/export/editor_export_preset.h"
#include "scene/resources/image_texture.h"

namespace TestExportVersion {

// A platform with the version options of the platforms that export them.
class VersionExportPlatform : public EditorExportPlatformExtension {
public:
	virtual void get_export_options(List<ExportOption> *r_options) const override {
		r_options->push_back(ExportOption(PropertyInfo(Variant::INT, "version/code"), 0));
		r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/version"), ""));
		r_options->push_back(ExportOption(PropertyInfo(Variant::STRING, "application/file_version"), ""));
	}
	virtual void get_platform_features(List<String> *r_features) const override {}
	virtual void get_preset_features(const Ref<EditorExportPreset> &p_preset, List<String> *r_features) const override {}
};

// Sets the version project settings for a test, and restores them after it.
class VersionSettings {
	Variant version = GLOBAL_GET("application/config/version");
	Variant version_code = GLOBAL_GET("application/config/version_code");
	EditorExport *editor_export = nullptr;

public:
	Ref<EditorExportPreset> preset;

	VersionSettings(const String &p_version, int p_version_code) {
		ProjectSettings::get_singleton()->set_setting("application/config/version", p_version);
		ProjectSettings::get_singleton()->set_setting("application/config/version_code", p_version_code);
		// Presets save themselves through EditorExport when an option changes.
		if (!EditorExport::get_singleton()) {
			editor_export = memnew(EditorExport);
		}
		Ref<VersionExportPlatform> platform;
		platform.instantiate();
		preset = platform->create_preset();
	}

	void set(const StringName &p_name, const Variant &p_value) {
		// The save timer of EditorExport isn't in a tree, so it can't start.
		ERR_PRINT_OFF;
		preset->set(p_name, p_value);
		ERR_PRINT_ON;
	}

	~VersionSettings() {
		preset.unref();
		if (editor_export) {
			memdelete(editor_export);
		}
		ProjectSettings::get_singleton()->set_setting("application/config/version", version);
		ProjectSettings::get_singleton()->set_setting("application/config/version_code", version_code);
	}
};

TEST_CASE("[EditorExportPreset] The version code of the preset comes before the one of the project") {
	VersionSettings settings("1.2", 0);
	CHECK(settings.preset->get_version_code("version/code") == 0);

	ProjectSettings::get_singleton()->set_setting("application/config/version_code", 7);
	CHECK(settings.preset->get_version_code("version/code") == 7);

	settings.set("version/code", 3);
	CHECK(settings.preset->get_version_code("version/code") == 3);
}

TEST_CASE("[EditorExportPreset] Large version codes aren't narrowed") {
	VersionSettings settings("1.2", 0);
	ProjectSettings::get_singleton()->set_setting("application/config/version_code", (int64_t)3000000000);
	CHECK(settings.preset->get_version_code("version/code") == (int64_t)3000000000);
	CHECK(settings.preset->get_build_version("application/version") == "3000000000");
}

TEST_CASE("[EditorExportPreset] Build versions use the version code of the project when it's set") {
	VersionSettings settings("1.2", 0);
	CHECK(settings.preset->get_build_version("application/version") == "1.2");

	ProjectSettings::get_singleton()->set_setting("application/config/version_code", 7);
	CHECK(settings.preset->get_build_version("application/version") == "7");

	settings.set("application/version", "45");
	CHECK(settings.preset->get_build_version("application/version") == "45");
}

TEST_CASE("[EditorExportPreset] Windows versions take the version code as their fourth component") {
	VersionSettings settings("1.2", 0);
	CHECK(settings.preset->get_build_version("application/file_version", true) == "1.2.0.0");

	ProjectSettings::get_singleton()->set_setting("application/config/version_code", 7);
	CHECK(settings.preset->get_build_version("application/file_version", true) == "1.2.0.7");

	// A version code that doesn't fit in 16 bits isn't used.
	ProjectSettings::get_singleton()->set_setting("application/config/version_code", 70000);
	CHECK(settings.preset->get_build_version("application/file_version", true) == "1.2.0.0");

	// A project version with four components already has its fourth one.
	ProjectSettings::get_singleton()->set_setting("application/config/version", "1.2.3.4");
	ProjectSettings::get_singleton()->set_setting("application/config/version_code", 7);
	CHECK(settings.preset->get_build_version("application/file_version", true) == "1.2.3.4");

	settings.set("application/file_version", "2.0.0.9");
	CHECK(settings.preset->get_build_version("application/file_version", true) == "2.0.0.9");
}

} // namespace TestExportVersion

#endif // TOOLS_ENABLED
