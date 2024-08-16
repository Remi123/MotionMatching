@tool
class_name MMScene extends VBoxContainer

var gizmoplugin := EditorNode3DGizmoPlugin.new()

func _ready() -> void:
	pass
	
func on_pose_selected(lib:MMAnimationLibrary,animname : String, time:float):
	prints("Showing Poses",animname,time)
	
func on_lib_selected(lib:MMAnimationLibrary):
	on_pose_selected.call(lib,lib.get_animation_list()[0],0.0)
