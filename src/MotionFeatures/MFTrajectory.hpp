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

#include <KForm.hpp>
#include <MMAnimationLibrary.hpp>
#include <MotionFeatures/MotionFeatures.hpp>

#include <cmath>
#include <format>


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
	GETSET(bool, use_y_coordinate, false);

	GETSET(float, weight_history_pos, 1.0f);
	GETSET(float, weight_prediction_pos, 1.0f);
	GETSET(float, weight_prediction_angle, 1.0f);
	virtual PackedFloat32Array get_weights() override {
		const unsigned int size = use_y_coordinate ? 3 : 2;
		PackedFloat32Array result{};
		for (auto i = 0; i < size * past_time_dt.size(); ++i) {
			result.append(weight_history_pos);
		}
		for (auto i = 0; i < size * future_time_dt.size(); ++i) {
			result.append(weight_prediction_pos);
		}
		for (auto i = 0; i < size * future_time_dt.size(); ++i) {
			result.append(weight_prediction_angle);
		}
		return result;
	}

public:
	virtual int get_dimension() override {
		const unsigned int size = use_y_coordinate ? 3 : 2;
		// Offset for each
		const size_t past_pos = size * past_time_dt.size();
		const size_t future_pos = size * future_time_dt.size();
		const size_t future_rot_angle = size * future_time_dt.size();
		return past_pos + future_pos + future_rot_angle;
	}

	int root_tracks[3] = { 0, 0, 0 };
	kform first_kform{}, last_kform{};
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
		if (animation->get_loop_mode() == Animation::LOOP_PINGPONG) {
			WARN_PRINT(std::format("animation is loop type Ping Pong, which isn't supported for now. ").c_str());
		}
		const float delta_diff = 0.05;
		start_time = 0.1f;
		end_time = animation->get_length();
		root_tracks[0] = animation->find_track(root_bone_track, Animation::TrackType::TYPE_POSITION_3D);
		root_tracks[1] = animation->find_track(root_bone_track, Animation::TrackType::TYPE_ROTATION_3D);
		root_tracks[2] = animation->find_track(root_bone_track, Animation::TrackType::TYPE_SCALE_3D);
		{
			first_kform = kform{
				animation->position_track_interpolate(root_tracks[0], start_time),
				animation->rotation_track_interpolate(root_tracks[1], start_time)
			};
			const kform starting_diff = kform{
				animation->position_track_interpolate(root_tracks[0], start_time + delta_diff),
				animation->rotation_track_interpolate(root_tracks[1], start_time + delta_diff)
			};
			first_kform.finite_difference(starting_diff, delta_diff);

			kform ending_diff = kform{
				animation->position_track_interpolate(root_tracks[0], end_time - delta_diff),
				animation->rotation_track_interpolate(root_tracks[1], end_time - delta_diff)
			};
			last_kform = kform{
				animation->position_track_interpolate(root_tracks[0], end_time),
				animation->rotation_track_interpolate(root_tracks[1], end_time)
			};
			last_kform = kform::finite_difference(ending_diff, last_kform, delta_diff);
		}
		u::prints("Start Vel", first_kform.vel, "End Vel", last_kform.vel);
		return true;
	}

	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) override {
		PackedFloat32Array result{};
		kform current_kform{
			animation->position_track_interpolate(root_tracks[0], time),
			animation->rotation_track_interpolate(root_tracks[1], time)
		};

		std::vector<kform> past_kform{}, future_kform{};
		past_kform.reserve(past_time_dt.size());
		future_kform.reserve(future_time_dt.size());

		// Past Trajectory Position
		for (size_t index = 0; index < past_time_dt.size(); ++index) {
			const float t = time - abs(past_time_dt[index]);
			const float safe_t = std::fmodf(t, animation->get_length());
			Vector3 pos{};
			Quaternion rot{};
			if (t >= 0.0f) { // The offset can be accessed through the anim data
				kform pre_t{};
				pre_t.pos = animation->position_track_interpolate(root_tracks[0], t);
				pre_t.rot = animation->rotation_track_interpolate(root_tracks[1], t);
				pre_t = current_kform.inverse() * pre_t;
				past_kform.emplace_back(std::move(pre_t));
			} else { // The offset must be calculated using the starting velocity and extrapoling
				if (animation->get_loop_mode() == Animation::LOOP_LINEAR) {
					kform looped_kform = first_kform;
					const kform entire_k = first_kform.inverse() * last_kform;
					for (uint64_t i = 0; i < uint64_t(std::abs(t) / end_time) + 1; ++i) {
						looped_kform = looped_kform * entire_k.inverse();
					}
					kform safe_t_kform = kform{
						animation->position_track_interpolate(root_tracks[0], safe_t),
						animation->rotation_track_interpolate(root_tracks[1], safe_t)
					};
					safe_t_kform = current_kform.inverse() * looped_kform * first_kform.inverse() * safe_t_kform;
					past_kform.emplace_back(std::move(safe_t_kform));
				} else {
					kform pre_t = first_kform;
					pre_t.pos = pre_t.pos + pre_t.vel * t; // t is negative
					pre_t.rot = Spring::quat_integrate_angular_velocity(pre_t.ang, pre_t.rot, t);
					pre_t = current_kform.inverse() * pre_t;
					past_kform.emplace_back(std::move(pre_t));
				}
			}
		}
		// Future Trajectory
		for (size_t index = 0; index < future_time_dt.size(); ++index) {
			const float t = time + abs(future_time_dt[index]);
			const float safe_t = std::fmodf(t, animation->get_length());
			Vector3 pos{};
			Quaternion rot{};
			if (t <= end_time) { // The offset can be accessed through the anim data
				kform post_t = current_kform.inverse() * kform{ animation->position_track_interpolate(root_tracks[0], t), animation->rotation_track_interpolate(root_tracks[1], t) };
				future_kform.emplace_back(std::move(post_t));
			} else { // The offset must be calculated using the end velocity and extrapoling
				if (animation->get_loop_mode() == Animation::LOOP_LINEAR) {
					kform looped_kform = first_kform;
					const kform entire_k = first_kform.inverse() * last_kform;
					for (uint64_t i = 0; i < uint64_t(t / end_time); ++i) {
						looped_kform = looped_kform * entire_k;
					}
					kform safe_t_kform = kform{
						animation->position_track_interpolate(root_tracks[0], safe_t),
						animation->rotation_track_interpolate(root_tracks[1], safe_t)
					};
					safe_t_kform = current_kform.inverse() * looped_kform * first_kform.inverse() * safe_t_kform;
					future_kform.emplace_back(std::move(safe_t_kform));
				} else {
					kform post_t = last_kform;
					post_t.pos = post_t.pos + post_t.vel * (t - end_time);
					post_t.rot = Spring::quat_integrate_angular_velocity(post_t.ang, post_t.rot, (t - end_time));
					post_t = current_kform.inverse() * post_t;
					future_kform.emplace_back(std::move(post_t));
				}
			}
		}
		for (auto &past : past_kform) {
			result.push_back(past.pos.x);
			if (use_y_coordinate)
				result.push_back(past.pos.y);
			result.push_back(past.pos.z);
		}
		for (auto &future : future_kform) {
			result.push_back(future.pos.x);
			if (use_y_coordinate)
				result.push_back(future.pos.y);
			result.push_back(future.pos.z);
		}
		for (auto &future : future_kform) {
			Vector3 direction = future.rot.xform(Vector3(0, 0, 1));
			result.push_back(direction.x);
			if (use_y_coordinate)
				result.push_back(direction.y);
			result.push_back(direction.z);
		}

		return result;
	}

	virtual float calculate_cost(PackedFloat32Array query, PackedFloat32Array data) const override {
		float cost = 0.0f;
		const size_t dim_size = use_y_coordinate ? 3 : 2;
		const size_t past_pos_offset = 0,
					 fut_pos_offset = past_time_dt.size() * dim_size,
					 fut_dir_offset = (past_time_dt.size() + future_time_dt.size()) * dim_size;

		std::vector<Vector3> query_past(past_time_dt.size()), data_past(past_time_dt.size());
		std::vector<Vector3> query_pos_future(future_time_dt.size()), data_pos_future(past_time_dt.size());
		std::vector<Vector3> query_dir_future(future_time_dt.size()), data_dir_future(past_time_dt.size());
		// Past Cost
		const size_t x_offset = 0, y_offset = 1, z_offset = use_y_coordinate ? 2 : 1;
		for (size_t i = 0; i < past_time_dt.size(); ++i) {
			query_past[i].x = query[past_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				query_past[i].y = query[past_pos_offset + i * dim_size + y_offset];
			query_past[i].z = query[past_pos_offset + i * dim_size + z_offset];

			data_past[i].x = data[past_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				data_past[i].y = data[past_pos_offset + i * dim_size + y_offset];
			data_past[i].z = data[past_pos_offset + i * dim_size + z_offset];

			cost += query_past[i].distance_to(data_past[i]);
		}
		// Future Post Cost
		for (size_t i = 0; i < future_time_dt.size(); ++i) {
			query_pos_future[i].x = query[fut_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				query_pos_future[i].y = query[fut_pos_offset + i * dim_size + y_offset];
			query_pos_future[i].z = query[fut_pos_offset + i * dim_size + z_offset];

			data_pos_future[i].x = data[fut_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				data_pos_future[i].y = data[fut_pos_offset + i * dim_size + y_offset];
			data_pos_future[i].z = data[fut_pos_offset + i * dim_size + z_offset];

			cost += query_pos_future[i].distance_to(data_pos_future[i]);
		}

		// Future Post Cost
		for (size_t i = 0; i < future_time_dt.size(); ++i) {
			query_dir_future[i].x = query[fut_dir_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				query_dir_future[i].y = query[fut_dir_offset + i * dim_size + y_offset];
			query_dir_future[i].z = query[fut_dir_offset + i * dim_size + z_offset];

			data_dir_future[i].x = data[fut_dir_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				data_dir_future[i].y = data[fut_dir_offset + i * dim_size + y_offset];
			data_dir_future[i].z = data[fut_dir_offset + i * dim_size + z_offset];

			float dot = query_dir_future[i].dot(data_dir_future[i]);
			cost += std::fabs(2.0f - (1.0f + dot)) * 0.5f;
		}

		return cost;
	}

	GETSET(PackedVector3Array, history_pos)
	GETSET(PackedVector3Array, future_pos)
	GETSET(PackedFloat32Array, future_dir)

	PackedFloat32Array serialize_trajectory_local(PackedVector3Array p_history_pos, PackedVector3Array p_future_pos, PackedVector3Array p_future_dir) {
		PackedFloat32Array result{};
		for (auto elem : p_history_pos) {
			result.append(elem.x);
			if (use_y_coordinate)
				result.append(elem.y);
			result.append(elem.z);
		}
		for (auto elem : p_future_pos) {
			result.append(elem.x);
			if (use_y_coordinate)
				result.append(elem.y);
			result.append(elem.z);
		}
		for (auto elem : p_future_dir) {
			result.append(elem.x);
			if (use_y_coordinate)
				result.append(elem.y);
			result.append(elem.z);
		}
		return result;
	}

	virtual PackedStringArray get_hints() const override {
		PackedStringArray result{};

		for (auto elem : past_time_dt) {
			result.append("Px-" + u::str(std::format("{:.2f}", elem).c_str()));
			if (use_y_coordinate)
				result.append("Py-" + u::str(std::format("{:.2f}", elem).c_str()));
			result.append("Pz-" + u::str(std::format("{:.2f}", elem).c_str()));
		}
		for (auto elem : future_time_dt) {
			result.append("Px+" + u::str(std::format("{:.2f}", elem).c_str()));
			if (use_y_coordinate)
				result.append("Py+" + u::str(std::format("{:.2f}", elem).c_str()));
			result.append("Pz+" + u::str(std::format("{:.2f}", elem).c_str()));
		}
		for (auto elem : future_time_dt) {
			result.append("Dx+" + u::str(std::format("{:.2f}", elem).c_str()));
			if (use_y_coordinate)
				result.append("Dy+" + u::str(std::format("{:.2f}", elem).c_str()));
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

		ClassDB::bind_method(D_METHOD("set_use_y_coordinate", "value"), &MFTrajectory::set_use_y_coordinate, false);
		ClassDB::bind_method(D_METHOD("get_use_y_coordinate"), &MFTrajectory::get_use_y_coordinate);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL, "use_y_coordinate"), "set_use_y_coordinate", "get_use_y_coordinate");

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