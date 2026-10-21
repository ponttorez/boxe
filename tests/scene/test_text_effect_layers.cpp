/**************************************************************************/
/*  test_text_effect_layers.cpp                                           */
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

TEST_FORCE_LINK(test_text_effect_layers)

#include "scene/2d/canvas_group.h"
#include "scene/gui/control.h"
#include "scene/gui/text_effect_layers.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/canvas_item_material.h"

namespace TestTextEffectLayers {

TEST_CASE("[SceneTree][TextEffectLayers] Layers need canvas groups that aren't nested") {
	Window *root = SceneTree::get_singleton()->get_root();
	Control *parent = memnew(Control);
	root->add_child(parent);
	Control *control = memnew(Control);
	parent->add_child(control);
	CHECK(TextEffectLayers::is_supported(control));

	parent->set_composite_enabled(true);
	CHECK_FALSE_MESSAGE(TextEffectLayers::is_supported(control), "Not inside a composite.");
	parent->set_composite_enabled(false);

	parent->set_clip_children_mode(CanvasItem::CLIP_CHILDREN_ONLY);
	CHECK_FALSE_MESSAGE(TextEffectLayers::is_supported(control), "Not inside a node that clips its children.");
	parent->set_clip_children_mode(CanvasItem::CLIP_CHILDREN_DISABLED);

	CanvasGroup *group = memnew(CanvasGroup);
	root->add_child(group);
	Control *grouped = memnew(Control);
	group->add_child(grouped);
	CHECK_FALSE_MESSAGE(TextEffectLayers::is_supported(grouped), "Not inside a CanvasGroup.");

	memdelete(group);
	memdelete(parent);
}

TEST_CASE("[SceneTree][TextEffectLayers] Material of the effects") {
	Control *parent = memnew(Control);
	SceneTree::get_singleton()->get_root()->add_child(parent);
	Control *control = memnew(Control);
	parent->add_child(control);
	CHECK(TextEffectLayers::get_material_of(control) == RID());

	Ref<CanvasItemMaterial> material;
	material.instantiate();
	parent->set_material(material);
	CHECK(TextEffectLayers::get_material_of(control) == RID());
	control->set_use_parent_material(true);
	CHECK_MESSAGE(TextEffectLayers::get_material_of(control) == material->get_rid(), "The material of the parent applies with use_parent_material.");

	memdelete(parent);
}

TEST_CASE("[SceneTree][TextEffectLayers] Layers are kept while their count doesn't change") {
	Control *control = memnew(Control);
	SceneTree::get_singleton()->get_root()->add_child(control);
	TextEffectLayers layers;

	layers.begin(control, control->get_canvas_item(), 2, Color(1, 1, 1), 4);
	CHECK(layers.get_count() == 2);
	const RID text_item = layers.get_text_item();
	CHECK(text_item.is_valid());
	CHECK(layers.get_content(1, 0.5).is_valid());
	CHECK(layers.get_eraser(0).is_valid());

	layers.begin(control, control->get_canvas_item(), 2, Color(1, 1, 1), 4);
	CHECK(layers.get_text_item() == text_item);

	layers.begin(control, control->get_canvas_item(), 3, Color(1, 1, 1), 4);
	CHECK(layers.get_count() == 3);

	layers.clear();
	CHECK(layers.get_count() == 0);
	CHECK_FALSE(layers.get_text_item().is_valid());

	memdelete(control);
}

} // namespace TestTextEffectLayers
