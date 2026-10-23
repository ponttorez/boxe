class_name GhostCursor
extends Control
## A cursor that is only drawn and sends no input, so several of them can play at once.
## As a finger, it draws a touch point instead of an arrow.

const ARROW := preload("res://tools/capture/cursor.svg")

@export var finger := false
@export var ripple_color := Color(0.44, 0.73, 1.0)

var _ripple := 1.0


func _init() -> void:
	mouse_filter = MOUSE_FILTER_IGNORE
	z_index = 10
	position = Vector2(-100, -100)


## Moves to a point in global coordinates. The cursor stays a child of its parent, so it moves along with it.
func move_to(target: Vector2, duration := 0.6) -> void:
	var local: Vector2 = target - (get_parent() as CanvasItem).get_global_transform().origin
	if duration <= 0.0:
		position = local
		return
	var tween := create_tween().set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_IN_OUT)
	tween.tween_property(self, ^"position", local, duration)
	await tween.finished


func move_to_control(control: Control, duration := 0.6, anchor := Vector2(0.5, 0.5), offset := Vector2()) -> void:
	var rect := control.get_global_rect()
	await move_to(rect.position + rect.size * anchor + offset, duration)


func tap(color := ripple_color) -> void:
	ripple_color = color
	var tween := create_tween().set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
	tween.tween_method(_set_ripple, 0.0, 1.0, 0.45)


func _set_ripple(value: float) -> void:
	_ripple = value
	queue_redraw()


func _draw() -> void:
	if _ripple < 1.0:
		var color := ripple_color
		color.a = 0.7 * (1.0 - _ripple)
		draw_circle(Vector2.ZERO, 8.0 + 22.0 * _ripple, color, false, 3.0, true)
	if finger:
		draw_circle(Vector2.ZERO, 15.0, Color(1, 1, 1, 0.22), true, -1.0, true)
		draw_circle(Vector2.ZERO, 15.0, Color(1, 1, 1, 0.75), false, 2.0, true)
	else:
		draw_texture(ARROW, Vector2(-5, -5))
