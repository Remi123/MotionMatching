#pragma once

#include <KForm.hpp>
#include <MMAnimationLibrary.hpp>
#include <MotionFeatures/MotionFeatures.hpp>
#include <algorithm>

// Friends
#include <PostProcessAnimation/PPInertialization3D.hpp>

using namespace godot;

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }
#define STR(x) #x
#define STRING_PREFIX(prefix, s) STR(prefix##s)
#define BINDER_PROPERTY_PARAMS(type, variant_type, variable, ...)                                  \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(set_, variable), "value"), &type::set_##variable); \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(get_, variable)), &type::get_##variable);          \
	ADD_PROPERTY(PropertyInfo(variant_type, #variable, __VA_ARGS__), STRING_PREFIX(set_, variable), STRING_PREFIX(get_, variable));
struct MFBonesInfo : public MotionFeature {
	GDCLASS(MFBonesInfo, MotionFeature)
public:
	// Skeleton
	Ref<SkeletonProfile> _skel = nullptr;

	GETSET(String, relative_to_bone, "");

	PackedStringArray bone_names{};
	void set_bone_names(PackedStringArray value) {
		bone_names = value;
	}
	PackedStringArray get_bone_names() { return bone_names; }

	enum BoneInfoType {
		Position,
		Velocity,
		Rotation,
		AngularVel,

		MAX_SIZE
	};

	std::bitset<BoneInfoType::MAX_SIZE> bone_info_type{};
	int get_bone_info_type() { return (int)bone_info_type.to_ulong(); }
	void set_bone_info_type(int value) { bone_info_type = value; }

	PackedInt32Array bones_id{};

	HashMap<uint32_t, PackedInt32Array> bone_tracks{};

	virtual int get_dimension() override {
		// if(use_inertialization)
		// {
		//     return bone_names.size() * 3;
		// }
		return bone_names.size() * 3 * bone_info_type.count();
	}

	virtual bool setup_bake_animation(Ref<Animation> animation) override {
		return true;
	}

	NodePath _skel_path;

	virtual bool setup_bake_init(Ref<MMAnimationLibrary> animlib) override {
		ERR_FAIL_COND_V_EDMSG(animlib->skeleton_path.is_empty(), false, "SkeletonPath is Empty");
		ERR_FAIL_COND_V_EDMSG(animlib->skeleton_profile == nullptr, false, "SkeletonProfile is null");
		ERR_FAIL_COND_V_EDMSG(relative_to_bone != "" && animlib->skeleton_profile->find_bone(relative_to_bone) == -1, false, "SkeletonProfile doesn't contain the relative bone ( Empty for global)");
		_skel = animlib->skeleton_profile;
		_skel_path = NodePath(animlib->skeleton_path);
		bones_id.clear();
		if (_skel != nullptr) {
			for (size_t i = 0; i < bone_names.size(); ++i) {
				const size_t id = _skel->find_bone(bone_names[i]);
				if (id >= 0)
					bones_id.push_back(id);
				else
					ERR_FAIL_V_EDMSG(false, "Missing Bone " + bone_names[i] + " in the SkeletonProfile");
			}
			return true;
		}
		return false;
	}

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) override {
		PackedFloat32Array result{};
		kform kbone{};
		auto relative_to_bone_path = u::str(_skel_path) + u::str(":") + relative_to_bone;
		for (size_t index = 0; index < bone_names.size(); ++index) {
			auto bone_path = u::str(_skel_path) + u::str(":") + bone_names[index];
			auto bone = bone_names[index];

			// kbone = MMAnimationLibrary::sample_bone_rootmotion_kform(animation, time, _skel, path);
			kbone = _get_bone_kform_global(bone, animation, time);
			kbone = _get_bone_kform_global(relative_to_bone, animation, time).inverse() * kbone;

			// Serialize
			// if (bone_info_type == PositionAndVelocity && use_inertialization)
			// {
			//     const auto cost = inertialization_cost_function(kbone.pos, kbone.vel, inertialization_halflife);
			//     result.push_back(cost.x);
			//     result.push_back(cost.y);
			//     result.push_back(cost.z);
			// }
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
		}

		return result;
	}

private:
	kform _get_bone_kform_global(String bone, Ref<Animation> animation, float time) {
		if (bone.is_empty())
			return kform{};
		std::vector<kform> trs{};
		do {
			trs.emplace_back(kform{ _skel, u::str(_skel_path) + u::str(":") + bone, animation, time });
			// if (bone == _skel->get_root_bone() || bone == relative_to_bone) {
			if (bone == _skel->get_root_bone()) {
				break;
			}
			bone = _skel->get_bone_parent(_skel->find_bone(bone)); // Now bone is its parent
			// if (bone == relative_to_bone) {
			// 	break;
			// }
		} while (!bone.is_empty());

		return std::reduce(trs.rbegin(), trs.rend(), kform{},
				[](const kform &acc, const kform &i) {
					return acc * i;
				});
	}

	kform _get_bone_kform_global(const kforms &bones, String bone) {
		if (bone.is_empty())
			return kform{};
		std::vector<kform> trs{};
		do {
			trs.push_back(bones[_skel->find_bone(bone)]);
			// if (bone == _skel->get_root_bone() || bone == relative_to_bone) {
			if (bone == _skel->get_root_bone()) {
				break;
			}
			bone = _skel->get_bone_parent(_skel->find_bone(bone)); // Now bone is its parent
			// if (bone == relative_to_bone) {
			// 	break;
			// }
		} while (!bone.is_empty());

		return std::reduce(trs.crbegin(), trs.crend(), kform{},
				[](const kform &acc, const kform &i) {
					return acc * i;
				});
	}

public:
	Vector3 inertialization_cost_function(Vector3 pos, Vector3 vel, float halflife) {
		const auto halfdamp = Spring::halflife_to_damping(halflife) / 2.0;
		return (2 * pos) / halfdamp + vel / (halfdamp * halfdamp);
	}

	GETSET(PackedVector3Array, bones_pos);
	GETSET(PackedVector3Array, bones_vel);

	float weight_bone_pos{ 1.0f };
	float get_weight_bone_pos() { return weight_bone_pos; }
	void set_weight_bone_pos(float value) { weight_bone_pos = value; }
	float weight_bone_vel{ 1.0f };
	float get_weight_bone_vel() { return weight_bone_vel; }
	void set_weight_bone_vel(float value) { weight_bone_vel = value; }
	float weight_bone_rot{ 1.0f };
	float get_weight_bone_rot() { return weight_bone_rot; }
	void set_weight_bone_rot(float value) { weight_bone_rot = value; }
	float weight_bone_ang{ 1.0f };
	float get_weight_bone_ang() { return weight_bone_ang; }
	void set_weight_bone_ang(float value) { weight_bone_ang = value; }

	float weight_inertialization{ 1.0f };
	float get_weight_inertialization() { return weight_inertialization; }
	void set_weight_inertialization(float value) { weight_inertialization = value; }

	virtual PackedFloat32Array get_weights() override {
		PackedFloat32Array result{};

		// if (use_inertialization)
		// {
		//     for (auto i = 0; i < 3 * bone_names.size(); ++i)
		//     {
		//         result.append(weight_inertialization);
		//     }
		//     return result;
		// }

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
		}
		return result;
	}

	GETSET(bool, use_inertialization)
	GETSET(float, inertialization_halflife, 0.1);

	PackedFloat32Array serialize_ppinertialization3d(PPInertialization3D *node) {
		PackedFloat32Array result{};
		for (size_t i = 0; i < bone_names.size(); ++i) {
			String bone = bone_names[i];

			kform kbone = _get_bone_kform_global(node->bones, bone);
			if (bone != relative_to_bone)
				kbone = _get_bone_kform_global(node->bones, relative_to_bone).inverse() * kbone;
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
		}
		return result;
	}

	virtual float calculate_cost(PackedFloat32Array query, PackedFloat32Array data) const override {
		float result = 0.0f;
		for (size_t i = 0; i < bone_names.size(); ++i) {
			unsigned int offset = i * bone_info_type.count() * 3;
			String bone = bone_names[i];
			if (bone_info_type.test(Position)) {
				Vector3 p_query = Vector3(query[offset + 0], query[offset + 1], query[offset + 2]);
				Vector3 p_data = Vector3(data[offset + 0], data[offset + 1], data[offset + 2]);
				result += p_query.distance_to(p_data) * weight_bone_pos;
				offset += 3;
			}
			if (bone_info_type.test(Velocity)) {
				Vector3 p_query = Vector3(query[offset + 0], query[offset + 1], query[offset + 2]);
				Vector3 p_data = Vector3(data[offset + 0], data[offset + 1], data[offset + 2]);
				result += p_query.distance_to(p_data) * weight_bone_vel;
				offset += 3;
			}
			if (bone_info_type.test(Rotation)) {
				Vector3 p_query = Vector3(query[offset + 0], query[offset + 1], query[offset + 2]);
				Vector3 p_data = Vector3(data[offset + 0], data[offset + 1], data[offset + 2]);
				float dot = p_query[i].dot(p_data[i]);
				result += std::fabs(2.0f - (1.0f + dot)) * 0.5f * weight_bone_rot;
				offset += 3;
			}
			if (bone_info_type.test(AngularVel)) {
				Vector3 p_query = Vector3(query[offset + 0], query[offset + 1], query[offset + 2]);
				Vector3 p_data = Vector3(data[offset + 0], data[offset + 1], data[offset + 2]);
				result += p_query.distance_to(p_data) * weight_bone_ang;
				offset += 3;
			}
		}
		return result;
	}

	PackedFloat32Array serialize_mmplayer(MMAnimationPlayer *mm_player) {
		ERR_FAIL_NULL_V_MSG(mm_player, {}, "MMAnimationPlayer is null");
		constexpr size_t size = 3;
		PackedFloat32Array result{};
		// if (use_inertialization)
		// {
		//     result.resize(bone_names.size() * 3);
		//     for (size_t i = 0; i < bone_names.size(); ++i)
		//     {
		//         // _skel isn't init
		//         kform b = mm_player->get_bone_global_kform(_skel->find_bone(bone_names[i]));
		//         Vector3 pos = b.pos, vel = b.vel;
		//         auto cost = inertialization_cost_function(pos, vel, inertialization_halflife);
		//         result[i * size] = cost.x;
		//         result[i * size + 1] = cost.y;
		//         result[i * size + 2] = cost.z;
		//     }
		//     return result;
		// }
		// else
		{
			// result.resize(bone_names.size() * 3 * std::bitset<10>(bone_info_type).count());
			for (size_t i = 0; i < bone_names.size(); ++i) {
				String bone = bone_names[i];

				kform kbone = _get_bone_kform_global(mm_player->bones_kform, bone);
				if (bone != relative_to_bone)
					kbone = _get_bone_kform_global(mm_player->bones_kform, relative_to_bone).inverse() * kbone;
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
			}
			return result;
		}
		return result;
	}

	virtual PackedStringArray get_hints() const override {
		PackedStringArray result{};

		// if (use_inertialization)
		// {
		//     for (auto i = 0; i < bone_names.size(); ++i)
		//     {
		//         result.append_array(Array::make("ixB"+u::str(i),"iyB"+u::str(i),"izB"+u::str(i)));
		//     }
		//     return result;
		// }
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
		}
		return result;
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_bone_info_type", "value"), &MFBonesInfo::set_bone_info_type, DEFVAL(1));
		ClassDB::bind_method(D_METHOD("get_bone_info_type"), &MFBonesInfo::get_bone_info_type);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "bone_info_type", godot::PROPERTY_HINT_FLAGS, "Position,Velocity,Rotation,AngularVel", godot::PROPERTY_USAGE_DEFAULT), "set_bone_info_type", "get_bone_info_type");

		{
			ClassDB::bind_method(D_METHOD("serialize_MMAnimationPlayer", "body"), &MFBonesInfo::serialize_mmplayer);
			ClassDB::bind_method(D_METHOD("serialize_PPInertialization3D", "body"), &MFBonesInfo::serialize_ppinertialization3d);
		}

		ClassDB::bind_method(D_METHOD("get_hints"), &MFBonesInfo::get_hints);

		ClassDB::bind_method(D_METHOD("set_relative_to_bone", "value"), &MFBonesInfo::set_relative_to_bone);
		ClassDB::bind_method(D_METHOD("get_relative_to_bone"), &MFBonesInfo::get_relative_to_bone);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING, "relative_to_bone"), "set_relative_to_bone", "get_relative_to_bone");

		ClassDB::bind_method(D_METHOD("set_bone_names", "value"), &MFBonesInfo::set_bone_names);
		ClassDB::bind_method(D_METHOD("get_bone_names"), &MFBonesInfo::get_bone_names);
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "Bones Names"), "set_bone_names", "get_bone_names");

		ClassDB::bind_method(D_METHOD("set_weight_bone_pos", "value"), &MFBonesInfo::set_weight_bone_pos);
		ClassDB::bind_method(D_METHOD("get_weight_bone_pos"), &MFBonesInfo::get_weight_bone_pos);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_pos"), "set_weight_bone_pos", "get_weight_bone_pos");

		ClassDB::bind_method(D_METHOD("set_weight_bone_vel", "value"), &MFBonesInfo::set_weight_bone_vel);
		ClassDB::bind_method(D_METHOD("get_weight_bone_vel"), &MFBonesInfo::get_weight_bone_vel);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_vel"), "set_weight_bone_vel", "get_weight_bone_vel");

		ClassDB::bind_method(D_METHOD("set_weight_bone_ang", "value"), &MFBonesInfo::set_weight_bone_ang);
		ClassDB::bind_method(D_METHOD("get_weight_bone_ang"), &MFBonesInfo::get_weight_bone_ang);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_ang"), "set_weight_bone_ang", "get_weight_bone_ang");

		ClassDB::bind_method(D_METHOD("set_weight_inertialization", "value"), &MFBonesInfo::set_weight_inertialization);
		ClassDB::bind_method(D_METHOD("get_weight_inertialization"), &MFBonesInfo::get_weight_inertialization);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_inertialization"), "set_weight_inertialization", "get_weight_inertialization");

		ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
		{
			// TODO Change use inertialization setup to only be available when choosing PositionAndVelocity
			ClassDB::bind_method(D_METHOD("set_use_inertialization", "value"), &MFBonesInfo::set_use_inertialization, DEFVAL(false));
			ClassDB::bind_method(D_METHOD("get_use_inertialization"), &MFBonesInfo::get_use_inertialization);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "use_inertialization"), "set_use_inertialization", "get_use_inertialization");

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

		//BINDER_PROPERTY_PARAMS(MFBonesInfo,Variant::PACKED_VECTOR3_ARRAY,bones_pos,PROPERTY_HINT_NONE,"",PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY);
		ClassDB::bind_method(D_METHOD("set_bones_pos", "value"), &MFBonesInfo::set_bones_pos);
		ClassDB::bind_method(D_METHOD("get_bones_pos"), &MFBonesInfo::get_bones_pos);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "bones_pos", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY), "set_bones_pos", "get_bones_pos");

		//BINDER_PROPERTY_PARAMS(MFBonesInfo,Variant::PACKED_VECTOR3_ARRAY,bones_vel,PROPERTY_HINT_NONE,"",PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY);
		ClassDB::bind_method(D_METHOD("set_bones_vel", "value"), &MFBonesInfo::set_bones_vel);
		ClassDB::bind_method(D_METHOD("get_bones_vel"), &MFBonesInfo::get_bones_vel);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "bones_vel", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY), "set_bones_vel", "get_bones_vel");

		ClassDB::bind_method(D_METHOD("get_weights"), &MFBonesInfo::get_weights);
		ClassDB::bind_method(D_METHOD("get_dimension"), &MFBonesInfo::get_dimension);

		ClassDB::bind_method(D_METHOD("setup_bake_init", "mm_animation_library"), &MFBonesInfo::setup_bake_init);

		ClassDB::bind_method(D_METHOD("setup_bake_animation", "animation"), &MFBonesInfo::setup_bake_animation);
		ClassDB::bind_method(D_METHOD("bake_animation_pose", "animation", "time"), &MFBonesInfo::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &MFBonesInfo::debug_pose_gizmo);
	}

	GETSET(Color, debug_color_position, godot::Color(1.0f, 1.0f, 1.0f));
	GETSET(Color, debug_color_velocity, godot::Color(0.0f, 0.0f, 0.0f));

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, godot::Transform3D tr = godot::Transform3D{}) override {
		const auto mat_name_pos = "pos" + get_path();
		const auto mat_name_vel = "vel" + get_path();
		if (gizmo->get_plugin()->get_material(mat_name_pos, gizmo) == nullptr) {
			gizmo->get_plugin()->create_material(mat_name_pos, debug_color_position);
		}
		if (gizmo->get_plugin()->get_material(mat_name_vel, gizmo) == nullptr) {
			gizmo->get_plugin()->create_material(mat_name_vel, debug_color_velocity);
		}

		auto position_color = gizmo->get_plugin()->get_material(mat_name_pos, gizmo);
		auto velocity_color = gizmo->get_plugin()->get_material(mat_name_vel, gizmo);
		position_color->set_albedo(debug_color_position);
		velocity_color->set_albedo(debug_color_velocity);

		if (use_inertialization) {
			return;
		}

		constexpr int s = 3;
		for (size_t index = 0; index < bone_names.size(); ++index) {
			//i*size*2+size+2
			Vector3 pos = Vector3(data[index * s * 2 + 0], data[index * s * 2 + 1], data[index * s * 2 + 2]);
			Vector3 vel = Vector3(data[index * s * 2 + s + 0], data[index * s * 2 + s + 1], data[index * s * 2 + s + 2]);
			pos = tr.xform(pos);
			vel = tr.xform(vel);

			gizmo->add_lines(Array::make(pos, pos + vel), velocity_color);
			auto box = Ref<BoxMesh>();
			box.instantiate();
			box->set_size(Vector3(0.05f, 0.05f, 0.05f));
			Transform3D tr = Transform3D(Basis(), pos);
			gizmo->add_mesh(box, position_color, tr);
		}
	}
};

// VARIANT_ENUM_CAST(MFBonesInfo::BoneInfoType);

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX