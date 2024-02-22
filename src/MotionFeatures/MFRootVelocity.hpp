#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>


#include <godot_cpp/classes/time.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>


#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>


#include <godot_cpp/classes/character_body3d.hpp>

#include <MMAnimationLibrary.hpp>
#include <MotionFeatures/MotionFeatures.hpp>


using namespace godot;
using u = godot::UtilityFunctions;

struct MFRootVelocity : public MotionFeature {
	GDCLASS(MFRootVelocity, MotionFeature);

public:
	int root_track_pos = -1, root_track_quat = -1; //, root_track_scale = -1;

	String root_bone_track = "%GeneralSkeleton:Root";
	Transform3D rest_pose = Transform3D();

	virtual int get_dimension() override {
		return 3;
	}

	GETSET(float, weight, 1.0f);
	virtual PackedFloat32Array get_weights() override {
		return Array::make(weight, weight, weight);
	}

	virtual bool setup_bake_init(Ref<MMAnimationLibrary> animlib) override {
		ERR_FAIL_COND_V_EDMSG(animlib->skeleton_path.is_empty(), false, "SkeletonPath is Empty");
		ERR_FAIL_COND_V_EDMSG(animlib->skeleton_profile == nullptr, false, "SkeletonProfile is null");
		ERR_FAIL_COND_V_EDMSG(animlib->skeleton_profile->get_root_bone().is_empty(), false, "No Root bone to extract data");
		rest_pose = animlib->skeleton_profile->get_reference_pose(animlib->skeleton_profile->find_bone(animlib->skeleton_profile->get_root_bone()));
		root_bone_track = u::str(animlib->skeleton_path) + ":" + animlib->skeleton_profile->get_root_bone();
		return true;
	}
	virtual bool setup_bake_animation(Ref<Animation> animation) override {
		root_track_pos = animation->find_track(NodePath(root_bone_track), Animation::TrackType::TYPE_POSITION_3D);
		root_track_quat = animation->find_track(NodePath(root_bone_track), Animation::TrackType::TYPE_ROTATION_3D);
		return true;
	}

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) override {
		Vector3 pos, prev_pos;
		if (root_track_pos >= 0) {
			pos = animation->position_track_interpolate(root_track_pos, time + 0.032);
			prev_pos = animation->position_track_interpolate(root_track_pos, time);
		} else {
			pos = rest_pose.get_origin();
			prev_pos = rest_pose.get_origin();
		}

		Quaternion rotation = root_track_quat >= 0 ? animation->rotation_track_interpolate(root_track_quat, time).normalized() : rest_pose.get_basis().get_rotation_quaternion();

		Vector3 vel = rotation.xform_inv(pos - prev_pos) / 0.032;

		PackedFloat32Array result{};
		result.push_back(vel.x);
		result.push_back(vel.y);
		result.push_back(vel.z);
		return result;
	}

	PackedFloat32Array serialize_charbody3d(CharacterBody3D *body) {
		PackedFloat32Array result{};
		auto vel = body->get_global_transform().basis.get_quaternion().xform_inv(body->get_velocity());
		result.push_back(vel.x);
		result.push_back(vel.y);
		result.push_back(vel.z);
		return result;
	}
	PackedFloat32Array serialize_vec3(Vector3 local_vel) {
		PackedFloat32Array result{};
		result.push_back(local_vel.x);
		result.push_back(local_vel.y);
		result.push_back(local_vel.z);
		return result;
	}

	virtual PackedStringArray get_hints() const override {
		return Array::make("Vx", "Vy", "Vz");
	}

protected:
	static void _bind_methods() {
		{
			ClassDB::bind_method(D_METHOD("serialize_CharacterBody3d", "body"), &MFRootVelocity::serialize_charbody3d);
			ClassDB::bind_method(D_METHOD("serialize_Local_Velocity", "local_velocity"), &MFRootVelocity::serialize_vec3);
		}
		ClassDB::bind_method(D_METHOD("get_hints"), &MFRootVelocity::get_hints);

		ClassDB::bind_method(D_METHOD("set_weight", "value"), &MFRootVelocity::set_weight, DEFVAL(1.0f));
		ClassDB::bind_method(D_METHOD("get_weight"), &MFRootVelocity::get_weight);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");

		ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
		{
			ClassDB::bind_method(D_METHOD("set_debug_color", "value"), &MFRootVelocity::set_debug_color);
			ClassDB::bind_method(D_METHOD("get_debug_color"), &MFRootVelocity::get_debug_color);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color"), "set_debug_color", "get_debug_color");
		}

		ClassDB::add_property_group(get_class_static(), "", "");

		ClassDB::bind_method(D_METHOD("get_weights"), &MFRootVelocity::get_weights);
		ClassDB::bind_method(D_METHOD("get_dimension"), &MFRootVelocity::get_dimension);

		ClassDB::bind_method(D_METHOD("setup_bake_init", "mm_animation_library"), &MFRootVelocity::setup_bake_init);
		ClassDB::bind_method(D_METHOD("setup_bake_animation", "animation"), &MFRootVelocity::setup_bake_animation);

		ClassDB::bind_method(D_METHOD("bake_animation_pose", "animation", "time"), &MFRootVelocity::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &MFRootVelocity::debug_pose_gizmo);
	}

	GETSET(Color, debug_color, godot::Color(1.0f, 1.0f, 1.0f));

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, godot::Transform3D tr = godot::Transform3D{}) override {
		const auto material_name = "rootvel" + get_path();
		if (gizmo->get_plugin()->get_material(material_name) == nullptr) {
			gizmo->get_plugin()->create_material(material_name, debug_color);
		}
		if (data.size() == get_dimension()) {
			Vector3 vel = tr.xform(Vector3(data[0], data[1], data[2]));
			auto mat = gizmo->get_plugin()->get_material(material_name, gizmo);
			mat->set_albedo(debug_color);
			gizmo->add_lines(Array::make(tr.origin, tr.origin + vel), mat);
		}
	}
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX