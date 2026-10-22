extends VBoxContainer

const MENU_ITEMS: Array[String] = ["[b]New[/b] game", "Continue  [color=#a1a6ad]autosave 2[/color]", "", "[color=#ff786b]Quit[/color]"]

@onready var _godot: Control = %GodotControls
@onready var _boxe: Control = %BoxeControls


func _ready() -> void:
	_godot.theme = ThemeDB.get_default_theme()
	_fill_menu(_godot.get_node(^"Toggles/Menu").get_popup(), false)
	_fill_menu(_boxe.get_node(^"Toggles/Menu").get_popup(), true)
	_boxe.get_node(^"Rich").meta_clicked.connect(func(meta: Variant) -> void: OS.shell_open(str(meta)))


func _fill_menu(popup: PopupMenu, bbcode: bool) -> void:
	popup.bbcode_enabled = bbcode
	for item in MENU_ITEMS:
		if item.is_empty():
			popup.add_separator()
		else:
			popup.add_item(tr(item if bbcode else _strip_tags(item)))


static func _strip_tags(text: String) -> String:
	var regex := RegEx.create_from_string("\\[/?[a-z_]+(=[^\\]]*)?\\]")
	return regex.sub(text, "", true)
