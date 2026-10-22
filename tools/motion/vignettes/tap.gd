extends Vignette
## A tap just next to a small close button misses it in Godot. In boxe, the raycast padding catches it.

const AREA_COLOR := Color(1.0, 0.8, 0.25)


func play() -> void:
	godot_cursor.finger = true
	boxe_cursor.finger = true
	var stock_close: Button = in_godot("Close")
	var boxe_close: Button = in_boxe("Close")
	var area := _show_hit_area(boxe_close)
	# Just outside of the 22 px buttons, but inside the padding.
	await move_both(stock_close, boxe_close, 0.8, Vector2(0.5, 0.5), Vector2(-15, 12))
	create_tween().tween_property(area, ^"modulate:a", 1.0, 0.3)
	await wait(0.45)
	var hit := _is_hit(boxe_close, boxe_cursor.global_position)
	godot_cursor.tap(Color("ff786b"))
	boxe_cursor.tap()
	if hit:
		show_state(boxe_close, &"hover")
	await wait(0.35)
	var card: Control = in_boxe("Card")
	var close := create_tween().set_parallel().set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_IN)
	close.tween_property(card, ^"modulate:a", 0.0, 0.35)
	close.tween_property(card, ^"position:x", card.position.x + 60, 0.35)
	close.tween_property(area, ^"modulate:a", 0.0, 0.2)
	await wait(1.1)


## Whether a point is in the input area of a control, like Control does with raycast padding.
static func _is_hit(control: Control, point: Vector2) -> bool:
	var area := Rect2(Vector2(), control.size).grow_individual(
			-control.get_raycast_padding(SIDE_LEFT), -control.get_raycast_padding(SIDE_TOP),
			-control.get_raycast_padding(SIDE_RIGHT), -control.get_raycast_padding(SIDE_BOTTOM))
	return area.has_point(control.get_global_transform().affine_inverse() * point)


func _show_hit_area(control: Control) -> Panel:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(AREA_COLOR, 0.14)
	style.border_color = Color(AREA_COLOR, 0.9)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	var area := Panel.new()
	area.mouse_filter = Control.MOUSE_FILTER_IGNORE
	area.add_theme_stylebox_override(&"panel", style)
	area.modulate.a = 0.0
	add_child(area)
	var rect := control.get_global_rect().grow_individual(
			-control.get_raycast_padding(SIDE_LEFT), -control.get_raycast_padding(SIDE_TOP),
			-control.get_raycast_padding(SIDE_RIGHT), -control.get_raycast_padding(SIDE_BOTTOM))
	area.position = rect.position - global_position
	area.size = rect.size
	return area
