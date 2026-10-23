extends Chapter

const BUTTONS: Array[String] = ["Continue", "Settings", "Quit"]


func prepare() -> void:
	super()
	for button_name in BUTTONS:
		Chapter.snap(in_godot(button_name))


func play() -> void:
	for button_name in BUTTONS:
		await move_both(button_name, 0.45)
		show_both(button_name, &"hover")
		await wait(0.35)
		show_both(button_name, &"pressed")
		await wait(0.18)
		show_both(button_name, &"hover")
		await wait(0.3)
		show_both(button_name, &"normal")
	await move_both("Quit", 0.4, Vector2(0.9, 1.6))
