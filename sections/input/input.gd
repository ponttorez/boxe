extends VBoxContainer

const SAVE_TIME := 1.6

@onready var _lock: CheckButton = %Lock
@onready var _stock_form: Control = %StockForm
@onready var _boxe_form: Control = %BoxeForm
@onready var _status: Label = %Status
@onready var _show: CheckButton = %Show
@onready var _overlay: Control = %Overlay

var _saves := {}


func _ready() -> void:
	_overlay.visible = _show.button_pressed
	_show.toggled.connect(_overlay.set_visible)
	_lock.toggled.connect(_on_lock_toggled)
	for side in [[_stock_form, %StockProgress, %StockCancel], [_boxe_form, %BoxeProgress, %BoxeCancel]]:
		var form: Control = side[0]
		form.get_node(^"Save").pressed.connect(_start_save.bind(form, side[1], side[2]))
		side[2].pressed.connect(_end_save.bind(form, side[1], side[2], false))


func _on_lock_toggled(locked: bool) -> void:
	_set_locked(_stock_form, locked or _saves.has(_stock_form))
	_set_locked(_boxe_form, locked or _saves.has(_boxe_form))


func _set_locked(form: Control, locked: bool) -> void:
	if form == _boxe_form:
		form.interactable = not locked
		return
	for control in form.get_children():
		if control is BaseButton:
			control.disabled = locked
		elif control is LineEdit:
			control.editable = not locked


func _start_save(form: Control, progress: ProgressBar, cancel: Button) -> void:
	_set_locked(form, true)
	cancel.disabled = false
	progress.value = 0.0
	_status.text = tr("Saving…")
	var tween := create_tween()
	tween.tween_property(progress, ^"value", progress.max_value, SAVE_TIME)
	tween.finished.connect(_end_save.bind(form, progress, cancel, true))
	_saves[form] = tween


func _end_save(form: Control, progress: ProgressBar, cancel: Button, saved: bool) -> void:
	var tween: Tween = _saves.get(form)
	if tween:
		tween.kill()
		_saves.erase(form)
	if not saved:
		progress.value = 0.0
	cancel.disabled = true
	_set_locked(form, _lock.button_pressed)
	_status.text = tr("Saved. The form takes input again.") if saved else tr("Canceled.")
