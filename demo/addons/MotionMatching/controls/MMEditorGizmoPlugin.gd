@tool

class_name MMEditorGizmoPlugin extends EditorNode3DGizmoPlugin

var current_animlib :MMAnimationLibrary

var current_animation_name :String= "" :
	set(value):
		current_animation_name = value
	get:
		return current_animation_name
var current_timestamp := 0.0

signal on_pose_changed(lib:MMAnimationLibrary,animname:String,timestamp:float)

func _init():
	prints("Gizmo Init")
	create_material("white", Color.WHITE)
	create_material("blue", Color.BLUE)
	create_material("red", Color.RED)
	create_material("green", Color.GREEN)
	create_material("orange", Color.ORANGE_RED)

	create_handle_material("handles")

func _get_gizmo_name() -> String:
	return "MMGizmo"


func _has_gizmo(node):
	if node is MMSkeletonEditor :
		print(node.name)
	return node is MMSkeletonEditor

func _create_gizmo(for_node_3d: Node3D) -> EditorNode3DGizmo:
	var skel_gizmo := MMEditorNode3DGizmo.new()
	
	return skel_gizmo
	



func _redraw(gizmo : EditorNode3DGizmo):
	gizmo.clear()
	#if current_animlib == null || current_animation_name == null:
		#return

	var node3d :Skeleton3D = gizmo.get_node_3d()
	var lines = PackedVector3Array()
	
	var tr := node3d.global_transform * node3d.get_bone_global_pose(node3d.find_bone("Root"))
	

	lines.push_back(tr.origin)
	lines.push_back(tr.origin + tr.basis.z + Vector3(0,0.1,0))
	gizmo.add_lines(lines, get_material("orange", gizmo),true)

	if current_animlib == null || current_animation_name.is_empty():
		return
	
	var current_animation :Animation= current_animlib.get_animation(current_animation_name)
	var trail :=PackedVector3Array() 
	for i in range(current_animation.get_length()/current_animlib.time_interval):
		var box := BoxMesh.new()
		box.size = Vector3.ONE * 0.05
		var TR = current_animlib.sample_bone_global(current_animation_name,i * current_animlib.time_interval,"%GeneralSkeleton:Root")
		tr = Transform3D(Basis(TR.rotation),TR.position)
		gizmo.add_mesh(box,get_material("white"),tr)
		trail.append(tr.origin)
	
	gizmo.add_lines(trail,get_material("orange"),true)
	


	pass
