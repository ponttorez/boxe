extends Chapter
## Stock Godot puts the number into one text, boxe picks the plural form of each language.

const LOCALES: Array[String] = ["en", "ru"]
const LINES: Array[String] = ["English", "Russian"]
const NAIVE: Array[String] = ["You have %d apples.", "У тебя %d яблок."]

var _translations: Array[Translation] = []


func prepare() -> void:
	super()
	for locale in LOCALES:
		_translations.append(load("res://localization/translations.%s.translation" % locale))
	_show(1)


func play() -> void:
	for count in [2, 5, 21, 1]:
		await wait(0.85)
		_show(count)
	await wait(0.6)


func _show(count: int) -> void:
	for side: Control in [godot_stage, boxe_stage]:
		side.find_child("Count", true, false).text = "count = %d" % count
	for i in LOCALES.size():
		in_godot(LINES[i]).text = NAIVE[i] % count
		var text := String(_translations[i].get_message("loc.apples"))
		in_boxe(LINES[i]).text = TranslationServer.format_arguments(text, { "count": count }, LOCALES[i])
