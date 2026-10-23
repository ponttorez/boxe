extends Control

const NARROW_WIDTH := 860.0
const UI_BUS := &"UI"
const SOUND_ICON := preload("res://assets/icons/sound.svg")
const SOUND_MUTED_ICON := preload("res://assets/icons/sound_muted.svg")
const LANGUAGE_NAMES := {
	"en": "English",
	"ru": "Русский",
	"ar": "العربية",
}

var _section: Control
var _links: Array[SectionLink] = []

@onready var _safe_area: MarginContainer = %SafeArea
@onready var _nav_scroll: ScrollContainer = %NavScroll
@onready var _nav: Container = %Nav
@onready var _scroll: ScrollContainer = %Scroll
@onready var _page: MarginContainer = %Page
@onready var _sound: Button = %Sound
@onready var _sound_popup: PopupPanel = %SoundPopup
@onready var _volume: VSlider = %Volume
@onready var _mute: Button = %Mute
@onready var _language: MenuButton = %Language


func _ready() -> void:
	add_to_group(&"app")
	for link in find_children("*", "SectionLink", true, false):
		_links.append(link)
		link.pressed.connect(_show_section.bind(link))
	var args := _get_user_args()
	if args.has("locale"):
		TranslationServer.set_locale(args["locale"])
	_setup_languages()
	_setup_volume()
	resized.connect(_update_layout)
	_update_layout()
	open_section(args.get("section", _nav.get_child(0).name))


func _get_user_args() -> Dictionary:
	var args := {}
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--") and arg.contains("="):
			args[arg.trim_prefix("--").get_slice("=", 0)] = arg.get_slice("=", 1)
	return args


func get_section() -> Control:
	return _section


func open_section(link_name: StringName) -> void:
	for link in _links:
		if link.name == link_name:
			link.button_pressed = true
			_show_section(link)
			return


func _show_section(link: SectionLink) -> void:
	if _section:
		_section.queue_free()
	_section = link.section.instantiate()
	_page.add_child(_section)
	_scroll.scroll_vertical = 0
	if link.get_parent() == _nav:
		_nav_scroll.ensure_control_visible(link)

	_section.modulate.a = 0.0
	create_tween().tween_property(_section, ^"modulate:a", 1.0, 0.25)


func _setup_volume() -> void:
	var bus := AudioServer.get_bus_index(UI_BUS)
	_volume.value = db_to_linear(AudioServer.get_bus_volume_db(bus))
	_volume.value_changed.connect(_on_volume_changed)
	_mute.button_pressed = AudioServer.is_bus_mute(bus)
	_mute.toggled.connect(_on_mute_toggled)
	_sound.pressed.connect(_show_sound_popup)
	_update_sound_icons()


func _show_sound_popup() -> void:
	var button := Rect2(_sound.get_screen_transform() * Rect2(Vector2.ZERO, _sound.size))
	_sound_popup.reset_size()
	var popup_size := _sound_popup.get_contents_minimum_size()
	_sound_popup.popup(Rect2i(Vector2i(int(button.get_center().x - popup_size.x * 0.5), int(button.end.y + 4)), Vector2i(popup_size)))


func _on_volume_changed(value: float) -> void:
	AudioServer.set_bus_volume_db(AudioServer.get_bus_index(UI_BUS), linear_to_db(value))


func _on_mute_toggled(muted: bool) -> void:
	AudioServer.set_bus_mute(AudioServer.get_bus_index(UI_BUS), muted)
	_update_sound_icons()


func _update_sound_icons() -> void:
	var icon := SOUND_MUTED_ICON if _mute.button_pressed else SOUND_ICON
	_mute.icon = icon
	_sound.icon = icon


func _setup_languages() -> void:
	var popup := _language.get_popup()
	for locale in TranslationServer.get_loaded_locales():
		popup.add_radio_check_item(LANGUAGE_NAMES.get(locale, TranslationServer.get_locale_name(locale)))
		popup.set_item_metadata(popup.item_count - 1, locale)
	popup.auto_translate_mode = Node.AUTO_TRANSLATE_MODE_DISABLED
	popup.id_pressed.connect(_on_language_pressed)
	TranslationServer.locale_changed.connect(_update_languages)
	_update_languages()


func _update_languages() -> void:
	var popup := _language.get_popup()
	var locale := TranslationServer.get_locale()
	for index in popup.item_count:
		popup.set_item_checked(index, TranslationServer.compare_locales(locale, popup.get_item_metadata(index)) >= 5)


func _on_language_pressed(id: int) -> void:
	var popup := _language.get_popup()
	TranslationServer.set_locale(popup.get_item_metadata(popup.get_item_index(id)))


func get_safe_area() -> int:
	return _safe_area.safe_area


func set_safe_area(mode: int) -> void:
	_safe_area.safe_area = mode as MarginContainer.SafeArea


func _update_layout() -> void:
	var narrow := size.x < NARROW_WIDTH
	var margin := 16 if narrow else 36
	for side in ["left", "right"]:
		_page.add_theme_constant_override("margin_" + side, margin)
	_page.add_theme_constant_override("margin_top", 16 if narrow else 32)
