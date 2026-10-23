extends Chapter


func prepare() -> void:
	super()
	boxe_cursor.finger = true
	godot_cursor.finger = true


func play() -> void:
	await move_both("Phone", 0.5, Vector2(0.5, 0.6))
	for mode in [1, 2]:
		await wait(0.5)
		godot_cursor.tap(Color(1.0, 0.47, 0.42))
		boxe_cursor.tap()
		in_boxe("Phone").mode = mode
		await wait(0.7)
