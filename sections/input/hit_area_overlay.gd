extends Control

const FILL := Color(1.0, 0.8, 0.25, 0.14)
const OUTLINE := Color(1.0, 0.8, 0.25, 0.9)


func _process(_delta: float) -> void:
	global_position = Vector2.ZERO
	size = get_viewport_rect().size
	queue_redraw()


func _draw() -> void:
	var to_local := get_global_transform().affine_inverse()
	for control: Control in get_tree().get_nodes_in_group(&"hit_area"):
		if not control.is_visible_in_tree():
			continue
		var area := Rect2(Vector2.ZERO, control.size).grow_individual(
				-control.get_raycast_padding(SIDE_LEFT), -control.get_raycast_padding(SIDE_TOP),
				-control.get_raycast_padding(SIDE_RIGHT), -control.get_raycast_padding(SIDE_BOTTOM))
		var rect := to_local * control.get_global_transform() * area
		draw_rect(rect, FILL)
		draw_rect(rect, OUTLINE, false, 1.0)
