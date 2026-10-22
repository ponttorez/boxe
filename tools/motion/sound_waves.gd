extends Control
## Sound waves next to a speaker, which pulse when a sound plays.

const COLOR := Color(0.44, 0.73, 1.0)

var _pulse := 1.0


func pulse() -> void:
	create_tween().tween_method(_set_pulse, 0.0, 1.0, 0.7)


func _set_pulse(value: float) -> void:
	_pulse = value
	queue_redraw()


func _draw() -> void:
	var center := Vector2(-6, size.y * 0.5)
	for i in 3:
		# Each wave lights up a little after the previous one.
		var light := clampf(1.0 - absf(_pulse * 1.6 - i * 0.3) * 2.0, 0.0, 1.0)
		var color := COLOR
		color.a = 0.15 + 0.85 * light
		draw_arc(center, 12.0 + i * 9.0, -0.8, 0.8, 16, color, 3.0, true)
