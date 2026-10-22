extends Vignette
## Fading a group shows the overlapping parts in Godot. A composite fades as one image.


func play() -> void:
	var groups: Array[Control] = [in_godot("Group"), in_boxe("Group")]
	godot_cursor.visible = false
	boxe_cursor.visible = false
	var tween := create_tween().set_parallel().set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	for group in groups:
		tween.tween_property(group, ^"modulate:a", 0.3, 1.1)
	await tween.finished
	await wait(0.9)
	tween = create_tween().set_parallel().set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	for group in groups:
		tween.tween_property(group, ^"modulate:a", 1.0, 0.8)
	await tween.finished
	await wait(0.3)
