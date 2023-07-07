/**************************************************************************/
/*  motion_features.h                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef MOTION_FEATURES_H
#define MOTION_FEATURES_H

#include "core/io/resource.h"
#include "core/math/math_defs.h"
#include "core/object/class_db.h"
#include "core/object/object.h"
#include "core/os/time.h"
#include "core/string/node_path.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"
#ifdef TOOLS_ENABLED
#include "editor/editor_plugin.h"
#include "editor/plugins/node_3d_editor_gizmos.h"
#endif
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/main/node.h"
#include "scene/resources/animation.h"
#include "scene/resources/animation_library.h"
#include "scene/resources/material.h"
#include "scene/resources/primitive_meshes.h"

#include "crit_spring_damper.h"

#define MAKE_RESOURCE_TYPE_HINT(m_type) vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, m_type)
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }
#define STR(x) #x
#define STRING_PREFIX(prefix, s) STR(prefix##s)
#define BINDER(type, variable, ...)                                                                             \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(set_, variable), "value"), &type::set_##variable, __VA_ARGS__); \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(get_, variable)), &type::get_##variable);
#define BINDER_PROPERTY(type, variant_type, variable, ...) \
	BINDER(type, variable, __VA_ARGS__)                    \
	ADD_PROPERTY(PropertyInfo(variant_type, #variable), STRING_PREFIX(set_, variable), STRING_PREFIX(get_, variable));

struct MotionFeature : public Resource {
	GDCLASS(MotionFeature, Resource)
public:
	static constexpr float delta = 0.016f;

	MotionFeature() {
		set_local_to_scene(true);
	}

	virtual void physics_update(double p_delta) {}

	virtual int get_dimension() { return 0; }

	virtual PackedFloat32Array get_weights() { return {}; }

	virtual void setup_nodes(Variant character) {}

	virtual void setup_for_animation(Ref<Animation> animation) {}
	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float p_time) { return {}; }

	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float p_delta) { return {}; }

	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) { return 0.0; }
#ifdef TOOLS_ENABLED
	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) { return; }
#endif
protected:
	static void _bind_methods();
};

#include "scene/3d/physics_body_3d.h"

struct RootVelocityMotionFeature : public MotionFeature {
	GDCLASS(RootVelocityMotionFeature, MotionFeature)
public:
	CharacterBody3D *body;
	CharacterBody3D *get_body() { return body; }
	void set_body(CharacterBody3D *value) { body = value; }
	int root_track_pos = -1, root_track_quat = -1; //, root_track_scale = -1;

	String root_bone_name = "%GeneralSkeleton:Root";
	void set_root_bone_name(String value) {
		root_bone_name = value;
	}
	String get_root_bone_name() { return root_bone_name; }

	virtual int get_dimension() override {
		return 3;
	}

	GETSET(float, weight, 1.0f);
	virtual PackedFloat32Array get_weights() override {
		PackedFloat32Array weights{ weight, weight, weight };
		return weights;
	}

	virtual void setup_nodes(Variant character) override {
		body = Object::cast_to<CharacterBody3D>(character);
	}
	virtual void setup_for_animation(Ref<Animation> animation) override {
		root_track_pos = animation->find_track(NodePath(root_bone_name), Animation::TrackType::TYPE_POSITION_3D);
		root_track_quat = animation->find_track(NodePath(root_bone_name), Animation::TrackType::TYPE_ROTATION_3D);
	}

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) override {
		Vector3 pos = animation->position_track_interpolate(root_track_pos, time);
		Vector3 prev_pos = animation->position_track_interpolate(root_track_pos, time - 0.1);
		Quaternion rotation = animation->rotation_track_interpolate(root_track_quat, time).normalized();

		Vector3 vel = rotation.xform_inv(pos - prev_pos) / 0.1;

		PackedFloat32Array result{};
		result.push_back(vel.x);
		result.push_back(vel.y);
		result.push_back(vel.z);
		return result;
	}

	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float p_delta) override {
		Vector3 vel = body->get_quaternion().xform_inv(body->get_velocity());
		PackedFloat32Array result{};
		result.push_back(vel.x);
		result.push_back(vel.y);
		result.push_back(vel.z);
		return result;
	}

	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) override {
		Vector3 data_vel = { to_convert[0], to_convert[1], to_convert[2] };
		return (body->get_velocity() - data_vel).length_squared();
	}

protected:
	static void _bind_methods();
#ifdef TOOLS_ENABLED
	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) override {
		if (data.size() == get_dimension()) {
			Vector3 vel = tr.xform(Vector3(data[0], data[1], data[2]));
			Ref<StandardMaterial3D> mat = gizmo->get_plugin()->get_material("white", gizmo);
			PackedVector3Array lines{ tr.origin, tr.origin + vel };
			gizmo->add_lines(lines, mat);
		}
	}
#endif
};

struct BonePositionVelocityMotionFeature : public MotionFeature {
	GDCLASS(BonePositionVelocityMotionFeature, MotionFeature)
	Skeleton3D *skeleton = nullptr;
	NodePath to_skeleton{};
	void set_to_skeleton(NodePath path);
	NodePath get_to_skeleton() { return to_skeleton; }
	PackedStringArray bone_names{};
	void set_bone_names(PackedStringArray value);
	PackedStringArray get_bone_names() { return bone_names; }
	PackedInt32Array bones_id{};
	HashMap<uint32_t, PackedInt32Array> bone_tracks{};
	CharacterBody3D *the_char = nullptr;
	virtual int get_dimension() override;
	virtual void setup_nodes(Variant character) override;
	virtual void setup_for_animation(Ref<Animation> animation) override;
	void set_skeleton_to_animation_timestamp(Ref<Animation> anim, float time);
	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) override;
	PackedVector3Array last_known_positions{};
	PackedVector3Array last_known_velocities{};
	PackedFloat32Array last_known_result{};
	float last_time_queried = 0.0f;
	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float p_delta) override;
	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) override;
	GETSET(float, weight_bone_pos, 1.0f);
	GETSET(float, weight_bone_vel, 1.0f);
	virtual PackedFloat32Array get_weights() override;
	float get_weight_bone_pos() const;

protected:
	static void _bind_methods();
#ifdef TOOLS_ENABLED
	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) override;
#endif
};

struct PredictionMotionFeature : public MotionFeature {
	GDCLASS(PredictionMotionFeature, MotionFeature)

	GETSET(Skeleton3D *, skeleton, nullptr);
	GETSET(String, root_bone_name, "%GeneralSkeleton:Root")

	GETSET(NodePath, character_path);

	GETSET(float, halflife_velocity, 0.2);
	GETSET(float, halflife_angular_velocity, 0.13);

	GETSET(PackedFloat32Array, past_time_dt);
	GETSET(PackedFloat32Array, future_time_dt);
	GETSET(int, past_count, 4);
	GETSET(float, past_delta, 0.7f / past_count);
	GETSET(int, future_count, 6);
	GETSET(float, future_delta, 1.2f / future_count);

	GETSET(float, weight_history_pos, 1.0f);
	GETSET(float, weight_prediction_pos, 1.0f);
	GETSET(float, weight_prediction_angle, 1.0f);
	virtual PackedFloat32Array get_weights() override;
	PredictionMotionFeature();

private:
	void create_default_dt();

public:
	virtual int get_dimension() override;

	CharacterBody3D *body = nullptr;
	virtual void setup_nodes(Variant character) override;

	int root_tracks[3] = { 0, 0, 0 };
	Vector3 start_pos, start_vel, end_pos, end_vel;
	Quaternion start_rot, end_rot, end_ang_vel;
	float start_time = 0.0f, end_time = 0.0f;

	virtual void setup_for_animation(Ref<Animation> animation) override;

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float p_time) override;

	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float p_delta) override;

	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) override { return 0.0; }

protected:
	static void _bind_methods();
#ifdef TOOLS_ENABLED
	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) override;
#endif
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX
#undef BINDER

#endif // MOTION_FEATURES_H