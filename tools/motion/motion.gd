extends Control
## A short video of the problems boxe solves, recorded with tools/media.py motion.
## The banner opens it, each chapter shows a problem of stock Godot next to its fix, and the banner closes it.

const SLIDE := 0.5
## The chapters are laid out for this height and scaled to the window, so the video is sharp at any size.
const LAYOUT_HEIGHT := 600.0
## The banner is laid out for 1280×720 and scaled to the layout.
const BANNER_SIZE := Vector2(1280, 720)

@onready var _banner: Control = %Banner
@onready var _chapters: Array[Node] = %Chapters.get_children()


func _ready() -> void:
	get_window().content_scale_factor = get_window().size.y / LAYOUT_HEIGHT
	_banner.set_anchors_preset(Control.PRESET_TOP_LEFT)
	_banner.size = BANNER_SIZE
	_banner.scale = Vector2.ONE * LAYOUT_HEIGHT / BANNER_SIZE.y
	for chapter: Control in _chapters:
		chapter.visible = false
	# Played from the start when the scene runs on its own.
	if get_parent() == get_tree().root:
		play()


func play() -> void:
	await _banner.appear("The Godot Engine you love,\nwith extra features inside")
	await _wait(1.6)
	var previous: Control = _banner
	for chapter: Chapter in _chapters:
		chapter.visible = true
		chapter.position = Vector2(size.x, 0)
		# The containers of the chapter place its controls on the next frame.
		await get_tree().process_frame
		chapter.prepare()
		_leave(previous)
		await _slide(chapter, Vector2(size.x, 0), Vector2.ZERO, 0.0, 1.0)
		await chapter.play()
		await _wait(chapter.hold)
		previous = chapter
	_banner.visible = true
	_banner.position = Vector2(size.x, 0)
	_leave(previous)
	_slide(_banner, Vector2(size.x, 0), Vector2.ZERO, 0.0, 1.0)
	await _banner.appear("Get it on GitHub:\ngithub.com/ponttorez/boxe")
	await _wait(2.6)


func _leave(node: Control) -> void:
	await _slide(node, Vector2.ZERO, Vector2(-size.x * 0.4, 0), 1.0, 0.0)
	node.visible = false


func _slide(node: Control, from: Vector2, to: Vector2, from_alpha: float, to_alpha: float) -> void:
	node.position = from
	node.modulate.a = from_alpha
	var tween := create_tween().set_parallel().set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_OUT)
	tween.tween_property(node, ^"position", to, SLIDE)
	tween.tween_property(node, ^"modulate:a", to_alpha, SLIDE)
	await tween.finished


func _wait(seconds: float) -> void:
	await get_tree().create_timer(seconds).timeout
