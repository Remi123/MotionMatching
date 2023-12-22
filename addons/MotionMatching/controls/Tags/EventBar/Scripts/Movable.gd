@tool
class_name MoveableUI extends Button
var offset := Vector2.ZERO
var ref : EventButton
func _can_drop_data(at_position: Vector2, data: Variant) -> bool:
	if data is MoveableUI and data.ref == ref :
		return true
	if data is ResizeableUI and data.ref == ref :
		return true
	else:
		return false
func _get_drag_data(at_position: Vector2) -> Variant:
	var cpb = ref.duplicate()
	# Allows us to center the color picker on the mouse
	var preview = Control.new()
	preview.add_child(cpb)
	offset = Vector2(-at_position.x,-0.5*size.y)
	cpb.position = offset
	cpb.size = size
	# Sets what the user will see they are dragging
	set_drag_preview(preview)
	return self
func _drop_data(at_position: Vector2, data: Variant) -> void:
	if data is MoveableUI:
		data.ref.position.x += at_position.x + data.offset.x
		data.ref.tag.timestamp = max(0,(data.ref.position.x/ data.ref.get_parent().size.x) * data.ref.animation.length)
		data.ref.tag.duration = min(data.ref.tag.duration,ref.animation.length - data.ref.tag.timestamp)

	elif data is ResizeableUI:
		if data.drag_left == true:
			var diff = (at_position.x)/ data.ref.get_parent().size.x * data.ref.animation.length
			data.ref.tag.duration -= diff
			data.ref.tag.timestamp += diff
		elif data.drag_right == true:
			var diff = (data.ref.size.x - at_position.x)/ data.ref.get_parent().size.x * data.ref.animation.length
			data.ref.tag.duration -= diff
	data.ref.get_parent().queue_redraw()

