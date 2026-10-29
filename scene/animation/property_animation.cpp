/**************************************************************************/
/*  property_animation.cpp                                                */
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

#include "property_animation.h"

#include "core/object/class_db.h"

bool PropertyAnimation::_get_components(const Variant &p_value, LocalVector<real_t> &r_components) {
	r_components.clear();
	switch (p_value.get_type()) {
		case Variant::FLOAT:
		case Variant::INT: {
			r_components.push_back(real_t(p_value));
		} break;
		case Variant::VECTOR2: {
			const Vector2 v = p_value;
			r_components = LocalVector<real_t>{ v.x, v.y };
		} break;
		case Variant::VECTOR2I: {
			const Vector2i v = p_value;
			r_components = LocalVector<real_t>{ real_t(v.x), real_t(v.y) };
		} break;
		case Variant::VECTOR3: {
			const Vector3 v = p_value;
			r_components = LocalVector<real_t>{ v.x, v.y, v.z };
		} break;
		case Variant::VECTOR3I: {
			const Vector3i v = p_value;
			r_components = LocalVector<real_t>{ real_t(v.x), real_t(v.y), real_t(v.z) };
		} break;
		case Variant::VECTOR4: {
			const Vector4 v = p_value;
			r_components = LocalVector<real_t>{ v.x, v.y, v.z, v.w };
		} break;
		case Variant::VECTOR4I: {
			const Vector4i v = p_value;
			r_components = LocalVector<real_t>{ real_t(v.x), real_t(v.y), real_t(v.z), real_t(v.w) };
		} break;
		case Variant::COLOR: {
			const Color c = p_value;
			r_components = LocalVector<real_t>{ c.r, c.g, c.b, c.a };
		} break;
		case Variant::QUATERNION: {
			const Quaternion q = p_value;
			r_components = LocalVector<real_t>{ q.x, q.y, q.z, q.w };
		} break;
		case Variant::RECT2: {
			const Rect2 r = p_value;
			r_components = LocalVector<real_t>{ r.position.x, r.position.y, r.size.x, r.size.y };
		} break;
		case Variant::RECT2I: {
			const Rect2i r = p_value;
			r_components = LocalVector<real_t>{ real_t(r.position.x), real_t(r.position.y), real_t(r.size.x), real_t(r.size.y) };
		} break;
		default: {
			return false;
		}
	}
	return true;
}

Variant PropertyAnimation::_make_value(Variant::Type p_type, const LocalVector<real_t> &p_c) {
	switch (p_type) {
		case Variant::FLOAT:
			return p_c[0];
		case Variant::INT:
			return int64_t(Math::round(p_c[0]));
		case Variant::VECTOR2:
			return Vector2(p_c[0], p_c[1]);
		case Variant::VECTOR2I:
			return Vector2i(Math::round(p_c[0]), Math::round(p_c[1]));
		case Variant::VECTOR3:
			return Vector3(p_c[0], p_c[1], p_c[2]);
		case Variant::VECTOR3I:
			return Vector3i(Math::round(p_c[0]), Math::round(p_c[1]), Math::round(p_c[2]));
		case Variant::VECTOR4:
			return Vector4(p_c[0], p_c[1], p_c[2], p_c[3]);
		case Variant::VECTOR4I:
			return Vector4i(Math::round(p_c[0]), Math::round(p_c[1]), Math::round(p_c[2]), Math::round(p_c[3]));
		case Variant::COLOR:
			return Color(p_c[0], p_c[1], p_c[2], p_c[3]);
		case Variant::QUATERNION: {
			const Quaternion q(p_c[0], p_c[1], p_c[2], p_c[3]);
			return q.length_squared() > CMP_EPSILON2 ? q.normalized() : Quaternion();
		}
		case Variant::RECT2:
			return Rect2(p_c[0], p_c[1], p_c[2], p_c[3]);
		case Variant::RECT2I:
			return Rect2i(Math::round(p_c[0]), Math::round(p_c[1]), Math::round(p_c[2]), Math::round(p_c[3]));
		default:
			return Variant();
	}
}

void PropertyAnimation::_apply() {
	Object *target = ObjectDB::get_instance(object);
	if (target) {
		target->set_indexed(subnames, _make_value(type, current));
	}
}

bool PropertyAnimation::_step(double p_seconds) {
	timing.advance(p_seconds);
	const real_t weight = timing.get_weight();
	for (uint32_t i = 0; i < current.size(); i++) {
		current[i] = Math::lerp(from[i], to[i], weight);
	}
	_apply();
	return timing.is_playing();
}

void PropertyAnimation::_end(bool p_interrupted) {
	if (!running) {
		return;
	}
	running = false;
	interrupted = p_interrupted;
	emit_signal(SNAME("finished"));
}

Object *PropertyAnimation::get_object() const {
	return ObjectDB::get_instance(object);
}

Variant PropertyAnimation::get_final_value() const {
	return _make_value(type, to);
}

RequiredResult<PropertyAnimation> PropertyAnimation::set_ignore_time_scale(bool p_ignore) {
	ignore_time_scale = p_ignore;
	return this;
}

void PropertyAnimation::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_object"), &PropertyAnimation::get_object);
	ClassDB::bind_method(D_METHOD("get_property"), &PropertyAnimation::get_property);
	ClassDB::bind_method(D_METHOD("get_final_value"), &PropertyAnimation::get_final_value);
	ClassDB::bind_method(D_METHOD("set_ignore_time_scale", "ignore"), &PropertyAnimation::set_ignore_time_scale, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("is_ignoring_time_scale"), &PropertyAnimation::is_ignoring_time_scale);
	ClassDB::bind_method(D_METHOD("is_running"), &PropertyAnimation::is_running);
	ClassDB::bind_method(D_METHOD("is_interrupted"), &PropertyAnimation::is_interrupted);

	ADD_SIGNAL(MethodInfo("finished"));
}
