extends VBoxContainer

const MENU_ITEMS: Array[String] = ["[b]New[/b] game", "Continue  [color=#a1a6ad]autosave 2[/color]", "", "[color=#ff786b]Quit[/color]"]
const TREE_ITEMS := {
	"[b]Inbox[/b]  [color=#a1a6ad](2)[/color]": ["Welcome to [i]boxe[/i]", "Release [color=#5ed58b]notes[/color]"],
	"[b]Archive[/b]": ["Old [s]drafts[/s]"],
}


func _ready() -> void:
	for controls: Control in [%NormalControls, %PixelControls]:
		_fill_menu(controls.get_node(^"Pickers/Menu").get_popup())
		_fill_tree(controls.get_node(^"Tree"))
		controls.get_node(^"Rich").meta_clicked.connect(func(meta: Variant) -> void: OS.shell_open(str(meta)))


func _fill_menu(popup: PopupMenu) -> void:
	popup.bbcode_enabled = true
	for item in MENU_ITEMS:
		if item.is_empty():
			popup.add_separator()
		else:
			popup.add_item(tr(item))


func _fill_tree(tree: Tree) -> void:
	var root := tree.create_item()
	for folder: String in TREE_ITEMS:
		var parent := tree.create_item(root)
		parent.set_text(0, tr(folder))
		for child: String in TREE_ITEMS[folder]:
			tree.create_item(parent).set_text(0, tr(child))
