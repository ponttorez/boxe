extends Control

const NARROW_WIDTH := 860.0
const UI_BUS := &"UI"
const SPEAKER_ICON := preload("res://assets/speak/speaker.svg")
const SPEAKER_MUTED_ICON := preload("res://assets/speak/speaker_muted.svg")
const LANGUAGE_NAMES := {
	"en": "English",
	"ru": "Русский",
	"ar": "العربية",
}

var _section: Control

@onready var _safe_area: MarginContainer = %SafeArea
@onready var _layout: BoxContainer = %Layout
@onready var _sidebar: PanelContainer = %Sidebar
@onready var _nav_scroll: ScrollContainer = %NavScroll
@onready var _nav: BoxContainer = %Nav
@onready var _brand: BoxContainer = %Brand
@onready var _footer: BoxContainer = %Footer
@onready var _language_box: Control = %LanguageBox
@onready var _scroll: ScrollContainer = %Scroll
@onready var _page: MarginContainer = %Page
@onready var _language: OptionButton = %Language
@onready var _volume_box: Control = %VolumeBox
@onready var _volume: HSlider = %Volume
@onready var _mute: Button = %Mute


func _ready() -> void:
	add_to_group(&"app")
	for link: SectionLink in _nav.get_children():
		link.pressed.connect(_show_section.bind(link))
	var args := _get_user_args()
	if args.has("locale"):
		TranslationServer.set_locale(args["locale"])
	_fill_languages()
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
	var link: SectionLink = _nav.get_node_or_null(NodePath(link_name))
	if link:
		link.button_pressed = true
		_show_section(link)


func _show_section(link: SectionLink) -> void:
	if _section:
		_section.queue_free()
	_section = link.section.instantiate()
	_page.add_child(_section)
	_scroll.scroll_vertical = 0
	_nav_scroll.ensure_control_visible(link)

	_section.modulate.a = 0.0
	create_tween().tween_property(_section, ^"modulate:a", 1.0, 0.25)


func _setup_volume() -> void:
	var bus := AudioServer.get_bus_index(UI_BUS)
	_volume.value = db_to_linear(AudioServer.get_bus_volume_db(bus))
	_volume.value_changed.connect(_on_volume_changed)
	_mute.button_pressed = AudioServer.is_bus_mute(bus)
	_mute.toggled.connect(_on_mute_toggled)
	_update_mute_icon()


func _on_volume_changed(value: float) -> void:
	AudioServer.set_bus_volume_db(AudioServer.get_bus_index(UI_BUS), linear_to_db(value))


func _on_mute_toggled(muted: bool) -> void:
	AudioServer.set_bus_mute(AudioServer.get_bus_index(UI_BUS), muted)
	_update_mute_icon()


func _update_mute_icon() -> void:
	_mute.icon = SPEAKER_MUTED_ICON if _mute.button_pressed else SPEAKER_ICON


func _fill_languages() -> void:
	var current := TranslationServer.get_locale()
	var best_score := 0
	for locale in TranslationServer.get_loaded_locales():
		_language.add_item(LANGUAGE_NAMES.get(locale, TranslationServer.get_locale_name(locale)))
		var index := _language.item_count - 1
		_language.set_item_metadata(index, locale)
		var score := TranslationServer.compare_locales(current, locale)
		if score > best_score:
			best_score = score
			_language.select(index)
	_language.item_selected.connect(_on_language_selected)
	_language.get_popup().about_to_popup.connect(_place_language_popup.call_deferred)
	TranslationServer.locale_changed.connect(_on_locale_changed)


func _on_locale_changed() -> void:
	var locale := TranslationServer.get_locale()
	for index in _language.item_count:
		if TranslationServer.compare_locales(locale, _language.get_item_metadata(index)) >= 5:
			_language.select(index)


func _place_language_popup() -> void:
	var popup := _language.get_popup()
	var button := Rect2i(_language.get_screen_transform() * Rect2(Vector2.ZERO, _language.size))
	if Rect2i(popup.position, popup.size).intersects(button):
		popup.position.y = button.position.y - popup.size.y - 4


func _on_language_selected(index: int) -> void:
	TranslationServer.set_locale(_language.get_item_metadata(index))


func get_safe_area() -> int:
	return _safe_area.safe_area


func set_safe_area(mode: int) -> void:
	_safe_area.safe_area = mode as MarginContainer.SafeArea


func _update_layout() -> void:
	var narrow := size.x < NARROW_WIDTH
	_layout.vertical = narrow
	_nav.vertical = not narrow
	_nav_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_SHOW_NEVER if narrow else ScrollContainer.SCROLL_MODE_DISABLED
	_nav_scroll.vertical_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED if narrow else ScrollContainer.SCROLL_MODE_AUTO
	_nav_scroll.size_flags_vertical = Control.SIZE_FILL if narrow else Control.SIZE_EXPAND_FILL
	_sidebar.custom_minimum_size.x = 0.0 if narrow else 240.0
	_footer.visible = not narrow
	var parent: BoxContainer = _brand if narrow else _footer
	for control: Control in [_volume_box, _language_box]:
		if control.get_parent() != parent:
			control.reparent(parent)
			parent.move_child(control, -1)
	_volume.visible = not narrow
	var margin := 16 if narrow else 36
	for side in ["left", "right"]:
		_page.add_theme_constant_override("margin_" + side, margin)
	_page.add_theme_constant_override("margin_top", 16 if narrow else 32)
