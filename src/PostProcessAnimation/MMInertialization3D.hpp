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

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skeleton_ik3d.hpp>
#include <godot_cpp/classes/skeleton_modifier3d.hpp>

#include <Math/KForm.hpp>

#include <MotionFeatures/MFBonesInfo.hpp>

using namespace godot;

// TODO : Inherit SkeletonModifier
struct MMInertialization3D : godot::SkeletonModifier3D {
	GDCLASS(MMInertialization3D, SkeletonModifier3D);
	friend class MFBonesInfo;

public:
	using u = godot::UtilityFunctions;

	enum InertializationType {
		Simple,
		OffsetDecay
	};

	GETSET(InertializationType, type, Simple);

	kforms offsets = { 0 };
	kforms bones = { 0 };
	kforms bone_model = { 0 };

	GETSET(float, halflife, 0.1f);

	virtual void _ready() override {
		inertialize_reset();
	}

	void inertialize_reset() {
		auto *skeleton = get_skeleton();
		if (skeleton == nullptr)
			return;
		const auto bone_count = skeleton->get_bone_count();
		bones.reserve(bone_count);
		offsets.reserve(bone_count);
		bone_model.reserve(bone_count);
		for (int b = 0; b < bone_count; ++b) {
			bones.reset(b);
			bones.pos[b] = skeleton->get_bone_pose_position(b);
			bones.rot[b] = skeleton->get_bone_pose_rotation(b);
			bones.scl[b] = skeleton->get_bone_pose_scale(b);

			offsets.reset(b);
		}
		for (int id = 0; id < skeleton->get_bone_count(); ++id) {
			int parent = skeleton->get_bone_parent(id);
			if (parent != -1) {
				bone_model[id] = (kform)bone_model[parent] * (kform)bones[id];
			} else {
				bone_model[id] = (kform)bones[id];
			}
		}
	}

	virtual void _process_modification() override {
		// Find delta
		const float delta = get_skeleton()->get_modifier_callback_mode_process() == Skeleton3D::ModifierCallbackModeProcess::MODIFIER_CALLBACK_MODE_PROCESS_IDLE ? get_process_delta_time() : get_physics_process_delta_time();
		if (is_active())
			advance(delta);
	}

	void advance(double delta) {
		auto *skeleton = get_skeleton();
		if (is_active() == false || skeleton == nullptr)
			return;
		switch (type) {
			case InertializationType::Simple: {
				_simple(delta);
				break;
			}
			case InertializationType::OffsetDecay: {
				_decay(delta);
				break;
			}
		}

		for (int id = 0; id < skeleton->get_bone_count(); ++id) {
			int parent = skeleton->get_bone_parent(id);
			kform step{};
			if (parent != -1) {
				step = (kform)bone_model[parent] * (kform)bones[id];
			} else {
				step = (kform)bones[id];
			}
			bone_model.pos[id] = step.pos;
			bone_model.vel[id] = step.vel;
			bone_model.rot[id] = step.rot;
			bone_model.ang[id] = step.ang;
			bone_model.scl[id] = step.scl;
			bone_model.svl[id] = step.svl;
		}
	}

	void _simple(double delta) {
		auto *skeleton = get_skeleton();

		bones.reserve(skeleton->get_bone_count());
		offsets.reserve(skeleton->get_bone_count());
		bone_model.reserve(skeleton->get_bone_count());

		for (auto bone_id = 0; bone_id < skeleton->get_bone_count(); ++bone_id) {
			kform desired{};
			desired.pos = skeleton->get_bone_pose_position(bone_id);
			desired.rot = skeleton->get_bone_pose_rotation(bone_id);
			// desired.vel = (desired.pos - bones.pos[bone_id]) / delta;
			// desired.ang = Spring::quat_differentiate_angular_velocity(desired.rot, bones.rot[bone_id], delta);

			Spring::_simple_spring_damper_exact(
					bones.pos[bone_id], bones.vel[bone_id], desired.pos, halflife, delta);
			Spring::_simple_spring_damper_exact(
					bones.rot[bone_id], bones.ang[bone_id], desired.rot, halflife, delta);

			skeleton->set_bone_pose_position(bone_id, bones.pos[bone_id]);
			skeleton->set_bone_pose_rotation(bone_id, bones.rot[bone_id]);
		}
	}

	void _decay(double delta) {
		auto *skeleton = get_skeleton();
		if (is_active() == false || skeleton == nullptr)
			return;

		bones.reserve(skeleton->get_bone_count());
		offsets.reserve(skeleton->get_bone_count());

		for (auto bone_id = 0; bone_id < skeleton->get_bone_count(); ++bone_id) {
			kform desired{};
			desired.pos = skeleton->get_bone_pose_position(bone_id);
			desired.rot = skeleton->get_bone_pose_rotation(bone_id);

			// Calculate offset
			kform offset{};
			Spring::inertialize_transition(
					offset.pos, offset.vel,
					bones.pos[bone_id], bones.vel[bone_id],
					desired.pos, desired.vel);
			Spring::inertialize_transition(
					offset.rot, offset.ang,
					bones.rot[bone_id], bones.ang[bone_id],
					desired.rot, desired.ang);

			// Reduce Offset
			Spring::inertialize_update(
					bones.pos[bone_id], bones.vel[bone_id],
					offset.pos, offset.vel,
					desired.pos, desired.vel,
					halflife,
					delta);
			Spring::inertialize_update(
					bones.rot[bone_id], bones.ang[bone_id],
					offset.rot, offset.ang,
					desired.rot, desired.ang,
					halflife,
					delta);

			skeleton->set_bone_pose_position(bone_id, bones.pos[bone_id]);
			skeleton->set_bone_pose_rotation(bone_id, bones.rot[bone_id]);
		}
	}

	Dictionary get_bone_model(StringName bone) const {
		return (Dictionary)bone_model[get_skeleton()->find_bone(bone)];
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_type", "value"), &MMInertialization3D::set_type, DEFVAL(InertializationType::Simple));
		ClassDB::bind_method(D_METHOD("get_type"), &MMInertialization3D::get_type);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "type", godot::PROPERTY_HINT_ENUM, "Simple"), "set_type", "get_type");

		ClassDB::bind_method(D_METHOD("set_halflife", "value"), &MMInertialization3D::set_halflife);
		ClassDB::bind_method(D_METHOD("get_halflife"), &MMInertialization3D::get_halflife);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "halflife", PROPERTY_HINT_RANGE, "0.0,1.0,0.01,or_greater"), "set_halflife", "get_halflife");

		ClassDB::bind_method(D_METHOD("get_bone_model", "bone"), &MMInertialization3D::get_bone_model);

		BIND_ENUM_CONSTANT(Simple);
		BIND_ENUM_CONSTANT(OffsetDecay);
	}
};

VARIANT_ENUM_CAST(MMInertialization3D::InertializationType);