@tool
extends EditorPlugin
## Takes the screenshots of the editor for the README, then quits.
## tools/media.py enables it in a copy of the demo and sets BOXE_CAPTURE_OUT to the output folder.

var _out := OS.get_environment("BOXE_CAPTURE_OUT")


func _enter_tree() -> void:
	if _out:
		_run.call_deferred()


func _run() -> void:
	var window := EditorInterface.get_base_control().get_window()
	window.mode = Window.MODE_WINDOWED
	window.size = Vector2i(1600, 1000)
	# The editor loads the project and its docks during the first frames.
	await _frames(90)
	# A wider inspector and a taller bottom panel, so the screenshots show more.
	for split: SplitContainer in _find_all(EditorInterface.get_base_control(), "SplitContainer"):
		match String(split.name):
			"DockHSplitMain":
				split.set("split_offsets", PackedInt32Array([280, -480]))
			"DockVSplitMain":
				split.set("split_offsets", PackedInt32Array([-340]))
	await _frames(10)
	await _localization_dock()
	await _key_picker()
	EditorInterface.get_base_control().get_tree().quit()


func _localization_dock() -> void:
	var editor := _find_class(EditorInterface.get_base_control(), "LocalizationDock")
	var dock := editor
	while dock and not dock is EditorDock:
		dock = dock.get_parent()
	dock.make_visible()
	await _frames(20)
	await _save("editor_localization_dock", dock)


func _key_picker() -> void:
	EditorInterface.open_scene_from_path("res://sections/text/text.tscn")
	await _frames(30)
	var label := EditorInterface.get_edited_scene_root().get_node("Cards/Arguments/Box/Apples")
	EditorInterface.edit_node(label)
	await _frames(20)
	await _save_inspector("editor_key_picker", 236)


func _save_inspector(shot: String, height: int) -> void:
	var inspector := EditorInterface.get_inspector()
	var rect := inspector.get_global_rect()
	if height > 0:
		rect.size.y = minf(rect.size.y, height)
	await _save_rect(shot, rect)


func _save(shot: String, control: Control) -> void:
	await _save_rect(shot, control.get_global_rect())


func _save_rect(shot: String, rect: Rect2) -> void:
	await RenderingServer.frame_post_draw
	var image := EditorInterface.get_base_control().get_viewport().get_texture().get_image()
	var region := Rect2i(rect).intersection(Rect2i(Vector2i.ZERO, image.get_size()))
	image.get_region(region).save_png(_out.path_join(shot + ".png"))


func _frames(count: int) -> void:
	for i in count:
		await EditorInterface.get_base_control().get_tree().process_frame


func _find_class(root: Node, type: String) -> Node:
	var found := _find_all(root, type)
	return found[0] if found else null


func _find_all(root: Node, type: String) -> Array[Node]:
	var found: Array[Node] = []
	for child in root.get_children(true):
		if child.is_class(type):
			found.append(child)
		found.append_array(_find_all(child, type))
	return found
