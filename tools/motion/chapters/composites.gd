extends Chapter


func play() -> void:
	for alpha in [0.35, 1.0, 0.35]:
		var tween := create_tween().set_parallel().set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
		tween.tween_property(in_godot("Avatars"), ^"modulate:a", alpha, 0.8)
		tween.tween_property(in_boxe("Avatars"), ^"modulate:a", alpha, 0.8)
		await tween.finished
		await wait(0.5)
