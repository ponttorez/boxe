extends Control
## A short motion design video of the problems boxe solves, recorded with tools/media.py motion.
## Each vignette shows a problem of stock Godot next to its fix.

const SLIDE := 0.45

## Scale of the content, so the controls are large enough in a GIF. Applied to the window, so text stays sharp.
@export var content_scale := 1.3

@onready var _intro: Control = %Intro
@onready var _logo: Control = %Logo
@onready var _tagline: Label = %Tagline
@onready var _outro: Control = %Outro
@onready var _vignettes: Array[Node] = %Vignettes.get_children()


func _ready() -> void:
	get_window().content_scale_factor = content_scale
	# Played from the start when the scene runs on its own.
	if get_parent() == get_tree().root:
		play()


func play() -> void:
	await _play_intro()
	var previous: Vignette
	for vignette: Vignette in _vignettes:
		vignette.visible = true
		vignette.position = Vector2(size.x, 0)
		# The containers of the vignette place its controls on the next frame.
		await get_tree().process_frame
		vignette.prepare()
		# The next vignette comes in while the previous one leaves.
		if previous:
			_leave(previous)
		await _slide(vignette, Vector2(size.x, 0), Vector2.ZERO, 0.0, 1.0)
		await vignette.play()
		await get_tree().create_timer(vignette.hold).timeout
		previous = vignette
	await _leave(previous)
	await _play_outro()


func _leave(vignette: Vignette) -> void:
	await _slide(vignette, Vector2.ZERO, Vector2(-size.x * 0.4, 0), 1.0, 0.0)
	vignette.visible = false


func _play_intro() -> void:
	_logo.scale = Vector2.ZERO
	_tagline.visible_ratio = 0.0
	_intro.modulate.a = 1.0
	var tween := create_tween()
	tween.tween_property(_logo, ^"scale", Vector2.ONE, 0.6).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(_tagline, ^"visible_ratio", 1.0, 0.7)
	tween.tween_interval(1.0)
	tween.tween_property(_intro, ^"modulate:a", 0.0, 0.35)
	await tween.finished
	_intro.visible = false


func _play_outro() -> void:
	_outro.visible = true
	_outro.modulate.a = 0.0
	var tween := create_tween()
	tween.tween_property(_outro, ^"modulate:a", 1.0, 0.5)
	tween.tween_interval(2.4)
	await tween.finished


func _slide(node: Control, from: Vector2, to: Vector2, from_alpha: float, to_alpha: float) -> void:
	node.position = from
	node.modulate.a = from_alpha
	var tween := create_tween().set_parallel().set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
	tween.tween_property(node, ^"position", to, SLIDE)
	tween.tween_property(node, ^"modulate:a", to_alpha, SLIDE)
	await tween.finished
