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

struct MFTrajectoryOptions : public Resource {
	GDCLASS(MFTrajectoryOptions, Resource)
public:
	GETSET(float, time_offset, 0.0f)
	GETSET(float, weights, 1.0f);
	GETSET(int, coordinate, Coordinates::XYZ)
	GETSET(int, options, 3);
	GETSET(Color, debug_color, Color{ "RED" });

	int get_dimensions() {
		if (coordinate == Coordinates::XZ)
			return 2 * std::bitset<32>(options).count();
		else if (coordinate == Coordinates::XYZ)
			return 3 * std::bitset<32>(options).count();
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

public:
	int get_dimension() const {
		int result = 0;
		for (int i = 0; i < options.size(); ++i) {
			auto *option = cast_to<MFTrajectoryOptions>(options[i]);
			if (option) {
				result += option->get_dimensions();
			}
		}
		return result;
	}

	PackedFloat32Array get_weights() const {
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
		return MMUtil::standardize(result);
	}

	PackedStringArray get_hints() const {
		PackedStringArray result{};
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

	SkeletonProfile *profile = nullptr;
	MMAnimationLibrary *m_library = nullptr;

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

	PackedFloat32Array bake_pose(Ref<MMAnimationLibrary> mmlib, String animation_name, float time) {
		ERR_FAIL_COND_V_EDMSG(mmlib->skeleton_path.is_empty(), {}, "SkeletonPath is Empty");
		ERR_FAIL_COND_V_EDMSG(mmlib->skeleton_profile == nullptr, {}, "SkeletonProfile is null");
		ERR_FAIL_COND_V_EDMSG(mmlib->get_skeleton_profile()->get_root_bone().is_empty(), {}, "No Root bone to extract data");
		root_bone_track = String(mmlib->skeleton_path) + ':' + mmlib->skeleton_profile->get_root_bone();
		PackedFloat32Array result{};
		Ref<Animation> animation = mmlib->get_animation(animation_name);

		const kform current = get_global_kform(mmlib->skeleton_profile, animation, time, root_bone_track);
		for (int i = 0; i < options.size(); ++i) {
			auto *option = cast_to<MFTrajectoryOptions>(options[i]);
			if (option) {
				kform global = _get_global_root_kform(mmlib, animation, time + option->time_offset);
				kform difference = current.remove_velocities().inverse() * global;
				std::bitset<32> bit = option->options;
				for (int o = 0; o < bit.size(); ++o) {
					Vector3 I{};
					if (o == MFTrajectoryOptions::Options::Position && bit.test(MFTrajectoryOptions::Options::Position)) {
						I = difference.pos;
					} else if (o == MFTrajectoryOptions::Options::Velocity && bit.test(MFTrajectoryOptions::Options::Velocity)) {
						I = difference.vel;
					} else if (o == MFTrajectoryOptions::Options::Direction && bit.test(MFTrajectoryOptions::Options::Direction)) {
						I = difference.rot.xform(Vector3(0, 0, 1));
					} else {
						continue;
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

	// TODO Fix
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

	// TODO Fix for options
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
				if (gizmo->get_plugin()->get_material(material_name, gizmo) == nullptr) {
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
			// ClassDB::bind_method(D_METHOD("serialize", "unnormalized_values_local_to_character"), &MFTrajectory::serialize);
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
		}
		ClassDB::add_property_group(get_class_static(), "", "");

		ClassDB::bind_method(D_METHOD("get_weights"), &MFTrajectory::get_weights);
		ClassDB::bind_method(D_METHOD("get_dimension"), &MFTrajectory::get_dimension);

		ClassDB::bind_method(D_METHOD("bake_pose", "animation_library", "animation_name", "time"), &MFTrajectory::bake_pose);
		ClassDB::bind_method(D_METHOD("show_debug_info", "gizmo", "lib", "animation_name", "timestamp"
																						   "skeleton"),
				&MFTrajectory::show_debug_info);
	}

	GETSET(Color, debug_color_history, godot::Color(1.0f, 1.0f, 1.0f));
	GETSET(Color, debug_color_future, godot::Color(0.0f, 0.0f, 0.0f));
};