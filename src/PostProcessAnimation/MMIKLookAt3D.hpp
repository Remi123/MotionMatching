#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/classes/animation_mixer.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skeleton_modifier3d.hpp>

#include <Math/KForm.hpp>
#include <Util/Util.hpp>

using namespace godot;

struct MMIKLookAt3D : godot::SkeletonModifier3D {
	GDCLASS(MMIKLookAt3D, SkeletonModifier3D);

public:
	using u = godot::UtilityFunctions;

	GETSET(String, bone);

	virtual void _process_modification() {
		// Find delta
		const float delta = get_skeleton()->get_modifier_callback_mode_process() == Skeleton3D::ModifierCallbackModeProcess::MODIFIER_CALLBACK_MODE_PROCESS_IDLE ? get_process_delta_time() : get_physics_process_delta_time();
		if (is_active())
			advance(delta);
	}

	void advance(double delta) {
		if (is_active() == false || bone.is_empty())
			return;
		auto * skeleton = get_skeleton();
		int bone_id = skeleton->find_bone(bone);
		if (bone_id == -1)
			return;		

		Vector3 target_pos = get_global_position();
		Vector3 bone_global_pos = skeleton->get_global_position() + skeleton->get_bone_global_pose(bone_id).origin;
		Quaternion bone_global_rot = skeleton->get_global_transform().get_basis().get_rotation_quaternion() * skeleton->get_bone_global_pose(bone_id).basis.get_rotation_quaternion();

		Vector3 tar_pos = bone_global_rot.xform_inv(target_pos - bone_global_pos);

		Quaternion diff = Quaternion(get_global_basis().get_rotation_quaternion().xform(Vector3(0, 0, -1)), tar_pos.normalized());

		// Rotate the head to face toward the target
		Quaternion bone_local_rot = skeleton->get_bone_pose_rotation(bone_id);
		skeleton->set_bone_pose_rotation(bone_id, bone_local_rot * diff);
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_bone", "value"), &MMIKLookAt3D::set_bone);
		ClassDB::bind_method(D_METHOD("get_bone"), &MMIKLookAt3D::get_bone);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING, "bone"), "set_bone", "get_bone");
	}
};