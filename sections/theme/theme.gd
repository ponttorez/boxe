extends VBoxContainer

@onready var _bus: OptionButton = %Bus
@onready var _bus_panel: Control = %BusPanel


func _ready() -> void:
	_bus.item_selected.connect(_on_bus_selected)


func _on_bus_selected(index: int) -> void:
	_bus_panel.sound_bus = StringName(_bus.get_item_text(index))
