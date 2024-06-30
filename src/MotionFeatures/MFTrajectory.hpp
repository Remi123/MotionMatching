#pragma once

#include <godot_cpp/core/math.hpp>
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

#include <godot_cpp/classes/prism_mesh.hpp>

#include <MMAnimationLibrary.hpp>
#include <Math/KForm.hpp>
#include <MotionFeatures/MotionFeatures.hpp>
#include <Util/Util.hpp>

#include <cmath>
#include <format>

using namespace godot;
using u = godot::UtilityFunctions;

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }

struct MFTrajectoryOptions : public Resource {
	GDCLASS(MFTrajectoryOptions, Resource)
public:
	GETSET(float, time_offset, 0.0f)
	GETSET(float, weights, 1.0f);
	GETSET(int, coordinate, 1)
	GETSET(int, options, 3);
	GETSET(Color, debug_color, Color{ "RED" });

	int get_dimensions() {
		if (coordinate == Coordinates::XZ)
			return 2;
		else if (coordinate == Coordinates::XYZ)
			return 3;
		else
			return 0;
	}

	enum Coordinates {
		XZ,
		XYZ
	};
	enum Options {
		Position,
		Velocity,
		Direction
	};

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_time_offset", "value"), &MFTrajectoryOptions::set_time_offset, DEFVAL(0.0));
		ClassDB::bind_method(D_METHOD("get_time_offset"), &MFTrajectoryOptions::get_time_offset);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "time_offset"), "set_time_offset", "get_time_offset");
		ClassDB::bind_method(D_METHOD("set_weights", "value"), &MFTrajectoryOptions::set_weights, DEFVAL(1.0));
		ClassDB::bind_method(D_METHOD("get_weights"), &MFTrajectoryOptions::get_weights);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weights"), "set_weights", "get_weights");
		ClassDB::bind_method(D_METHOD("set_coordinate", "value"), &MFTrajectoryOptions::set_coordinate);
		ClassDB::bind_method(D_METHOD("get_coordinate"), &MFTrajectoryOptions::get_coordinate);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "coordinate", PROPERTY_HINT_ENUM, "XY,XYZ", PROPERTY_USAGE_DEFAULT), "set_coordinate", "get_coordinate");
		ClassDB::bind_method(D_METHOD("set_options", "value"), &MFTrajectoryOptions::set_options, DEFVAL(3));
		ClassDB::bind_method(D_METHOD("get_options"), &MFTrajectoryOptions::get_options);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "options", PROPERTY_HINT_FLAGS, "Position,Velocity,Direction,", PROPERTY_USAGE_DEFAULT), "set_options", "get_options");
		ClassDB::bind_method(D_METHOD("set_debug_color", "value"), &MFTrajectoryOptions::set_debug_color, Color{ "RED" });
		ClassDB::bind_method(D_METHOD("get_debug_color"), &MFTrajectoryOptions::get_debug_color);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color"), "set_debug_color", "get_debug_color");
	}
};

struct MFTrajectory : public MotionFeature {
	GDCLASS(MFTrajectory, MotionFeature)
public:
	virtual ~MFTrajectory() = default;

	Skeleton3D *skeleton{ nullptr };
	Skeleton3D *get_skeleton() { return skeleton; }
	void set_skeleton(Skeleton3D *value) { skeleton = value; }
	String root_bone_track = "%GeneralSkeleton:Root";

	GETSET(TypedArray<MFTrajectoryOptions>, options);

	GETSET(PackedFloat32Array, past_time_dt);
	GETSET(PackedFloat32Array, future_time_dt);
	GETSET(bool, use_y_coordinate, false);

	GETSET(float, weight_history_pos, 1.0f);
	GETSET(float, weight_prediction_pos, 1.0f);
	GETSET(float, weight_prediction_angle, 1.0f);

	bool use_refactor = false;

public:
	int get_dimension() const {
		if (use_refactor) {
			int result = 0;
			for (int i = 0; i < options.size(); ++i) {
				auto *option = cast_to<MFTrajectoryOptions>(options[i]);
				if (option) {
					result += option->get_dimensions();
				}
			}
			return result;
		}

		const unsigned int size = use_y_coordinate ? 3 : 2;
		// Offset for each
		const size_t past_pos = size * past_time_dt.size();
		const size_t future_pos = size * future_time_dt.size();
		const size_t future_rot_angle = size * future_time_dt.size();
		return past_pos + future_pos + future_rot_angle;
	}

	PackedFloat32Array get_weights() const {
		if (use_refactor) {
			PackedFloat32Array result{};
			for (int i = 0; i < options.size(); ++i) {
				auto *option = cast_to<MFTrajectoryOptions>(options[i]);
				if (option) {
					float weight = option->weights;
					for (int w = 0; w < option->get_dimensions(); ++w) {
						result.append(weight);
					}
				}
			}
			// Standardize
			standardize(result);

			return result;
		}

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
	PackedStringArray get_hints() const {
		PackedStringArray result{};

		if (use_refactor) {
			for (int i = 0; i < options.size(); ++i) {
				auto *option = cast_to<MFTrajectoryOptions>(options[i]);
				if (option) {
					std::bitset<32> bit = option->options;
					for (int o = 0; o < bit.size(); ++o) {
						String I = "";
						if (o == 0 && bit.test(MFTrajectoryOptions::Options::Position)) {
							I = "P";
						} else if (o == 1 && bit.test(MFTrajectoryOptions::Options::Velocity)) {
							I = "V";
						} else if (o == 2 && bit.test(MFTrajectoryOptions::Options::Direction)) {
							I = "D";
						}
						if (I.is_empty()) {
							continue;
						}
						if (option->coordinate == MFTrajectoryOptions::Coordinates::XZ) {
							result.append(I + "x" + u::str(std::format("{:.2f}", option->time_offset).c_str()));
							result.append(I + "z" + u::str(std::format("{:.2f}", option->time_offset).c_str()));
						} else if (option->coordinate == MFTrajectoryOptions::Coordinates::XYZ) {
							result.append(I + "x" + u::str(std::format("{:.2f}", option->time_offset).c_str()));
							result.append(I + "y" + u::str(std::format("{:.2f}", option->time_offset).c_str()));
							result.append(I + "z" + u::str(std::format("{:.2f}", option->time_offset).c_str()));
						}
					}
				}
			}
			return result;
		}

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

	int root_tracks[3] = { 0, 0, 0 };
	kform first_kform{}, last_kform{};
	Vector3 start_pos, start_vel, end_pos, end_vel;
	Quaternion start_rot, end_rot, end_ang_vel;
	float start_time = 0.0f, end_time = 0.0f;

	SkeletonProfile *profile = nullptr;
	MMAnimationLibrary *m_library = nullptr;

	bool setup_bake_init(Ref<MMAnimationLibrary> animlib) {
		ERR_FAIL_COND_V_EDMSG(animlib->get_skeleton_path().is_empty(), false, "SkeletonPath is Empty");
		ERR_FAIL_COND_V_EDMSG(animlib->get_skeleton_profile() == nullptr, false, "SkeletonProfile is null");
		ERR_FAIL_COND_V_EDMSG(animlib->get_skeleton_profile()->get_root_bone().is_empty(), false, "No Root bone to extract data");
		root_bone_track = u::str(animlib->get_skeleton_path()) + ":" + animlib->get_skeleton_profile()->get_root_bone();
		profile = animlib->skeleton_profile.ptr();
		m_library = animlib.ptr();
		return true;
	};

	bool setup_bake_animation(Ref<Animation> animation) {
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

	kform _get_global_root_kform(Ref<MMAnimationLibrary> p_lib, Ref<Animation> animation, float time) {
		auto root_path = u::str(p_lib->get_skeleton_path()) + ":" + p_lib->get_skeleton_profile()->get_root_bone();
		if (0.0 <= time && time <= animation->get_length()) {
			// In range
			return get_global_kform(p_lib->skeleton_profile, animation, time, root_bone_track);
		} else if (animation->get_loop_mode() == Animation::LOOP_NONE) {
			// Take last ( or first) velocities and extrapolate as if it continue.
			auto starting_kform = get_global_kform(p_lib->skeleton_profile, animation, 0.0, root_path);
			auto ending_kform = get_global_kform(p_lib->skeleton_profile, animation, animation->get_length() - 0.032f, root_path);
			kform to_extrapolate = std::signbit(time) ? starting_kform : ending_kform;
			float delta = std::signbit(time) ? time : time - animation->get_length();
			to_extrapolate.pos += to_extrapolate.vel * delta;
			to_extrapolate.rot = Spring::quat_integrate_angular_velocity(to_extrapolate.ang, to_extrapolate.rot, delta);
			return to_extrapolate;
		} else if (animation->get_loop_mode() == Animation::LOOP_LINEAR) {
			// Loop X time, then add the kform at the modulo.
			auto starting_kform = get_global_kform(p_lib->skeleton_profile, animation, 0.0, root_path);
			auto ending_kform = get_global_kform(p_lib->skeleton_profile, animation, animation->get_length() - 0.032f, root_path);
			float delta = std::signbit(time) ? time : time - animation->get_length();
			Transform3D entire_k = std::signbit(time) ? (starting_kform.inverse() * ending_kform).inverse() : (starting_kform.inverse() * ending_kform);
			Transform3D looped = starting_kform;
			for (int loop = 0; loop < abs(floor((time) / animation->get_length())); ++loop) {
				looped = looped * entire_k;
			}
			kform reference = starting_kform.remove_velocities();
			float safe_time = Math::fposmod(time, (float)animation->get_length());
			kform safe_k = reference.inverse() * get_global_kform(p_lib->skeleton_profile, animation, safe_time, root_bone_track);

			return (kform)looped * safe_k;
		} else {
			return get_global_kform(p_lib->skeleton_profile, animation, time, root_bone_track);
		}
	}

	PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) {
		PackedFloat32Array result{};

		if (use_refactor) {
			kform current = get_global_kform(profile, animation, time, root_bone_track);
			for (int i = 0; i < options.size(); ++i) {
				auto *option = cast_to<MFTrajectoryOptions>(options[i]);
				if (option) {
					kform offset = _get_global_root_kform(m_library, animation, time + option->time_offset);
					kform difference = current.remove_velocities().inverse() * offset;
					std::bitset<32> bit = option->options;
					for (int o = 0; o < bit.size(); ++o) {
						Vector3 I{};
						if (o == MFTrajectoryOptions::Options::Position && bit.test(MFTrajectoryOptions::Options::Position)) {
							I = difference.pos;
						} else if (o == MFTrajectoryOptions::Options::Velocity && bit.test(MFTrajectoryOptions::Options::Velocity)) {
							I = difference.vel;
						} else if (o == MFTrajectoryOptions::Options::Direction && bit.test(MFTrajectoryOptions::Options::Direction)) {
							I = difference.rot.xform(Vector3(0, 0, 1));
						}
						if (option->coordinate == MFTrajectoryOptions::Coordinates::XZ) {
							result.append(I.x);
							result.append(I.z);
						} else if (option->coordinate == MFTrajectoryOptions::Coordinates::XYZ) {
							result.append(I.x);
							result.append(I.y);
							result.append(I.z);
						}
					}
				}
			}

			return result;
		}

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

		// Past Cost
		const size_t x_offset = 0, y_offset = 1, z_offset = use_y_coordinate ? 2 : 1;
		const size_t past_size = past_time_dt.size();
		for (size_t i = 0; i < past_size; ++i) {
			Vector3 query_past{}, data_past{};
			query_past.x = query[past_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				query_past.y = query[past_pos_offset + i * dim_size + y_offset];
			query_past.z = query[past_pos_offset + i * dim_size + z_offset];

			data_past.x = data[past_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				data_past.y = data[past_pos_offset + i * dim_size + y_offset];
			data_past.z = data[past_pos_offset + i * dim_size + z_offset];

			cost += query_past.distance_to(data_past) * weight_history_pos;
		}
		// Future Post Cost
		for (size_t i = 0; i < future_time_dt.size(); ++i) {
			Vector3 query_pos_future{}, data_pos_future{};
			query_pos_future.x = query[fut_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				query_pos_future.y = query[fut_pos_offset + i * dim_size + y_offset];
			query_pos_future.z = query[fut_pos_offset + i * dim_size + z_offset];

			data_pos_future.x = data[fut_pos_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				data_pos_future.y = data[fut_pos_offset + i * dim_size + y_offset];
			data_pos_future.z = data[fut_pos_offset + i * dim_size + z_offset];

			cost += query_pos_future.distance_to(data_pos_future) * weight_prediction_pos;
		}

		// Future Dir Cost
		for (size_t i = 0; i < future_time_dt.size(); ++i) {
			Vector3 query_dir_future{}, data_dir_future{};
			query_dir_future.x = query[fut_dir_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				query_dir_future.y = query[fut_dir_offset + i * dim_size + y_offset];
			query_dir_future.z = query[fut_dir_offset + i * dim_size + z_offset];

			data_dir_future.x = data[fut_dir_offset + i * dim_size + x_offset];
			if (use_y_coordinate)
				data_dir_future.y = data[fut_dir_offset + i * dim_size + y_offset];
			data_dir_future.z = data[fut_dir_offset + i * dim_size + z_offset];

			float dot = query_dir_future.dot(data_dir_future);
			cost += std::fabs(2.0f - (1.0f + dot)) * 0.5f * weight_prediction_angle;
		}

		return cost;
	}

	GETSET(PackedVector3Array, history_pos)
	GETSET(PackedVector3Array, future_pos)
	GETSET(PackedFloat32Array, future_dir)

	PackedFloat32Array serialize(PackedVector3Array array) {
		PackedFloat32Array result;

		int counter = 0;
		for (int i = 0; i < options.size(); ++i) {
			MFTrajectoryOptions *opt = cast_to<MFTrajectoryOptions>(options[i]);
			std::bitset<32> bit = opt->options;
			for (int o = 0; o < bit.size(); ++o) {
				Vector3 I = array[counter];
				if (opt->coordinate == MFTrajectoryOptions::Coordinates::XZ) {
					result.append(I.x);
					result.append(I.z);
				} else if (opt->coordinate == MFTrajectoryOptions::Coordinates::XYZ) {
					result.append(I.x);
					result.append(I.y);
					result.append(I.z);
				}
				++counter;
			}
		}
		return result;
	}

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

	virtual void show_debug_info(Ref<EditorNode3DGizmo> gizmo, Ref<MMAnimationLibrary> library, String animation_name, float time, Skeleton3D *skel) {
		auto root_bone_tr = skel->get_bone_global_pose(skel->find_bone(library->skeleton_profile->get_root_bone()));

		Ref<Animation> animation = library->get_animation(animation_name);
		const kform current = get_global_kform(library->skeleton_profile, animation, time, root_bone_track);
		for (int i = 0; i < options.size(); ++i) {
			auto *option = cast_to<MFTrajectoryOptions>(options[i]);

			if (option) {
				kform global = _get_global_root_kform(library, animation, time + option->time_offset);
				kform offset = current.remove_velocities().inverse() * global;

				const auto material_name = "traj" + get_path();
				if (gizmo->get_plugin()->get_material(material_name,gizmo) == nullptr) {
					gizmo->get_plugin()->create_material(material_name, option->debug_color);
				}
				auto mat = gizmo->get_plugin()->get_material(material_name, gizmo);

				Ref<PrismMesh> mesh{};
				mesh.instantiate();
				mesh->set_size(Vector3(1, 1.1, 1) * 0.1);
				std::bitset<32> bit = option->options;
				Transform3D global_point{};
				global_point.origin = global.pos;
				global_point.set_basis(global.rot);
				gizmo->add_mesh(mesh, mat, global_point.rotated_local(Vector3(1, 0, 0), godot::Math::deg_to_rad(90.0)));

				if (bit.test(MFTrajectoryOptions::Options::Velocity)) {
					gizmo->add_lines(Array::make(global.pos, root_bone_tr.xform(offset.vel)), mat);
				}
			}
		}
	}

protected:
	static void _bind_methods() {
		{
			ClassDB::bind_method(D_METHOD("serialize_trajectory_local", "history_local_pos", "prediction_local_pos", "prediction_local_direction"), &MFTrajectory::serialize_trajectory_local);
			ClassDB::bind_method(D_METHOD("serialize", "unnormalized_values_local_to_character"), &MFTrajectory::serialize);
		}

		ClassDB::bind_method(D_METHOD("get_hints"), &MFTrajectory::get_hints);

		ClassDB::bind_method(D_METHOD("set_options", "value"), &MFTrajectory::set_options);
		ClassDB::bind_method(D_METHOD("get_options"), &MFTrajectory::get_options);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "options", godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":MFTrajectoryOptions", PROPERTY_USAGE_DEFAULT), "set_options", "get_options");

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

		ClassDB::bind_method(D_METHOD("calculate_cost", "query", "data"), &MFTrajectory::calculate_cost);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &MFTrajectory::debug_pose_gizmo);

		ClassDB::bind_method(D_METHOD("show_debug_info", "gizmo", "lib", "animation_name", "timestamp"
																						   "skeleton"),
				&MFTrajectory::show_debug_info);
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
		if (use_refactor) {
			int counter = 0;
			for (int i = 0; i < options.size(); ++i) {
				MFTrajectoryOptions *opt = cast_to<MFTrajectoryOptions>(options[i]);
				gizmo->get_plugin()->create_material(opt->get_path(), debug_color_history, false, true);
				auto mat = gizmo->get_plugin()->get_material(opt->get_path(), gizmo);
				std::bitset<32> bit = opt->options;
				for (int o = 0; o < bit.size(); ++o) {
					if (!bit.test(o))
						continue;
					Vector3 P{}, V{}, D{};
					Vector3 I{};
					if (opt->coordinate == MFTrajectoryOptions::Coordinates::XZ) {
						I.x = data[counter++];
						I.z = data[counter++];
					} else if (opt->coordinate == MFTrajectoryOptions::Coordinates::XYZ) {
						I.x = data[counter++];
						I.y = data[counter++];
						I.z = data[counter++];
					}
					auto t = tr;
					Ref<BoxMesh> mesh = new BoxMesh{};
					mesh->set_size(Vector3{ 0.1, 0.1, 0.2 });
					if (o == MFTrajectoryOptions::Options::Position) {
						P = I;
						t.translate_local(P);
					}
					if (o == MFTrajectoryOptions::Options::Direction) {
						D = I;
						Quaternion q = Quaternion(Vector3{ 0, 0, 1 }, D);
						t.basis *= q;
					}
					// TODO This might get ugly
					if (o == MFTrajectoryOptions::Options::Velocity) {
						V = t.xform(I);
						gizmo->add_lines(Array::make(t.origin, V), mat);
					}

					gizmo->add_mesh(mesh, mat, t);
				}
			}

			return;
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