@tool
class_name EventButton extends PanelContainer

@onready var margin_container: ResizeableUI = $MarginContainer


@onready var button: Button = $MarginContainer/Button
@onready var popup_menu: PopupMenu = $PopupMenu

signal duplicate_event
signal delete_event

@export var tag : TagInfo
var animation :Animation = null



func _get_drag_data(at_position: Vector2) -> Variant:
	var cpb = duplicate()
	# Allows us to center the color picker on the mouse
	var preview = Control.new()
	preview.add_child(cpb)
	cpb.position = Vector2(0,-0.5*size.y)
	cpb.size = size

	# Sets what the user will see they are dragging
	set_drag_preview(preview)
	return self





# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	margin_container.ref = self
	button.ref = self

	popup_menu.id_pressed.connect(_on_id_pressed)
	button.pressed.connect(_on_button_pressed)
	pass # Replace with function body.

enum {DUPLICATE = 0, DELETE = 1}

func _on_id_pressed(id:int):
	match id:
		DUPLICATE:
			duplicate_event.emit()
		DELETE:
			delete_event.emit()
			queue_free()

func _on_button_pressed():
	EditorInterface.inspect_object(tag)



