/**************************************************************************/
/*  localization_dock_plugin.cpp                                          */
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

#include "localization_dock_plugin.h"

#include "core/config/project_settings.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/translations/localization_dock.h"
#include "editor/translations/translation_document.h"

void LocalizationDockPlugin::edit(Object *p_object) {
	const Translation *translation = Object::cast_to<Translation>(p_object);
	if (translation) {
		dock->edit(TranslationDocument::get_document_for_file(translation->get_path()));
	}
}

bool LocalizationDockPlugin::handles(Object *p_object) const {
	// The translations of the project, the ones edited in the dock.
	const Translation *translation = Object::cast_to<Translation>(p_object);
	if (!translation || !translation->get_path().is_resource_file()) {
		return false;
	}
	const PackedStringArray translations = GLOBAL_GET("internationalization/locale/translations");
	return translations.has(translation->get_path());
}

void LocalizationDockPlugin::make_visible(bool p_visible) {
	// The dock stays open with the last translations, to edit them while previewing scenes.
	if (p_visible) {
		dock->make_visible();
	}
}

LocalizationDockPlugin::LocalizationDockPlugin() {
	dock = memnew(LocalizationDock);
	EditorDockManager::get_singleton()->add_dock(dock);
	dock->close();
}
