@tool
class_name EventButton extends PanelContainer
@onready var margin_container: MarginContainer = $MarginContainer

@onready var button: Button = $MarginContainer/Button
@onready var popup_menu: PopupMenu = $PopupMenu

signal duplicate_event
signal delete_event

@export var tag : AnimTag
var animation :Animation = null

class MoveableUI extends Control:
	var ref : EventButton
	func _can_drop_data(at_position: Vector2, data: Variant) -> bool:
		if data is EventButton.MoveableUI and data.ref == ref :
			return true
		if data is EventButton.ResizeableUI and data.ref == ref :
			return true
		else:
			return false
	func _get_drag_data(at_position: Vector2) -> Variant:
		var cpb = ref.duplicate()
		# Allows us to center the color picker on the mouse
		var preview = Control.new()
		preview.add_child(cpb)
		cpb.position = Vector2(0,-0.5*size.y)
		cpb.size = size
		# Sets what the user will see they are dragging
		set_drag_preview(preview)
		return self
	func _drop_data(at_position: Vector2, data: Variant) -> void:
		if data is EventButton.MoveableUI:
			data.ref.position.x += at_position.x
			data.ref.tag.timestamp = (data.ref.position.x/ data.ref.size.x) * data.ref.animation.length
		elif data is EventButton.ResizeableUI:
			if data.drag_left == true:
				var diff = (at_position.x)/ data.ref.get_parent().size.x * data.ref.animation.length
				data.ref.tag.duration -= diff
				data.ref.tag.timestamp += diff
			elif data.drag_right == true:
				var diff = (data.ref.size.x - at_position.x)/ data.ref.get_parent().size.x * data.ref.animation.length
				data.ref.tag.duration -= diff
			data.ref.get_parent().queue_redraw()


class ResizeableUI extends MarginContainer:
	var ref : EventButton
	var drag_left := false
	var drag_right := false
	func _get_drag_data(at_position: Vector2) -> Variant:
		prints("GHello")
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


func _get_drag_data(at_position: Vector2) -> Variant:
	prints("Preview drag")
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
	margin_container.set_script(ResizeableUI)
	button.set_script(MoveableUI)
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



