@tool
class_name ResizeableUI extends MarginContainer

var ref : EventButton
var drag_left := false
var drag_right := false
func _get_drag_data(at_position: Vector2) -> Variant:
	return self
func _notification(what: int) -> void:
	match what:
		NOTIFICATION_DRAG_BEGIN:
			if (get_local_mouse_position().x - size.x/2) > 0:
				drag_right = true
			else:
				drag_left = true
		NOTIFICATION_DRAG_END:
			drag_left = false
			drag_right = false
