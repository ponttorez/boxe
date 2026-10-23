extends VBoxContainer

const TRANSITION_DURATION := 0.7
const INDICATOR_STYLES: Array[StringName] = [&"checked", &"unchecked", &"checked_mirrored", &"unchecked_mirrored"]

@onready var _bus: OptionButton = %Bus
@onready var _overrides: Control = %Overrides
@onready var _all: CheckButton = %All
@onready var _switches: Container = %Switches


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


func _on_bus_selected(index: int) -> void:
	_overrides.sound_bus = StringName(_bus.get_item_text(index))


func _on_all_toggled(on: bool) -> void:
	for switch: CheckButton in _switches.get_children():
		switch.button_pressed = on
