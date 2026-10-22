class_name Vignette
extends Control
## One problem of stock Godot next to its fix in boxe. Subclasses animate both sides in play().
## Both sides are driven without input, so they always show the same moment.

## Seconds the vignette stays on screen after play() returns.
@export var hold := 0.5

var godot_cursor: GhostCursor
var boxe_cursor: GhostCursor

@onready var godot_stage: Control = %Godot
@onready var boxe_stage: Control = %Boxe


func _ready() -> void:
	godot_cursor = GhostCursor.new()
	boxe_cursor = GhostCursor.new()
	add_child(godot_cursor)
	add_child(boxe_cursor)


## Places the cursors before the vignette comes in. By default below the middle of each side.
func prepare() -> void:
	for pair in [[godot_cursor, godot_stage], [boxe_cursor, boxe_stage]]:
		var rect: Rect2 = pair[1].get_global_rect()
		pair[0].move_to(rect.position + rect.size * Vector2(0.72, 0.92), 0.0)


## Plays the animation of the vignette. Overridden by each vignette.
func play() -> void:
	await wait(1.0)


## Returns a node of the Godot side.
func in_godot(node_name: String) -> Node:
	return godot_stage.find_child(node_name, true, false)


## Returns a node of the boxe side.
func in_boxe(node_name: String) -> Node:
	return boxe_stage.find_child(node_name, true, false)


func wait(seconds: float) -> void:
	await get_tree().create_timer(seconds).timeout


## Moves both cursors to the same anchor of a control in each side.
func move_both(godot_target: Control, boxe_target: Control, duration := 0.6, anchor := Vector2(0.5, 0.5), offset := Vector2()) -> void:
	godot_cursor.move_to_control(godot_target, duration, anchor, offset)
	await boxe_cursor.move_to_control(boxe_target, duration, anchor, offset)


## Shows a theme state of a button, like hover or pressed, as if the cursor caused it.
static func show_state(button: Button, state: StringName) -> void:
	if state == &"normal":
		button.remove_theme_stylebox_override(&"normal")
		button.remove_theme_color_override(&"font_color")
		return
	button.add_theme_stylebox_override(&"normal", button.get_theme_stylebox(state))
	button.add_theme_color_override(&"font_color", button.get_theme_color(StringName("font_%s_color" % state)))
