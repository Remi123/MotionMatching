class_name simpleMotionMatchingSetupTimer extends Timer

@onready var MM := preload("res://assets/resources/locomotion.res")
@onready var mm_inertialization_3d: MMInertialization3D = %"MMInertialization3D"
@onready var best_index = -1
@onready var animation_tree: AnimationTree = %AnimationTree


func _on_timeout():
	var query := PackedFloat32Array()
	var bones_feature :MFBonesInfo= MM.motion_features[0]
	var root_velocity_feature :MFRootVelocity= MM.motion_features[1]
	
	query.append_array(bones_feature.serialize_MMInertialization3D(mm_inertialization_3d))
	query.append_array(root_velocity_feature.serialize_CharacterBody3d(owner))

	
	var queryoptions := MMQueryOptions.new()
	queryoptions.query = query
	queryoptions.custom_weights = MM.weights
	queryoptions.result_count = 1
	queryoptions.continuation_index = best_index
	queryoptions.continuation_bias = 0.5
	queryoptions.ignore_surrounding_frames = 4
	
	var best_pose :Dictionary= MM.query_pose_aabb(queryoptions)[0]
	
	if best_pose.size() != 0 && best_pose["index"] != best_index:
		best_index = best_pose["index"]
		prints(best_pose)
		var anim :AnimationNodeAnimation= animation_tree.tree_root.get_node("Locomotion")
		anim.animation = "locomotion/" + best_pose["animation"]
		anim.start_offset = best_pose["timestamp"]
		animation_tree["parameters/TimeSeek/seek_request"] = best_pose["timestamp"]
		#animation_tree.clear_caches()
	else :
		best_index += 1

		
	
	
	pass
