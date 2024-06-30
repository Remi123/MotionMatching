#pragma once

#include <cmath>
#include <vector>
#include <godot_cpp/variant/Variant.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>


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