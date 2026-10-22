extends Vignette
## Stock buttons switch their style at once, boxe buttons blend into it.


func play() -> void:
	var buttons: Array[Button] = [in_godot("Button"), in_boxe("Button")]
	await move_both(buttons[0], buttons[1], 0.7, Vector2(0.62, 0.62))
	for button in buttons:
		show_state(button, &"hover")
	await wait(0.8)
	godot_cursor.tap()
	boxe_cursor.tap()
	for button in buttons:
		show_state(button, &"pressed")
	await wait(0.3)
	for button in buttons:
		show_state(button, &"hover")
	await wait(0.7)
	for button in buttons:
		show_state(button, &"normal")
	godot_cursor.move_to(godot_cursor.global_position + Vector2(70, 90), 0.5)
	await boxe_cursor.move_to(boxe_cursor.global_position + Vector2(70, 90), 0.5)
	await wait(0.6)
