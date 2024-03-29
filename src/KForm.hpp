// Acknowledgement : This file wouldn't be possible without the blog from Daniel Holden, a.k.a TheOrangeDuck
// https://theorangeduck.com/page/propagating-velocities-through-animation-systems
// The code has been adapted to work with Godot's Vector3 and Quaternion.

#pragma once

#include <cmath>
#include <numeric>

#include <boost/container/vector.hpp>

#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/bone_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skeleton_profile.hpp>

#include <godot_cpp/variant/dictionary.hpp>

#include <Spring.hpp>

using namespace godot;

using u = godot::UtilityFunctions;

struct kform {
	Quaternion rot = Quaternion();
	Vector3 pos = Vector3();
	Vector3 scl = Vector3(1.0, 1.0, 1.0);
	Vector3 vel = Vector3();
	Vector3 ang = Vector3();
	Vector3 svl = Vector3();

	kform() = default;
	~kform() = default;

	kform(Transform3D tr) :
			pos{ tr.origin },
			rot{ tr.basis.get_rotation_quaternion() },
			scl{ tr.basis.get_scale() },
			vel{},
			ang{},
			svl{} {}

	kform(Vector3 p, Quaternion r, Vector3 s = Vector3{ 1, 1, 1 }, Vector3 lv = Vector3{}, Vector3 av = Vector3{}, Vector3 sv = Vector3{}) :
			pos{ p }, rot{ r }, scl{ s }, vel{ lv }, ang{ av }, svl{ sv } {}

	static Vector3 _log(Vector3 v) {
		return Vector3(std::log(v.x), std::log(v.y), std::log(v.z));
	}

	kform &finite_difference(const kform input_next, real_t _dt) {
		vel = (input_next.pos - pos) / _dt;

		ang = Spring::quat_to_scaled_angle_axis(Spring::quat_abs(
					  input_next.rot * rot.inverse())) /
				_dt;

		svl = _log(input_next.scl / scl) / _dt;
		return *this;
	}

	static kform finite_difference(const kform &input_curr, const kform &input_next, real_t _dt) {
		kform out = input_curr;
		out.vel = (input_next.pos - out.pos) / _dt;

		out.ang = Spring::quat_to_scaled_angle_axis(Spring::quat_abs(
						  input_next.rot * out.rot.inverse())) /
				_dt;

		out.svl = _log(input_next.scl / out.scl) / _dt;
		return out;
	}

	inline operator Transform3D() const {
		return Transform3D(Basis(rot, scl), pos);
	}
	inline explicit operator Dictionary() const {
		Dictionary result{};
		result["position"] = pos;
		result["velocity_linear"] = vel;
		result["rotation"] = rot;
		result["velocity_angular"] = ang;
		result["scale"] = scl;
		result["velocity_scalar"] = svl;
		return result;
	}

	friend kform operator*(const kform parent, const kform w) {
		kform out;
		out.pos = parent.rot.xform(w.pos * parent.scl) + parent.pos;
		out.rot = parent.rot * w.rot;
		out.scl = w.scl * parent.scl;
		out.vel = parent.rot.xform(w.vel * parent.scl) + parent.vel +
				parent.ang.cross(parent.rot.xform(w.pos * parent.scl)) +
				parent.rot.xform(w.pos * parent.scl * parent.svl);
		out.ang = parent.rot.xform(w.ang) + parent.ang;
		out.svl = w.svl + parent.svl;
		return out;
	}
	friend kform operator/(const kform v, const kform w) {
		kform out;
		out.pos = v.rot.xform_inv(w.pos - v.pos);
		out.rot = v.rot.inverse() * w.rot;
		out.scl = w.scl / v.scl;
		out.vel = v.rot.xform_inv(w.vel - v.vel - v.ang.cross(v.rot.xform(out.pos * v.scl))) -
				v.rot.xform(out.pos * v.scl * v.svl);
		out.ang = v.rot.xform_inv(w.ang - v.ang);
		out.svl = w.svl - v.svl;
		return out;
	}
	kform inverse() const {
		kform out;
		out.pos = rot.xform_inv(-pos);
		out.rot = rot.inverse();
		out.scl = Vector3(1.0f, 1.0f, 1.0f) / scl;
		out.vel = rot.xform_inv(-vel - ang.cross(rot.xform(out.pos * scl))) - rot.xform(out.pos * scl * svl);
		out.ang = rot.xform_inv(-ang);
		out.svl = -svl;
		return out;
	}
};

struct kforms {
	template <typename T>
	using b_vector = std::vector<T>;
	b_vector<Vector3> pos; // Position
	b_vector<Quaternion> rot; // Rotation
	b_vector<Vector3> scl; // Scale
	b_vector<Vector3> vel; // Linear Velocity
	b_vector<Vector3> ang; // Angular Velocity
	b_vector<Vector3> svl; // Scalar Velocity

	kforms(std::size_t N) :
			pos(N, Vector3()), rot(N, Quaternion()), scl(N, Vector3(1, 1, 1)), vel(N, Vector3()), ang(N, Vector3()), svl(N, Vector3()) {}

	Transform3D get_transform(std::size_t N) {
		return Transform3D(Basis(rot[N], scl[N]), pos[N]);
	}

	void reserve(std::size_t N) {
		pos.reserve(N);
		rot.reserve(N);
		scl.reserve(N);
		vel.reserve(N);
		ang.reserve(N);
		svl.reserve(N);
	}

	std::size_t count() const noexcept {
		return pos.size();
	}

	inline const kform operator[](const std::size_t N) const noexcept {
		kform out{};
		out.pos = pos[N];
		out.rot = rot[N];
		out.scl = scl[N];
		out.vel = vel[N];
		out.ang = ang[N];
		out.svl = svl[N];
		return out;
	}

	void reset(const std::size_t N) {
		pos[N] = Vector3();
		rot[N] = Quaternion();
		scl[N] = Vector3(1, 1, 1);
		vel[N] = Vector3();
		ang[N] = Vector3();
		svl[N] = Vector3();
	}
};

static kform get_local_kform(Ref<SkeletonProfile> skel, Ref<Animation> anim, double time, NodePath bonepath) {
	static constexpr double dt = 0.032;
	kform out = skel->get_reference_pose(skel->find_bone(bonepath.get_concatenated_subnames()));
	auto tpos = anim->find_track(bonepath, Animation::TrackType::TYPE_POSITION_3D);
	auto trot = anim->find_track(bonepath, Animation::TrackType::TYPE_ROTATION_3D);
	auto tscl = anim->find_track(bonepath, Animation::TrackType::TYPE_SCALE_3D);
	kform s1 = out;
	if (tpos != -1) {
		out.pos = anim->position_track_interpolate(tpos, time);
		s1.pos = anim->position_track_interpolate(tpos, time + dt);
	}
	if (trot != -1) {
		out.rot = anim->rotation_track_interpolate(trot, time);
		s1.rot = anim->rotation_track_interpolate(trot, time + dt);
	}
	if (tscl != -1) {
		out.scl = anim->scale_track_interpolate(tscl, time);
		s1.scl = anim->scale_track_interpolate(tscl, time + dt);
	}
	out.finite_difference(s1, dt);
	return out;
}

static kform get_root_model_kform(Ref<SkeletonProfile> skel, Ref<Animation> anim, double time, NodePath bonepath) {
	if (bonepath.is_empty())
		return kform{};
	const StringName _skel_path = bonepath.get_concatenated_names();
	StringName bone = bonepath.get_concatenated_subnames();
	std::vector<kform> trs{};
	do {
		kform _local = get_local_kform(skel, anim, time, NodePath{ u::str(_skel_path) + u::str(":") + bone });
		kform &back = trs.emplace_back(std::move(_local));
		if (bone == skel->get_root_bone()) {
			back.vel = back.rot.xform_inv(back.vel);
			back.pos = Vector3{};
			back.rot = Quaternion();
			break;
		}
		bone = skel->get_bone_parent(skel->find_bone(bone)); // Now bone is its parent
	} while (!bone.is_empty());

	return std::reduce(trs.rbegin(), trs.rend(), kform{},
			[](const kform &acc, const kform &i) {
				return acc * i;
			});
}

static kform get_model_kform(Ref<SkeletonProfile> skel, Ref<Animation> anim, double time, NodePath bonepath) {
	if (bonepath.is_empty())
		return kform{};
	const StringName _skel_path = bonepath.get_concatenated_names();
	StringName bone = bonepath.get_concatenated_subnames();
	std::vector<kform> trs{};
	do {
		kform _local = get_local_kform(skel, anim, time, NodePath{ u::str(_skel_path) + u::str(":") + bone });
		kform &back = trs.emplace_back(std::move(_local));
		if (bone == skel->get_root_bone()) {
			back = {};
			break;
		}
		bone = skel->get_bone_parent(skel->find_bone(bone)); // Now bone is its parent
	} while (!bone.is_empty());

	return std::reduce(trs.rbegin(), trs.rend(), kform{},
			[](const kform &acc, const kform &i) {
				return acc * i;
			});
}

static kform get_global_kform(Ref<SkeletonProfile> skel, Ref<Animation> anim, double time, NodePath bonepath) {
	if (bonepath.is_empty())
		return kform{};
	const StringName _skel_path = bonepath.get_concatenated_names();
	StringName bone = bonepath.get_concatenated_subnames();
	std::vector<kform> trs{};
	do {
		kform _local = get_local_kform(skel, anim, time, NodePath{ u::str(_skel_path) + u::str(":") + bone });
		trs.emplace_back(std::move(_local));
		if (bone == skel->get_root_bone()) {
			break;
		}
		bone = skel->get_bone_parent(skel->find_bone(bone)); // Now bone is its parent
	} while (!bone.is_empty());

	return std::reduce(trs.rbegin(), trs.rend(), kform{},
			[](const kform &acc, const kform &i) {
				return acc * i;
			});
}