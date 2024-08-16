@tool
class_name TagsPanel extends MarginContainer

var _lib : MMAnimationLibrary
var _anim : Animation
var _animname

@onready var text_category: LineEdit = %TextCategory

@onready var tracklist: VBoxContainer = %Tracklist
const TAGGED_BAR = preload("res://addons/MotionMatching/scenes/TagsScene/tagged_bar.tscn")

func _on_lib_changed(lib:MMAnimationLibrary):
	_lib = lib
	_animname = _lib.get_animation_list()[0]
	text_category.text = _lib.category_hint_string
	queue_redraw()
	
func _on_anim_changed(lib:MMAnimationLibrary,animname:String):
	_lib = lib
	text_category.text = _lib.category_hint_string
	_anim = _lib.get_animation(animname)
	_animname = animname
	queue_redraw()
	
func _draw() -> void:	
	for previous_tags in tracklist.get_children():
		tracklist.remove_child(previous_tags)
		previous_tags.queue_free()
		
	if _lib == null:		
		return
	elif _anim == null:		
		return;
		
	var animtags :Array[TagInfo]= _lib.tags.filter(func (x:TagInfo):
		return x.animation_name == _animname
		)
	
	for tag in animtags:
		var new_tag :TaggedBar= TAGGED_BAR.instantiate()
		new_tag.lib = _lib
		new_tag.tag = tag
		new_tag.duration = _lib.get_animation(_animname).length
		
		tracklist.add_child(new_tag)
		new_tag.set_owner(tracklist)
			
		


func _on_text_category_text_submitted(new_text: String) -> void:
	if _lib != null:
		_lib.category_hint_string = new_text # will update the rest.
		_lib.emit_changed()
		ResourceSaver.save(_lib)
	pass # Replace with function body.

@onready var options := %PopupMenu
func _on_scroll_container_gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.is_pressed():
		match event.button_index:
			MOUSE_BUTTON_LEFT:
				pass
			MOUSE_BUTTON_RIGHT:
				options.visible = true
				options.position = get_global_mouse_position()
				accept_event()
				pass
	pass # Replace with function body.

class EditorTagSelector extends RefCounted:
	@export var tag : TagInfo
	

func _on_popup_menu_index_pressed(index: int) -> void:
	if options.get_item_text(index) == "Add New Tag":
		var new_tag :TaggedBar= TAGGED_BAR.instantiate()
		new_tag.lib = _lib
		new_tag.animname = _animname
		new_tag.duration = _lib.get_animation(_animname).length
		
		tracklist.add_child(new_tag)
		new_tag.set_owner(self)
		var tag_selector := EditorTagSelector.new()
	pass # Replace with function body.
