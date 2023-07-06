
#ifndef MOTION_FEATURES_HPP
#define MOTION_FEATURES_HPP

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
#include "editor/editor_plugin.h"
#include "editor/plugins/node_3d_editor_gizmos.h"
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

	virtual void physics_update(double delta) {}

	virtual int get_dimension() { return 0; }

	virtual PackedFloat32Array get_weights() { return {}; }

	virtual void setup_nodes(Variant character) {}

	virtual void setup_for_animation(Ref<Animation> animation) {}
	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) { return {}; }

	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float delta) { return {}; }

	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) { return 0.0; }

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) { return; }

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_dimension"), &MotionFeature::get_dimension);

		ClassDB::bind_method(D_METHOD("get_weights"), &MotionFeature::get_weights);

		// BIND_VIRTUAL_METHOD(MotionFeature,get_dimension);

		ClassDB::bind_method(D_METHOD("setup_nodes", "character"), &MotionFeature::setup_nodes);

		ClassDB::bind_method(D_METHOD("setup_for_animation", "animation"), &MotionFeature::setup_for_animation);
		ClassDB::bind_method(D_METHOD("bake_animation_pose", "animation", "time"), &MotionFeature::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("broadphase_query_pose", "blackboard", "delta"), &MotionFeature::broadphase_query_pose);
		ClassDB::bind_method(D_METHOD("narrowphase_evaluate_cost", "data_to_evaluate"), &MotionFeature::narrowphase_evaluate_cost);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &MotionFeature::debug_pose_gizmo);
	}
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
		// Node::get_node();
		body = Object::cast_to<CharacterBody3D>(character);
	}
	virtual void setup_for_animation(Ref<Animation> animation) override {
		root_track_pos = animation->find_track(NodePath(root_bone_name), Animation::TrackType::TYPE_POSITION_3D);
		root_track_quat = animation->find_track(NodePath(root_bone_name), Animation::TrackType::TYPE_ROTATION_3D);
		// root_track_scale = animation->find_track(NodePath(root_bone_name),Animation::TrackType::TYPE_SCALE_3D);
		// u::prints("Root Tracks for",root_track_pos,root_track_quat);
	}

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) {
		auto pos = animation->position_track_interpolate(root_track_pos, time);
		auto prev_pos = animation->position_track_interpolate(root_track_pos, time - 0.1);
		Quaternion rotation = animation->rotation_track_interpolate(root_track_quat, time).normalized();

		Vector3 vel = rotation.xform_inv(pos - prev_pos) / 0.1;

		PackedFloat32Array result{};
		result.push_back(vel.x);
		result.push_back(vel.y);
		result.push_back(vel.z);
		return result;
	}

	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float delta) override {
		auto vel = body->get_quaternion().xform_inv(body->get_velocity());
		PackedFloat32Array result{};
		result.push_back(vel.x);
		result.push_back(vel.y);
		result.push_back(vel.z);
		return result;
	}

	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) {
		Vector3 data_vel = { to_convert[0], to_convert[1], to_convert[2] };
		return (body->get_velocity() - data_vel).length_squared();
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_weight", "value"), &RootVelocityMotionFeature::set_weight, DEFVAL(1.0f));
		ClassDB::bind_method(D_METHOD("get_weight"), &RootVelocityMotionFeature::get_weight);
		ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");

		// ClassDB::bind_method( D_METHOD("set_body","value"), &RootVelocityMotionFeature::set_body);
		// ClassDB::bind_method( D_METHOD("get_body"), &RootVelocityMotionFeature::get_body);
		// ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH,"Character",godot::PROPERTY_HINT_NODE_PATH_VALID_TYPES,"CharacterBody3D"),"set_body","get_body");

		ClassDB::bind_method(D_METHOD("set_root_bone_name", "value"), &RootVelocityMotionFeature::set_root_bone_name, DEFVAL("%GeneralSkeleton:Root"));
		ClassDB::bind_method(D_METHOD("get_root_bone_name"), &RootVelocityMotionFeature::get_root_bone_name);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "Root Bone"), "set_root_bone_name", "get_root_bone_name");

		ClassDB::bind_method(D_METHOD("get_dimension"), &RootVelocityMotionFeature::get_dimension);
		ClassDB::bind_method(D_METHOD("setup_nodes", "character"), &RootVelocityMotionFeature::setup_nodes);

		// ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &RootVelocityMotionFeature::setup_for_animation);
		// ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &RootVelocityMotionFeature::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("broadphase_query_pose", "blackboard", "delta"), &RootVelocityMotionFeature::broadphase_query_pose);
		// ClassDB::bind_method( D_METHOD("narrowphase_evaluate_cost","data_to_evaluate"), &RootVelocityMotionFeature::narrowphase_evaluate_cost);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &RootVelocityMotionFeature::debug_pose_gizmo);
	}

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) override {
		if (data.size() == get_dimension()) {
			Vector3 vel = tr.xform(Vector3(data[0], data[1], data[2]));
			auto mat = gizmo->get_plugin()->get_material("white", gizmo);
			PackedVector3Array lines{ tr.origin, tr.origin + vel };
			gizmo->add_lines(lines, mat);
		}
	}
};

struct BonePositionVelocityMotionFeature : public MotionFeature {
	GDCLASS(BonePositionVelocityMotionFeature, MotionFeature)
	Skeleton3D *skeleton = nullptr;
	NodePath to_skeleton{};
	void set_to_skeleton(NodePath path) {
		if (is_local_to_scene() && get_local_scene() != nullptr) {
			auto mp = get_local_scene()->get_node_or_null(NodePath("MotionPlayer"));
			skeleton = cast_to<Skeleton3D>(mp->get_node(path));
			to_skeleton = get_local_scene()->get_path_to(skeleton);
		}
	}
	NodePath get_to_skeleton() { return to_skeleton; }

	PackedStringArray bone_names{};
	void set_bone_names(PackedStringArray value) {
		bone_names = value;
	}
	PackedStringArray get_bone_names() { return bone_names; }

	PackedInt32Array bones_id{};

	HashMap<uint32_t, PackedInt32Array> bone_tracks{};

	CharacterBody3D *the_char = nullptr;

	virtual int get_dimension() override {
		return bone_names.size() * 3 * 2;
	}
	virtual void setup_nodes(Variant character) override {
		the_char = Object::cast_to<CharacterBody3D>(character);
		skeleton = cast_to<Skeleton3D>(the_char->get_node(NodePath("Armature/GeneralSkeleton")));

		bones_id.clear();
		if (skeleton != nullptr) {
			for (size_t i = 0; i < bone_names.size(); ++i) {
				const size_t id = skeleton->find_bone(bone_names[i]);
				if (id >= 0) {
					bones_id.push_back(id);
				}
			}
			print_line(vformat("Bones id %s %s", bone_names, bones_id));
		}

		last_known_positions.resize(bones_id.size());
		last_known_positions.fill({});
		last_known_velocities.resize(bones_id.size());
		last_known_velocities.fill({});
		last_known_result.resize(bones_id.size() * 2 * 3);
		last_known_velocities.fill({});
	}
	virtual void setup_for_animation(Ref<Animation> animation) override {
		skeleton->reset_bone_poses();
		bone_tracks.clear();
		bones_id.clear();
		for (size_t i = 0; i < bone_names.size(); ++i) {
			const size_t id = skeleton->find_bone(bone_names[i]);
			if (id >= 0) {
				bones_id.push_back(id);
			}
		}
		for (auto bone_id = 0; bone_id < skeleton->get_bone_count(); ++bone_id) {
			const auto bone_name = "%GeneralSkeleton:" + skeleton->get_bone_name(bone_id);
			PackedInt32Array tracks{};
			tracks.push_back(animation->find_track(NodePath(bone_name), Animation::TrackType::TYPE_POSITION_3D));
			tracks.push_back(animation->find_track(NodePath(bone_name), Animation::TrackType::TYPE_ROTATION_3D));
			//tracks.push_back(animation->find_track(NodePath(bone_name),Animation::TrackType::TYPE_SCALE_3D));
			bone_tracks[bone_id] = tracks;
		}
	}

	void set_skeleton_to_animation_timestamp(Ref<Animation> anim, float time) {
		// UtilityFunctions::print((skeleton == nullptr)?"Skeleton error, path not found":"Skeleton set");
		if (anim == nullptr || skeleton == nullptr) {
			return;
		}
		for (size_t bone_id = 0; bone_id < skeleton->get_bone_count(); ++bone_id) {
			if (!bone_tracks.has(bone_id)) {
				continue;
			}
			const auto pos = bone_tracks[bone_id][0];
			const auto quat = bone_tracks[bone_id][1];
			// const auto scale = bone_tracks[bone_id][2];
			if (pos >= 0) {
				const Vector3 position = anim->position_track_interpolate(pos, time);
				skeleton->set_bone_pose_position(bone_id, position);
			}

			if (quat >= 0) {
				const Quaternion rotation = anim->rotation_track_interpolate(quat, time);
				skeleton->set_bone_pose_rotation(bone_id, rotation);
			}

			// const Vector3 scaling = anim->scale_track_interpolate(scale,time);

			// skeleton->set_bone_pose_scale(bone_id,scaling);
		}
	}

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) {
		PackedVector3Array prev_pos{}, curr_pos{};
		PackedFloat32Array result{};
		set_skeleton_to_animation_timestamp(animation, time - 0.1);
		for (size_t index = 0; index < bones_id.size(); ++index) {
			const auto bone_id = bones_id[index];
			prev_pos.push_back(skeleton->get_bone_global_pose(bone_id).get_origin() * skeleton->get_motion_scale());
		}
		set_skeleton_to_animation_timestamp(animation, time);
		for (size_t index = 0; index < bones_id.size(); ++index) {
			const auto bone_id = bones_id[index];
			curr_pos.push_back(skeleton->get_bone_global_pose(bone_id).get_origin() * skeleton->get_motion_scale());
		}
		const size_t root_id = 0;
		const Transform3D root = skeleton->get_bone_global_pose(root_id) * skeleton->get_motion_scale();

		for (size_t index = 0; index < bones_id.size(); ++index) {
			const auto pos = root.basis.xform_inv(curr_pos[index] - root.get_origin());
			result.push_back(pos.x);
			result.push_back(pos.y);
			result.push_back(pos.z);
			const auto vel = root.basis.xform_inv(curr_pos[index] - prev_pos[index]) / 0.1;
			result.push_back(vel.x);
			result.push_back(vel.y);
			result.push_back(vel.z);
		}
		return result;
	}

	PackedVector3Array last_known_positions{};
	PackedVector3Array last_known_velocities{};
	PackedFloat32Array last_known_result{};
	float last_time_queried = 0.0f;

	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float delta) override {
		PackedVector3Array current_positions{}, current_velocities{};
		last_known_result.resize(bones_id.size() * 2 * 3);
		current_positions.resize(bones_id.size());
		current_velocities.resize(bones_id.size());

		float curr_time = Time::get_singleton()->get_ticks_msec();

		for (size_t index = 0; index < bones_id.size(); ++index) {
			Vector3 pos = skeleton->get_bone_global_pose(bones_id[index]).origin;
			Vector3 vel = (pos - last_known_positions[index]) / delta;
			current_positions.write[index] = pos;
			current_velocities.write[index] = vel;
		}

		last_time_queried = curr_time;
		last_known_positions = current_positions.duplicate();
		last_known_velocities = current_velocities.duplicate();

		const size_t size = 3;
		for (size_t i = 0; i < bones_id.size(); ++i) {
			Vector3 pos = current_positions[i], vel = current_velocities[i];

			last_known_result.write[i * size * 2] = pos.x;
			last_known_result.write[i * size * 2 + 1] = pos.y;
			last_known_result.write[i * size * 2 + 2] = pos.z;
			last_known_result.write[i * size * 2 + size] = vel.x;
			last_known_result.write[i * size * 2 + size + 1] = vel.y;
			last_known_result.write[i * size * 2 + size + 2] = vel.z;
		}
		return last_known_result;
	}

	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) {
		if (to_convert.size() != last_known_result.size()) {
			return std::numeric_limits<float>::max();
		}
		float cost = 0.0;
		for (auto i = 0; i < to_convert.size(); ++i) {
			cost += abs(to_convert[i] * to_convert[i] - last_known_result[i] * last_known_result[i]);
		}

		return cost;
	}

	GETSET(float, weight_bone_pos, 1.0f);
	GETSET(float, weight_bone_vel, 1.0f);
	virtual PackedFloat32Array get_weights() override {
		PackedFloat32Array result{};
		for (auto i = 0; i < 3 * bone_names.size(); ++i) {
			result.append(weight_bone_pos);
		}
		for (auto i = 0; i < 3 * bone_names.size(); ++i) {
			result.append(weight_bone_vel);
		}
		return result;
	}
    float get_weight_bone_pos() const {
        return weight_bone_pos;
    }

protected:
	static void _bind_methods() {
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BonePositionVelocityMotionFeature"), "set_weight_bone_pos", "get_weight_bone_pos");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "BonePositionVelocityMotionFeature"), "set_weight_bone_vel", "get_weight_bone_vel");
		ClassDB::bind_method(D_METHOD("set_to_skeleton", "value"), &BonePositionVelocityMotionFeature::set_to_skeleton);
		ClassDB::bind_method(D_METHOD("get_to_skeleton"), &BonePositionVelocityMotionFeature::get_to_skeleton);
		ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "Skeleton", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_to_skeleton", "get_to_skeleton");

		ClassDB::bind_method(D_METHOD("set_bone_names", "value"), &BonePositionVelocityMotionFeature::set_bone_names);
		ClassDB::bind_method(D_METHOD("get_bone_names"), &BonePositionVelocityMotionFeature::get_bone_names);
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "Bones"), "set_bone_names", "get_bone_names");

		ClassDB::bind_method(D_METHOD("get_dimension"), &BonePositionVelocityMotionFeature::get_dimension);
		ClassDB::bind_method(D_METHOD("setup_nodes", "character"), &BonePositionVelocityMotionFeature::setup_nodes);

		ClassDB::bind_method(D_METHOD("setup_for_animation", "animation"), &RootVelocityMotionFeature::setup_for_animation);
		ClassDB::bind_method(D_METHOD("bake_animation_pose", "animation", "time"), &RootVelocityMotionFeature::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("broadphase_query_pose", "blackboard", "delta"), &BonePositionVelocityMotionFeature::broadphase_query_pose);
		// ClassDB::bind_method( D_METHOD("narrowphase_evaluate_cost","data_to_evaluate"), &RootVelocityMotionFeature::narrowphase_evaluate_cost);
		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &BonePositionVelocityMotionFeature::debug_pose_gizmo);
	}

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) {
		// if (data.size() == get_dimension())
		{
			constexpr int s = 3;
			for (size_t index = 0; index < bone_names.size(); ++index) {
				//i*size*2+size+2
				Vector3 pos = Vector3(data[index * s * 2 + 0], data[index * s * 2 + 1], data[index * s * 2 + 2]);
				Vector3 vel = Vector3(data[index * s * 2 + s + 0], data[index * s * 2 + s + 1], data[index * s * 2 + s + 2]);
				pos = tr.xform(pos);
				vel = tr.xform(vel);
				auto white = gizmo->get_plugin()->get_material("white", gizmo);
				auto blue = gizmo->get_plugin()->get_material("blue", gizmo);
				gizmo->add_lines(PackedVector3Array{ pos, pos + vel }, blue);
				auto box = Ref<BoxMesh>();
				box.instantiate();
				box->set_size(Vector3(0.05f, 0.05f, 0.05f));
				Transform3D tr = Transform3D(Basis(), pos);
				gizmo->add_mesh(box, white, tr);
			}
		}
	}
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
	virtual PackedFloat32Array get_weights() override {
		PackedFloat32Array result{};
		for (auto i = 0; i < 2 * past_time_dt.size(); ++i) {
			result.append(weight_history_pos);
		}
		for (auto i = 0; i < 2 * future_time_dt.size(); ++i) {
			result.append(weight_prediction_pos);
		}
		for (auto i = 0; i < 1 * future_time_dt.size(); ++i) {
			result.append(weight_prediction_angle);
		}
		return result;
	}

	PredictionMotionFeature() {
		set_local_to_scene(true);
	}

private:
	void create_default_dt() {
		past_time_dt.clear();
		future_time_dt.clear();
		float time = past_delta;
		for (int count = 0; count < past_count; ++count, time += past_delta) {
			past_time_dt.push_back(-time);
		}
		time = future_delta;
		for (int count = 0; count < future_count; ++count, time += future_delta) {
			future_time_dt.push_back(time);
		}
	}

public:
	virtual int get_dimension() override {
		// Offset for each
		const size_t past_pos = 2 * past_time_dt.size();
		const size_t future_pos = 2 * future_time_dt.size();
		const size_t future_rot_angle = future_time_dt.size();
		return past_pos + future_pos + future_rot_angle;
	}

	CharacterBody3D *body = nullptr;
	virtual void setup_nodes(Variant character) override {
		auto n = Object::cast_to<CharacterBody3D>(character);
		skeleton = cast_to<Skeleton3D>(n->get_node(NodePath("Armature/GeneralSkeleton")));
	}

	int root_tracks[3] = { 0, 0, 0 };
	Vector3 start_pos, start_vel, end_pos, end_vel;
	Quaternion start_rot, end_rot, end_ang_vel;
	float start_time = 0.0f, end_time = 0.0f;

	virtual void setup_for_animation(Ref<Animation> animation) override {
		start_time = 0.1f;
		end_time = std::floor(animation->get_length() * 10) / 10.0f;
		root_tracks[0] = animation->find_track(root_bone_name, Animation::TrackType::TYPE_POSITION_3D);
		root_tracks[1] = animation->find_track(root_bone_name, Animation::TrackType::TYPE_ROTATION_3D);
		root_tracks[2] = animation->find_track(root_bone_name, Animation::TrackType::TYPE_SCALE_3D);
		{
			start_pos = animation->position_track_interpolate(root_tracks[0], 0.0);
			start_rot = animation->rotation_track_interpolate(root_tracks[1], 0.0);
			start_vel = (animation->position_track_interpolate(root_tracks[0], 0.1) - start_pos) / 0.1;
		}
		{
			end_pos = animation->position_track_interpolate(root_tracks[0], end_time);
			end_rot = animation->rotation_track_interpolate(root_tracks[1], end_time);
			end_vel = (end_pos - animation->position_track_interpolate(root_tracks[0], end_time - 0.1)) / 0.1;

			end_ang_vel = animation->rotation_track_interpolate(root_tracks[1], animation->get_length() - delta - 0.1).inverse() * animation->rotation_track_interpolate(root_tracks[1], animation->get_length() - delta);
		}
	}

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) override {
		PackedFloat32Array result{};
		Vector3 curr_pos = animation->position_track_interpolate(root_tracks[0], time);
		Quaternion curr_rot = animation->rotation_track_interpolate(root_tracks[1], time);

		for (size_t index = 0; index < past_time_dt.size(); ++index) {
			const float t = time - abs(past_time_dt[index]);
			Vector3 pos{};
			Quaternion rot{};
			if (t >= 0.0f) { // The offset can be accessed through the anim data
				pos = animation->position_track_interpolate(root_tracks[0], t) - curr_pos;
				rot = animation->rotation_track_interpolate(root_tracks[1], t);
				pos = curr_rot.xform_inv(pos);
			} else { // The offset must be calculated using the starting velocity and extrapoling
				pos = start_pos + (start_vel * t) - curr_pos;
				pos = curr_rot.xform_inv(pos);
			}
			result.push_back(pos.x);
			result.push_back(pos.z);
		}
		for (size_t index = 0; index < future_time_dt.size(); ++index) {
			const float t = time + abs(future_time_dt[index]);
			Vector3 pos{};
			Quaternion rot{};
			if (t <= end_time) { // The offset can be accessed through the anim data
				pos = animation->position_track_interpolate(root_tracks[0], t) - curr_pos;
				rot = animation->rotation_track_interpolate(root_tracks[1], t);
				pos = curr_rot.xform_inv(pos);
			} else { // The offset must be calculated using the end velocity and extrapoling
				pos = end_pos + end_vel * (t - end_time) - curr_pos;
				pos = curr_rot.xform_inv(pos);
			}

			result.push_back(pos.x);
			result.push_back(pos.z);
		}
		for (size_t index = 0; index < future_time_dt.size(); ++index) {
			const float t = time + abs(future_time_dt[index]);
			Vector3 pos{};
			Quaternion rot{};
			if (t <= end_time) { // The offset can be accessed through the anim data
				rot = animation->rotation_track_interpolate(root_tracks[1], t) * curr_rot.inverse();
			} else { // The offset must be calculated using the end velocity and extrapoling
				rot = animation->rotation_track_interpolate(root_tracks[1], animation->get_length() - delta) * curr_rot.inverse();
			}

			result.push_back(rot.get_euler(EulerOrder::XYZ).y);
		}
		return result;
	}

	virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float delta) override {
		PackedFloat32Array result{};
		Array blackboard_array;
		blackboard_array.resize(3);
		blackboard_array[0] = "history";
		blackboard_array[1] = "prediction";
		blackboard_array[2] = "pred_dir";
		if (!blackboard.has_all(blackboard_array)) {
			return result;
		}

		PackedVector3Array history = PackedVector3Array(blackboard["history"]);
		PackedVector3Array prediction = PackedVector3Array(blackboard["prediction"]);
		PackedFloat32Array direction = PackedFloat32Array(blackboard["pred_dir"]);
		bool valid = false;
		{
			for (auto elem : history) {
				result.append(elem.x);
				result.append(elem.z);
			}
			for (auto elem : prediction) {
				result.append(elem.x);
				result.append(elem.z);
			}
			for (auto elem : direction) {
				result.append(elem);
			}
		}
		return result;
	}

	virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert) { return 0.0; }

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_weight_history_pos"), &PredictionMotionFeature::set_weight_history_pos);
		ClassDB::bind_method(D_METHOD("get_weight_history_pos"), &PredictionMotionFeature::get_weight_history_pos);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight_history_pos"), "set_weight_history_pos", "get_weight_history_pos");
		ClassDB::bind_method(D_METHOD("set_weight_prediction_pos"), &PredictionMotionFeature::set_weight_prediction_pos);
		ClassDB::bind_method(D_METHOD("get_weight_prediction_pos"), &PredictionMotionFeature::get_weight_prediction_pos);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight_prediction_pos"), "set_weight_prediction_pos", "get_weight_prediction_pos");
		ClassDB::bind_method(D_METHOD("set_weight_prediction_angle"), &PredictionMotionFeature::set_weight_prediction_angle);
		ClassDB::bind_method(D_METHOD("get_weight_prediction_angle"), &PredictionMotionFeature::get_weight_prediction_angle);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight_prediction_angle"), "set_weight_prediction_angle", "get_weight_prediction_angle");

		PackedFloat32Array m_default{};
		m_default.push_back(0.2);
		m_default.push_back(0.4);
		ClassDB::bind_method(D_METHOD("set_root_bone_name", "root_bone_name"), &PredictionMotionFeature::set_weight_prediction_angle, DEFVAL("%GeneralSkeleton:Root"));
		ClassDB::bind_method(D_METHOD("get_root_bone_name"), &PredictionMotionFeature::get_weight_prediction_angle);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "root_bone_name"), "set_root_bone_name", "get_root_bone_name");
		ClassDB::bind_method(D_METHOD("set_past_time_dt", "past_time_delta"), &PredictionMotionFeature::set_past_time_dt, DEFVAL(m_default));
		ClassDB::bind_method(D_METHOD("get_past_time_dt"), &PredictionMotionFeature::get_past_time_dt);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "past_time_dt"), "set_past_time_dt", "get_past_time_dt");
		ClassDB::bind_method(D_METHOD("set_future_time_dt", "future_time_delta"), &PredictionMotionFeature::set_future_time_dt);
		ClassDB::bind_method(D_METHOD("get_future_time_dt"), &PredictionMotionFeature::get_future_time_dt);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "future_time_dt"), "set_future_time_dt", "get_future_time_dt");

		ClassDB::bind_method(D_METHOD("get_dimension"), &PredictionMotionFeature::get_dimension);

		ClassDB::bind_method(D_METHOD("setup_nodes", "character"), &PredictionMotionFeature::setup_nodes);

		ClassDB::bind_method(D_METHOD("setup_for_animation", "animation"), &PredictionMotionFeature::setup_for_animation);
		ClassDB::bind_method(D_METHOD("bake_animation_pose", "animation", "time"), &PredictionMotionFeature::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("broadphase_query_pose", "blackboard", "delta"), &PredictionMotionFeature::broadphase_query_pose);
		ClassDB::bind_method(D_METHOD("narrowphase_evaluate_cost", "data_to_evaluate"), &PredictionMotionFeature::narrowphase_evaluate_cost);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &PredictionMotionFeature::debug_pose_gizmo);
	}

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, Transform3D tr = Transform3D{}) {
		{
			constexpr int s = 3;
			auto white = gizmo->get_plugin()->get_material("white", gizmo);
			auto green = gizmo->get_plugin()->get_material("green", gizmo);
			auto orange = gizmo->get_plugin()->get_material("orange", gizmo);
			for (size_t i = 0; i < past_time_dt.size(); ++i) {
				const size_t offset = i * 2;
				Vector3 pos = Vector3(data[offset + 0], 0, data[offset + 1]);
				pos = tr.xform(pos);
				gizmo->add_lines(PackedVector3Array{ pos, pos + Vector3(0, 1, 0) }, green);
			}
			const size_t pos_offset = past_time_dt.size();
			const size_t traj_offset = past_time_dt.size() * 2 + future_time_dt.size() * 2;
			for (size_t i = 0; i < future_time_dt.size(); ++i) {
				const size_t offset = (pos_offset + i) * 2;
				Vector3 pos = Vector3(data[offset + 0], 0, data[offset + 1]);
				Vector3 traj = tr.xform(Vector3(0, 0, 1)).rotated(Vector3(0, 1, 0), data[traj_offset + i]);
				pos = tr.xform(pos);
				// traj = tr.xform(traj);
				gizmo->add_lines(PackedVector3Array{ pos, pos + traj }, orange);
			}
		}
	}
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX
#undef BINDER

#endif