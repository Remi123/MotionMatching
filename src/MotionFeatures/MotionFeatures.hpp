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

#include <AnimTags/AnimTag.hpp>
#include <MMAnimationLibrary.hpp>
#include <MMAnimationPlayer.hpp>

#include <Util/Util.hpp>

class MMAnimationLibrary;

struct MotionFeature : public Resource {
	GDCLASS(MotionFeature, Resource)

public:
	enum NormalizationType {
		Standardized,
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

	static constexpr float delta = 0.016f;

	GDVIRTUAL0RC(int, get_dimension);
	GDVIRTUAL0RC(PackedStringArray, get_hints);
	GDVIRTUAL0RC(PackedFloat32Array, get_weights);
	GDVIRTUAL0RC(Color, get_tag_color);

	GDVIRTUAL3R(PackedFloat32Array, bake_pose, Ref<AnimationLibrary>, String, float);
	GDVIRTUAL5C(show_debug_info, Ref<EditorNode3DGizmo>, Ref<AnimationLibrary>, String, float, Skeleton3D *);

	static void _bind_methods() {
		BIND_ENUM_CONSTANT(Standardized);
		BIND_ENUM_CONSTANT(RawValue);

		BINDER_PROPERTY_PARAMS(MotionFeature, Variant::INT, normalization_type, godot::PROPERTY_HINT_ENUM, "Standardized,RawValue");

		// REFACTOR : This is the new API.
		GDVIRTUAL_BIND(get_dimension);
		GDVIRTUAL_BIND(get_weights);
		GDVIRTUAL_BIND(get_hints);

		GDVIRTUAL_BIND(bake_pose, "mmanimationlibrary", "animation_name", "time");
		GDVIRTUAL_BIND(show_debug_info, "gizmo", "profile", "animation", "timestamp", "skeleton");

		{
			MethodInfo mi;
			mi.arguments.push_back(PropertyInfo(Variant::STRING, "variants"));
			mi.name = "serialize_variants";
			ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT, "serialize_variants", &MotionFeature::serialize_variants, mi);
		}
	}

	auto serialize_variants(const Variant **args, GDExtensionInt arg_count, GDExtensionCallError &error) -> PackedFloat32Array {
		PackedFloat32Array result{};
		for(auto i = 0; i < arg_count;++i)
		{
			serialize_variant(*(args[i]),result);
		}
		return result;
	}
	
	static void serialize_variant(const Variant &v, PackedFloat32Array &result) {
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
		else if (v.get_type() == Variant::PACKED_FLOAT32_ARRAY){
			result.append_array((PackedFloat32Array)v);
		}
	}
};

VARIANT_ENUM_CAST(MotionFeature::NormalizationType);
