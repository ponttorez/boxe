/**************************************************************************/
/*  text_effect_layers.h                                                  */
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

#include "core/math/color.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"

class CanvasItem;

// Draws text shadows and outlines as separate layers, each one blended as a whole, so the
// overlapping glyphs of a translucent effect don't add up. The shapes of the upper layers
// and of the text are cut out of the lower ones, so nothing shows through the text.
class TextEffectLayers {
	struct Layer {
		RID group;
		RID content;
		RID eraser;
	};

	LocalVector<Layer> layers;
	RID text_item;
	RID parent;
	uint32_t used = 0;

	static RID eraser_shader;
	static RID eraser_material;

	LocalVector<RID> _get_items() const;
	void _free_items();

public:
	// Layers need canvas groups, which can't be nested in other canvas groups.
	static bool is_supported(const CanvasItem *p_item);
	// The material that applies to p_item, which may come from a parent with use_parent_material.
	static RID get_material_of(const CanvasItem *p_item);
	static void finish();

	// Starts drawing under p_parent, an item of p_owner, with p_count layers, plus the text above them.
	// The items get the material and the drawing settings of p_owner, as the text has without layers.
	void begin(const CanvasItem *p_owner, RID p_parent, uint32_t p_count, const Color &p_self_modulate, float p_margin);
	// Content of a layer, drawn with opaque effect colors and faded as a whole by p_alpha.
	RID get_content(uint32_t p_layer, float p_alpha);
	// Item of a layer where shapes drawn in white are cut out of the layer.
	RID get_eraser(uint32_t p_layer) const;
	RID get_text_item() const { return text_item; }
	uint32_t get_count() const { return used; }
	void clear();

	~TextEffectLayers();
};
