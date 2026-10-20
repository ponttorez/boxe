/**************************************************************************/
/*  test_control.cpp                                                      */
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

TEST_FORCE_LINK(test_control)

#include "core/input/input.h"
#include "core/input/input_map.h" // IWYU pragma: keep // Used by `SEND_GUI_ACTION` macro.
#include "scene/2d/canvas_group.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/slider.h"
#include "scene/gui/virtual_joystick.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/display_server_mock.h"
#include "tests/signal_watcher.h"

namespace TestControl {

TEST_CASE("[SceneTree][Control] Transforms") {
	SUBCASE("[Control][Global Transform] Global Transform should be accessible while not in SceneTree.") { // GH-79453
		Control *test_node = memnew(Control);
		Control *test_child = memnew(Control);
		test_node->add_child(test_child);

		test_node->set_global_position(Point2(1, 1));
		CHECK_EQ(test_node->get_global_position(), Point2(1, 1));
		CHECK_EQ(test_child->get_global_position(), Point2(1, 1));
		test_node->set_global_position(Point2(2, 2));
		CHECK_EQ(test_node->get_global_position(), Point2(2, 2));
		test_node->set_scale(Vector2(4, 4));
		CHECK_EQ(test_node->get_global_transform(), Transform2D(0, Size2(4, 4), 0, Vector2(2, 2)));
		test_node->set_scale(Vector2(1, 1));
		test_node->set_rotation_degrees(90);
		CHECK_EQ(test_node->get_global_transform(), Transform2D(Math::PI / 2, Vector2(2, 2)));
		test_node->set_pivot_offset(Vector2(1, 0));
		CHECK_EQ(test_node->get_global_transform(), Transform2D(Math::PI / 2, Vector2(3, 1)));

		memdelete(test_child);
		memdelete(test_node);
	}
}

TEST_CASE("[SceneTree][Control] Focus") {
	Control *ctrl = memnew(Control);
	SceneTree::get_singleton()->get_root()->add_child(ctrl);

	SUBCASE("[SceneTree][Control] Default focus") {
		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Can't grab focus with default focus mode") {
		ERR_PRINT_OFF
		ctrl->grab_focus();
		ERR_PRINT_ON

		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Can grab focus") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();

		CHECK_UNARY(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Can release focus") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());

		ctrl->release_focus();
		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Only one can grab focus at the same time") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());

		Control *other_ctrl = memnew(Control);
		SceneTree::get_singleton()->get_root()->add_child(other_ctrl);
		other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		other_ctrl->grab_focus();

		CHECK_UNARY(other_ctrl->has_focus());
		CHECK_UNARY_FALSE(ctrl->has_focus());

		memdelete(other_ctrl);
	}

	SUBCASE("[SceneTree][Control] Hide control will cause the focus to be released") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());

		ctrl->hide();
		CHECK_UNARY_FALSE(ctrl->has_focus());

		ctrl->show();
		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] The parent node is hidden causing the focus to be released") {
		Control *child_ctrl = memnew(Control);
		ctrl->add_child(child_ctrl);

		child_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		child_ctrl->grab_focus();
		CHECK_UNARY(child_ctrl->has_focus());

		ctrl->hide();
		CHECK_UNARY_FALSE(child_ctrl->has_focus());

		ctrl->show();
		CHECK_UNARY_FALSE(child_ctrl->has_focus());

		memdelete(child_ctrl);
	}

	SUBCASE("[SceneTree][Control] Grab focus with focus behavior recursive") {
		CHECK_UNARY_FALSE(ctrl->has_focus());

		// Cannot grab focus if focus behavior is disabled.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_DISABLED);

		ERR_PRINT_OFF
		ctrl->grab_focus();
		ERR_PRINT_ON
		CHECK_UNARY_FALSE(ctrl->has_focus());

		// Cannot grab focus if focus behavior is enabled but focus mode is none.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_NONE);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_ENABLED);

		ERR_PRINT_OFF
		ctrl->grab_focus();
		ERR_PRINT_ON
		CHECK_UNARY_FALSE(ctrl->has_focus());

		// Can grab focus if focus behavior is enabled and focus mode is all.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_ENABLED);

		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Children focus with focus behavior recursive") {
		Control *child_control = memnew(Control);
		ctrl->add_child(child_control);

		// Can grab focus on child if parent focus behavior is inherit.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_INHERITED);
		child_control->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		child_control->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_INHERITED);

		child_control->grab_focus();
		CHECK_UNARY(child_control->has_focus());

		// Cannot grab focus on child if parent focus behavior is disabled.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_DISABLED);
		child_control->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		child_control->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_INHERITED);

		ERR_PRINT_OFF
		child_control->grab_focus();
		ERR_PRINT_ON
		CHECK_UNARY_FALSE(child_control->has_focus());

		memdelete(child_control);
	}

	memdelete(ctrl);
}

TEST_CASE("[SceneTree][Control] Find next/prev valid focus") {
	Node *intermediate = memnew(Node);
	Control *ctrl = memnew(Control);
	intermediate->add_child(ctrl);
	SceneTree::get_singleton()->get_root()->add_child(intermediate);

	SUBCASE("[SceneTree][Control] In FOCUS_CLICK mode") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
		ctrl->grab_focus();
		REQUIRE_UNARY(ctrl->has_focus());

		SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
			SEND_GUI_ACTION("ui_focus_next");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
			SEND_GUI_ACTION("ui_focus_prev");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Has a sibling control and the parent is a window") {
			Control *ctrl1 = memnew(Control);
			Control *ctrl2 = memnew(Control);
			Control *ctrl3 = memnew(Control);
			Window *win = SceneTree::get_singleton()->get_root();

			ctrl1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			ctrl2->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			ctrl3->set_focus_mode(Control::FocusMode::FOCUS_ALL);

			ctrl2->add_child(ctrl3);
			win->add_child(ctrl1);
			win->add_child(ctrl2);

			SUBCASE("[SceneTree][Control] Focus Next") {
				ctrl1->grab_focus();
				CHECK_UNARY(ctrl1->has_focus());

				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl2->has_focus());

				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl3->has_focus());

				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Focus Prev") {
				ctrl1->grab_focus();
				CHECK_UNARY(ctrl1->has_focus());

				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl3->has_focus());

				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl2->has_focus());

				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl1->has_focus());
			}

			memdelete(ctrl3);
			memdelete(ctrl1);
			memdelete(ctrl2);
		}

		SUBCASE("[SceneTree][Control] Has a sibling control but the parent node is not a control or window") {
			Control *other_ctrl = memnew(Control);
			intermediate->add_child(other_ctrl);

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_ALL") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_CLICK") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_NONE") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_NONE);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}
			}

			memdelete(other_ctrl);
		}
	}

	SUBCASE("[SceneTree][Control] In FOCUS_ALL mode") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		REQUIRE_EQ(ctrl->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

		ctrl->grab_focus();
		REQUIRE_UNARY(ctrl->has_focus());

		SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
			SEND_GUI_ACTION("ui_focus_next");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
			SEND_GUI_ACTION("ui_focus_prev");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Has a sibling control but the parent node is not a control") {
			Control *other_ctrl = memnew(Control);
			SceneTree::get_singleton()->get_root()->add_child(other_ctrl);

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_ALL") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_CLICK") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_NONE") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_NONE);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}
			}

			memdelete(other_ctrl);
		}

		SUBCASE("[SceneTree][Control] Simple control tree") {
			Control *ctrl_0 = memnew(Control);
			Control *ctrl_1 = memnew(Control);
			Node2D *node_2d_2 = memnew(Node2D);

			ctrl->add_child(ctrl_0);
			ctrl->add_child(ctrl_1);
			ctrl->add_child(node_2d_2);

			ctrl_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			ctrl_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			REQUIRE_EQ(ctrl_0->get_focus_mode(), Control::FocusMode::FOCUS_ALL);
			REQUIRE_EQ(ctrl_1->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

			SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl_0->has_focus());

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl_1->has_focus());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl_1->has_focus());

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl_0->has_focus());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Skip next hidden control") {
				ctrl_0->hide();
				REQUIRE_UNARY_FALSE(ctrl_0->is_visible());
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip next control with FOCUS_NONE") {
				ctrl_0->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(ctrl_0->get_focus_mode(), Control::FocusMode::FOCUS_NONE);
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip next control with FOCUS_CLICK") {
				ctrl_0->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(ctrl_0->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip next top level control") {
				ctrl_0->set_as_top_level(true);
				REQUIRE_UNARY(ctrl_0->is_set_as_top_level());
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev hidden control") {
				ctrl_1->hide();
				REQUIRE_UNARY_FALSE(ctrl_1->is_visible());
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev control with FOCUS_NONE") {
				ctrl_1->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(ctrl_1->get_focus_mode(), Control::FocusMode::FOCUS_NONE);
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev control with FOCUS_CLICK") {
				ctrl_1->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(ctrl_1->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev top level control") {
				ctrl_1->set_as_top_level(true);
				REQUIRE_UNARY(ctrl_1->is_set_as_top_level());
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Add more node controls") {
				Control *ctrl_0_0 = memnew(Control);
				Control *ctrl_0_1 = memnew(Control);
				Control *ctrl_0_2 = memnew(Control);
				ctrl_0->add_child(ctrl_0_0);
				ctrl_0->add_child(ctrl_0_1);
				ctrl_0->add_child(ctrl_0_2);
				ctrl_0_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_0_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_0_2->set_focus_mode(Control::FocusMode::FOCUS_ALL);

				Control *ctrl_1_0 = memnew(Control);
				Control *ctrl_1_1 = memnew(Control);
				Control *ctrl_1_2 = memnew(Control);
				ctrl_1->add_child(ctrl_1_0);
				ctrl_1->add_child(ctrl_1_1);
				ctrl_1->add_child(ctrl_1_2);
				ctrl_1_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_1_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_1_2->set_focus_mode(Control::FocusMode::FOCUS_ALL);

				Control *ctrl_2_0 = memnew(Control);
				Control *ctrl_2_1 = memnew(Control);
				Control *ctrl_2_2 = memnew(Control);
				node_2d_2->add_child(ctrl_2_0);
				node_2d_2->add_child(ctrl_2_1);
				node_2d_2->add_child(ctrl_2_2);
				ctrl_2_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_2_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_2_2->set_focus_mode(Control::FocusMode::FOCUS_ALL);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl_0->has_focus());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl_0_0->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl_1_2->has_focus());
				}

				SUBCASE("[SceneTree][Control] Exist top level tree") {
					ctrl_0->set_as_top_level(true);
					REQUIRE_UNARY(ctrl_0->is_set_as_top_level());

					SUBCASE("[SceneTree][Control] Outside top level tree") {
						ctrl->grab_focus();
						REQUIRE_UNARY(ctrl->has_focus());
						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl_1->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl->has_focus());
							}
						}
					}

					SUBCASE("[SceneTree][Control] Inside top level tree") {
						ctrl_0->grab_focus();
						REQUIRE_UNARY(ctrl_0->has_focus());
						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl_0_0->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl_0->has_focus());
							}
						}
						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl_0_2->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY(ctrl_0->has_focus());
							}
						}
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next") {
						ctrl->set_focus_next(ctrl->get_path_to(ctrl_2_1));
						ctrl_2_1->set_focus_next(ctrl_2_1->get_path_to(ctrl_1_0));
						ctrl_1_0->set_focus_next(ctrl_1_0->get_path_to(ctrl_0));
						ctrl_0->set_focus_next(ctrl_0->get_path_to(ctrl));

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl_2_1->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY(ctrl_1_0->has_focus());

								SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
									SEND_GUI_ACTION("ui_focus_next");
									CHECK_UNARY(ctrl_0->has_focus());

									SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
										SEND_GUI_ACTION("ui_focus_next");
										CHECK_UNARY(ctrl->has_focus());
									}

									SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
										SEND_GUI_ACTION("ui_focus_prev");
										CHECK_UNARY(ctrl_0_2->has_focus());
									}
								}

								SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
									SEND_GUI_ACTION("ui_focus_prev");
									CHECK_UNARY(ctrl_1->has_focus());
								}
							}

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl_2_1->has_focus());
							}
						}

						SUBCASE("[SceneTree][Control] The parent node is not visible") {
							node_2d_2->hide();
							REQUIRE_UNARY(ctrl_2_1->is_visible());
							REQUIRE_UNARY_FALSE(ctrl_2_1->is_visible_in_tree());
							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY_FALSE(ctrl->has_focus());
								CHECK_UNARY_FALSE(ctrl_2_1->has_focus());
								CHECK_UNARY_FALSE(ctrl_0->has_focus());
								CHECK_UNARY(ctrl_1->has_focus());
							}
						}
					}

					SUBCASE("[SceneTree][Control] Manually specified focus prev") {
						ctrl->set_focus_previous(ctrl->get_path_to(ctrl_0_2));
						ctrl_0_2->set_focus_previous(ctrl_0_2->get_path_to(ctrl_1_1));
						ctrl_1_1->set_focus_previous(ctrl_1_1->get_path_to(ctrl_2_0));
						ctrl_2_0->set_focus_previous(ctrl_2_0->get_path_to(ctrl));

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl_0_2->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl_1_1->has_focus());

								SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
									SEND_GUI_ACTION("ui_focus_prev");
									CHECK_UNARY(ctrl_2_0->has_focus());

									SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
										SEND_GUI_ACTION("ui_focus_prev");
										CHECK_UNARY(ctrl->has_focus());
									}

									SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
										SEND_GUI_ACTION("ui_focus_next");
										CHECK_UNARY(ctrl_2_0->has_focus());
									}
								}

								SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
									SEND_GUI_ACTION("ui_focus_next");
									CHECK_UNARY(ctrl_1_2->has_focus());
								}
							}

							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY(ctrl_0->has_focus());
							}
						}

						SUBCASE("[SceneTree][Control] The parent node is not visible") {
							ctrl_0->hide();
							REQUIRE_UNARY(ctrl_0_2->is_visible());
							REQUIRE_UNARY_FALSE(ctrl_0_2->is_visible_in_tree());
							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY_FALSE(ctrl->has_focus());
								CHECK_UNARY_FALSE(ctrl_0_2->has_focus());
								CHECK_UNARY(ctrl_1_2->has_focus());
							}
						}
					}
				}

				SUBCASE("[SceneTree][Control] Exist hidden control tree") {
					ctrl_0->hide();
					REQUIRE_UNARY_FALSE(ctrl_0->is_visible());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl_1->has_focus());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
						}
					}
				}

				memdelete(ctrl_2_2);
				memdelete(ctrl_2_1);
				memdelete(ctrl_2_0);
				memdelete(ctrl_1_2);
				memdelete(ctrl_1_1);
				memdelete(ctrl_1_0);
				memdelete(ctrl_0_2);
				memdelete(ctrl_0_1);
				memdelete(ctrl_0_0);
			}

			memdelete(node_2d_2);
			memdelete(ctrl_1);
			memdelete(ctrl_0);
		}
	}

	memdelete(ctrl);
	memdelete(intermediate);
}

TEST_CASE("[SceneTree][Control] Anchoring") {
	Control *test_control = memnew(Control);
	Control *test_child = memnew(Control);
	test_control->add_child(test_child);
	test_control->set_size(Size2(2, 2));
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(test_control);

	SUBCASE("Anchoring without offsets") {
		test_child->set_anchor(SIDE_RIGHT, 0.75);
		test_child->set_anchor(SIDE_BOTTOM, 0.1);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(1.5, 0.2)),
				"With no LEFT or TOP anchors, positive RIGHT and BOTTOM anchors should be proportional to the size.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0, 0)),
				"With positive RIGHT and BOTTOM anchors set and no LEFT or TOP anchors, the position should not change.");

		test_child->set_anchor(SIDE_LEFT, 0.5);
		test_child->set_anchor(SIDE_TOP, 0.01);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.5, 0.18)),
				"With all anchors set, the size should fit between all four anchors.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(1, 0.02)),
				"With all anchors set, the LEFT and TOP anchors should proportional to the position.");
	}

	SUBCASE("Anchoring with offsets") {
		test_child->set_offset(SIDE_RIGHT, 0.33);
		test_child->set_offset(SIDE_BOTTOM, 0.2);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.33, 0.2)),
				"With no anchors or LEFT or TOP offsets set, the RIGHT and BOTTOM offsets should be equal to size.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0, 0)),
				"With only positive RIGHT and BOTTOM offsets set, the position should not change.");

		test_child->set_offset(SIDE_LEFT, 0.1);
		test_child->set_offset(SIDE_TOP, 0.05);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.23, 0.15)),
				"With no anchors set, the size should fit between all four offsets.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0.1, 0.05)),
				"With no anchors set, the LEFT and TOP offsets should be equal to the position.");

		test_child->set_anchor(SIDE_RIGHT, 0.5);
		test_child->set_anchor(SIDE_BOTTOM, 0.3);
		test_child->set_anchor(SIDE_LEFT, 0.2);
		test_child->set_anchor(SIDE_TOP, 0.1);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.83, 0.55)),
				"Anchors adjust size first then it is affected by offsets.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0.5, 0.25)),
				"Anchors adjust positions first then it is affected by offsets.");

		test_child->set_offset(SIDE_RIGHT, -0.1);
		test_child->set_offset(SIDE_BOTTOM, -0.01);
		test_child->set_offset(SIDE_LEFT, -0.33);
		test_child->set_offset(SIDE_TOP, -0.16);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.83, 0.55)),
				"Keeping offset distance equal when changing offsets, keeps size equal.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0.07, 0.04)),
				"Negative offsets move position in top left direction.");
	}

	SUBCASE("Anchoring is preserved on parent size changed") {
		test_child->set_offset(SIDE_RIGHT, -0.05);
		test_child->set_offset(SIDE_BOTTOM, 0.1);
		test_child->set_offset(SIDE_LEFT, 0.05);
		test_child->set_offset(SIDE_TOP, 0.1);
		test_child->set_anchor(SIDE_RIGHT, 0.3);
		test_child->set_anchor(SIDE_BOTTOM, 0.85);
		test_child->set_anchor(SIDE_LEFT, 0.2);
		test_child->set_anchor(SIDE_TOP, 0.55);
		CHECK(test_child->get_rect().is_equal_approx(
				Rect2(Vector2(0.45, 1.2), Size2(0.1, 0.6))));

		test_control->set_size(Size2(4, 1));
		CHECK(test_child->get_rect().is_equal_approx(
				Rect2(Vector2(0.85, 0.65), Size2(0.3, 0.3))));
	}

	memdelete(test_child);
	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Custom maximum size") {
	Control *test_control = memnew(Control);
	test_control->set_custom_maximum_size(Size2(4, 2));
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(test_control);
	test_control->set_size(Size2(4, 2));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size is allowed to increase to match custom maximum size.");

	test_control->set_size(Size2(3, 1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(3, 1)),
			"Size does not change if below custom maximum size.");

	test_control->set_size(Size2(5, 4));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size is limited to custom maximum size.");

	test_control->set_size(Size2(5, 1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 1)),
			"Adjust only x axis if x is above custom maximum size.");

	test_control->set_size(Size2(3, 3));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(3, 2)),
			"Adjust only y axis if y is above custom maximum size.");

	test_control->set_custom_minimum_size(Size2(5, 3));
	SceneTree::get_singleton()->process(0);
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Custom maximum size overrides custom minimum size.");

	Control *test_child = memnew(Control);
	test_control->add_child(test_child);

	CHECK_MESSAGE(
			test_child->get_combined_maximum_size().is_equal_approx(Vector2(-1, -1)),
			"Child combined maximum size does not factor in parent's custom maximum size if not propagating.");

	test_child->set_size(Size2(5, 3));
	CHECK_MESSAGE(
			test_child->get_size().is_equal_approx(Vector2(5, 3)),
			"Child size is not constrained by parent's custom maximum size if not propagating.");

	test_control->set_propagate_maximum_size(true);

	CHECK_MESSAGE(
			test_child->get_combined_maximum_size().is_equal_approx(Vector2(4, 2)),
			"Child combined maximum size factors in parent's custom maximum size if propagating.");

	test_child->set_size(Size2(5, 3));
	CHECK_MESSAGE(
			test_child->get_size().is_equal_approx(Vector2(4, 2)),
			"Child size is constrained by parent's custom maximum size if propagating.");

	test_child->set_size(Size2(3, 1));
	CHECK_MESSAGE(
			test_child->get_size().is_equal_approx(Vector2(3, 1)),
			"Child size can be smaller than parent's custom maximum size.");

	memdelete(test_child);
	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Custom minimum size") {
	Control *test_control = memnew(Control);
	test_control->set_custom_minimum_size(Size2(4, 2));
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(test_control);
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size increases to match custom minimum size.");

	test_control->set_size(Size2(5, 4));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(5, 4)),
			"Size does not change if above custom minimum size.");

	test_control->set_size(Size2(1, 1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size matches minimum size if set below custom minimum size.");

	test_control->set_size(Size2(3, 3));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 3)),
			"Adjusts only x axis size if x is below custom minimum size.");

	test_control->set_size(Size2(10, 0.1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(10, 2)),
			"Adjusts only y axis size if y is below custom minimum size.");

	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Grow direction") {
	Control *test_control = memnew(Control);
	test_control->set_size(Size2(1, 1));
	Window *root = SceneTree::get_singleton()->get_root();
	root->add_child(test_control);

	SUBCASE("Defaults") {
		CHECK(test_control->get_h_grow_direction() == Control::GROW_DIRECTION_END);
		CHECK(test_control->get_v_grow_direction() == Control::GROW_DIRECTION_END);
	}

	SIGNAL_WATCH(test_control, SNAME("minimum_size_changed"))
	Array signal_args = { {} };

	SUBCASE("Horizontal grow direction begin") {
		test_control->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
		test_control->set_custom_minimum_size(Size2(2, 2));
		SceneTree::get_singleton()->process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args)
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(-1, 0), Size2(2, 2))),
				"Expand leftwards.");
	}

	SUBCASE("Vertical grow direction begin") {
		test_control->set_v_grow_direction(Control::GROW_DIRECTION_BEGIN);
		test_control->set_custom_minimum_size(Size2(4, 3));
		SceneTree::get_singleton()->process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, -2), Size2(4, 3))),
				"Expand upwards.");
	}

	SUBCASE("Horizontal grow direction end") {
		test_control->set_h_grow_direction(Control::GROW_DIRECTION_END);
		test_control->set_custom_minimum_size(Size2(5, 3));
		SceneTree::get_singleton()->process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, 0), Size2(5, 3))),
				"Expand rightwards.");
	}

	SUBCASE("Vertical grow direction end") {
		test_control->set_v_grow_direction(Control::GROW_DIRECTION_END);
		test_control->set_custom_minimum_size(Size2(4, 4));
		SceneTree::get_singleton()->process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, 0), Size2(4, 4))),
				"Expand downwards.");
		;
	}

	SUBCASE("Horizontal grow direction both") {
		test_control->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
		test_control->set_custom_minimum_size(Size2(2, 4));
		SceneTree::get_singleton()->process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(-0.5, 0), Size2(2, 4))),
				"Expand equally leftwards and rightwards.");
	}

	SUBCASE("Vertical grow direction both") {
		test_control->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
		test_control->set_custom_minimum_size(Size2(6, 3));
		SceneTree::get_singleton()->process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, -1), Size2(6, 3))),
				"Expand equally upwards and downwards.");
	}

	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Composite") {
	Control *test_control = memnew(Control);
	SceneTree::get_singleton()->get_root()->add_child(test_control);

	SUBCASE("[Control] The composite is disabled by default") {
		CHECK_FALSE(test_control->is_composite_enabled());
		CHECK(test_control->get_composite_margin() == 0.0);
	}

	SUBCASE("[Control] The composite properties can be set") {
		test_control->set_composite_enabled(true);
		test_control->set_composite_margin(8.0);
		CHECK(test_control->is_composite_enabled());
		CHECK(test_control->get_composite_margin() == 8.0);
	}

	SUBCASE("[Control] The composite margin can't be negative") {
		test_control->set_composite_margin(8.0);
		ERR_PRINT_OFF;
		test_control->set_composite_margin(-1.0);
		ERR_PRINT_ON;
		CHECK(test_control->get_composite_margin() == 8.0);
	}

	SUBCASE("[Control] A composite inside another composite warns about it") {
		Control *nested_control = memnew(Control);
		test_control->add_child(nested_control);
		nested_control->set_composite_enabled(true);
		CHECK(nested_control->get_configuration_warnings().is_empty());

		test_control->set_composite_enabled(true);
		CHECK(nested_control->get_configuration_warnings().size() == 1);
		CHECK(test_control->get_configuration_warnings().is_empty());
	}

	SUBCASE("[Control] A composite inside a CanvasGroup warns about it") {
		CanvasGroup *canvas_group = memnew(CanvasGroup);
		SceneTree::get_singleton()->get_root()->add_child(canvas_group);
		Control *nested_control = memnew(Control);
		canvas_group->add_child(nested_control);
		nested_control->set_composite_enabled(true);
		CHECK(nested_control->get_configuration_warnings().size() == 1);
		memdelete(canvas_group);
	}

	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Raycast padding") {
	Window *root = SceneTree::get_singleton()->get_root();
	Control *test_control = memnew(Control);
	root->add_child(test_control);
	test_control->set_position(Point2(100, 100));
	test_control->set_size(Size2(100, 100));

	SUBCASE("[Control] The raycast padding is 0 by default") {
		for (int side = 0; side < 4; side++) {
			CHECK(test_control->get_raycast_padding(Side(side)) == 0.0);
		}
		CHECK(test_control->get_raycast_rect() == Rect2(0, 0, 100, 100));
	}

	SUBCASE("[Control] A positive padding shrinks the area that receives input") {
		test_control->set_raycast_padding(SIDE_LEFT, 20);
		CHECK(test_control->get_raycast_padding(SIDE_LEFT) == 20.0);
		CHECK_FALSE(test_control->has_point(Point2(10, 50)));
		CHECK(test_control->has_point(Point2(30, 50)));
		CHECK(test_control->get_size() == Size2(100, 100));
	}

	SUBCASE("[Control] A negative padding expands the area that receives input") {
		test_control->set_raycast_padding(SIDE_TOP, -20);
		CHECK(test_control->has_point(Point2(50, -10)));
		CHECK_FALSE(test_control->has_point(Point2(50, -30)));
		CHECK(root->gui_find_control(Point2(150, 90)) == test_control);
	}

	SUBCASE("[Control] An invalid side is rejected") {
		ERR_PRINT_OFF;
		test_control->set_raycast_padding(Side(4), 10);
		CHECK(test_control->get_raycast_padding(Side(4)) == 0.0);
		ERR_PRINT_ON;
	}

	SUBCASE("[Control] The padding of a control that clips its contents doesn't clip its children") {
		Control *child_control = memnew(Control);
		test_control->add_child(child_control);
		child_control->set_size(Size2(100, 100));
		test_control->set_clip_contents(true);
		test_control->set_raycast_padding(SIDE_LEFT, 20);
		CHECK(root->gui_find_control(Point2(110, 150)) == child_control);
	}

	SUBCASE("[Control] A control that clips its contents clips the expanded area of its children") {
		Control *child_control = memnew(Control);
		test_control->add_child(child_control);
		child_control->set_size(Size2(100, 100));
		child_control->set_raycast_padding(SIDE_LEFT, -20);
		CHECK(root->gui_find_control(Point2(90, 150)) == child_control);
		test_control->set_clip_contents(true);
		CHECK(root->gui_find_control(Point2(90, 150)) != child_control);
	}

	memdelete(test_control);
}

// A control that starts dragging from anywhere.
class DragSourceControl : public Control {
public:
	virtual Variant get_drag_data(const Point2 &p_point) override {
		return 1;
	}
};

TEST_CASE("[SceneTree][Control] Interactable") {
	Window *root = SceneTree::get_singleton()->get_root();

	SUBCASE("[Control] A control that isn't interactable loses the focus and can't grab it") {
		Control *test_control = memnew(Control);
		root->add_child(test_control);
		test_control->set_focus_mode(Control::FOCUS_ALL);
		test_control->grab_focus();
		CHECK(test_control->has_focus());

		test_control->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_DISABLED);
		CHECK_FALSE(test_control->has_focus());
		test_control->grab_focus();
		CHECK_FALSE(test_control->has_focus());

		memdelete(test_control);
	}

	SUBCASE("[Control] Input is consumed and doesn't reach the controls behind") {
		Control *back_control = memnew(Control);
		Control *front_control = memnew(Control);
		root->add_child(back_control);
		root->add_child(front_control);
		back_control->set_size(Size2(100, 100));
		front_control->set_size(Size2(100, 100));
		front_control->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_DISABLED);
		SIGNAL_WATCH(back_control, SNAME("gui_input"));
		SIGNAL_WATCH(front_control, SNAME("gui_input"));

		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(50, 50), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(50, 50), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		SIGNAL_CHECK_FALSE(SNAME("gui_input"));

		SIGNAL_UNWATCH(back_control, SNAME("gui_input"));
		SIGNAL_UNWATCH(front_control, SNAME("gui_input"));
		memdelete(back_control);
		memdelete(front_control);
	}

	SUBCASE("[Control] A control that isn't interactable doesn't start dragging") {
		DragSourceControl *drag_control = memnew(DragSourceControl);
		root->add_child(drag_control);
		drag_control->set_size(Size2(100, 100));
		drag_control->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_DISABLED);

		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(20, 20), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(60, 60), MouseButtonMask::LEFT, Key::NONE);
		CHECK_FALSE(root->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(60, 60), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);

		memdelete(drag_control);
	}

	SUBCASE("[Control] A slider stops being dragged when it stops being interactable") {
		HSlider *slider = memnew(HSlider);
		root->add_child(slider);
		slider->set_size(Size2(200, 20));
		slider->set_max(100);

		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(100, 10), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		const double value = slider->get_value();
		slider->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_DISABLED);
		slider->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_INHERITED);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(180, 10), MouseButtonMask::NONE, Key::NONE);
		CHECK(slider->get_value() == value);

		memdelete(slider);
	}

	SUBCASE("[Control] A virtual joystick releases its actions when it stops being interactable") {
		VirtualJoystick *joystick = memnew(VirtualJoystick);
		root->add_child(joystick);
		joystick->set_position(Point2());
		joystick->set_size(Size2(200, 200));
		joystick->set_joystick_mode(VirtualJoystick::JOYSTICK_DYNAMIC);

		Ref<InputEventScreenTouch> touch;
		touch.instantiate();
		touch->set_index(0);
		touch->set_position(Point2(100, 100));
		touch->set_pressed(true);
		root->push_input(touch);
		Ref<InputEventScreenDrag> drag;
		drag.instantiate();
		drag->set_index(0);
		drag->set_position(Point2(190, 100));
		root->push_input(drag);
		REQUIRE(Input::get_singleton()->is_action_pressed("ui_right"));

		joystick->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_DISABLED);
		CHECK_FALSE(Input::get_singleton()->is_action_pressed("ui_right"));
		touch->set_pressed(false);
		root->push_input(touch);
		joystick->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_INHERITED);
		CHECK_FALSE(Input::get_singleton()->is_action_pressed("ui_right"));

		memdelete(joystick);
	}

	SUBCASE("[Control] The shortcuts of a menu don't work when it isn't interactable") {
		MenuButton *menu_button = memnew(MenuButton);
		root->add_child(menu_button);
		menu_button->get_popup()->add_item("Item", 1, KeyModifierMask::CTRL | Key::B);
		SIGNAL_WATCH(menu_button->get_popup(), SNAME("id_pressed"));

		menu_button->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_DISABLED);
		SEND_GUI_KEY_EVENT(KeyModifierMask::CTRL | Key::B);
		SIGNAL_CHECK_FALSE(SNAME("id_pressed"));

		menu_button->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_INHERITED);
		SEND_GUI_KEY_EVENT(KeyModifierMask::CTRL | Key::B);
		SIGNAL_CHECK(SNAME("id_pressed"), { { 1 } });

		SIGNAL_UNWATCH(menu_button->get_popup(), SNAME("id_pressed"));
		memdelete(menu_button);
	}

	SUBCASE("[Control] The popup of a menu is closed when it stops being interactable") {
		MenuButton *menu_button = memnew(MenuButton);
		root->add_child(menu_button);
		menu_button->get_popup()->add_item("Item");
		menu_button->show_popup();
		CHECK(menu_button->get_popup()->is_visible());

		menu_button->set_interaction_behavior_recursive(Control::INTERACTION_BEHAVIOR_DISABLED);
		CHECK_FALSE(menu_button->get_popup()->is_visible());

		memdelete(menu_button);
	}
}

} // namespace TestControl
