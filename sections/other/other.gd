extends VBoxContainer

const SAVE_TIME := 1.6
const OUTLINE_WIDTH := 7.5
const SHADOW_OFFSET := Vector2(4.5, 9)
const TRANSITION_DURATION := 0.7
const INDICATOR_STYLES: Array[StringName] = [&"checked", &"unchecked", &"checked_mirrored", &"unchecked_mirrored"]

@onready var _lock: CheckButton = %Lock
@onready var _content: Control = %Content
@onready var _form: Control = %Form
@onready var _cancel: Button = %Cancel
@onready var _progress: ProgressBar = %Progress
@onready var _status: Label = %Status
@onready var _show: CheckButton = %Show
@onready var _overlay: Control = %Overlay
@onready var _opacity: HSlider = %Opacity
@onready var _opacity_value: Label = %OpacityValue
@onready var _avatars: Control = %Avatars
@onready var _sample: Label = %Sample
@onready var _sticker: Material = _avatars.material
@onready var _bus: OptionButton = %Bus
@onready var _overrides: Control = %Overrides
@onready var _all: CheckButton = %All
@onready var _switches: Container = %Switches

var _save: Tween
var _styles := {}


func _ready() -> void:
	_overlay.visible = _show.button_pressed
	_show.toggled.connect(_overlay.set_visible)
	_lock.toggled.connect(func(locked: bool) -> void: _content.interactable = not locked)
	_form.get_node(^"Save").pressed.connect(_start_save)
	_cancel.pressed.connect(_end_save.bind(false))
	for child: Control in _avatars.get_children():
		_styles[child] = child.get_theme_stylebox(&"panel" if child is Panel else &"normal")
	_opacity.value_changed.connect(_on_opacity_changed)
	_on_opacity_changed(_opacity.value)
	%Composite.toggled.connect(_on_composite_toggled)
	get_viewport().size_changed.connect(_update_sticker)
	_update_sticker()
	resized.connect(_fit_sample)
	_fit_sample()
	_bus.item_selected.connect(_on_bus_selected)
	for switch: CheckButton in _switches.get_children():
		for style_name in INDICATOR_STYLES:
			var style := switch.get_theme_stylebox(style_name).duplicate() as StyleBox
			style.transition_duration = TRANSITION_DURATION
			style.transition_type = switch.get_meta(&"transition_type")
			style.transition_ease = switch.get_meta(&"transition_ease")
			switch.add_theme_stylebox_override(style_name, style)
	_all.toggled.connect(_on_all_toggled)


func _start_save() -> void:
	_form.interactable = false
	_cancel.disabled = false
	_status.text = tr("Saving…")
	_progress.value = 0.0
	_save = create_tween()
	_save.tween_property(_progress, ^"value", _progress.max_value, SAVE_TIME)
	_save.finished.connect(_end_save.bind(true))


func _end_save(saved: bool) -> void:
	if _save:
		_save.kill()
		_save = null
	if not saved:
		_progress.value = 0.0
	_cancel.disabled = true
	_form.interactable = true
	_status.text = tr("Saved. The form takes input again.") if saved else tr("Canceled.")


func _update_sticker() -> void:
	var scale := get_viewport().get_final_transform().get_scale().x * _avatars.get_global_transform_with_canvas().get_scale().x
	_sticker.set_shader_parameter(&"outline_width", OUTLINE_WIDTH * scale)
	_sticker.set_shader_parameter(&"shadow_offset", SHADOW_OFFSET * scale)


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
		_layer(Color(0, 0, 0, 0.45), -OUTLINE_WIDTH, SHADOW_OFFSET),
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


func _fit_sample() -> void:
	_sample.label_settings.font_size = 66 if size.x > 720.0 else 45


func _on_bus_selected(index: int) -> void:
	_overrides.sound_bus = StringName(_bus.get_item_text(index))


func _on_all_toggled(on: bool) -> void:
	for switch: CheckButton in _switches.get_children():
		switch.button_pressed = on
