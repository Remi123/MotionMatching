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


#include <MMAnimationLibrary.hpp>
#include <MotionFeatures/MotionFeatures.hpp>


using namespace godot;
using u = godot::UtilityFunctions;

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }

struct MFTrajectory : public MotionFeature {
	GDCLASS(MFTrajectory, MotionFeature)
public:
	virtual ~MFTrajectory() = default;

	Skeleton3D *skeleton{ nullptr };
	Skeleton3D *get_skeleton() { return skeleton; }
	void set_skeleton(Skeleton3D *value) { skeleton = value; }
	String root_bone_track = "%GeneralSkeleton:Root";

	GETSET(PackedFloat32Array, past_time_dt);
	GETSET(PackedFloat32Array, future_time_dt);

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
		for (auto i = 0; i < 2 * future_time_dt.size(); ++i) {
			result.append(weight_prediction_angle);
		}
		return result;
	}

public:
	virtual int get_dimension() override {
		// Offset for each
		const size_t past_pos = 2 * past_time_dt.size();
		const size_t future_pos = 2 * future_time_dt.size();
		const size_t future_rot_angle = 2 * future_time_dt.size();
		return past_pos + future_pos + future_rot_angle;
	}

	int root_tracks[3] = { 0, 0, 0 };
	Vector3 start_pos, start_vel, end_pos, end_vel;
	Quaternion start_rot, end_rot, end_ang_vel;
	float start_time = 0.0f, end_time = 0.0f;

	virtual bool setup_bake_init(Ref<MMAnimationLibrary> animlib) override {
		ERR_FAIL_COND_V_EDMSG(animlib->get_skeleton_path().is_empty(), false, "SkeletonPath is Empty");
		ERR_FAIL_COND_V_EDMSG(animlib->get_skeleton_profile() == nullptr, false, "SkeletonProfile is null");
		ERR_FAIL_COND_V_EDMSG(animlib->get_skeleton_profile()->get_root_bone().is_empty(), false, "No Root bone to extract data");
		root_bone_track = u::str(animlib->get_skeleton_path()) + ":" + animlib->get_skeleton_profile()->get_root_bone();
		return true;
	};

	virtual bool setup_bake_animation(Ref<Animation> animation) override {
		start_time = 0.1f;
		end_time = std::floor(animation->get_length() * 10) / 10.0f;
		root_tracks[0] = animation->find_track(root_bone_track, Animation::TrackType::TYPE_POSITION_3D);
		root_tracks[1] = animation->find_track(root_bone_track, Animation::TrackType::TYPE_ROTATION_3D);
		root_tracks[2] = animation->find_track(root_bone_track, Animation::TrackType::TYPE_SCALE_3D);
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
		return true;
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

			Vector3 direction = rot.xform(Vector3(0, 0, 1));

			result.push_back(direction.x);
			result.push_back(direction.z);
		}
		return result;
	}

	GETSET(PackedVector3Array, history_pos)
	GETSET(PackedVector3Array, future_pos)
	GETSET(PackedFloat32Array, future_dir)

	PackedFloat32Array serialize_trajectory_local(PackedVector3Array p_history_pos, PackedVector3Array p_future_pos, PackedVector3Array p_future_dir) {
		PackedFloat32Array result{};
		for (auto elem : p_history_pos) {
			result.append(elem.x);
			result.append(elem.z);
		}
		for (auto elem : p_future_pos) {
			result.append(elem.x);
			result.append(elem.z);
		}
		for (auto elem : p_future_dir) {
			result.append(elem.x);
			result.append(elem.z);
		}
		return result;
	}

	virtual PackedStringArray get_hints() const override {
		PackedStringArray result{};

		for (auto elem : past_time_dt) {
			result.append("Px-" + u::str(std::format("{:.2f}", elem).c_str()));
			result.append("Pz-" + u::str(std::format("{:.2f}", elem).c_str()));
		}
		for (auto elem : future_time_dt) {
			result.append("Px+" + u::str(std::format("{:.2f}", elem).c_str()));
			result.append("Pz+" + u::str(std::format("{:.2f}", elem).c_str()));
		}
		for (auto elem : future_time_dt) {
			result.append("Dx+" + u::str(std::format("{:.2f}", elem).c_str()));
			result.append("Dz+" + u::str(std::format("{:.2f}", elem).c_str()));
		}

		return result;
	}

protected:
	static void _bind_methods() {
		{
			ClassDB::bind_method(D_METHOD("serialize_trajectory_local", "history_local_pos", "prediction_local_pos", "prediction_local_direction"), &MFTrajectory::serialize_trajectory_local);
		}

		ClassDB::bind_method(D_METHOD("get_hints"), &MFTrajectory::get_hints);

		PackedFloat32Array m_default{};
		m_default.push_back(0.2);
		m_default.push_back(0.4);
		ClassDB::bind_method(D_METHOD("set_past_time_dt", "value"), &MFTrajectory::set_past_time_dt, (m_default));
		ClassDB::bind_method(D_METHOD("get_past_time_dt"), &MFTrajectory::get_past_time_dt);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "past_time_dt"), "set_past_time_dt", "get_past_time_dt");
		ClassDB::bind_method(D_METHOD("set_future_time_dt", "value"), &MFTrajectory::set_future_time_dt);
		ClassDB::bind_method(D_METHOD("get_future_time_dt"), &MFTrajectory::get_future_time_dt);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "future_time_dt"), "set_future_time_dt", "get_future_time_dt");

		ClassDB::bind_method(D_METHOD("set_debug_color_history", "value"), &MFTrajectory::set_debug_color_history);
		ClassDB::bind_method(D_METHOD("get_debug_color_history"), &MFTrajectory::get_debug_color_history);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_history"), "set_debug_color_history", "get_debug_color_history");

		ClassDB::bind_method(D_METHOD("set_weight_history_pos", "value"), &MFTrajectory::set_weight_history_pos);
		ClassDB::bind_method(D_METHOD("get_weight_history_pos"), &MFTrajectory::get_weight_history_pos);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_history_pos"), "set_weight_history_pos", "get_weight_history_pos");
		ClassDB::bind_method(D_METHOD("set_weight_prediction_pos", "value"), &MFTrajectory::set_weight_prediction_pos);
		ClassDB::bind_method(D_METHOD("get_weight_prediction_pos"), &MFTrajectory::get_weight_prediction_pos);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_prediction_pos"), "set_weight_prediction_pos", "get_weight_prediction_pos");
		ClassDB::bind_method(D_METHOD("set_weight_prediction_angle", "value"), &MFTrajectory::set_weight_prediction_angle);
		ClassDB::bind_method(D_METHOD("get_weight_prediction_angle"), &MFTrajectory::get_weight_prediction_angle);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_prediction_angle"), "set_weight_prediction_angle", "get_weight_prediction_angle");

		ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
		{
			ClassDB::bind_method(D_METHOD("set_debug_color_future", "value"), &MFTrajectory::set_debug_color_future);
			ClassDB::bind_method(D_METHOD("get_debug_color_future"), &MFTrajectory::get_debug_color_future);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_future"), "set_debug_color_future", "get_debug_color_future");
		}
		ClassDB::add_property_group(get_class_static(), "Queries to fill", "query");
		{
			//BINDER_PROPERTY_PARAMS(MFTrajectory, Variant::PACKED_VECTOR3_ARRAY, history_pos);
			ClassDB::bind_method(D_METHOD("set_history_pos", "value"), &MFTrajectory::set_history_pos);
			ClassDB::bind_method(D_METHOD("get_history_pos"), &MFTrajectory::get_history_pos);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "query_history_pos"), "set_history_pos", "get_history_pos");

			//BINDER_PROPERTY_PARAMS(MFTrajectory, Variant::PACKED_VECTOR3_ARRAY, future_pos);
			ClassDB::bind_method(D_METHOD("set_future_pos", "value"), &MFTrajectory::set_future_pos);
			ClassDB::bind_method(D_METHOD("get_future_pos"), &MFTrajectory::get_future_pos);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "query_future_pos"), "set_future_pos", "get_future_pos");

			//BINDER_PROPERTY_PARAMS(MFTrajectory, Variant::PACKED_FLOAT32_ARRAY, future_dir);
			ClassDB::bind_method(D_METHOD("set_future_dir", "value"), &MFTrajectory::set_future_dir);
			ClassDB::bind_method(D_METHOD("get_future_dir"), &MFTrajectory::get_future_dir);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "query_future_dir"), "set_future_dir", "get_future_dir");
		}
		ClassDB::add_property_group(get_class_static(), "", "");

		ClassDB::bind_method(D_METHOD("get_weights"), &MFTrajectory::get_weights);
		ClassDB::bind_method(D_METHOD("get_dimension"), &MFTrajectory::get_dimension);

		ClassDB::bind_method(D_METHOD("setup_bake_init", "mm_animation_library"), &MFTrajectory::setup_bake_init);
		ClassDB::bind_method(D_METHOD("setup_bake_animation", "animation"), &MFTrajectory::setup_bake_animation);

		ClassDB::bind_method(D_METHOD("bake_animation_pose", "animation", "time"), &MFTrajectory::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &MFTrajectory::debug_pose_gizmo);
	}

	GETSET(Color, debug_color_history, godot::Color(1.0f, 1.0f, 1.0f));
	GETSET(Color, debug_color_future, godot::Color(0.0f, 0.0f, 0.0f));

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, godot::Transform3D tr = godot::Transform3D{}) override {
		const auto mat_name_history = "history" + get_path();
		const auto mat_name_future = "future" + get_path();
		if (gizmo->get_plugin()->get_material(mat_name_history, gizmo) == nullptr) {
			gizmo->get_plugin()->create_material(mat_name_history, debug_color_history);
		}
		if (gizmo->get_plugin()->get_material(mat_name_future, gizmo) == nullptr) {
			gizmo->get_plugin()->create_material(mat_name_future, debug_color_future);
		}
		// if (data.size() == get_dimension())
		{
			constexpr int s = 3;
			auto history = gizmo->get_plugin()->get_material(mat_name_history, gizmo);
			history->set_albedo(debug_color_history);
			auto future = gizmo->get_plugin()->get_material(mat_name_future, gizmo);
			future->set_albedo(debug_color_future);
			for (size_t i = 0; i < past_time_dt.size(); ++i) {
				const size_t offset = i * 2;
				Vector3 pos = Vector3(data[offset + 0], 0, data[offset + 1]);
				pos = tr.xform(pos);
				gizmo->add_lines(Array::make(pos, pos + Vector3(0, 1, 0)), history);
			}
			const size_t pos_offset = past_time_dt.size();
			const size_t traj_offset = past_time_dt.size() * 2 + future_time_dt.size() * 2;
			for (size_t i = 0; i < future_time_dt.size(); ++i) {
				const size_t offset = (pos_offset + i) * 2;
				Vector3 pos = Vector3(data[offset + 0], 0, data[offset + 1]);
				Vector3 traj = tr.xform(Vector3(0, 0, 1)).rotated(Vector3(0, 1, 0), data[traj_offset + i]);
				pos = tr.xform(pos);
				// traj = tr.xform(traj);
				gizmo->add_lines(Array::make(pos, pos + traj), future);
			}
		}
	}
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX