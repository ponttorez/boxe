extends Chapter

const BUTTONS: Array[String] = ["Raised", "Neon", "Skewed"]


func play() -> void:
	for button_name in BUTTONS:
		await move_both(button_name, 0.45)
		show_both(button_name, &"hover")
		await wait(0.3)
		show_both(button_name, &"pressed")
		godot_cursor.tap()
		boxe_cursor.tap()
		await wait(0.35)
		show_both(button_name, &"hover")
		await wait(0.25)
		show_both(button_name, &"normal")
	await move_both("Skewed", 0.4, Vector2(0.8, 2.4))
