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
	int get_dimension() const {
		return 3;
	}

	GETSET(float, weight, 1.0f);

	PackedFloat32Array get_weights() const {
		return Array::make(weight, weight, weight);
	}
	PackedStringArray get_hints() const {
		return Array::make("Vx", "Vy", "Vz");
	}

	PackedFloat32Array bake_pose(Ref<MMAnimationLibrary> mmlib, String animation_name, float time) {
		ERR_FAIL_COND_V_EDMSG(mmlib->skeleton_path.is_empty(), {}, "SkeletonPath is Empty");
		ERR_FAIL_COND_V_EDMSG(mmlib->skeleton_profile == nullptr, {}, "SkeletonProfile is null");
		ERR_FAIL_COND_V_EDMSG(mmlib->skeleton_profile->get_root_bone().is_empty(), {}, "No Root bone to extract data");
		Ref<Animation> anim = mmlib->get_animation(animation_name);
		auto _root_bone_track = u::str(mmlib->skeleton_path) + ":" + mmlib->skeleton_profile->get_root_bone();

		kform root_motion = get_root_model_kform(mmlib->skeleton_profile, anim, time, _root_bone_track);

		PackedFloat32Array result{};
		result.append(root_motion.vel.x);
		result.append(root_motion.vel.y);
		result.append(root_motion.vel.z);
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

	virtual float calculate_cost(PackedFloat32Array query, PackedFloat32Array data) const {
		Vector3 v_query = Vector3(query[0], query[1], query[2]);
		Vector3 v_data = Vector3(data[0], data[1], data[2]);
		return v_query.distance_to(v_data) * weight;
	}

	virtual void show_debug_info(Ref<EditorNode3DGizmo> gizmo, Ref<MMAnimationLibrary> library, String animation_name, float timestamp, Skeleton3D *skel) const {
		Ref<Animation> animation = library->get_animation(animation_name);
		String root_bone_path = String(library->skeleton_path) + ":" + library->skeleton_profile->get_root_bone();
		Vector3 local_vel = get_root_model_kform(library->skeleton_profile, animation, timestamp, root_bone_path).vel;
		PackedVector3Array lines{};
		auto root_bone_tr = skel->get_bone_global_pose(skel->find_bone(library->skeleton_profile->get_root_bone()));

		lines.append(root_bone_tr.origin);
		lines.append(root_bone_tr.xform(local_vel));

		const auto material_name = "rootvel" + get_path();
		if (gizmo->get_plugin()->get_material(material_name) == nullptr) {
			gizmo->get_plugin()->create_material(material_name, debug_color);
		}
		auto mat = gizmo->get_plugin()->get_material(material_name, gizmo);
		gizmo->add_lines(lines, mat);
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

		ClassDB::bind_method(D_METHOD("bake_pose", "animation_library", "animation_name", "time"), &MFRootVelocity::bake_pose);

		ClassDB::bind_method(D_METHOD("calculate_cost", "query", "data"), &MFRootVelocity::calculate_cost);

		ClassDB::bind_method(D_METHOD("show_debug_info", "gizmo", "lib", "animation_name", "timestamp"
																						   "skeleton"),
				&MFRootVelocity::show_debug_info);
	}

	GETSET(Color, debug_color, godot::Color(1.0f, 1.0f, 1.0f));
};