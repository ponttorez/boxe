extends Chapter


func play() -> void:
	for step in 2:
		await move_both("Press", 0.45)
		show_both("Press", &"pressed")
		_tap()
		await wait(0.2)
		show_both("Press", &"normal")
		await wait(0.35)
		await move_both("Music", 0.45, Vector2(0.85, 0.5))
		for music: CheckButton in [in_godot("Music"), in_boxe("Music")]:
			music.button_pressed = not music.button_pressed
		_tap()
		await wait(0.55)


func _tap() -> void:
	godot_cursor.tap()
	boxe_cursor.tap()
	in_boxe("Waves").pulse()
