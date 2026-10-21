/**************************************************************************/
/*  localized_font.cpp                                                    */
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

#include "localized_font.h"

#include "core/object/class_db.h"

// The base font of the variation is the font of the current locale.
void LocalizedFont::_update_font() {
	if (fonts.update()) {
		FontVariation::set_base_font(fonts.current);
	}
}

void LocalizedFont::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == "base_font") {
		p_property.usage = PROPERTY_USAGE_NONE;
	}
}

void LocalizedFont::set_default_font(const Ref<Font> &p_font) {
	ERR_FAIL_COND_MSG(fonts.makes_cycle(p_font, this), "A LocalizedFont can't contain itself.");
	fonts.default_variant = p_font;
	_update_font();
	emit_changed();
}

Ref<Font> LocalizedFont::get_default_font() const {
	return fonts.default_variant;
}

void LocalizedFont::set_fonts(const Dictionary &p_fonts) {
	fonts.set_dictionary(p_fonts, this);
	_update_font();
	emit_changed();
}

bool LocalizedFont::uses_resource(const Object *p_resource) const {
	return fonts.uses_resource<LocalizedFont>(p_resource);
}

Dictionary LocalizedFont::get_fonts() const {
	return fonts.get_dictionary();
}

Ref<Font> LocalizedFont::get_current_font() const {
	return fonts.current;
}

void LocalizedFont::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_default_font", "font"), &LocalizedFont::set_default_font);
	ClassDB::bind_method(D_METHOD("get_default_font"), &LocalizedFont::get_default_font);
	ClassDB::bind_method(D_METHOD("set_fonts", "fonts"), &LocalizedFont::set_fonts);
	ClassDB::bind_method(D_METHOD("get_fonts"), &LocalizedFont::get_fonts);
	ClassDB::bind_method(D_METHOD("get_current_font"), &LocalizedFont::get_current_font);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_font", PROPERTY_HINT_RESOURCE_TYPE, Font::get_class_static()), "set_default_font", "get_default_font");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "fonts", PROPERTY_HINT_DICTIONARY_TYPE, vformat("%d/%d:;%s", Variant::STRING, PROPERTY_HINT_LOCALE_ID, Font::get_class_static())), "set_fonts", "get_fonts");
}

LocalizedFont::LocalizedFont() {
	fonts.setup(callable_mp(this, &LocalizedFont::_update_font));
}
