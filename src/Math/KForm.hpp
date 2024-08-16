// Acknowledgement : This file wouldn't be possible without the blog from Daniel Holden, a.k.a TheOrangeDuck
// https://theorangeduck.com/page/propagating-velocities-through-animation-systems
// The code has been adapted to work with Godot's Vector3 and Quaternion.

#pragma once

#include <cmath>
#include <numeric>

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

#include <Math/Spring.hpp>

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

private:
	static Vector3 _log(Vector3 v) {
		return Vector3(std::log(v.x), std::log(v.y), std::log(v.z));
	}

public:
	kform remove_velocities() const {
		kform result = *this;
		result.vel = {};
		result.ang = {};
		result.svl = {};
		return result;
	}

	kform finite_difference(const kform input_next, real_t _dt) {
		kform out = *this;
		out.vel = (input_next.pos - pos) / _dt;

		out.ang = Spring::quat_to_scaled_angle_axis(Spring::quat_abs(
						  input_next.rot * rot.inverse())) /
				_dt;

		out.svl = _log(input_next.scl / scl) / _dt;
		return out;
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

	friend static kform operator*(const kform parent, const kform w) {
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
	friend static kform operator/(const kform v, const kform w) {
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
		pos.resize(N);
		rot.resize(N);
		scl.resize(N);
		vel.resize(N);
		ang.resize(N);
		svl.resize(N);
	}

	std::size_t count() const noexcept {
		return pos.size();
	}

	template <bool is_const>
	struct kform_ref {
		using vec3 = std::conditional_t<is_const, const Vector3, Vector3>;
		using quat = std::conditional_t<is_const, const Quaternion, Quaternion>;
		quat &rot;
		vec3 &pos;
		vec3 &scl;
		vec3 &vel;
		vec3 &ang;
		vec3 &svl;
		kform_ref() = delete;
		kform_ref(kform_ref &other) = default;
		kform_ref(kform other) :
				pos{ other.pos }, rot{ other.rot }, scl{ other.scl }, vel{ other.vel }, ang{ other.ang }, svl{ other.svl } {
		}
		kform_ref(vec3 p, quat q, vec3 s, vec3 V, vec3 A, vec3 S) :
				pos{ p }, rot{ q }, scl{ s }, vel{ V }, ang{ A }, svl{ S } {
		}
		void operator=(const kform &rhs) {
			pos = rhs.pos;
			rot = rhs.rot;
			scl = rhs.scl;
			vel = rhs.vel;
			ang = rhs.ang;
			svl = rhs.svl;
		}
		explicit operator kform() const {
			return kform{ pos, rot, scl, vel, ang, svl };
		}
		operator Dictionary() const {
			return (Dictionary)kform{ pos, rot, scl, vel, ang, svl };
		}
		operator Transform3D() const {
			return Transform3D(Basis(rot, scl), pos);
		}
	};

	inline kform_ref<false> operator[](const std::size_t N) noexcept {
		return { pos[N], rot[N], scl[N], vel[N], ang[N], svl[N] };
	}
	inline const kform_ref<true> operator[](const std::size_t N) const noexcept {
		return { pos[N], rot[N], scl[N], vel[N], ang[N], svl[N] };
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
	out = out.finite_difference(s1, dt);
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

struct Kform : public RefCounted {
	GDCLASS(Kform, RefCounted);

private:
public:
	kform k{};
#define VAR(type, variable)                            \
	type get_##variable() const { return k.variable; } \
	void set_##variable(type value) {                  \
		k.variable = value;                            \
	}
	VAR(Vector3, pos);
	VAR(Quaternion, rot);
	VAR(Vector3, scl);
	VAR(Vector3, vel);
	VAR(Vector3, ang);
	VAR(Vector3, svl);
#undef VAR
	void inverse() {
		k.inverse();
	}
	Kform() = default;

	Kform &operator=(const kform &rhs) noexcept {
		k = rhs;
		return *this;
	}

	Ref<Kform> finite_difference(Ref<Kform> other, float delta) {
		kform _other(other->k);
		_other = k.finite_difference(_other, delta);
		Ref<Kform> result = new Kform();
		result.instantiate();
		result->k = _other;
		return result;
	}

	Ref<Kform> multiply(Ref<Kform> other) {
		kform _other(other->k);
		_other = k * _other;
		Ref<Kform> result{};
		result.instantiate();
		result->k = _other;
		return result;
	}
	Ref<Kform> divide(Ref<Kform> other) {
		kform _other(other->k);
		_other = k / _other;
		Ref<Kform> result{};
		result.instantiate();
		result->k = _other;
		return result;
	}

	TypedArray<Kform> character_prediction(
			Vector3 linear_acceleration,
			Vector3 desired_velocity,
			Quaternion desired_rotation,
			real_t halflife_velocity, real_t halflife_rotation,
			PackedFloat32Array deltas) {
		TypedArray<Kform> result{};
		for (auto dt : deltas) {
			kform _k = k;
			auto a = linear_acceleration;
			Spring::_character_update(_k.pos, _k.vel, a, _k.rot, _k.ang, desired_velocity, desired_rotation, halflife_velocity, halflife_rotation, dt);
			Ref<Kform> r{};
			r.instantiate();
			r->k = k;
			result.append(r);
		}
		return result;
	}

	Vector3 character_update(
			Vector3 linear_acceleration,
			Vector3 desired_velocity,
			Quaternion desired_rotation,
			real_t halflife_velocity, real_t halflife_rotation,
			real_t delta) {
		auto a = linear_acceleration;
		Spring::_character_update(k.pos, k.vel, a, k.rot, k.ang, desired_velocity, desired_rotation, halflife_velocity, halflife_rotation, delta);
		return a;
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_position", "value"), &Kform::set_pos);
		ClassDB::bind_method(D_METHOD("get_position"), &Kform::get_pos);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");

		ClassDB::bind_method(D_METHOD("set_rotation", "value"), &Kform::set_rot);
		ClassDB::bind_method(D_METHOD("get_rotation"), &Kform::get_rot);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::QUATERNION, "rotation"), "set_rotation", "get_rotation");

		ClassDB::bind_method(D_METHOD("set_scale", "value"), &Kform::set_scl);
		ClassDB::bind_method(D_METHOD("get_scale"), &Kform::get_scl);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::VECTOR3, "scale"), "set_scale", "get_scale");

		ClassDB::bind_method(D_METHOD("set_linear_velocity", "value"), &Kform::set_vel);
		ClassDB::bind_method(D_METHOD("get_linear_velocity"), &Kform::get_vel);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::VECTOR3, "linear_velocity"), "set_linear_velocity", "get_linear_velocity");

		ClassDB::bind_method(D_METHOD("set_angular_velocity", "value"), &Kform::set_ang);
		ClassDB::bind_method(D_METHOD("get_angular_velocity"), &Kform::get_ang);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::VECTOR3, "angular_velocity"), "set_angular_velocity", "get_angular_velocity");

		ClassDB::bind_method(D_METHOD("set_scalar_velocity", "value"), &Kform::set_svl);
		ClassDB::bind_method(D_METHOD("get_scalar_velocity"), &Kform::get_svl);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::VECTOR3, "scalar_velocity"), "set_scalar_velocity", "get_scalar_velocity");

		ClassDB::bind_method(D_METHOD("inverse"), &Kform::inverse);
		ClassDB::bind_method(D_METHOD("multiply"), &Kform::multiply);
		ClassDB::bind_method(D_METHOD("divide"), &Kform::divide);

		ClassDB::bind_method(D_METHOD("character_update", "lin_acc", "desired_velocity", "desired_ang", "halflife_vel", "halflife_ang", "dt"), &Kform::character_update);
		ClassDB::bind_method(D_METHOD("character_prediction", "lin_acc", "desired_velocity", "desired_ang", "halflife_vel", "halflife_ang", "dt"), &Kform::character_update);
	}
};