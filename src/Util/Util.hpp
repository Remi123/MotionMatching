#pragma once

#include <cmath>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <type_traits>
#include <vector>

using namespace godot;
namespace mm {
namespace impl {
static void MMEMITCHANGED(Resource *that) {
	// Call emit_changed signal
	that->emit_changed();
}
static void MMEMITCHANGED(Object *that) {
	// Nothing
}
}; //namespace impl
}; //namespace mm

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) {          \
		variable = value;                      \
		mm::impl::MMEMITCHANGED(this);         \
	}
#define GETSET_NoVar(type, variable, ...)      \
	type get_##variable() { return variable; } \
	void set_##variable(type value) {          \
		variable = value;                      \
		mm::impl::MMEMITCHANGED(this);         \
	}
#define STR(x) #x
#define BINDER_PROPERTY_PARAMS(type, variant_type, variable, ...)                        \
	ClassDB::bind_method(D_METHOD(STR(set_##variable), "value"), &type::set_##variable); \
	ClassDB::bind_method(D_METHOD(STR(get_##variable)), &type::get_##variable);          \
	::godot::ClassDB::add_property(get_class_static(), PropertyInfo{ variant_type, #variable, __VA_ARGS__ }, STR(set_##variable), STR(get_##variable));

using namespace godot;
struct MMUtil : godot::RefCounted {
	GDCLASS(MMUtil, RefCounted)
public:
	static PackedFloat32Array standardize(PackedFloat32Array arr) {
		// Calculate the mean
		double mean = 0;
		for (const float x : arr) {
			mean += x;
		}
		mean /= arr.size();

		// Calculate the standard deviation
		double stdev = 0;
		for (const float x : arr) {
			stdev += std::pow(x - mean, 2);
		}
		stdev = std::sqrt(stdev / arr.size());

		PackedFloat32Array output{ arr };
		// Standardize the array
		for (float &x : output) {
			x = (x - mean) / stdev;
		}

		return output;
	}

	static PackedFloat32Array softmax(PackedFloat32Array input) {
		// Calculate the sum of exponentials
		float sum = 0.0f;
		for (float value : input) {
			sum += std::exp(value);
		}
		PackedFloat32Array output{ input };
		// Apply softmax to each element
		for (size_t i = 0; i < input.size(); ++i) {
			output[i] = std::exp(input[i]) / sum;
		}
		return output;
	}

	static Quaternion get_twist(const Quaternion q, const Vector3 p_axis) {
		Vector3 rotationAxis = Vector3(q.x, q.y, q.z);
		double dotProd = p_axis.dot(rotationAxis);
		Vector3 projection = p_axis * dotProd;
		Quaternion twist = Quaternion(projection.x, projection.y, projection.z, q.w).normalized();
		if (dotProd < 0.0) {
			twist = -twist;
		}
		return twist;
	}

	static Quaternion get_swing(const Quaternion q, const Vector3 p_axis) {
		return q * MMUtil::get_twist(q, p_axis).inverse();
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_static_method("MMUtil", D_METHOD("standardize", "arr_f32"), &MMUtil::standardize);
		ClassDB::bind_static_method("MMUtil", D_METHOD("softmax", "arr_f32"), &MMUtil::softmax);
		ClassDB::bind_static_method("MMUtil", D_METHOD("get_twist", "quaternion", "axis"), &MMUtil::get_twist);
		ClassDB::bind_static_method("MMUtil", D_METHOD("get_swing", "quaternion", "axis"), &MMUtil::get_swing);
	}
};