class_name FakeCursor
extends CanvasLayer
## A visible mouse cursor for recordings. It sends real mouse and keyboard events,
## so the controls react like they do for a user: hovering, pressing, typing and dragging.

const ARROW := preload("res://tools/capture/cursor.svg")
const RIPPLE_COLOR := Color(0.44, 0.73, 1.0)

var cursor_position := Vector2(-64, -64)
var _pressed := false
var _ripple_progress := 1.0
var _arrow: Sprite2D
var _ripple: Node2D


func _init() -> void:
	layer = 128


func _ready() -> void:
	_ripple = Node2D.new()
	_ripple.draw.connect(_draw_ripple)
	add_child(_ripple)
	_arrow = Sprite2D.new()
	_arrow.texture = ARROW
	_arrow.centered = false
	_arrow.offset = Vector2(-5, -5)
	_arrow.position = cursor_position
	add_child(_arrow)


## Moves the cursor to a point of the viewport.
func move_to(target: Vector2, duration := 0.6) -> void:
	if duration <= 0.0:
		_set_cursor_position(target)
		return
	var tween := create_tween().set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_IN_OUT)
	tween.tween_method(_set_cursor_position, cursor_position, target, duration)
	await tween.finished


## Moves the cursor over a control, by default to its center.
func move_to_control(control: Control, duration := 0.6, anchor := Vector2(0.5, 0.5)) -> void:
	var rect := control.get_global_rect()
	await move_to(rect.position + rect.size * anchor, duration)


func press() -> void:
	_pressed = true
	_send_button(true)
	_play_ripple()


func release() -> void:
	_pressed = false
	_send_button(false)


func click(hold := 0.1) -> void:
	press()
	await get_tree().create_timer(hold).timeout
	release()


## Presses at the current point, moves while pressed and releases, like dragging a slider.
func drag_to(target: Vector2, duration := 0.6) -> void:
	press()
	await move_to(target, duration)
	release()


func type_text(text: String, interval := 0.07) -> void:
	for character in text:
		var event := InputEventKey.new()
		event.pressed = true
		event.unicode = character.unicode_at(0)
		event.keycode = OS.find_keycode_from_string(character.to_upper())
		Input.parse_input_event(event)
		var release_event: InputEventKey = event.duplicate()
		release_event.pressed = false
		Input.parse_input_event(release_event)
		await get_tree().create_timer(interval).timeout


func press_key(keycode: Key) -> void:
	for pressed in [true, false]:
		var event := InputEventKey.new()
		event.pressed = pressed
		event.keycode = keycode
		Input.parse_input_event(event)


func _set_cursor_position(target: Vector2) -> void:
	var event := InputEventMouseMotion.new()
	event.position = target
	event.global_position = target
	event.relative = target - cursor_position
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if _pressed else 0
	cursor_position = target
	_arrow.position = target
	_ripple.position = target
	Input.parse_input_event(event)


func _send_button(pressed: bool) -> void:
	var event := InputEventMouseButton.new()
	event.position = cursor_position
	event.global_position = cursor_position
	event.button_index = MOUSE_BUTTON_LEFT
	event.button_mask = MOUSE_BUTTON_MASK_LEFT if pressed else 0
	event.pressed = pressed
	Input.parse_input_event(event)


func _play_ripple() -> void:
	var tween := create_tween()
	tween.tween_method(_set_ripple_progress, 0.0, 1.0, 0.45).set_ease(Tween.EASE_OUT).set_trans(Tween.TRANS_CUBIC)


func _set_ripple_progress(progress: float) -> void:
	_ripple_progress = progress
	_ripple.queue_redraw()


func _draw_ripple() -> void:
	if _ripple_progress >= 1.0:
		return
	var color := RIPPLE_COLOR
	color.a = 0.6 * (1.0 - _ripple_progress)
	_ripple.draw_circle(Vector2.ZERO, 6.0 + 18.0 * _ripple_progress, color, false, 2.5, true)
