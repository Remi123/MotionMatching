class_name MMEditorNode3DGizmo extends EditorNode3DGizmo

func changed_anim(lib:MMAnimationLibrary,animname:String,timestamp:float):
	var skeleton :MMSkeletonEditor= get_node_3d()
	var anim := lib.get_animation(animname)
	var anim_timestep := timestamp

	# Set the skeleton to pose
	for i in range(anim.get_track_count()):
		var bone_id = skeleton.find_bone(anim.track_get_path(i).get_subname(0))
		if bone_id == -1:
			continue
		elif anim.track_get_type(i) == Animation.TYPE_POSITION_3D:
			var position := anim.position_track_interpolate(i,anim_timestep)
			skeleton.set_bone_pose_position(bone_id,position * skeleton.get_motion_scale())
		elif anim.track_get_type(i) == Animation.TYPE_ROTATION_3D:
			var rotation := anim.rotation_track_interpolate(i,anim_timestep)
			skeleton.set_bone_pose_rotation(bone_id,rotation)
		elif anim.track_get_type(i) == Animation.TYPE_SCALE_3D:
			var scale := anim.scale_track_interpolate(i,anim_timestep)
			skeleton.set_bone_pose_scale(bone_id,scale)
	
	# Retrieve root bone transform
	var root_transform := skeleton.get_bone_pose(skeleton.find_bone(lib.skeleton_profile.root_bone))
	
	# Call every feature 
	for feature in lib.motion_features:
		pass
	
	pass
