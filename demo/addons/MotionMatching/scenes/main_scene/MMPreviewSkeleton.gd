@tool
class_name MMPreviewSkeleton extends Skeleton3D

var __profile : SkeletonProfile


func _ready() -> void:
	
	#self.add_gizmo()
	pass
	
func set_bones(profile : SkeletonProfile) -> void:
	__profile = profile
	clear_bones()
	for counter in range(profile.bone_size):
		var name = profile.get_bone_name(counter)
		var rest := profile.get_reference_pose(counter)
		var parent = profile.get_bone_parent(counter)
		var tail = profile.get_bone_tail(counter)
		var id := add_bone(name)
		set_bone_rest(id,rest)
		set_bone_parent(id,find_bone(parent))
	reset_bone_poses()
	force_update_all_bone_transforms()
	
func set_skeleton_to_pose(library:MMAnimationLibrary,anim_name:StringName,timestamp:float):
	var anim := library.get_animation(anim_name)
	var anim_timestep := timestamp

	for i in range(anim.get_track_count()):
		var bone_id = find_bone(anim.track_get_path(i).get_subname(0))
		if bone_id == -1:
			continue
		elif anim.track_get_type(i) == Animation.TYPE_POSITION_3D:
			var position := anim.position_track_interpolate(i,anim_timestep)
			set_bone_pose_position(bone_id,position * get_motion_scale())
		elif anim.track_get_type(i) == Animation.TYPE_ROTATION_3D:
			var rotation := anim.rotation_track_interpolate(i,anim_timestep)
			set_bone_pose_rotation(bone_id,rotation)
		elif anim.track_get_type(i) == Animation.TYPE_SCALE_3D:
			var scale := anim.scale_track_interpolate(i,anim_timestep)
			set_bone_pose_scale(bone_id,scale)
