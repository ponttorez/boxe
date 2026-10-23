extends Chapter

## Where the finger lands: next to the close button, inside the larger hit area of boxe.
const MISS := Vector2(16, 14)


func prepare() -> void:
	super()
	godot_cursor.finger = true
	boxe_cursor.finger = true


func play() -> void:
	await move_both("Close", 0.6, Vector2(1.0, 1.0), MISS)
	await wait(0.2)
	godot_cursor.tap(Color(1.0, 0.47, 0.42))
	boxe_cursor.tap()
	Chapter.show_state(in_boxe("Close"), &"pressed")
	await wait(0.25)
	var message: Control = in_boxe("Message")
	message.pivot_offset = message.size * 0.5
	var tween := create_tween().set_parallel().set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_IN)
	tween.tween_property(message, ^"modulate:a", 0.0, 0.35)
	tween.tween_property(message, ^"scale", Vector2(0.92, 0.92), 0.35)
	await wait(0.5)
	# Stock Godot keeps missing.
	godot_cursor.tap(Color(1.0, 0.47, 0.42))
	await wait(0.6)
