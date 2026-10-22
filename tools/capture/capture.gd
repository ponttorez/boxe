extends Node
## Plays a recording scenario of the demo, see scenarios.gd and tools/media.py.
## [codeblock]
## godot --path demo res://tools/capture/capture.tscn --write-movie frames/frame.png -- --scenario=transitions --out=media
## [/codeblock]
## The frames written before the scenario starts are listed in <out>/<scenario>.json, so they can be cut.

const MAIN := preload("res://main.tscn")

var cursor: FakeCursor
var app: Control
var out_dir: String

var _scenario: String
var _first_frame := 0
var _start_frame := 0
var _crop := Rect2()


func _ready() -> void:
	var args := {}
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--") and arg.contains("="):
			args[arg.trim_prefix("--").get_slice("=", 0)] = arg.get_slice("=", 1)
	_first_frame = Engine.get_frames_drawn()
	_scenario = args.get("scenario", "")
	out_dir = args.get("out", "user://captures")
	DirAccess.make_dir_recursive_absolute(out_dir)

	cursor = FakeCursor.new()
	add_child(cursor)
	var scenarios := CaptureScenarios.new(self)
	if not scenarios.has_method(_scenario):
		push_error("Unknown scenario: %s" % _scenario)
		get_tree().quit(1)
		return
	await scenarios.call(_scenario)
	_write_info()
	get_tree().quit()


## Adds the demo with a section open. Sections and the locale can also be set with --section and --locale.
func open_app(section := "") -> void:
	app = MAIN.instantiate()
	add_child(app)
	move_child(cursor, -1)
	if section:
		app.open_section(section)
	await wait(0.5)


## Adds any scene, e.g. the motion design scene.
func open_scene(scene: PackedScene) -> Node:
	var node := scene.instantiate()
	add_child(node)
	move_child(cursor, -1)
	return node


## Returns a node of the open section, e.g. "%Lock" or "Cards/Easing".
func find(path: NodePath) -> Node:
	return app.get_section().get_node(path)


## Crops the GIF to the given controls, with a margin around them.
func focus(controls: Array, margin := 20.0) -> void:
	var rect := Rect2()
	for control: Control in controls:
		rect = control.get_global_rect() if not rect.has_area() else rect.merge(control.get_global_rect())
	_crop = rect.grow(margin).intersection(get_viewport().get_visible_rect())


## Marks the start of the recording. The frames before it are cut from the GIF.
func begin() -> void:
	await RenderingServer.frame_post_draw
	_start_frame = Engine.get_frames_drawn()


func wait(seconds: float) -> void:
	await get_tree().create_timer(seconds).timeout


## Saves the current frame as <out>/<name>.png.
func still(name: String) -> void:
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(out_dir.path_join(name + ".png"))


func _write_info() -> void:
	var file := FileAccess.open(out_dir.path_join(_scenario + ".json"), FileAccess.WRITE)
	# Frame numbers of the movie, which starts with the first frame of this scene.
	var info := {
		"start_frame": _start_frame - _first_frame,
		"end_frame": Engine.get_frames_drawn() - _first_frame,
	}
	if _crop.has_area():
		# Even sizes, as video encoders expect.
		info["crop"] = [int(_crop.position.x), int(_crop.position.y), int(_crop.size.x) / 2 * 2, int(_crop.size.y) / 2 * 2]
	file.store_string(JSON.stringify(info))
