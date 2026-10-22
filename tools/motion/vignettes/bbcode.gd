extends Vignette
## The same text in a Label without and with BBCode, typed out.


func prepare() -> void:
	super()
	godot_cursor.visible = false
	boxe_cursor.visible = false
	for label: Label in [in_godot("Text"), in_boxe("Text")]:
		label.visible_ratio = 0.0


func play() -> void:
	var tween := create_tween().set_parallel()
	for label: Label in [in_godot("Text"), in_boxe("Text")]:
		tween.tween_property(label, ^"visible_ratio", 1.0, 1.6)
	await tween.finished
	await wait(1.3)
