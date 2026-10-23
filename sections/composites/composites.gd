extends VBoxContainer

const OUTLINE_WIDTH := 5

@onready var _opacity: HSlider = %Opacity
@onready var _opacity_value: Label = %OpacityValue
@onready var _avatars: Control = %Avatars
@onready var _sample: Label = %Sample
@onready var _sticker: Material = _avatars.material

var _styles := {}


func _ready() -> void:
	for child: Control in _avatars.get_children():
		_styles[child] = child.get_theme_stylebox(&"panel" if child is Panel else &"normal")
	_opacity.value_changed.connect(_on_opacity_changed)
	_on_opacity_changed(_opacity.value)
	%Composite.toggled.connect(_on_composite_toggled)


func _on_composite_toggled(on: bool) -> void:
	_avatars.composite_enabled = on
	_avatars.material = _sticker if on else null
	for child: Control in _avatars.get_children():
		var style_name := &"panel" if child is Panel else &"normal"
		child.add_theme_stylebox_override(style_name, _styles[child] if on else _outlined(_styles[child]))
	_sample.label_settings.composite_effects = on


func _outlined(style: StyleBoxFlat) -> StyleBoxLayered:
	var outlined := StyleBoxLayered.new()
	for corner: Corner in [CORNER_TOP_LEFT, CORNER_TOP_RIGHT, CORNER_BOTTOM_RIGHT, CORNER_BOTTOM_LEFT]:
		outlined.set_corner_radius(corner, style.get_corner_radius(corner))
	for side: Side in [SIDE_LEFT, SIDE_TOP, SIDE_RIGHT, SIDE_BOTTOM]:
		outlined.set_content_margin(side, style.get_content_margin(side))
	outlined.corner_detail = 16
	var layers: Array[StyleBoxLayer] = [
		_layer(Color(0, 0, 0, 0.45), -OUTLINE_WIDTH, Vector2(3, 6)),
		_layer(Color.WHITE, -OUTLINE_WIDTH),
		_layer(style.bg_color, 0),
	]
	outlined.layers = layers
	return outlined


func _layer(color: Color, margin: float, offset := Vector2.ZERO) -> StyleBoxLayer:
	var layer := StyleBoxLayer.new()
	layer.color = color
	layer.offset = offset
	layer.corner_radius_adjust = -margin
	for side: Side in [SIDE_LEFT, SIDE_TOP, SIDE_RIGHT, SIDE_BOTTOM]:
		layer.set_margin(side, margin)
	return layer


func _on_opacity_changed(value: float) -> void:
	_opacity_value.text = "%d%%" % roundi(value * 100.0)
	for item: CanvasItem in [_avatars, _sample]:
		item.modulate.a = value
