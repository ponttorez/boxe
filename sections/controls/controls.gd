extends VBoxContainer

const MENU_ITEMS: Array[String] = ["[b]New[/b] game", "Continue  [color=#a1a6ad]autosave 2[/color]", "", "[color=#ff786b]Quit[/color]"]
const TREE_ITEMS := {
	"[b]Inbox[/b]  [color=#a1a6ad](2)[/color]": ["Welcome to [i]boxe[/i]", "Release [color=#5ed58b]notes[/color]"],
	"[b]Archive[/b]": ["Old [s]drafts[/s]"],
}


func _ready() -> void:
	_fill_menu(%Menu.get_popup())
	_fill_tree(%Tree)
	%OpenDialog.pressed.connect(%Dialog.popup_centered)


func _fill_menu(popup: PopupMenu) -> void:
	popup.bbcode_enabled = true
	for item in MENU_ITEMS:
		if item.is_empty():
			popup.add_separator()
		else:
			popup.add_item(tr(item))
	popup.add_separator(tr("Options"))
	popup.add_check_item(tr("Autosave"))
	popup.set_item_checked(popup.item_count - 1, true)


func _fill_tree(tree: Tree) -> void:
	var root := tree.create_item()
	for folder: String in TREE_ITEMS:
		var parent := tree.create_item(root)
		parent.set_text(0, tr(folder))
		for child: String in TREE_ITEMS[folder]:
			tree.create_item(parent).set_text(0, tr(child))
	var settings := tree.create_item(root)
	settings.set_text(0, tr("[b]Settings[/b]"))
	var check := tree.create_item(settings)
	check.set_cell_mode(0, TreeItem.CELL_MODE_CHECK)
	check.set_editable(0, true)
	check.set_checked(0, true)
	check.set_text(0, tr("Notifications"))
	var volume := tree.create_item(settings)
	volume.set_cell_mode(0, TreeItem.CELL_MODE_RANGE)
	volume.set_editable(0, true)
	volume.set_range_config(0, 0.0, 10.0, 1.0)
	volume.set_range(0, 7.0)
