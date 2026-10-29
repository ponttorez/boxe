/**************************************************************************/
/*  property_animation.h                                                  */
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
#include "core/templates/local_vector.h"
#include "scene/animation/transition_timing.h"

class SceneTree;

// A property animated by SceneTree.animate_property(). Animating the property again continues from where it is, so
// the animation only exists until it ends or another one replaces it.
class PropertyAnimation : public RefCounted {
	GDCLASS(PropertyAnimation, RefCounted);
	friend class SceneTree;

	ObjectID object;
	NodePath property;
	Vector<StringName> subnames;
	Variant::Type type = Variant::NIL;
	// The numbers making up the values, e.g. the components of a Vector2.
	LocalVector<real_t> from;
	LocalVector<real_t> to;
	LocalVector<real_t> current;
	TransitionTiming timing;
	bool ignore_time_scale = false;
	bool running = false;
	bool interrupted = false;

	// Whether a value of p_type can be animated, and the numbers making it up.
	static bool _get_components(const Variant &p_value, LocalVector<real_t> &r_components);
	static Variant _make_value(Variant::Type p_type, const LocalVector<real_t> &p_components);

	void _apply();
	// Moves the animation forward and sets the property. Returns false once it ended.
	bool _step(double p_seconds);
	void _end(bool p_interrupted);

protected:
	static void _bind_methods();

public:
	Object *get_object() const;
	NodePath get_property() const { return property; }
	Variant get_final_value() const;

	RequiredResult<PropertyAnimation> set_ignore_time_scale(bool p_ignore = true);
	bool is_ignoring_time_scale() const { return ignore_time_scale; }

	bool is_running() const { return running; }
	bool is_interrupted() const { return interrupted; }
};
