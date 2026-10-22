extends VBoxContainer

@onready var _opacity: HSlider = %Opacity
@onready var _opacity_value: Label = %OpacityValue
@onready var _faded: Array[CanvasItem] = [%StockAvatars, %BoxeAvatars, %StockText, %BoxeText]


func _ready() -> void:
	_opacity.value_changed.connect(_on_opacity_changed)
	_on_opacity_changed(_opacity.value)


func _on_opacity_changed(value: float) -> void:
	_opacity_value.text = "%d%%" % roundi(value * 100.0)
	for item in _faded:
		item.modulate.a = value
