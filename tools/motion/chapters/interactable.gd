extends Chapter

const SAVE_TIME := 1.6


func play() -> void:
	await move_both("Buy", 0.5)
	show_both("Buy", &"pressed")
	godot_cursor.tap()
	boxe_cursor.tap()
	await wait(0.15)
	show_both("Buy", &"normal")
	# Stock Godot can only disable controls, which grays them out. boxe keeps their look.
	_set_godot_disabled(true)
	in_boxe("Shop").interactable = false
	var tween := create_tween().set_parallel()
	tween.tween_property(in_godot("Progress"), ^"value", 100.0, SAVE_TIME)
	tween.tween_property(in_boxe("Progress"), ^"value", 100.0, SAVE_TIME)
	await tween.finished
	_set_godot_disabled(false)
	in_boxe("Shop").interactable = true
	await wait(0.4)


func _set_godot_disabled(disabled: bool) -> void:
	in_godot("Buy").disabled = disabled
	in_godot("Gift").disabled = disabled
	var dim := Color(1, 1, 1, 0.45) if disabled else Color.WHITE
	in_godot("Item").modulate = dim
