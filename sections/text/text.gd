extends VBoxContainer
## Plays the typewriter, opens links and fills the text arguments of the localized labels.

const CHARACTERS_PER_SECOND := 24.0

@onready var _rich: Label = %Rich
@onready var _dialogue: Label = %Dialog
@onready var _replay: Button = %Replay
@onready var _name: LineEdit = %Name
@onready var _count: HSlider = %Count
@onready var _count_value: Label = %CountValue
@onready var _apples: Label = %Apples
@onready var _days: Label = %Days

var _typewriter: Tween


func _ready() -> void:
	_rich.meta_clicked.connect(_on_meta_clicked)
	_replay.pressed.connect(_type)
	_name.text_changed.connect(_update_name.unbind(1))
	_count.value_changed.connect(_update_count.unbind(1))
	_update_name()
	_update_count()
	_type()


func _type() -> void:
	if _typewriter:
		_typewriter.kill()
	_dialogue.visible_characters = 0
	var count := _dialogue.get_total_character_count()
	_typewriter = create_tween()
	_typewriter.tween_method(_show_characters, 0, count, count / CHARACTERS_PER_SECOND)


func _show_characters(count: int) -> void:
	if count == _dialogue.visible_characters:
		return
	_dialogue.visible_characters = count
	# The label plays the "typed" sound overridden in the scene, like a line edit does.
	var character := _dialogue.get_parsed_text()[count - 1]
	if character != " ":
		_dialogue.play_theme_sound(&"typed")


func _on_meta_clicked(meta: Variant) -> void:
	OS.shell_open(str(meta))


## With a name, the text addresses the player by it. The name isn't declined, so it fits any language.
func _update_name() -> void:
	var player_name := _name.text.strip_edges()
	_apples.text = "loc.apples_named" if player_name else "loc.apples"
	_apples.set_text_argument("name", player_name)


func _update_count() -> void:
	var count := int(_count.value)
	_count_value.text = str(count)
	_apples.set_text_argument("count", count)
	_days.set_text_argument("days", count)
