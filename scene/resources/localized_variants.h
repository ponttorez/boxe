/**************************************************************************/
/*  localized_variants.h                                                  */
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

#include "core/object/callable_mp.h"
#include "core/object/message_queue.h"
#include "core/os/thread.h"
#include "core/string/translation_server.h"

// The resources of a localized resource by locale, and the one used for the current locale.
// The owner calls update() when the locale or the resources change.
template <typename T>
class LocalizedVariants {
	Callable on_current_changed;

public:
	Ref<T> default_variant;
	HashMap<String, Ref<T>> variants;
	Ref<T> current;

	Dictionary get_dictionary() const {
		Dictionary ret;
		for (const KeyValue<String, Ref<T>> &E : variants) {
			ret[E.key] = E.value;
		}
		return ret;
	}

	// Without the resources containing the owner, which would use themselves.
	template <typename Owner>
	void set_dictionary(const Dictionary &p_variants, const Owner *p_owner) {
		variants.clear();
		for (const KeyValue<Variant, Variant> &E : p_variants) {
			const String locale = TranslationServer::get_singleton()->standardize_locale(E.key);
			const Ref<T> variant = E.value;
			if (locale.is_empty()) {
				continue;
			}
			ERR_CONTINUE_MSG(makes_cycle(variant, p_owner), vformat("A %s can't contain itself, the resource of the locale '%s' is ignored.", Owner::get_class_static(), locale));
			variants[locale] = variant;
		}
	}

	// True when p_resource is used, or used by the localized resources of the same type which are used.
	template <typename Owner>
	static bool uses(const Ref<T> &p_variant, const Object *p_resource) {
		if (p_variant.is_null()) {
			return false;
		}
		if (p_variant.ptr() == p_resource) {
			return true;
		}
		const Owner *nested = Object::cast_to<Owner>(p_variant.ptr());
		return nested && nested->uses_resource(p_resource);
	}

	template <typename Owner>
	bool uses_resource(const Object *p_resource) const {
		if (uses<Owner>(default_variant, p_resource)) {
			return true;
		}
		for (const KeyValue<String, Ref<T>> &E : variants) {
			if (uses<Owner>(E.value, p_resource)) {
				return true;
			}
		}
		return false;
	}

	// True when using p_variant would make p_owner use itself.
	template <typename Owner>
	static bool makes_cycle(const Ref<T> &p_variant, const Owner *p_owner) {
		return uses<Owner>(p_variant, p_owner);
	}

	Ref<T> select() const {
		Vector<String> locales;
		for (const KeyValue<String, Ref<T>> &E : variants) {
			if (E.value.is_valid()) {
				locales.push_back(E.key);
			}
		}
		const String locale = TranslationServer::get_singleton()->find_resource_locale(locales);
		return locale.is_empty() ? default_variant : variants[locale];
	}

	// Returns true when the resource used changed.
	bool update() {
		const Ref<T> selected = select();
		if (selected == current) {
			return false;
		}
		if (current.is_valid() && on_current_changed.is_valid()) {
			current->disconnect_changed(on_current_changed);
		}
		current = selected;
		if (current.is_valid() && on_current_changed.is_valid()) {
			current->connect_changed(on_current_changed);
		}
		return true;
	}

	static void _connect_locale_changed(const Callable &p_callable) {
		TranslationServer::get_singleton()->connect(SNAME("locale_changed"), p_callable);
	}

	// Connects to the locale changes on the main thread, as resources can be loaded on other threads.
	void setup(const Callable &p_on_locale_changed, const Callable &p_on_current_changed = Callable()) {
		on_current_changed = p_on_current_changed;
		if (Thread::is_main_thread()) {
			_connect_locale_changed(p_on_locale_changed);
		} else {
			MessageQueue::get_main_singleton()->push_callable(callable_mp_static(&LocalizedVariants::_connect_locale_changed).bind(p_on_locale_changed));
		}
	}
};
