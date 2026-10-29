/**************************************************************************/
/*  test_property_animation.cpp                                           */
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

TEST_FORCE_LINK(test_property_animation)

#include "core/config/engine.h"
#include "scene/2d/node_2d.h"
#include "scene/animation/property_animation.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/signal_watcher.h"

namespace TestPropertyAnimation {

static Node2D *make_node() {
	Node2D *node = memnew(Node2D);
	SceneTree::get_singleton()->get_root()->add_child(node);
	return node;
}

TEST_CASE("[SceneTree][PropertyAnimation] Animating a property") {
	SceneTree *tree = SceneTree::get_singleton();
	Node2D *node = make_node();

	const Ref<PropertyAnimation> animation = tree->animate_property(node, NodePath("position"), Vector2(10, 0), 1.0, Tween::TRANS_LINEAR);
	REQUIRE(animation.is_valid());
	CHECK(animation->is_running());
	CHECK(animation->get_property() == NodePath(":position"));
	CHECK(tree->is_property_animating(node, NodePath("position")));
	CHECK(tree->is_property_animating(node));
	CHECK_FALSE(tree->is_property_animating(node, NodePath("rotation")));

	SIGNAL_WATCH(animation.ptr(), SNAME("finished"));
	tree->process(0.5);
	CHECK(node->get_position().is_equal_approx(Vector2(5, 0)));
	SIGNAL_CHECK_FALSE(SNAME("finished"));
	tree->process(0.6);
	CHECK(node->get_position().is_equal_approx(Vector2(10, 0)));
	SIGNAL_CHECK(SNAME("finished"), { {} });
	CHECK_FALSE(animation->is_running());
	CHECK_FALSE(animation->is_interrupted());
	CHECK_FALSE(tree->is_property_animating(node));
	SIGNAL_UNWATCH(animation.ptr(), SNAME("finished"));

	// Property paths.
	tree->animate_property(node, NodePath("position:y"), 4.0, 1.0, Tween::TRANS_LINEAR);
	tree->process(0.25);
	CHECK(node->get_position().is_equal_approx(Vector2(10, 1)));

	memdelete(node);
}

TEST_CASE("[SceneTree][PropertyAnimation] Animating again continues from where the property is") {
	SceneTree *tree = SceneTree::get_singleton();
	Node2D *node = make_node();

	const Ref<PropertyAnimation> forward = tree->animate_property(node, NodePath("position"), Vector2(10, 0), 1.0, Tween::TRANS_LINEAR);
	tree->process(0.5);
	CHECK(node->get_position().is_equal_approx(Vector2(5, 0)));

	// The same value and settings keep the animation going, e.g. when called every frame.
	CHECK(tree->animate_property(node, NodePath("position"), Vector2(10, 0), 1.0, Tween::TRANS_LINEAR) == forward);

	// Going back takes as long as the part travelled.
	const Ref<PropertyAnimation> back = tree->animate_property(node, NodePath("position"), Vector2(0, 0), 1.0, Tween::TRANS_LINEAR);
	CHECK(back != forward);
	CHECK_FALSE(forward->is_running());
	CHECK(forward->is_interrupted());
	CHECK(node->get_position().is_equal_approx(Vector2(5, 0)));
	tree->process(0.25);
	CHECK(node->get_position().is_equal_approx(Vector2(2.5, 0)));
	tree->process(0.25);
	CHECK(node->get_position().is_equal_approx(Vector2(0, 0)));
	CHECK_FALSE(back->is_running());
	CHECK_FALSE(back->is_interrupted());

	// Another target starts from where the property is. The way from the previous target, (10, 0), to (5, 10) is 0.2
	// covered at (5, 0), so the animation takes 0.8 seconds.
	tree->animate_property(node, NodePath("position"), Vector2(10, 0), 1.0, Tween::TRANS_LINEAR);
	tree->process(0.5);
	const Ref<PropertyAnimation> aside = tree->animate_property(node, NodePath("position"), Vector2(5, 10), 1.0, Tween::TRANS_LINEAR);
	CHECK(node->get_position().is_equal_approx(Vector2(5, 0)));
	tree->process(0.5);
	CHECK(node->get_position().is_equal_approx(Vector2(5, 6.25)));
	tree->process(0.3);
	CHECK(node->get_position().is_equal_approx(Vector2(5, 10)));
	CHECK_FALSE(aside->is_running());

	memdelete(node);
}

TEST_CASE("[SceneTree][PropertyAnimation] Stopping and finishing") {
	SceneTree *tree = SceneTree::get_singleton();
	Node2D *node = make_node();

	Ref<PropertyAnimation> animation = tree->animate_property(node, NodePath("position"), Vector2(10, 0), 1.0, Tween::TRANS_LINEAR);
	tree->animate_property(node, NodePath("rotation"), 1.0, 1.0, Tween::TRANS_LINEAR);
	tree->process(0.5);
	tree->stop_property_animation(node, NodePath("position"));
	CHECK(animation->is_interrupted());
	CHECK(tree->is_property_animating(node, NodePath("rotation")));
	tree->process(0.25);
	CHECK(node->get_position().is_equal_approx(Vector2(5, 0)));
	CHECK(node->get_rotation() == doctest::Approx(0.75));

	animation = tree->animate_property(node, NodePath("position"), Vector2(0, 8), 1.0);
	tree->finish_property_animation(node);
	CHECK(node->get_position().is_equal_approx(Vector2(0, 8)));
	CHECK(node->get_rotation() == doctest::Approx(1.0));
	CHECK_FALSE(animation->is_interrupted());
	CHECK_FALSE(tree->is_property_animating(node));

	memdelete(node);
}

TEST_CASE("[SceneTree][PropertyAnimation] Types of values") {
	SceneTree *tree = SceneTree::get_singleton();
	Node2D *node = make_node();

	tree->animate_property(node, NodePath("modulate"), Color(0, 0, 0, 0), 1.0, Tween::TRANS_LINEAR);
	tree->animate_property(node, NodePath("z_index"), 10, 1.0, Tween::TRANS_LINEAR);
	tree->process(0.25);
	CHECK(node->get_modulate().is_equal_approx(Color(0.75, 0.75, 0.75, 0.75)));
	CHECK_MESSAGE(node->get_z_index() == 3, "Integers are rounded.");

	// Values that can't be animated, and no duration, set the property at once.
	const Ref<PropertyAnimation> name = tree->animate_property(node, NodePath("name"), "Animated", 1.0);
	CHECK(node->get_name() == StringName("Animated"));
	CHECK_FALSE(name->is_running());
	tree->animate_property(node, NodePath("position"), Vector2(3, 3), 0.0);
	CHECK(node->get_position().is_equal_approx(Vector2(3, 3)));
	CHECK_FALSE(tree->is_property_animating(node, NodePath("position")));

	tree->finish_property_animation(node);
	memdelete(node);
}

TEST_CASE("[SceneTree][PropertyAnimation] Freed objects and the time scale") {
	SceneTree *tree = SceneTree::get_singleton();
	Node2D *node = make_node();
	const Ref<PropertyAnimation> animation = tree->animate_property(node, NodePath("position"), Vector2(10, 0), 1.0);
	memdelete(node);
	tree->process(0.1);
	CHECK_FALSE(animation->is_running());
	CHECK(animation->is_interrupted());
	CHECK(animation->get_object() == nullptr);

	node = make_node();
	const double time_scale = Engine::get_singleton()->get_time_scale();
	Engine::get_singleton()->set_time_scale(0.5);
	tree->animate_property(node, NodePath("position"), Vector2(10, 0), 1.0, Tween::TRANS_LINEAR);
	tree->animate_property(node, NodePath("rotation"), 1.0, 1.0, Tween::TRANS_LINEAR)->set_ignore_time_scale();
	// The delta of the frame is scaled.
	tree->process(0.25);
	CHECK(node->get_position().is_equal_approx(Vector2(2.5, 0)));
	CHECK(node->get_rotation() == doctest::Approx(0.5));
	Engine::get_singleton()->set_time_scale(time_scale);

	tree->finish_property_animation(node);
	memdelete(node);
}

} // namespace TestPropertyAnimation
