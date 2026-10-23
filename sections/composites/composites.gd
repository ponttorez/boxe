extends VBoxContainer

@onready var _opacity: HSlider = %Opacity
@onready var _opacity_value: Label = %OpacityValue
@onready var _avatars: Control = %Avatars
@onready var _sample: Label = %Sample


func _ready() -> void:
	_opacity.value_changed.connect(_on_opacity_changed)
	_on_opacity_changed(_opacity.value)
	%GroupComposite.toggled.connect(func(on: bool) -> void: _avatars.composite_enabled = on)
	%TextComposite.toggled.connect(func(on: bool) -> void: _sample.label_settings.composite_effects = on)


func _on_opacity_changed(value: float) -> void:
	_opacity_value.text = "%d%%" % roundi(value * 100.0)
	for item: CanvasItem in [_avatars, _sample]:
		item.modulate.a = value
