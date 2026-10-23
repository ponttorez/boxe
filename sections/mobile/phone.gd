@tool
extends Control

const PHONE_SIZE := Vector2(118.0, 220.0)

@export_enum("None", "Inside", "Outside") var mode := 0:
	set(value):
		mode = value
		_update_mode()

@export var zoom := 1.5:
	set(value):
		zoom = value
		_update_zoom()

@onready var _body: Control = $Body
@onready var _inside: Control = %Inside
@onready var _outside: Control = %Outside


func _ready() -> void:
	resized.connect(_update_zoom)
	_update_mode()
	_update_zoom()


func _get_minimum_size() -> Vector2:
	return PHONE_SIZE * zoom


func _update_mode() -> void:
	if not is_node_ready():
		return
	_inside.visible = mode == 1
	_outside.visible = mode == 2


func _update_zoom() -> void:
	update_minimum_size()
	if not is_node_ready():
		return
	_body.scale = Vector2(zoom, zoom)
	_body.position = Vector2((size.x - PHONE_SIZE.x * zoom) * 0.5, 0.0)
