extends VBoxContainer

const SAVE_TIME := 1.6

@onready var _lock: CheckButton = %Lock
@onready var _content: Control = %Content
@onready var _form: Control = %Form
@onready var _cancel: Button = %Cancel
@onready var _progress: ProgressBar = %Progress
@onready var _status: Label = %Status
@onready var _show: CheckButton = %Show
@onready var _overlay: Control = %Overlay

var _save: Tween


func _ready() -> void:
	_overlay.visible = _show.button_pressed
	_show.toggled.connect(_overlay.set_visible)
	_lock.toggled.connect(func(locked: bool) -> void: _content.interactable = not locked)
	_form.get_node(^"Save").pressed.connect(_start_save)
	_cancel.pressed.connect(_end_save.bind(false))


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
