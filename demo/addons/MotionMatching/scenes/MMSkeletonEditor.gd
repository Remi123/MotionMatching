@tool
## meta-name : MM Editor Skeleton
## meta-description : Don't touch
class_name MMSkeletonEditor extends Skeleton3D

var current_animlib : MMAnimationLibrary
var current_anim : Animation
var current_time : float = 0.0

func _enter_tree() -> void:
	prints("MMSkeleton enter_tree")

	
	
func _exit_tree() -> void:
	prints("MMSkeleton exit_tree")

func set_bones(profile : SkeletonProfile) -> void:
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

func preview_anim():

	pass
		
		
