extends Chapter


func prepare() -> void:
	super()
	in_godot("Text").visible_ratio = 0.0
	in_boxe("Text").visible_ratio = 0.0


func play() -> void:
	var tween := create_tween().set_parallel()
	tween.tween_property(in_godot("Text"), ^"visible_ratio", 1.0, 1.6)
	tween.tween_property(in_boxe("Text"), ^"visible_ratio", 1.0, 1.6)
	await tween.finished
	await wait(1.0)
