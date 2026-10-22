@tool
extends Control
## A phone with a camera cutout and a status bar, drawing a small app laid out like MarginContainer.safe_area does.

const PHONE_SIZE := Vector2(118.0, 220.0)
## The part of the screen covered by the status bar and the cutout.
const UNSAFE_HEIGHT := 18.0
const HEADER_HEIGHT := 34.0
const ACCENT := Color(0.2784, 0.549, 0.749)

@export_enum("None", "Inside", "Outside") var mode := 0:
	set(value):
		mode = value
		queue_redraw()

## Outlines the phone, for the mode used by the demo.
@export var highlighted := false:
	set(value):
		highlighted = value
		queue_redraw()


func _get_minimum_size() -> Vector2:
	return PHONE_SIZE


func _draw() -> void:
	var body := Rect2(Vector2((size.x - PHONE_SIZE.x) * 0.5, 0.0), PHONE_SIZE)
	_draw_rounded(body, Color(0.07, 0.08, 0.1), 20.0, ACCENT if highlighted else Color(0.25, 0.27, 0.31), 2.0)
	var screen := body.grow(-6.0)
	_draw_rounded(screen, Color(0.16, 0.18, 0.21), 15.0)

	# The header bar of the app, with its title.
	var header := Rect2(screen.position, Vector2(screen.size.x, HEADER_HEIGHT))
	var title_top := screen.position.y
	match mode:
		1: # Inside: below the unsafe part.
			header.position.y += UNSAFE_HEIGHT
			title_top += UNSAFE_HEIGHT
		2: # Outside: the background goes under the status bar, the title stays below it.
			header.size.y += UNSAFE_HEIGHT
			title_top += UNSAFE_HEIGHT
	_draw_rounded(header, ACCENT, 15.0 if header.position.y == screen.position.y else 0.0, Color.TRANSPARENT, 0.0, true)
	var font := get_theme_default_font()
	draw_string(font, Vector2(screen.position.x + 10.0, title_top + 22.0), "boxe", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color.WHITE)

	# The content of the app.
	var row := Rect2(screen.position.x + 8.0, header.end.y + 8.0, screen.size.x - 16.0, 22.0)
	for i in 4:
		_draw_rounded(row, Color(1.0, 1.0, 1.0, 0.07), 6.0)
		row.position.y += 30.0

	# The status bar and the cutout, over the app.
	var status_y := screen.position.y + 12.0
	draw_string(font, Vector2(screen.position.x + 12.0, status_y + 1.0), "9:41", HORIZONTAL_ALIGNMENT_LEFT, -1, 8, Color(1, 1, 1, 0.85))
	draw_rect(Rect2(screen.end.x - 24.0, status_y - 6.0, 12.0, 6.0), Color(1, 1, 1, 0.85))
	_draw_rounded(Rect2(screen.get_center().x - 14.0, screen.position.y + 4.0, 28.0, 10.0), Color.BLACK, 5.0)


func _draw_rounded(rect: Rect2, color: Color, radius: float, border := Color.TRANSPARENT, border_width := 0.0, top_only := false) -> void:
	var style := StyleBoxFlat.new()
	style.bg_color = color
	style.set_corner_radius_all(int(radius))
	if top_only:
		style.corner_radius_bottom_left = 0
		style.corner_radius_bottom_right = 0
	if border_width > 0.0:
		style.border_color = border
		style.set_border_width_all(int(border_width))
		style.draw_center = true
	draw_style_box(style, rect)
