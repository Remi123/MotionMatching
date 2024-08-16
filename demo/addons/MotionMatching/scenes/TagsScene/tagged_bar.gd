@tool
class_name TaggedBar extends Button

@export var tag : TagInfo :
	get:
		return tag
	set(value):
		tag = value
		notify_property_list_changed() 
@onready var panel: Panel = %Panel

var lib : MMAnimationLibrary
var animname : String
var duration : float

func _ready():
	if tag != null:
		tag.changed.connect(queue_redraw)
		duration = lib.get_animation(tag.animation_name).length
		text = tag.get_class()
	options.index_pressed.connect(_on_option_index)

func _on_option_index(index:int):
	if options.get_item_text(index) == "Delete":
		lib.tags.erase(tag)
		queue_free()
	if options.get_item_text(index) == "Duplicate":
		var new_tag := tag.duplicate()
		lib.tags.append(new_tag)
	
func _draw() -> void:
	if tag != null:
		if duration > 0.0:
			panel.position.x = (tag.timestamp / duration) * size.x
			panel.size.x = (tag.duration / duration) * size.x
		text = tag.get_class()
		if not tag.resource_name.is_empty():
			text += tag.resource_name
	
@onready var options :PopupMenu= %PopupMenu

class EditorTagSelector extends RefCounted:
	@export var tag : TagInfo:
		get:
			return tag
		set(value):
			tag = value
			notify_property_list_changed()
@onready var tag_selector := EditorTagSelector.new()

func _on_gui_input(event: InputEvent) -> void:	
	if event is InputEventMouseButton and event.is_pressed():
		if tag != null:
			EditorInterface.inspect_object(tag)
		else:
			tag_selector.tag = null
			EditorInterface.inspect_object(tag_selector)
			tag_selector.property_list_changed.connect(func():
				tag = tag_selector.tag
				lib.tags.append(tag)
				tag.animation_name = animname
				text = tag.get_class()

				if tag is TagCategory:
					tag.property_hint_string = get_owner().text_category.text
				owner.queue_redraw()
				self.call_deferred("_inspect_tag_deferred")
				,CONNECT_ONE_SHOT)
		match event.button_index:
			MOUSE_BUTTON_LEFT:
				pass
			MOUSE_BUTTON_RIGHT:
				options.visible = true
				options.position = get_global_mouse_position()
				accept_event()
				pass
	pass # Replace with function body.
	
func _inspect_tag_deferred():
	EditorInterface.inspect_object(tag)
