/**************************************************************************/
/*  localized_texture_2d.cpp                                              */
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

#include "localized_texture_2d.h"

#include "core/object/class_db.h"
#include "servers/rendering/rendering_server.h"

bool LocalizedTexture2D::_update_texture() {
	if (!textures.update()) {
		return false;
	}
	const Ref<Texture2D> &texture = textures.current;
	RS::get_singleton()->texture_proxy_update(proxy, texture.is_valid() ? texture->get_rid() : proxy_placeholder);
	return true;
}

// One of the textures changed: it may have a new RID when reimported, or contain this texture since then.
void LocalizedTexture2D::_texture_changed() {
	textures.update();
	const Ref<Texture2D> &texture = textures.current;
	RS::get_singleton()->texture_proxy_update(proxy, texture.is_valid() ? texture->get_rid() : proxy_placeholder);
	emit_changed();
}

void LocalizedTexture2D::_locale_changed() {
	if (_update_texture()) {
		emit_changed();
	}
}

void LocalizedTexture2D::set_default_texture(const Ref<Texture2D> &p_texture) {
	ERR_FAIL_COND_MSG(textures.makes_cycle(p_texture), "A LocalizedTexture2D can't contain itself.");
	textures.default_variant = p_texture;
	_update_texture();
	emit_changed();
}

Ref<Texture2D> LocalizedTexture2D::get_default_texture() const {
	return textures.default_variant;
}

void LocalizedTexture2D::set_textures(const Dictionary &p_textures) {
	textures.set_dictionary(p_textures);
	_update_texture();
	emit_changed();
}

Dictionary LocalizedTexture2D::get_textures() const {
	return textures.get_dictionary();
}

Ref<Texture2D> LocalizedTexture2D::get_current_texture() const {
	return textures.current;
}

Image::Format LocalizedTexture2D::get_format() const {
	return textures.current.is_valid() ? textures.current->get_format() : Image::FORMAT_RGBA8;
}

int LocalizedTexture2D::get_mipmap_count() const {
	return textures.current.is_valid() ? textures.current->get_mipmap_count() : 0;
}

int LocalizedTexture2D::get_width() const {
	return textures.current.is_valid() ? textures.current->get_width() : 1;
}

int LocalizedTexture2D::get_height() const {
	return textures.current.is_valid() ? textures.current->get_height() : 1;
}

// A proxy of the current texture, so materials show the texture of the new locale.
RID LocalizedTexture2D::get_rid() const {
	return proxy;
}

bool LocalizedTexture2D::is_pixel_opaque(int p_x, int p_y) const {
	return textures.current.is_valid() ? textures.current->is_pixel_opaque(p_x, p_y) : true;
}

bool LocalizedTexture2D::has_alpha() const {
	return textures.current.is_valid() ? textures.current->has_alpha() : false;
}

bool LocalizedTexture2D::has_mipmaps() const {
	return textures.current.is_valid() ? textures.current->has_mipmaps() : false;
}

// Drawn by the current texture, for the textures drawing a region like AtlasTexture.
void LocalizedTexture2D::draw(RID p_canvas_item, const Point2 &p_pos, const Color &p_modulate, bool p_transpose) const {
	if (textures.current.is_valid()) {
		textures.current->draw(p_canvas_item, p_pos, p_modulate, p_transpose);
	}
}

void LocalizedTexture2D::draw_rect(RID p_canvas_item, const Rect2 &p_rect, bool p_tile, const Color &p_modulate, bool p_transpose) const {
	if (textures.current.is_valid()) {
		textures.current->draw_rect(p_canvas_item, p_rect, p_tile, p_modulate, p_transpose);
	}
}

void LocalizedTexture2D::draw_rect_region(RID p_canvas_item, const Rect2 &p_rect, const Rect2 &p_src_rect, const Color &p_modulate, bool p_transpose, bool p_clip_uv) const {
	if (textures.current.is_valid()) {
		textures.current->draw_rect_region(p_canvas_item, p_rect, p_src_rect, p_modulate, p_transpose, p_clip_uv);
	}
}

bool LocalizedTexture2D::get_rect_region(const Rect2 &p_rect, const Rect2 &p_src_rect, Rect2 &r_rect, Rect2 &r_src_rect) const {
	if (textures.current.is_valid()) {
		return textures.current->get_rect_region(p_rect, p_src_rect, r_rect, r_src_rect);
	}
	return Texture2D::get_rect_region(p_rect, p_src_rect, r_rect, r_src_rect);
}

Ref<Image> LocalizedTexture2D::get_image() const {
	return textures.current.is_valid() ? textures.current->get_image() : Ref<Image>();
}

void LocalizedTexture2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_default_texture", "texture"), &LocalizedTexture2D::set_default_texture);
	ClassDB::bind_method(D_METHOD("get_default_texture"), &LocalizedTexture2D::get_default_texture);
	ClassDB::bind_method(D_METHOD("set_textures", "textures"), &LocalizedTexture2D::set_textures);
	ClassDB::bind_method(D_METHOD("get_textures"), &LocalizedTexture2D::get_textures);
	ClassDB::bind_method(D_METHOD("get_current_texture"), &LocalizedTexture2D::get_current_texture);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "default_texture", PROPERTY_HINT_RESOURCE_TYPE, Texture2D::get_class_static()), "set_default_texture", "get_default_texture");
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "textures", PROPERTY_HINT_DICTIONARY_TYPE, vformat("%d/%d:;%s", Variant::STRING, PROPERTY_HINT_LOCALE_ID, Texture2D::get_class_static())), "set_textures", "get_textures");
}

LocalizedTexture2D::LocalizedTexture2D() {
	proxy_placeholder = RS::get_singleton()->texture_2d_placeholder_create();
	proxy = RS::get_singleton()->texture_proxy_create(proxy_placeholder);
	textures.setup(this, callable_mp(this, &LocalizedTexture2D::_locale_changed), callable_mp(this, &LocalizedTexture2D::_texture_changed));
}

LocalizedTexture2D::~LocalizedTexture2D() {
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(proxy);
	RS::get_singleton()->free_rid(proxy_placeholder);
}
