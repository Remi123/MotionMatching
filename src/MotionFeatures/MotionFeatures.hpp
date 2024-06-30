#pragma once

#include <climits>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/core/gdvirtual.gen.inc>

#include "godot_cpp/core/math.hpp"
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

#include <MMAnimationLibrary.hpp>
#include <MMAnimationPlayer.hpp>
#include <AnimTags/AnimTag.hpp>
// #include <g0odot_cpp/classes/character_body3d.hpp>

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

class MMAnimationLibrary;

struct MotionFeature : public Resource {
	GDCLASS(MotionFeature, Resource)

public:
	enum NormalizationType {
		Standard,
		RawValue
	};

	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_POSTINITIALIZE:
				set_local_to_scene(true);
				break;
		}
	}

	GETSET(NormalizationType, normalization_type, RawValue);
	GETSET(real_t, norm_clamp_min, std::numeric_limits<float>::min());
	GETSET(real_t, norm_clamp_max, std::numeric_limits<float>::max());

	static constexpr float delta = 0.016f;

	GDVIRTUAL0RC(int, get_dimension);
	GDVIRTUAL0RC(PackedStringArray, get_hints);
	GDVIRTUAL0RC(PackedFloat32Array, get_weights);

	GDVIRTUAL3R(PackedFloat32Array, bake_pose, Ref<AnimationLibrary>, String, float);
	// PackedFloat32Array bake_pose(Ref<AnimationLibrary> mmlib, String animation_name, float time)

	GDVIRTUAL1R(bool, setup_bake_animation, Ref<Animation>);
	GDVIRTUAL2R(PackedFloat32Array, bake_animation_pose, Ref<Animation>, float);
	GDVIRTUAL1R(bool, setup_bake_init, Ref<AnimationLibrary>);

	virtual float calculate_cost(PackedFloat32Array query, PackedFloat32Array data) const {
		ERR_FAIL_V_MSG(query.size() != data.size(), "Query and Data not the same size");
		float cost = 0.0f;
		for (size_t i = 0; i < query.size(); ++i) {
			cost += std::fabs(query[i] - data[i]);
		}
		return cost;
	}

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, godot::Transform3D tr = godot::Transform3D{}) { return; }

	GDVIRTUAL5C(show_debug_info, Ref<EditorNode3DGizmo>, Ref<AnimationLibrary>, String, float, Skeleton3D *);
	// virtual void show_debug_info(Ref<EditorNode3DGizmo> gizmo, Ref<MMAnimationLibrary> library , Ref<Animation> animation, float timestamp,Skeleton3D * root_transform){}

	static void _bind_methods() {
		BIND_ENUM_CONSTANT(Standard);
		BIND_ENUM_CONSTANT(RawValue);

		GDVIRTUAL_BIND(get_dimension);

		GDVIRTUAL_BIND(get_weights);

		GDVIRTUAL_BIND(get_hints);

		ClassDB::bind_method(D_METHOD("set_normalization_type", "value"), &MotionFeature::set_normalization_type, DEFVAL(NormalizationType::Standard));
		ClassDB::bind_method(D_METHOD("get_normalization_type"), &MotionFeature::get_normalization_type);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "normalization_type", godot::PROPERTY_HINT_ENUM, "Standard,RawValue"), "set_normalization_type", "get_normalization_type");

		// REFACTOR : This is the new API.
		GDVIRTUAL_BIND(bake_pose, "mmanimationlibrary", "animation_name", "time", "current_tags");
		GDVIRTUAL_BIND(show_debug_info, "gizmo", "profile", "animation", "timestamp", "skeleton");

		// TODO Remove below. no longer required.
		GDVIRTUAL_BIND(setup_bake_init, "animation_library");
		// ClassDB::bind_method( D_METHOD("setup_bake_init","mm_animation_library"),   &MotionFeature::setup_bake_init);
		GDVIRTUAL_BIND(setup_bake_animation, "animation");
		// ClassDB::bind_method( D_METHOD("setup_bake_animation","animation"),         &MotionFeature::setup_bake_animation);
		GDVIRTUAL_BIND(bake_animation_pose, "animation", "timestamp");
		// ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"),   &MotionFeature::bake_animation_pose);
		BIND_VIRTUAL_METHOD(MotionFeature, calculate_cost);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &MotionFeature::debug_pose_gizmo);
	}

	static void serialize_variant(Variant &v, PackedFloat32Array &result) {
		using namespace godot;
		if (v.get_type() == Variant::BOOL) {
			result.append((bool)v);
		} else if (v.get_type() == Variant::INT) {
			result.append((int)v);
		} else if (v.get_type() == Variant::FLOAT) {
			result.append((real_t)v);
		} else if (v.get_type() == Variant::VECTOR2) {
			godot::Vector2 v2 = (Vector2)v;
			result.append(v2.x);
			result.append(v2.y);
		} else if (v.get_type() == Variant::VECTOR2I) {
			godot::Vector2i v2 = (Vector2i)v;
			result.append(v2.x);
			result.append(v2.y);
		} else if (v.get_type() == Variant::VECTOR3) {
			Vector3 v3 = (Vector3)v;
			result.append(v3.x);
			result.append(v3.y);
			result.append(v3.z);
		} else if (v.get_type() == Variant::VECTOR3I) {
			Vector3i v3 = (Vector3i)v;
			result.append(v3.x);
			result.append(v3.y);
			result.append(v3.z);
		} else if (v.get_type() == Variant::VECTOR4I) {
			Vector4i v3 = (Vector4i)v;
			result.append(v3.w);
			result.append(v3.x);
			result.append(v3.y);
			result.append(v3.z);
		} else if (v.get_type() == Variant::QUATERNION) {
			Quaternion q = (Quaternion)v;
			result.append(q.x);
			result.append(q.y);
			result.append(q.z);
			result.append(q.w);
		}
	}
};

VARIANT_ENUM_CAST(MotionFeature::NormalizationType);
