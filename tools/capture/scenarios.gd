class_name CaptureScenarios
extends RefCounted
## Recording scenarios of the demo, one method each. tools/media.py lists the window size of each one.
## Each scenario opens what it shows, calls begin() where the GIF starts and saves stills with still().

var c: Node
var cursor: FakeCursor


func _init(runner: Node) -> void:
	c = runner
	cursor = runner.cursor


func motion() -> void:
	var video: Control = c.open_scene(load("res://tools/motion/motion.tscn"))
	cursor.visible = false
	await c.begin()
	await video.play()


func overview() -> void:
	await c.open_app("Library")
	await c.begin()
	await cursor.move_to(Vector2(960, 450), 0.0)
	await cursor.move_to_control(c.find("%Save"), 0.6)
	await c.wait(0.5)
	await cursor.click(0.25)
	await c.wait(0.5)
	await c.still("overview")
	await c.wait(1.0)


func transitions() -> void:
	await c.open_app("Other")
	await c.reveal(c.find("Behavior/Easing"))
	await c.still("transitions")
	c.focus([c.find("Behavior/Easing")])
	await c.begin()
	await cursor.move_to(Vector2(960, 450), 0.0)
	# Flip all the switches, then flip them back midway.
	await cursor.move_to_control(c.find("%All"), 0.5)
	await cursor.click()
	await c.wait(1.2)
	await cursor.click()
	await c.wait(0.3)
	await cursor.click()
	await c.wait(1.2)


func layered() -> void:
	await c.open_app("Theme")
	await c.still("layered")
	c.focus([c.find("Cards/Layered")])
	await c.begin()
	await cursor.move_to(Vector2(900, 390), 0.0)
	for card in ["Raised", "Glass", "Neon", "Pixel", "Skewed", "Ring"]:
		var button: Control = c.find("Cards/Layered/Box/Styles/%s/Button" % card)
		await cursor.move_to_control(button, 0.45)
		await c.wait(0.35)
		await cursor.click(0.3)
		await c.wait(0.35)
	await cursor.move_to(cursor.cursor_position + Vector2(0, 120), 0.3)
	await c.wait(0.8)


func sounds() -> void:
	await c.open_app("Other")
	await c.reveal(c.find("Behavior/Sounds"))
	await c.still("sounds")


func interactable() -> void:
	await c.open_app("Other")
	c.focus([c.find("Input/Lock")])
	await c.begin()
	await cursor.move_to(Vector2(750, 450), 0.0)
	await cursor.move_to_control(c.find("%Lock"), 0.6, Vector2(0.9, 0.5))
	await cursor.click()
	await c.wait(0.5)
	await cursor.move_to_control(c.find("%Form").get_node(^"Save"), 0.6)
	await c.still("interactable")
	await cursor.click()
	await c.wait(0.5)
	# Unlocked again, saving locks the form until it's done.
	await cursor.move_to_control(c.find("%Lock"), 0.6, Vector2(0.9, 0.5))
	await cursor.click()
	await c.wait(0.4)
	await cursor.move_to_control(c.find("%Form").get_node(^"Save"), 0.6)
	await cursor.click()
	await c.wait(0.6)
	await cursor.click()
	await c.wait(1.6)


func hit_areas() -> void:
	await c.open_app("Other")
	await c.still("hit_areas")
	c.focus([c.find("Input/HitAreas")])
	await c.begin()
	await cursor.move_to(Vector2(750, 675), 0.0)
	# Tap 9 px next to the close button, inside its hit area.
	var close: Control = c.find("Input/HitAreas/Box/Stage/Content/Margin/Row/Close")
	await cursor.move_to_control(close, 0.7, Vector2(1.4, 1.2))
	await cursor.click()
	await c.wait(0.8)
	var tile: Control = c.find("Input/HitAreas/Box/Stage/Content/Margin/Row/Tile")
	await cursor.move_to_control(tile, 0.7, Vector2(0.04, 0.5))
	await c.wait(0.6)
	await cursor.move_to_control(tile, 0.6, Vector2(0.5, 0.5))
	await c.wait(0.9)


func composites() -> void:
	await c.open_app("Other")
	await c.reveal(c.find("OpacityRow"))
	await c.still("composites")
	c.focus([c.find("OpacityRow"), c.find("Composites/Group"), c.find("Composites/Text")])
	await c.begin()
	var slider: HSlider = c.find("%Opacity")
	var rect := slider.get_global_rect()
	var at := func(value: float) -> Vector2:
		return Vector2(rect.position.x + rect.size.x * value, rect.get_center().y)
	await cursor.move_to(at.call(0.7) + Vector2(0, 180), 0.0)
	await cursor.move_to(at.call(slider.value), 0.5)
	await cursor.drag_to(at.call(0.12), 1.4)
	await c.wait(0.5)
	await cursor.drag_to(at.call(1.0), 1.4)
	await c.wait(0.4)
	await cursor.drag_to(at.call(0.4), 1.0)
	await c.wait(0.8)


func bbcode() -> void:
	await c.open_app("Text")
	await c.wait(3.5)
	await c.still("bbcode")
	c.focus([c.find("Cards/Label"), c.find("Cards/Ellipsis")])
	await c.begin()
	await cursor.move_to(Vector2(600, 900), 0.0)
	await cursor.move_to_control(c.find("%Replay"), 0.5)
	await cursor.click()
	await c.wait(4.0)


func localization() -> void:
	await c.open_app("Text")
	c.focus([c.find("Cards/Arguments")])
	await c.begin()
	await cursor.move_to(Vector2(1050, 600), 0.0)
	var count: HSlider = c.find("%Count")
	for locale in ["en", "ru"]:
		TranslationServer.set_locale(locale)
		await c.wait(0.4)
		var rect := count.get_global_rect()
		var grab := rect.position + Vector2(rect.size.x * count.value / count.max_value, rect.size.y * 0.5)
		await cursor.move_to(grab, 0.5)
		var target: int = {"en": 1, "ru": 5}[locale]
		var x: float = rect.size.x * target / count.max_value
		await cursor.drag_to(Vector2(rect.position.x + x, grab.y), 0.8)
		await c.wait(0.6)
	await c.still("localization")
	TranslationServer.set_locale("en")
	await c.wait(0.8)


func mobile() -> void:
	await c.open_app("Mobile")
	await c.still("mobile")


func phone() -> void:
	await c.open_app("Library")
	await c.wait(1.0)
	await c.still("phone")
