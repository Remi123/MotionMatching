@tool
class_name EventButton extends PanelContainer

@onready var margin_container: ResizeableUI = $MarginContainer


@onready var button: Button = $MarginContainer/Button
@onready var popup_menu: PopupMenu = $PopupMenu


signal duplicate_event
signal delete_event

@export var tag : TagInfo
var animation :Animation = null

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	margin_container.ref = self
	button.ref = self



func _button_gui_input(event: InputEvent) -> void:
	var mouse := event as InputEventMouseButton
	if mouse != null and mouse.button_index == MOUSE_BUTTON_RIGHT:
		popup_menu.popup(Rect2i(get_global_mouse_position(),popup_menu.size))
	elif mouse != null and mouse.button_index == MOUSE_BUTTON_LEFT:
		EditorInterface.inspect_object(tag)


enum {DELETE = 1}
func _on_popup_menu_id_pressed(id: int) -> void:
	match id:
		DELETE:
			delete_event.emit(tag)
			queue_free()


func _on_button_pressed_all() -> void:
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		EditorInterface.inspect_object(tag)
	elif Input.is_mouse_button_pressed(MOUSE_BUTTON_RIGHT):
		popup_menu.popup(Rect2i(get_global_mouse_position(),popup_menu.size))

	pass # Replace with function body.
