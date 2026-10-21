/**************************************************************************/
/*  text_effect_layers.cpp                                                */
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

#include "text_effect_layers.h"

#include "scene/2d/canvas_group.h"
#include "scene/gui/control.h"
#include "scene/resources/material.h"
#include "servers/rendering/rendering_server.h"

RID TextEffectLayers::eraser_shader;
RID TextEffectLayers::eraser_material;

// Layers are drawn before the children nodes of the control, as the text is.
static const int LAYER_DRAW_INDEX = -1000;

bool TextEffectLayers::is_supported(const CanvasItem *p_item) {
	for (const CanvasItem *item = p_item; item; item = item->get_parent_item()) {
		if (Object::cast_to<CanvasGroup>(item) || item->get_clip_children_mode() != CanvasItem::CLIP_CHILDREN_DISABLED) {
			return false;
		}
		const Control *control = Object::cast_to<Control>(item);
		if (control && control->is_composite_enabled()) {
			return false;
		}
	}
	return true;
}

RID TextEffectLayers::get_material_of(const CanvasItem *p_item) {
	for (const CanvasItem *item = p_item; item; item = item->get_parent_item()) {
		if (!item->get_use_parent_material()) {
			return item->get_material().is_valid() ? item->get_material()->get_rid() : RID();
		}
	}
	return RID();
}

void TextEffectLayers::finish() {
	RenderingServer *rs = RenderingServer::get_singleton();
	if (eraser_material.is_valid()) {
		rs->free_rid(eraser_material);
		eraser_material = RID();
	}
	if (eraser_shader.is_valid()) {
		rs->free_rid(eraser_shader);
		eraser_shader = RID();
	}
}

LocalVector<RID> TextEffectLayers::_get_items() const {
	LocalVector<RID> items;
	for (const Layer &layer : layers) {
		items.push_back(layer.group);
		items.push_back(layer.content);
		items.push_back(layer.eraser);
	}
	if (text_item.is_valid()) {
		items.push_back(text_item);
	}
	return items;
}

void TextEffectLayers::_free_items() {
	RenderingServer *rs = RenderingServer::get_singleton();
	for (const Layer &layer : layers) {
		rs->free_rid(layer.content);
		rs->free_rid(layer.eraser);
		rs->free_rid(layer.group);
	}
	layers.clear();
	if (text_item.is_valid()) {
		rs->free_rid(text_item);
		text_item = RID();
	}
	used = 0;
}

void TextEffectLayers::begin(const CanvasItem *p_owner, RID p_parent, uint32_t p_count, const Color &p_self_modulate, float p_margin) {
	ERR_FAIL_NULL(p_owner);
	RenderingServer *rs = RenderingServer::get_singleton();

	if (eraser_material.is_null()) {
		// Multiplies the layer by the inverse coverage of the drawn glyph.
		eraser_shader = rs->shader_create();
		rs->shader_set_code(eraser_shader, R"(
shader_type canvas_item;
render_mode unshaded, blend_mul;

void fragment() {
	COLOR = vec4(1.0 - COLOR.a);
}
)");
		eraser_material = rs->material_create();
		rs->material_set_shader(eraser_material, eraser_shader);
	}

	// Recreate the items when the layer count changes, so they keep their drawing order.
	if (parent != p_parent || layers.size() != p_count || text_item.is_null()) {
		_free_items();
		parent = p_parent;
		for (uint32_t i = 0; i < p_count; i++) {
			Layer layer;
			layer.group = rs->canvas_item_create();
			rs->canvas_item_set_parent(layer.group, parent);
			rs->canvas_item_set_draw_index(layer.group, LAYER_DRAW_INDEX + i);

			layer.content = rs->canvas_item_create();
			rs->canvas_item_set_parent(layer.content, layer.group);
			rs->canvas_item_set_draw_index(layer.content, 0);

			layer.eraser = rs->canvas_item_create();
			rs->canvas_item_set_parent(layer.eraser, layer.group);
			rs->canvas_item_set_draw_index(layer.eraser, 1);
			rs->canvas_item_set_material(layer.eraser, eraser_material);

			layers.push_back(layer);
		}
		text_item = rs->canvas_item_create();
		rs->canvas_item_set_parent(text_item, parent);
		// The text follows the material of the parent, like when it's drawn on the parent itself.
		rs->canvas_item_set_use_parent_material(text_item, true);
		rs->canvas_item_set_draw_index(text_item, LAYER_DRAW_INDEX + p_count);
	}
	used = p_count;

	for (const Layer &layer : layers) {
		rs->canvas_item_clear(layer.content);
		rs->canvas_item_clear(layer.eraser);
		rs->canvas_item_set_canvas_group_mode(layer.group, RSE::CANVAS_GROUP_MODE_TRANSPARENT, p_margin, true, p_margin, false);
	}
	rs->canvas_item_clear(text_item);
	// The effects are below canvas groups, which would apply a parent material to the whole group, so they get the material itself.
	const RID material = get_material_of(p_owner);
	for (const Layer &layer : layers) {
		rs->canvas_item_set_material(layer.content, material);
	}

	// Nodes resolve the settings they inherit themselves, items made on the server would use the defaults of the viewport.
	const RSE::CanvasItemTextureFilter texture_filter = RSE::CanvasItemTextureFilter(p_owner->get_texture_filter_in_tree());
	const RSE::CanvasItemTextureRepeat texture_repeat = RSE::CanvasItemTextureRepeat(p_owner->get_texture_repeat_in_tree());
	const int light_mask = p_owner->get_light_mask();
	const uint32_t visibility_layer = p_owner->get_visibility_layer();
	for (const RID &item : _get_items()) {
		rs->canvas_item_set_default_texture_filter(item, texture_filter);
		rs->canvas_item_set_default_texture_repeat(item, texture_repeat);
		rs->canvas_item_set_light_mask(item, light_mask);
		rs->canvas_item_set_visibility_layer(item, visibility_layer);
	}
	// Children don't get the self modulation of the parent, so it's applied to every item here.
	rs->canvas_item_set_self_modulate(text_item, p_self_modulate);
	for (const Layer &layer : layers) {
		rs->canvas_item_set_self_modulate(layer.group, p_self_modulate);
	}
}

RID TextEffectLayers::get_content(uint32_t p_layer, float p_alpha) {
	ERR_FAIL_UNSIGNED_INDEX_V(p_layer, used, RID());
	const Layer &layer = layers[p_layer];
	// The group is drawn once with this alpha, its children are drawn into it unmodulated.
	RenderingServer::get_singleton()->canvas_item_set_modulate(layer.group, Color(1, 1, 1, p_alpha));
	return layer.content;
}

RID TextEffectLayers::get_eraser(uint32_t p_layer) const {
	ERR_FAIL_UNSIGNED_INDEX_V(p_layer, used, RID());
	return layers[p_layer].eraser;
}

void TextEffectLayers::clear() {
	_free_items();
	parent = RID();
}

TextEffectLayers::~TextEffectLayers() {
	_free_items();
}
