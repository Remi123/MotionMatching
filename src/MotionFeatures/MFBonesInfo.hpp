#pragma once

#include <MMAnimationLibrary.hpp>
#include <Math/KForm.hpp>
#include <MotionFeatures/MotionFeatures.hpp>
#include <algorithm>

#include <godot_cpp/classes/prism_mesh.hpp>

// Friends
#include <PostProcessAnimation/MMInertialization3D.hpp>

using namespace godot;

struct MFBonesInfo : public MotionFeature {
	GDCLASS(MFBonesInfo, MotionFeature)
public:
	GETSET(Color, debug_color_position, godot::Color(1.0f, 1.0f, 1.0f));
	GETSET(Color, debug_color_velocity, godot::Color(0.0f, 0.0f, 0.0f));

	GETSET(real_t, weight_bone_pos,1.0);
	GETSET(real_t, weight_bone_vel,1.0);
	GETSET(real_t, weight_bone_rot,1.0);
	GETSET(real_t, weight_bone_ang,1.0);
	GETSET(real_t, weight_inertialization,1.0);

	GETSET(String, relative_to_bone, "");

	GETSET(PackedStringArray, bone_names);

	enum BoneInfoType {
		Position, //3
		Velocity, //3
		Rotation, //3
		AngularVel, //3
		InertializationCost, //3

		MAX_SIZE
	};

	GETSET(float, inertialization_halflife, 0.1);
	std::bitset<BoneInfoType::MAX_SIZE> bone_info_type{};
	int get_bone_info_type() { return (int)bone_info_type.to_ulong(); }
	void set_bone_info_type(int value) { bone_info_type = value; }

	int get_dimension() const {
		return bone_names.size() * 3 * bone_info_type.count();
	}

	PackedFloat32Array get_weights() const {
		PackedFloat32Array result{};

		for (auto i = 0; i < bone_names.size(); ++i) {
			if (bone_info_type.test(Position))
				for (auto i = 0; i < 3; ++i)
					result.append(weight_bone_pos);
			if (bone_info_type.test(Velocity))
				for (auto i = 0; i < 3; ++i)
					result.append(weight_bone_vel);
			if (bone_info_type.test(Rotation))
				for (auto i = 0; i < 3; ++i)
					result.append(weight_bone_rot);
			if (bone_info_type.test(AngularVel))
				for (auto i = 0; i < 3; ++i)
					result.append(weight_bone_ang);
			if (bone_info_type.test(InertializationCost))
				for (auto i = 0; i < 3; ++i)
					result.append(weight_inertialization);
		}
		return result;
	}

	PackedStringArray get_hints() const {
		PackedStringArray result{};

		for (auto i = 0; i < bone_names.size(); ++i) {
			if (bone_info_type.test(Position)) {
				result.append("PxB" + u::str(i));
				result.append("PyB" + u::str(i));
				result.append("PzB" + u::str(i));
			}
			if (bone_info_type.test(Velocity)) {
				result.append("VxB" + u::str(i));
				result.append("VyB" + u::str(i));
				result.append("VzB" + u::str(i));
			}
			if (bone_info_type.test(Rotation)) {
				result.append("RxB" + u::str(i));
				result.append("RyB" + u::str(i));
				result.append("RzB" + u::str(i));
			}
			if (bone_info_type.test(AngularVel)) {
				result.append("AxB" + u::str(i));
				result.append("AyB" + u::str(i));
				result.append("AzB" + u::str(i));
			}
			if (bone_info_type.test(InertializationCost)) {
				result.append("ICxB" + u::str(i));
				result.append("ICyB" + u::str(i));
				result.append("ICzB" + u::str(i));
			}
		}
		return result;
	}

	PackedFloat32Array bake_pose(Ref<MMAnimationLibrary> mmlib, String animation_name, float time) {
		ERR_FAIL_COND_V_EDMSG(mmlib->skeleton_path.is_empty(), {}, "SkeletonPath is Empty");
		ERR_FAIL_COND_V_EDMSG(mmlib->skeleton_profile == nullptr, {}, "SkeletonProfile is null");
		ERR_FAIL_COND_V_EDMSG(relative_to_bone != "" && mmlib->skeleton_profile->find_bone(relative_to_bone) == -1, {}, "SkeletonProfile doesn't contain the relative bone ( Empty for global)");
		PackedFloat32Array result{};
		Ref<Animation> animation = mmlib->get_animation(animation_name);

		kform kbone{};
		auto relative_to_bone_path = u::str(mmlib->skeleton_path) + u::str(":") + relative_to_bone;
		for (size_t index = 0; index < bone_names.size(); ++index) {
			auto bone_path = u::str(mmlib->skeleton_path) + u::str(":") + bone_names[index];
			auto bone = bone_names[index];

			kbone = get_model_kform(mmlib->skeleton_profile, animation, time, bone_path);
			if (!relative_to_bone.is_empty() && relative_to_bone != mmlib->skeleton_profile->get_root_bone()) {
				kbone = get_model_kform(mmlib->skeleton_profile, animation, time, relative_to_bone_path).inverse() * kbone;
			}

			// Serialize
			if (bone_info_type.test(Position)) {
				result.push_back(kbone.pos.x);
				result.push_back(kbone.pos.y);
				result.push_back(kbone.pos.z);
			}
			if (bone_info_type.test(Velocity)) {
				result.push_back(kbone.vel.x);
				result.push_back(kbone.vel.y);
				result.push_back(kbone.vel.z);
			}
			if (bone_info_type.test(Rotation)) {
				Vector3 const dir = kbone.rot.xform(Vector3(0, 0, 1));
				result.push_back(dir.x);
				result.push_back(dir.y);
				result.push_back(dir.z);
			}
			if (bone_info_type.test(AngularVel)) {
				result.push_back(kbone.ang.x);
				result.push_back(kbone.ang.y);
				result.push_back(kbone.ang.z);
			}
			if (bone_info_type.test(InertializationCost)) {
				Vector3 const cost = inertialization_cost_function(kbone.pos, kbone.vel, inertialization_halflife);
				result.append(cost.x);
				result.append(cost.y);
				result.append(cost.z);
			}
		}
		return result;
	}

public:
	Vector3 inertialization_cost_function(Vector3 pos, Vector3 vel, float halflife) {
		const auto halfdamp = Spring::halflife_to_damping(halflife) / 2.0;
		return (2 * pos) / halfdamp + vel / (halfdamp * halfdamp);
	}

	PackedFloat32Array serialize_mminertialization3d(MMInertialization3D *node) {
		PackedFloat32Array result{};
		for (size_t i = 0; i < bone_names.size(); ++i) {
			String bone = bone_names[i];
			int id = node->get_skeleton()->find_bone(bone);

			kform kbone = (kform)node->bone_model[id];
			if (!relative_to_bone.is_empty()) {
				const int relative_id = node->get_skeleton()->find_bone(relative_to_bone);
				kbone = kform(node->bone_model[relative_id]).inverse() * kbone;
			}
			Vector3 const pos = kbone.pos, vel = kbone.vel, dir = kbone.rot.xform(Vector3(0, 0, 1)), ang = kbone.ang;

			if (bone_info_type.test(Position)) {
				result.append(pos.x);
				result.append(pos.y);
				result.append(pos.z);
			}
			if (bone_info_type.test(Velocity)) {
				result.append(vel.x);
				result.append(vel.y);
				result.append(vel.z);
			}
			if (bone_info_type.test(Rotation)) {
				result.append(dir.x);
				result.append(dir.y);
				result.append(dir.z);
			}
			if (bone_info_type.test(AngularVel)) {
				result.append(ang.x);
				result.append(ang.y);
				result.append(ang.z);
			}
			if (bone_info_type.test(InertializationCost)) {
				Vector3 const cost = inertialization_cost_function(pos, vel, inertialization_halflife);
				result.append(cost.x);
				result.append(cost.y);
				result.append(cost.z);
			}
		}
		return result;
	}

	PackedFloat32Array serialize_mmplayer(Ref<MMAnimationLibrary> mmlib, MMAnimationPlayer *mm_player) {
		ERR_FAIL_NULL_V_MSG(mm_player, {}, "MMAnimationPlayer is null");
		constexpr size_t size = 3;
		PackedFloat32Array result{};
		{
			for (size_t i = 0; i < bone_names.size(); ++i) {
				kform kbone = mm_player->_get_model_kform(bone_names[i]);
				if (!relative_to_bone.is_empty()) {
					kbone = mm_player->_get_model_kform(relative_to_bone).inverse() * kbone;
				}
				Vector3 const pos = kbone.pos, vel = kbone.vel, dir = kbone.rot.xform(Vector3(0, 0, 1)), ang = kbone.ang;

				if (bone_info_type.test(Position)) {
					result.append(pos.x);
					result.append(pos.y);
					result.append(pos.z);
				}
				if (bone_info_type.test(Velocity)) {
					result.append(vel.x);
					result.append(vel.y);
					result.append(vel.z);
				}
				if (bone_info_type.test(Rotation)) {
					result.append(dir.x);
					result.append(dir.y);
					result.append(dir.z);
				}
				if (bone_info_type.test(AngularVel)) {
					result.append(ang.x);
					result.append(ang.y);
					result.append(ang.z);
				}
				if (bone_info_type.test(InertializationCost)) {
					Vector3 const cost = inertialization_cost_function(pos, vel, mm_player->halflife);
					result.append(cost.x);
					result.append(cost.y);
					result.append(cost.z);
				}
			}
			return result;
		}
		return result;
	}

	virtual void show_debug_info(Ref<EditorNode3DGizmo> gizmo, Ref<MMAnimationLibrary> library, String animation_name, float time, Skeleton3D *skel) {
		const auto material_name = "bone" + get_path();
		if (gizmo->get_plugin()->get_material(material_name, gizmo) == nullptr) {
			gizmo->get_plugin()->create_material(material_name, debug_color_position);
		}
		auto mat = gizmo->get_plugin()->get_material(material_name, gizmo);

		const Ref<Animation> animation = library->get_animation(animation_name);
		const String reference_path = (String)library->skeleton_path + ":" + relative_to_bone;
		const Transform3D root_tr = get_global_kform(library->skeleton_profile, animation, time, reference_path);
		for (size_t i = 0; i < bone_names.size(); ++i) {
			const String bone_path = (String)library->skeleton_path + ":" + bone_names[i];

			const kform relative = get_model_kform(library->skeleton_profile, animation, time, reference_path);
			const kform model = get_model_kform(library->skeleton_profile, animation, time, bone_path);
			const kform kbone = relative_to_bone.is_empty() || relative_to_bone == library->skeleton_profile->get_root_bone() ? model : relative.inverse() * model;

			Transform3D global = root_tr * (Transform3D)kbone;

			Ref<PrismMesh> mesh{};
			mesh.instantiate();
			mesh->set_size(Vector3{ 1, 1.2, 1 } * 0.05);
			gizmo->add_mesh(mesh, mat, global);

			gizmo->add_lines(Array::make((root_tr * (Transform3D)relative).origin, global.origin), mat);
			gizmo->add_lines(Array::make(global.origin, global.xform(kbone.vel)), mat);
		}
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_bone_info_type", "value"), &MFBonesInfo::set_bone_info_type, DEFVAL(1));
		ClassDB::bind_method(D_METHOD("get_bone_info_type"), &MFBonesInfo::get_bone_info_type);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "bone_info_type", godot::PROPERTY_HINT_FLAGS, "Position,Velocity,Rotation,AngularVel,InertializationCost", godot::PROPERTY_USAGE_DEFAULT), "set_bone_info_type", "get_bone_info_type");

		{
			ClassDB::bind_method(D_METHOD("serialize_MMAnimationPlayer", "body"), &MFBonesInfo::serialize_mmplayer);
			ClassDB::bind_method(D_METHOD("serialize_MMInertialization3D", "body"), &MFBonesInfo::serialize_mminertialization3d);
		}

		ClassDB::bind_method(D_METHOD("get_hints"), &MFBonesInfo::get_hints);

		ClassDB::bind_method(D_METHOD("set_relative_to_bone", "value"), &MFBonesInfo::set_relative_to_bone, DEFVAL("Root"));
		ClassDB::bind_method(D_METHOD("get_relative_to_bone"), &MFBonesInfo::get_relative_to_bone);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING, "relative_to_bone"), "set_relative_to_bone", "get_relative_to_bone");

		ClassDB::bind_method(D_METHOD("set_bone_names", "value"), &MFBonesInfo::set_bone_names);
		ClassDB::bind_method(D_METHOD("get_bone_names"), &MFBonesInfo::get_bone_names);
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "Bones Names"), "set_bone_names", "get_bone_names");

		ClassDB::bind_method(D_METHOD("set_weight_bone_pos", "value"), &MFBonesInfo::set_weight_bone_pos, DEFVAL(real_t{ 1.0 }));
		ClassDB::bind_method(D_METHOD("get_weight_bone_pos"), &MFBonesInfo::get_weight_bone_pos);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_pos"), "set_weight_bone_pos", "get_weight_bone_pos");

		ClassDB::bind_method(D_METHOD("set_weight_bone_vel", "value"), &MFBonesInfo::set_weight_bone_vel, DEFVAL(real_t{ 1.0 }));
		ClassDB::bind_method(D_METHOD("get_weight_bone_vel"), &MFBonesInfo::get_weight_bone_vel);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_vel"), "set_weight_bone_vel", "get_weight_bone_vel");

		ClassDB::bind_method(D_METHOD("set_weight_bone_ang", "value"), &MFBonesInfo::set_weight_bone_ang, DEFVAL(real_t{ 1.0 }));
		ClassDB::bind_method(D_METHOD("get_weight_bone_ang"), &MFBonesInfo::get_weight_bone_ang);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_ang"), "set_weight_bone_ang", "get_weight_bone_ang");

		ClassDB::bind_method(D_METHOD("set_weight_inertialization", "value"), &MFBonesInfo::set_weight_inertialization, DEFVAL(real_t{ 1.0 }));
		ClassDB::bind_method(D_METHOD("get_weight_inertialization"), &MFBonesInfo::get_weight_inertialization);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_inertialization"), "set_weight_inertialization", "get_weight_inertialization");

		ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
		{
			ClassDB::bind_method(D_METHOD("set_inertialization_halflife", "value"), &MFBonesInfo::set_inertialization_halflife, DEFVAL(0.1f));
			ClassDB::bind_method(D_METHOD("get_inertialization_halflife"), &MFBonesInfo::get_inertialization_halflife);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "inertialization_halflife"), "set_inertialization_halflife", "get_inertialization_halflife");

			ClassDB::bind_method(D_METHOD("set_debug_color_position", "value"), &MFBonesInfo::set_debug_color_position);
			ClassDB::bind_method(D_METHOD("get_debug_color_position"), &MFBonesInfo::get_debug_color_position);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_position"), "set_debug_color_position", "get_debug_color_position");

			ClassDB::bind_method(D_METHOD("set_debug_color_velocity", "value"), &MFBonesInfo::set_debug_color_velocity);
			ClassDB::bind_method(D_METHOD("get_debug_color_velocity"), &MFBonesInfo::get_debug_color_velocity);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_velocity"), "set_debug_color_velocity", "get_debug_color_velocity");
		}

		ClassDB::add_property_group(get_class_static(), "", "");

		ClassDB::bind_method(D_METHOD("get_weights"), &MFBonesInfo::get_weights);
		ClassDB::bind_method(D_METHOD("get_dimension"), &MFBonesInfo::get_dimension);

		ClassDB::bind_method(D_METHOD("bake_pose", "animation_library", "animation_name", "time"), &MFBonesInfo::bake_pose);

		ClassDB::bind_method(D_METHOD("show_debug_info", "gizmo", "lib", "animation_name", "timestamp"
																						   "skeleton"),
				&MFBonesInfo::show_debug_info);
	}
};

// VARIANT_ENUM_CAST(MFBonesInfo::BoneInfoType);