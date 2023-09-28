@tool
extends EditorPlugin

var bottompanel :MMEditorPanel= preload("res://addons/MotionMatching/MMEditorPanel.tscn").instantiate()

# const MMEditorGizmoPlugin :MMEditorGizmoPlugin= preload("res://addons/MotionMatching/MMEditorGizmoPlugin.gd")

# var gizmo_plugin := MMEditorGizmoPlugin.new()

var last_path := ""
var al :MMAnimationLibrary

func _enter_tree() -> void:
	pass

func _get_plugin_icon() -> Texture2D:
	return preload("res://addons/MotionMatching/jump-icon.svg")

func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.is_released():
		if last_path != get_editor_interface().get_current_path():
			last_path = get_editor_interface().get_current_path()
			remove_control_from_bottom_panel(bottompanel)
			visibility()

	pass

func _exit_tree() -> void:
	remove_control_from_bottom_panel(bottompanel)
	bottompanel.queue_free()
	pass

func _has_main_screen()->bool:
	return false

func visibility() -> void:

	var nodes :Array= get_editor_interface().get_selection().get_selected_nodes()
	var v = nodes.any(func(x):return x is MotionMatcher)
	var l = ResourceLoader.load(get_editor_interface().get_current_path())
	if l is MMAnimationLibrary:
		prints("Selected MMAL",l.resource_path)
		al = l
		bottompanel._current = al
		bottompanel.plugin_ref = self

		add_control_to_bottom_panel(bottompanel,"MotionMatching")
		make_bottom_panel_item_visible(bottompanel)
		bottompanel.update_info()
	else :
		remove_control_from_bottom_panel(bottompanel)



