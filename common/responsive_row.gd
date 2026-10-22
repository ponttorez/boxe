@tool
class_name ResponsiveRow
extends Container
## Places its children side by side with equal widths when they fit, and stacks them otherwise.
## Unlike a BoxContainer, its minimum width is the widest child, so it never makes its parent overflow.

@export_range(0, 64, 1, "suffix:px") var separation := 12:
	set(value):
		separation = value
		queue_sort()


func _notification(what: int) -> void:
	match what:
		NOTIFICATION_SORT_CHILDREN:
			_sort()
		NOTIFICATION_RESIZED:
			# The height depends on whether the children fit side by side.
			update_minimum_size()


func _get_minimum_size() -> Vector2:
	var children := _get_children()
	var result := Vector2()
	var stacked_height := 0.0
	for child in children:
		var child_size := child.get_combined_minimum_size()
		result.x = maxf(result.x, child_size.x)
		result.y = maxf(result.y, child_size.y)
		stacked_height += child_size.y
	if not children.is_empty() and not _fits_in_row(children):
		result.y = stacked_height + separation * (children.size() - 1)
	return result


func _sort() -> void:
	var children := _get_children()
	if children.is_empty():
		return
	var rtl := is_layout_rtl()
	if _fits_in_row(children):
		var width := (size.x - separation * (children.size() - 1)) / children.size()
		for i in children.size():
			var x := i * (width + separation)
			if rtl:
				x = size.x - x - width
			fit_child_in_rect(children[i], Rect2(x, 0.0, width, size.y))
	else:
		var y := 0.0
		for child in children:
			var height := child.get_combined_minimum_size().y
			fit_child_in_rect(child, Rect2(0.0, y, size.x, height))
			y += height + separation


func _fits_in_row(children: Array[Control]) -> bool:
	var widest := 0.0
	for child in children:
		widest = maxf(widest, child.get_combined_minimum_size().x)
	return widest * children.size() + separation * (children.size() - 1) <= size.x


func _get_children() -> Array[Control]:
	var result: Array[Control] = []
	for child in get_children():
		if child is Control and child.visible and not child.top_level:
			result.append(child)
	return result
