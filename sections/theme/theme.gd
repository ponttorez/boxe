extends VBoxContainer

const COPIED_TIME := 1.5

@onready var _copy: Button = %Copy
@onready var _copied: PopupPanel = %Copied

var _copied_count := 0


func _ready() -> void:
	_copy.pressed.connect(_on_copy_pressed)


func _on_copy_pressed() -> void:
	DisplayServer.clipboard_set(%Text.text)
	var rect := _copy.get_screen_transform() * Rect2(Vector2.ZERO, _copy.size)
	_copied.reset_size()
	_copied.position = Vector2i(roundi(rect.get_center().x - _copied.size.x * 0.5), roundi(rect.position.y) - _copied.size.y - _copy.get_theme_constant(&"popup_separation", &"MenuButton"))
	_copied.popup()
	_copied_count += 1
	var count := _copied_count
	await get_tree().create_timer(COPIED_TIME).timeout
	if count == _copied_count:
		_copied.hide()
