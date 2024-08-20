@tool
class_name MMEditorPlugin
extends EditorPlugin

signal lib_changed(lib:MMAnimationLibrary)

var preview_scene_path := "res://addons/MotionMatching/scenes/main_scene/MMPreviewScene.tscn"
var preview_scene : MMPreviewSkeleton
const MM_BOTTOM_SCENE = preload("res://addons/MotionMatching/scenes/bottom_scene/MM_BottomScene.tscn")
var bottom_panel_instance : MMBottonScene

var gizmo_skeleton :MMEditorGizmoSkeletonPlugin

var current_lib : MMAnimationLibrary = null:
	set(value):
		if value && current_lib != value :
			current_lib = value
			prints("Lib changed")
			lib_changed.emit(current_lib)
			
	get:
		return current_lib
		



func _get_plugin_icon() -> Texture2D:
	return preload("res://addons/MotionMatching/icons/icon_mm.svg")

func _enter_tree() -> void:
	prints("MotionMatching Plugin Enter tree")
	#	Gizmo
	gizmo_skeleton = MMEditorGizmoSkeletonPlugin.new(self)
	add_node_3d_gizmo_plugin(gizmo_skeleton)
	# 	PreviewScene	
	var found := EditorInterface.get_open_scenes().find(preview_scene_path)
	if found < 0:
		EditorInterface.open_scene_from_path(preview_scene_path)
		preview_scene = EditorInterface.get_edited_scene_root() as MMPreviewSkeleton
	
	#	BOTTOM PANEL
	bottom_panel_instance = MM_BOTTOM_SCENE.instantiate()
	add_control_to_bottom_panel(bottom_panel_instance,"MotionMatching")
	
	
	
	# Hide the main panel.
	_make_visible(false)
	
	# Links
	lib_changed.connect(bottom_panel_instance.on_lib_selected)
	lib_changed.connect(func(lib:MMAnimationLibrary):preview_scene.set_bones(lib.skeleton_profile))
	
	
	#lib_changed.connect(main_panel_instance.on_lib_selected)
	#bottom_panel_instance.pose_selected.connect(main_panel_instance.on_pose_selected)
	#bottom_panel_instance.pose_selected.connect(func(l,a,t): gizmo_skeleton.please_redraw.emit())
	bottom_panel_instance.pose_selected.connect(gizmo_skeleton.on_pose_selected.emit)

func _exit_tree() -> void:
	prints("MotionMatching Plugin Exiting tree")
	
	#if main_panel_instance:
		#EditorInterface.get_editor_main_screen().remove_child(main_panel_instance)
		#main_panel_instance.queue_free()
	if bottom_panel_instance:
		remove_control_from_bottom_panel(bottom_panel_instance)
		bottom_panel_instance.queue_free()
	if gizmo_skeleton:
		remove_node_3d_gizmo_plugin(gizmo_skeleton)
	#EditorInterface.remo(preview_scene_path)
	pass

func _has_main_screen() -> bool:
	return false


func _make_visible(visible: bool) -> void:
	if bottom_panel_instance:
		if visible:			
			EditorInterface.open_scene_from_path(preview_scene_path)
			preview_scene = EditorInterface.get_edited_scene_root() as MMPreviewSkeleton
			#main_panel_instance.show()
			EditorInterface.set_main_screen_editor("3D")
			bottom_panel_instance.show()
			make_bottom_panel_item_visible(bottom_panel_instance)			
		else:
			#main_panel_instance.hide()
			hide_bottom_panel()
			bottom_panel_instance.hide()


func _get_plugin_name() -> String:
	return "MotionMatching"


func _edit(object: Object) -> void:
	current_lib = object as MMAnimationLibrary
	

func _handles(obj: Object) -> bool:
	return obj is MMAnimationLibrary
