extends VBoxContainer

const TRANSITION_DURATION := 0.7
const COPIED_TIME := 1.5
const INDICATOR_STYLES: Array[StringName] = [&"checked", &"unchecked", &"checked_mirrored", &"unchecked_mirrored"]

@onready var _bus: OptionButton = %Bus
@onready var _overrides: Control = %Overrides
@onready var _all: CheckButton = %All
@onready var _switches: Container = %Switches
@onready var _copy: Button = %Copy
@onready var _copied: PopupPanel = %Copied

var _copied_count := 0


func _ready() -> void:
	_bus.item_selected.connect(_on_bus_selected)
	for switch: CheckButton in _switches.get_children():
		for style_name in INDICATOR_STYLES:
			var style := switch.get_theme_stylebox(style_name).duplicate() as StyleBox
			style.transition_duration = TRANSITION_DURATION
			style.transition_type = switch.get_meta(&"transition_type")
			style.transition_ease = switch.get_meta(&"transition_ease")
			switch.add_theme_stylebox_override(style_name, style)
	_all.toggled.connect(_on_all_toggled)
	_copy.pressed.connect(_on_copy_pressed)


func _on_bus_selected(index: int) -> void:
	_overrides.sound_bus = StringName(_bus.get_item_text(index))


func _on_all_toggled(on: bool) -> void:
	for switch: CheckButton in _switches.get_children():
		switch.button_pressed = on


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
