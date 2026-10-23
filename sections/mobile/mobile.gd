extends VBoxContainer

const MAX_EVENTS := 6
const ORIENTATIONS := ["undefined", "portrait", "landscape"]

var _events: PackedStringArray

@onready var _previews: Control = %Previews
@onready var _safe_rect: Label = %SafeRect.get_node(^"Value")
@onready var _screen_size: Label = %ScreenSize.get_node(^"Value")
@onready var _note: Label = %Note
@onready var _event_log: Label = %Log
@onready var _refresh_rate: Label = %RefreshRate.get_node(^"Value")
@onready var _fps: Label = %Fps.get_node(^"Value")
@onready var _version: Label = %Version.get_node(^"Value")
@onready var _version_code: Label = %VersionCode.get_node(^"Value")


func _ready() -> void:
	var app := get_tree().get_first_node_in_group(&"app")
	for preview: Control in _previews.get_children():
		preview.gui_input.connect(_on_preview_input.bind(app, preview.get_index()))
	_highlight_preview(app.get_safe_area())
	_note.visible = not OS.has_feature("mobile")

	DisplayServer.safe_area_changed.connect(_on_safe_area_changed)
	DisplayServer.orientation_changed.connect(_on_orientation_changed)
	_log(tr("Waiting for rotations…"))

	_version.text = str(ProjectSettings.get_setting("application/config/version"))
	_version_code.text = str(ProjectSettings.get_setting("application/config/version_code"))
	_update_display()
	var timer := Timer.new()
	timer.wait_time = 0.5
	timer.autostart = true
	timer.timeout.connect(_update_display)
	add_child(timer)


func _on_preview_input(event: InputEvent, app: Node, mode: int) -> void:
	var mb := event as InputEventMouseButton
	if mb and mb.button_index == MOUSE_BUTTON_LEFT and not mb.pressed:
		app.set_safe_area(mode)
		_highlight_preview(mode)


func _highlight_preview(mode: int) -> void:
	for preview in _previews.get_children():
		preview.get_node(^"Phone").highlighted = preview.get_index() == mode


func _update_display() -> void:
	var safe := DisplayServer.get_display_safe_area()
	_safe_rect.text = "%d, %d  %d × %d" % [safe.position.x, safe.position.y, safe.size.x, safe.size.y]
	var screen := DisplayServer.screen_get_size()
	_screen_size.text = "%d × %d" % [screen.x, screen.y]
	_refresh_rate.text = "%d Hz" % roundi(DisplayServer.screen_get_refresh_rate())
	_fps.text = "%d" % Engine.get_frames_per_second()


func _on_safe_area_changed() -> void:
	_log("safe_area_changed")
	_update_display()


func _on_orientation_changed(orientation: int) -> void:
	_log("orientation_changed(%s)" % ORIENTATIONS[clampi(orientation, 0, 2)])


func _log(event: String) -> void:
	_events.append("%s  %s" % [Time.get_time_string_from_system(), event])
	if _events.size() > MAX_EVENTS:
		_events.remove_at(0)
	_event_log.text = "\n".join(_events)
