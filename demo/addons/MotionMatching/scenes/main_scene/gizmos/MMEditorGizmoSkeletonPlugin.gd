@tool
class_name MMEditorGizmoSkeletonPlugin extends EditorNode3DGizmoPlugin

const PREVIEWSKEL := preload("res://addons/MotionMatching/scenes/main_scene/MMPreviewSkeleton.gd")
var _plugin : MMEditorPlugin
signal please_redraw
signal on_pose_selected(mmlib : MMAnimationLibrary,name : String,time :float)

class MMPreviewGizmo extends EditorNode3DGizmo:
	var trs : Array[Transform3D] = []
	var mf_vel := MFRootVelocity.new()
	var dict := Dictionary()
	func on_pose_selected(mmlib : MMAnimationLibrary,name : String,time :float):
		trs.clear()
		mf_vel.debug_color = Color.RED
		get_plugin().create_material(mf_vel.resource_path,mf_vel.debug_color)
		
		dict.lib = mmlib
		dict.name = name
		dict.time = time
	
		var current_animation := mmlib.get_animation(name)
		for i in range(1+current_animation.get_length()/mmlib.time_interval):
			var TR = mmlib.sample_bone_global(name,i * mmlib.time_interval,"%GeneralSkeleton:Root")
			var tr = Transform3D(Basis(Quaternion(TR.rotation)),TR.position)
			trs.append(Transform3D(Basis(TR.rotation),TR.position))
			
		(get_node_3d() as MMPreviewSkeleton).set_skeleton_to_pose(mmlib,name,time)
		
		_redraw()
		
	func _redraw() -> void:
		clear()
		if get_plugin()._plugin == null || get_plugin()._plugin.current_lib == null || dict == Dictionary():
			return;
		var mfs :Array[MotionFeature] = get_plugin()._plugin.current_lib.motion_features
		
		for mf in mfs:
			if mf.has_method("show_debug_info"):
				mf.show_debug_info(self,dict.lib,dict.name,dict.time,self.get_node_3d() as Skeleton3D)
		for i in range(trs.size()):
			var box = PrismMesh.new()

			box.size = Vector3(1,1.5,1)*0.05
			var transform :Transform3D= trs[i]
			add_mesh(box,get_plugin().get_material("orange",self),transform.rotated_local(Vector3.RIGHT,deg_to_rad(90)))

func _init(plugin:MMEditorPlugin) -> void:
	prints("Creating MM Gizmo")
	_plugin = plugin
	create_material("orange",Color.ORANGE,false,true)	
	create_material("blue",Color.BLUE,false,true)
	_plugin.bottom_panel_instance.pose_selected.connect(on_pose_selected.emit)

	

func _get_gizmo_name():
	return "MM_Gizmos"

	
func _create_gizmo(for_node_3d) -> EditorNode3DGizmo:
	if for_node_3d is MMPreviewSkeleton:
		prints("Creating gizmo",for_node_3d.name)
		var preview := MMPreviewGizmo.new()
		#please_redraw.connect(preview._redraw)
		on_pose_selected.connect(preview.on_pose_selected)
		return preview
	return null

func _redraw(gizmo: EditorNode3DGizmo) -> void:
	
	pass
	
	
