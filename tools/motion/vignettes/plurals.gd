extends Vignette
## A label shows "1 apples" unless code picks the forms. boxe labels pick the plural form of each language.

const LOCALES := ["en", "ru", "ar"]
const LINES := ["English", "Russian", "Arabic"]
# What a label shows when the number is put into one text.
const NAIVE := ["You have %d apples.", "У тебя %d яблок.", "لديك %d تفاحات."]


func prepare() -> void:
	super()
	godot_cursor.visible = false
	boxe_cursor.visible = false
	_show(1)


func play() -> void:
	for count in [1, 2, 5, 21]:
		_show(count)
		await wait(0.85)


func _show(count: int) -> void:
	for side in [godot_stage, boxe_stage]:
		side.find_child("Count", true, false).text = "count = %d" % count
	for i in LOCALES.size():
		in_godot(LINES[i]).text = NAIVE[i] % count
		var translation: Translation = load("res://localization/demo.%s.translation" % LOCALES[i])
		var text := String(translation.get_message("loc.apples"))
		in_boxe(LINES[i]).text = TranslationServer.format_arguments(text, { "count": count }, LOCALES[i])
