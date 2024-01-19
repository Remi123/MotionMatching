@tool
class_name MMPlugin
extends EditorPlugin

var BOTTOMPANEL := preload("res://addons/MotionMatching/controls/MMEditorPanel.tscn")
@onready var bottompanel :MMEditor= BOTTOMPANEL.instantiate()

var last_path := ""

func _enter_tree() -> void:
	pass

func _get_plugin_icon() -> Texture2D:
	return preload("res://addons/MotionMatching/icons/icon_mm.svg")

func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.is_released():
		if last_path != get_editor_interface().get_current_path():
			last_path = get_editor_interface().get_current_path()
			visibility()

	pass

func _exit_tree() -> void:
	remove_control_from_bottom_panel(bottompanel)
	bottompanel.queue_free()
	pass

func _has_main_screen()->bool:
	return false

func visibility() -> void:
	var current_path := get_editor_interface().get_current_path()
	if FileAccess.file_exists(current_path):
		var l = ResourceLoader.load(current_path) # Load what is selected in filesystem
		if l is MMAnimationLibrary:
			prints("Selected MMAL",l.resource_path)
			remove_control_from_bottom_panel(bottompanel)
			add_control_to_bottom_panel(bottompanel,"MotionMatching")
			make_bottom_panel_item_visible(bottompanel)
			bottompanel.library = l as MMAnimationLibrary
	else :
		remove_control_from_bottom_panel(bottompanel)



