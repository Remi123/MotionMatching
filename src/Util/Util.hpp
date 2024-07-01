#pragma once

#include <cmath>
#include <vector>
#include <godot_cpp/variant/Variant.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }
#define STR(x) #x
#define BINDER_PROPERTY_PARAMS(type,variant_type,variable,...)\
	ClassDB::bind_method(D_METHOD(STR(set_ ## variable),"value"),&type::set_##variable);\
	ClassDB::bind_method(D_METHOD(STR(get_ ## variable)), &type::get_##variable); \
	ADD_PROPERTY(PropertyInfo(variant_type, #variable, __VA_ARGS__), STR(set_ ## variable), STR(get_ ## variable));

void standardize(PackedFloat32Array& arr) {
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

  // Standardize the array
  std::vector<double> standardized;
  for (float& x : arr) {
    x = (x-mean)/stdev;
  }
}