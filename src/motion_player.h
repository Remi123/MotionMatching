
#ifndef MOTION_MATCHING_HPP
#define MOTION_MATCHING_HPP

#include "core/string/node_path.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "scene/main/node.h"

#include "core/object/class_db.h"
#include "core/object/method_bind.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "editor/editor_plugin.h"

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
#include <sys/_types/_int64_t.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/skewness.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }
#define STR(x) #x
#define STRING_PREFIX(prefix, s) STR(prefix##s)

struct MotionPlayer : public Node {
	GDCLASS(MotionPlayer, Node)

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

#endif