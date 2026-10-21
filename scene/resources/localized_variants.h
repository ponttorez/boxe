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

#include "core/core_string_names.h"
#include "core/object/callable_mp.h"
#include "core/object/message_queue.h"
#include "core/os/thread.h"
#include "core/string/translation_server.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"

// The resources of a localized resource by locale, and the one used for the current locale.
// The owner calls update() when the locale or the resources change, and when one of the resources emits changed.
template <typename T>
class LocalizedVariants {
	const Object *owner = nullptr;
	Callable on_variant_changed;
	LocalVector<Ref<T>> connected_variants;

	// Deeper values aren't followed, so deeply nested values can't overflow the stack.
	static constexpr int MAX_DEPTH = 128;

	// Only the properties which can hold the owner are followed, as getting the others can be slow, like the image of an
	// ImageTexture.
	static bool _can_hold(const PropertyInfo &p_property, const Object *p_resource) {
		if (!(p_property.usage & PROPERTY_USAGE_STORAGE)) {
			return false;
		}
		if (p_property.type == Variant::ARRAY || p_property.type == Variant::DICTIONARY) {
			return true;
		}
		if (p_property.type != Variant::OBJECT) {
			return false;
		}
		if (p_property.hint != PROPERTY_HINT_RESOURCE_TYPE) {
			return true;
		}
		for (const String &type : p_property.hint_string.split(",")) {
			if (p_resource->is_class(type.strip_edges())) {
				return true;
			}
		}
		return false;
	}

	// True when p_value is p_resource or holds it, e.g. an AtlasTexture of p_resource. Objects, arrays and dictionaries
	// are visited once, as they can contain themselves.
	static bool _contains(const Variant &p_value, const Object *p_resource, HashSet<const void *> &r_visited, int p_depth = 0) {
		if (p_depth > MAX_DEPTH) {
			return false;
		}
		switch (p_value.get_type()) {
			case Variant::OBJECT: {
				const Object *object = p_value.get_validated_object();
				if (!object) {
					return false;
				}
				if (object == p_resource) {
					return true;
				}
				if (r_visited.has(object)) {
					return false;
				}
				r_visited.insert(object);
				List<PropertyInfo> properties;
				object->get_property_list(&properties);
				for (const PropertyInfo &E : properties) {
					if (_can_hold(E, p_resource) && _contains(object->get(E.name), p_resource, r_visited, p_depth + 1)) {
						return true;
					}
				}
			} break;
			case Variant::ARRAY: {
				const Array array = p_value;
				if (r_visited.has(array.id())) {
					return false;
				}
				r_visited.insert(array.id());
				for (const Variant &E : array) {
					if (_contains(E, p_resource, r_visited, p_depth + 1)) {
						return true;
					}
				}
			} break;
			case Variant::DICTIONARY: {
				const Dictionary dictionary = p_value;
				if (r_visited.has(dictionary.id())) {
					return false;
				}
				r_visited.insert(dictionary.id());
				for (const KeyValue<Variant, Variant> &E : dictionary) {
					if (_contains(E.key, p_resource, r_visited, p_depth + 1) || _contains(E.value, p_resource, r_visited, p_depth + 1)) {
						return true;
					}
				}
			} break;
			default:
				break;
		}
		return false;
	}

	// The resources which contain the owner since they were set, e.g. an AtlasTexture of the owner made later, are
	// removed: the owner would use itself, and the resources would keep each other in memory.
	void _remove_cycles() {
		if (makes_cycle(default_variant)) {
			ERR_PRINT(vformat("A %s can't contain itself, its default resource is removed.", owner->get_class()));
			default_variant.unref();
		}
		LocalVector<String> removed;
		for (const KeyValue<String, Ref<T>> &E : variants) {
			if (makes_cycle(E.value)) {
				removed.push_back(E.key);
			}
		}
		for (const String &locale : removed) {
			ERR_PRINT(vformat("A %s can't contain itself, the resource of the locale '%s' is removed.", owner->get_class(), locale));
			variants.erase(locale);
		}
	}

	void _connect_variant(const Ref<T> &p_variant) {
		if (p_variant.is_valid() && !p_variant->is_connected(CoreStringName(changed), on_variant_changed)) {
			p_variant->connect_changed(on_variant_changed);
			connected_variants.push_back(p_variant);
		}
	}

	// Every resource is followed, not only the current one, as any can come to contain the owner.
	void _connect_variants() {
		if (!on_variant_changed.is_valid()) {
			return;
		}
		for (const Ref<T> &variant : connected_variants) {
			if (variant->is_connected(CoreStringName(changed), on_variant_changed)) {
				variant->disconnect_changed(on_variant_changed);
			}
		}
		connected_variants.clear();
		_connect_variant(default_variant);
		for (const KeyValue<String, Ref<T>> &E : variants) {
			_connect_variant(E.value);
		}
	}

	static void _connect_locale_changed(const Callable &p_callable) {
		TranslationServer::get_singleton()->connect(SNAME("locale_changed"), p_callable);
	}

	// The locale can change before the connection, so the resource is selected again with it.
	static void _connect_locale_changed_and_update(const Callable &p_callable) {
		// The owner can be freed before.
		if (!p_callable.is_valid()) {
			return;
		}
		_connect_locale_changed(p_callable);
		p_callable.call();
	}

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
	void set_dictionary(const Dictionary &p_variants) {
		variants.clear();
		for (const KeyValue<Variant, Variant> &E : p_variants) {
			const String locale = TranslationServer::get_singleton()->standardize_locale(E.key);
			const Ref<T> variant = E.value;
			if (locale.is_empty()) {
				continue;
			}
			ERR_CONTINUE_MSG(makes_cycle(variant), vformat("A %s can't contain itself, the resource of the locale '%s' is ignored.", owner->get_class(), locale));
			variants[locale] = variant;
		}
	}

	// True when using p_variant would make the owner use itself, also through other resources.
	bool makes_cycle(const Ref<T> &p_variant) const {
		HashSet<const void *> visited;
		return _contains(p_variant, owner, visited);
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

	// Returns true when the resource used changed. The owner also calls it when one of the resources changes, as it can
	// contain the owner since then.
	bool update() {
		_remove_cycles();
		_connect_variants();
		const Ref<T> selected = select();
		if (selected == current) {
			return false;
		}
		current = selected;
		return true;
	}

	// Connects to the locale changes on the main thread, as resources can be loaded on other threads.
	void setup(const Object *p_owner, const Callable &p_on_locale_changed, const Callable &p_on_variant_changed) {
		owner = p_owner;
		on_variant_changed = p_on_variant_changed;
		if (Thread::is_main_thread()) {
			_connect_locale_changed(p_on_locale_changed);
		} else {
			MessageQueue::get_main_singleton()->push_callable(callable_mp_static(&LocalizedVariants::_connect_locale_changed_and_update).bind(p_on_locale_changed));
		}
	}
};
