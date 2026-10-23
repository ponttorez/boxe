extends Control
## The banner of boxe that opens and closes the video: the name, the tagline and the mascot.

@onready var _fork: Control = %Fork
@onready var _wordmark: Control = %Wordmark
@onready var _tagline: Label = %Tagline
@onready var _mascot: Control = %Mascot
@onready var _shadow: Control = %Shadow

var _mascot_rotation := 0.0
var _mascot_y := 0.0
var _bobbing: Tween


func _ready() -> void:
	_mascot_rotation = _mascot.rotation
	_mascot_y = _mascot.position.y
	_mascot.pivot_offset = _mascot.size * 0.5
	_wordmark.pivot_offset = Vector2(0.0, _wordmark.size.y * 0.5)


## Builds the banner up piece by piece.
func appear(tagline: String) -> void:
	if _bobbing:
		_bobbing.kill()
	_mascot.position.y = _mascot_y
	_tagline.text = tagline
	_fork.modulate.a = 0.0
	_wordmark.modulate.a = 0.0
	_wordmark.scale = Vector2(0.86, 0.86)
	_tagline.visible_ratio = 0.0
	_mascot.scale = Vector2(0.4, 0.4)
	_mascot.modulate.a = 0.0
	_mascot.rotation = _mascot_rotation - 0.35
	_shadow.modulate.a = 0.0
	var tween := create_tween().set_parallel().set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(_mascot, ^"scale", Vector2.ONE, 0.7)
	tween.tween_property(_mascot, ^"rotation", _mascot_rotation, 0.8)
	tween.tween_property(_mascot, ^"modulate:a", 1.0, 0.3)
	tween.tween_property(_shadow, ^"modulate:a", 1.0, 0.6)
	tween.tween_property(_wordmark, ^"modulate:a", 1.0, 0.35).set_delay(0.15)
	tween.tween_property(_wordmark, ^"scale", Vector2.ONE, 0.55).set_delay(0.15)
	tween.tween_property(_fork, ^"modulate:a", 1.0, 0.4).set_delay(0.35)
	tween.tween_property(_tagline, ^"visible_ratio", 1.0, 0.9).set_delay(0.55).set_trans(Tween.TRANS_LINEAR)
	await tween.finished
	_bob()


## Floats the mascot up and down while the banner is shown.
func _bob() -> void:
	_bobbing = create_tween().set_loops().set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	_bobbing.tween_property(_mascot, ^"position:y", _mascot_y - 10.0, 1.1)
	_bobbing.tween_property(_mascot, ^"position:y", _mascot_y, 1.1)
