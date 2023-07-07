/**************************************************************************/
/*  motion_player.h                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef MOTION_PLAYER_H
#define MOTION_PLAYER_H

#include "core/string/node_path.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "scene/main/node.h"

#include "core/object/class_db.h"
#include "core/object/method_bind.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#ifdef TOOLS_ENABLED
#include "editor/editor_plugin.h"
#endif

#include "scene/gui/box_container.h"

#include "scene/animation/animation_player.h"
#include "scene/resources/animation.h"
#include "scene/resources/animation_library.h"

#include "scene/3d/skeleton_3d.h"

#include <algorithm>
#include <bitset>
#include <chrono>
#include <numeric>
#include <vector>

#include "../thirdparty/kdtree.hpp"
#include "motion_features.h"

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }
#define STR(x) #x
#define STRING_PREFIX(prefix, s) STR(prefix##s)

struct MotionPlayer : public Node {
	GDCLASS(MotionPlayer, Node)

	struct Stats {
		float max;
		float min;
		float median;
		float variance;
		float skewness;
		float density;
		float sum;
	    int64_t count; 
	};

	Stats calculate_stats(const Vector<float>& p_data) {
		Stats stats{};
		
		if (p_data.is_empty()) {
			return stats;
		}

		float min = p_data[0];
		float max = p_data[0];

		for (int i = 1; i < p_data.size(); ++i) {
			if (p_data[i] < min) {
				min = p_data[i];
			}
			if (p_data[i] > max) {
				max = p_data[i];
			}
		}

		stats.min = min;
		stats.max = max;

		int size = p_data.size();
	    stats.count = size;
		
		Vector<float> sorted_data = p_data;
		sorted_data.sort();
		stats.median = (size % 2 != 0) ? sorted_data[size / 2] : (sorted_data[(size - 1) / 2] + sorted_data[size / 2]) / 2.0;
		double sum = 0.0;
		for (const float &num : p_data) {
			sum += num;
		}
		stats.sum = sum;
		double mean = sum / size;
		double sq_diff_sum = 0.0;
		for (const float &d : p_data) {
			sq_diff_sum += (d - mean) * (d - mean);
		}
		stats.variance = sq_diff_sum / size;

		double diff_sum = 0.0;
		for (const float &d : p_data) {
			diff_sum += pow(d - mean, 3);
		}
		stats.skewness = diff_sum / (size * std::pow(stats.variance, 1.5));
		stats.density = size / (stats.max - stats.min);
		return stats;
	}

	static constexpr float interval = 0.1;
	static constexpr float time_delta = 1.f / 30.f;

	MotionPlayer() = default;
	~MotionPlayer() = default;

	// Properties and variables

	// The array. Will be sent to the KDtree.
	GETSET(PackedFloat32Array, MotionData);

	// Useful for passing data at runtime.
	GETSET(Dictionary, blackboard)

	// The track names inside the animations that define the categories, usually a value track to a int.
	GETSET(TypedArray<String>, category_track_names)

	// Get the skeleton TODO : Might be able to use PROPERTY_HINT_NODE_PATH_TO_EDITED_NODE
	Skeleton3D *skeleton = nullptr;
	NodePath skeleton_path;
	void set_skeleton(NodePath path) {
		skeleton_path = path;
		skeleton = cast_to<Skeleton3D>(get_node(path));
	}
	NodePath get_skeleton() { return skeleton_path; }
	GETSET(NodePath, main_node)

	// Animation Library. Each one will be analysed
	GETSET(Ref<AnimationLibrary>, animation_library);

	// Array of the motion features.
	GETSET(Array, motion_features);

	// Dimensional Stats.
	GETSET(PackedFloat32Array, weights)
	GETSET(PackedFloat32Array, means)
	GETSET(PackedFloat32Array, variances)
	GETSET(Array, densities)

	// The KdTree.
	Kdtree::KdTree *kdt = nullptr;

	// Database. A pose is just the index of a row in the kdtree.
	// Usage : db_anim_*[result.index] =
	GETSET(PackedInt32Array, db_anim_index); // Index of the animation name in the animation library
	GETSET(PackedFloat32Array, db_anim_timestamp); // timestamp of the pose in the animation
	GETSET(PackedInt32Array, db_anim_category); // Category of the pose in the animation

	// Simple helper. Might be removed from this class
	GETSET(int, category_value);

	// How the kdtree calculate the distance.
	// 0 (L0) : Maximum of each difference in all dimensions.
	// 1 (L1) : Manhattan distance (default)
	// 2 (L2) : Distance squared.
	int distance_type = 1;
	int get_distance_type();
	void set_distance_type(int value);

protected:
	void _notification(int p_what);

public:
	// Useful while baking data and in editor.
	void set_skeleton_to_pose(Ref<Animation> animation, double time);

	// Reset the skeleton poses.
	void reset_skeleton_poses();

	// Bake the data into the KdTree.
	// Goes through all the animations and construct the kdtree with each features at the interval.
	virtual void baking_data();

	// A predicate for searching animation categories.
	// included_category_bitfield : The animations must at least contains those categories
	// excluded_category_bitfield : Exclude every animations with any of those categories
	struct Category_Pred : Kdtree::KdNodePredicate {
		const std::bitset<64> m_desired_category;
		const std::bitset<64> m_exclude_category;
		Category_Pred(int64_t included_category_bitfield, int64_t excluded_category_bitfield = 0) :
				m_desired_category{ static_cast<uint64_t>(included_category_bitfield) }, m_exclude_category{ static_cast<uint64_t>(excluded_category_bitfield) } {}

		virtual bool operator()(const Kdtree::KdNode &node) const {
			static constexpr std::bitset<64> zero = {};
			const std::bitset<64> node_category = *((int32_t *)node.data);
			const bool include = (m_desired_category & node_category) == node_category;
			const bool exclude = (m_exclude_category & node_category) == zero;
			return include && exclude;
		}
	};
	// Calculate the weights using the features get_weights() functions.
	// Take into consideration the number of dimensions.
	// The calculation might be reconsidered, but it's the best I found.
	void recalculate_weights();

	// query the kdtree.
	// Can include or exclude categories.
	TypedArray<Dictionary> query_pose(int64_t included_category = std::numeric_limits<int64_t>::max(), int64_t exclude = 0);

	// Bypass the feature query, and ask directly which poses is the most similar.
	// The query must be of the correct dimension.
	Array check_query_results(PackedFloat32Array query, int64_t nb_result = 1);

protected:
	static void _bind_methods();
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX
#undef BINDER
#undef BINDER_PROPERTY
#undef BINDER_PROPERTY_PARAMS

#endif // MOTION_PLAYER_H