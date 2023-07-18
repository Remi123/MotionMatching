@tool
extends EditorPlugin

var bottompanel

const MMEditorGizmoPlugin = preload("res://addons/MotionMatching/MMEditorGizmoPlugin.gd")

var gizmo_plugin := MMEditorGizmoPlugin.new()



func _enter_tree() -> void:
	add_node_3d_gizmo_plugin(gizmo_plugin)
	bottompanel = preload("res://addons/MotionMatching/MMEditorPanel.tscn").instantiate()
	bottompanel.gizmo = gizmo_plugin
	# Initialization of the plugin goes here.

#	_make_visible(false)

	get_editor_interface().get_selection().selection_changed.connect(visibility)
	pass


func _exit_tree() -> void:
	remove_node_3d_gizmo_plugin(gizmo_plugin)
	remove_control_from_bottom_panel(bottompanel)
	if(bottompanel != null):
		bottompanel.free()
	# Clean-up of the plugin goes here.
	pass

func _has_main_screen()->bool:
	return false

func visibility() -> void:
	var nodes :Array= get_editor_interface().get_selection().get_selected_nodes()
	var v = nodes.any(func(x):return x is MotionMatcher)
	bottompanel.visible = v

	if v:
		print(get_tree())

		bottompanel._current = nodes.filter(func (x): return x is MotionMatcher)[0]
		bottompanel._animplayer =bottompanel._current.owner.find_children("*","AnimationPlayer",true,true)[0]
		add_control_to_bottom_panel(bottompanel,"MotionMatching")
		make_bottom_panel_item_visible(bottompanel)
	else :
		remove_control_from_bottom_panel(bottompanel)


