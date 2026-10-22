@tool
class_name ResponsiveGrid
extends GridContainer

@export_range(100, 1000, 10, "suffix:px") var min_column_width := 380.0:
	set(value):
		min_column_width = value
		_update_columns()
@export_range(1, 6) var max_columns := 2:
	set(value):
		max_columns = value
		_update_columns()


func _notification(what: int) -> void:
	if what == NOTIFICATION_RESIZED or what == NOTIFICATION_READY:
		_update_columns()


func _update_columns() -> void:
	var separation := get_theme_constant(&"h_separation")
	var fit := floori((size.x + separation) / (min_column_width + separation))
	var new_columns := clampi(fit, 1, max_columns)
	if columns != new_columns:
		columns = new_columns
