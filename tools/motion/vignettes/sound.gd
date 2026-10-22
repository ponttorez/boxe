extends Vignette
## Stock controls make no sound. boxe controls play the sounds of the theme.


func play() -> void:
	var waves: Control = in_boxe("Waves")
	for target in ["Button", "Toggle"]:
		var stock: Control = in_godot(target)
		var boxe: Control = in_boxe(target)
		await move_both(stock, boxe, 0.6, Vector2(0.3 if target == "Toggle" else 0.6, 0.6))
		await wait(0.2)
		godot_cursor.tap()
		boxe_cursor.tap()
		if target == "Toggle":
			stock.set_pressed_no_signal(true)
			boxe.set_pressed_no_signal(true)
		else:
			show_state(stock, &"pressed")
			show_state(boxe, &"pressed")
		waves.pulse()
		await wait(0.25)
		if target == "Button":
			show_state(stock, &"hover")
			show_state(boxe, &"hover")
		await wait(0.6)
	await wait(0.4)
