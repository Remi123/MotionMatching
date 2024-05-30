#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

#include <godot_cpp/classes/v_box_container.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/bone_map.hpp>

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skeleton_profile.hpp>

#include <algorithm>
#include <bitset>
#include <chrono>
#include <numeric>
#include <vector>

#include "godot_cpp/core/math.hpp"

#include "MotionFeatures/MotionFeatures.hpp"
#include "kdtree-cpp/kdtree.hpp"
#include <AnimTags/AnimTag.hpp>
#include <AnimTags/IndexSet.hpp>

#include <Math/KForm.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

using namespace godot;

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }
#define STR(x) #x
#define STRING_PREFIX(prefix, s) STR(prefix##s)
#define BINDER_PROPERTY_PARAMS(type, variant_type, variable, ...)                                  \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(set_, variable), "value"), &type::set_##variable); \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(get_, variable)), &type::get_##variable);          \
	ADD_PROPERTY(PropertyInfo(variant_type, #variable, __VA_ARGS__), STRING_PREFIX(set_, variable), STRING_PREFIX(get_, variable));

struct MMAnimationLibrary : public AnimationLibrary {
	using u = godot::UtilityFunctions;
	GDCLASS(MMAnimationLibrary, AnimationLibrary)

public:
	MMAnimationLibrary() :
			AnimationLibrary() {
		u::prints("MMAL", "Constructor", MotionData.size());
	}
	~MMAnimationLibrary() {
		u::prints("MMAL", "Destructor");
		if (kdt != nullptr) {
			delete kdt;
		}
	}

	void _notification(int what) {
		switch (what) {
			case NOTIFICATION_POSTINITIALIZE: // Constructor
			{
				u::prints("MMAL NOTIFICATION_POSTINITIALIZE", "InEditor:", godot::Engine::get_singleton()->is_editor_hint(), MotionData.size());
				if (!godot::Engine::get_singleton()->is_editor_hint()) {
					// fill_kdtree();
				}
			} break;
			case NOTIFICATION_PREDELETE: // Destructor
			{
				u::prints("MMAL NOTIFICATION_PREDELETE", "InEditor:", godot::Engine::get_singleton()->is_editor_hint(), MotionData.size());
				if (kdt != nullptr) {
					delete kdt;
				}
			} break;
			default:
				u::prints("MMAL Default notification", what);
		}
	}

	GETSET(int, strategy)
	GETSET(StringName, skeleton_path);
	GETSET(Ref<SkeletonProfile>, skeleton_profile)
	float time_interval{};
	float get_time_interval() { return time_interval; }
	void set_time_interval(float value) { time_interval = std::abs(value); }

	GETSET(float, continuation_bias);

	String category_hint_string{};
	String get_category_hint_string() { return category_hint_string; }
	void set_category_hint_string(String value) {
		category_hint_string = value;
		int nb = 0;
		for (int i = 0; i < tags.size(); ++i) {
			TagCategory *category = Object::cast_to<TagCategory>(tags[i]);
			if (category != nullptr) {
				category->property_hint_string = category_hint_string;
				++nb;
			}
		}
	}
	GETSET(TypedArray<TagInfo>, tags);

	// Category tracks
	GETSET(TypedArray<String>, category_track_names)
	// Array of the motion features.
	GETSET(TypedArray<MotionFeature>, motion_features);
	// The data
	GETSET(PackedFloat32Array, MotionData);

	// Dimensional Stats.
	GETSET(int, nb_dimensions)
	GETSET(PackedFloat32Array, weights)

	GETSET(PackedFloat32Array, feature_offset);
	GETSET(PackedFloat32Array, feature_scale);

	// Database. A pose is just the index of a row in the kdtree.
	// Usage : db_anim_*[result.index] =
	GETSET(PackedInt32Array, db_anim_index); // Index of the animation name in the animation library
	GETSET(PackedFloat32Array, db_anim_timestamp); // timestamp of the pose in the animation
	GETSET(PackedInt32Array, db_anim_category); // Category of the pose in the animation

	// The KdTree.
	Kdtree::KdTree *kdt = nullptr;

	// How the kdtree calculate the distance.
	// 0 (L0) : Maximum of each difference in all dimensions.
	// 1 (L1) : Manhattan distance (default)
	// 2 (L2) : Distance squared.
	int distance_type = 1;
	int get_distance_type() { return distance_type; }
	void set_distance_type(int value) {
		distance_type = value;
		if (kdt != nullptr && 0 <= distance_type && distance_type <= 2)
			kdt->set_distance(distance_type);
	}

	void _cache_kdtree(bool reset = false) {
		if (reset) {
			if (kdt != nullptr)
				delete kdt;
			kdt = nullptr;
		}
		if (kdt == nullptr) {
			std::cout << "Creating kdtree" << std::endl;
			fill_kdtree();
		}
	}

	void fill_kdtree() {
		u::prints("MF size", motion_features.size());
		ERR_FAIL_COND_EDMSG(nb_dimensions == 0, "Number Dimensions is zero");
		ERR_FAIL_COND_EDMSG(MotionData.is_empty(), "Motion Data is Empty");

		u::prints("Total Dimension", nb_dimensions);
		if (kdt != nullptr)
			delete kdt;

		// Now we bake all the data
		u::prints("Preparing kdtree");
		Kdtree::KdNodeVector nodes{};
		for (size_t i = 0; i < MotionData.size() / nb_dimensions; ++i) {
			auto begin = MotionData.ptr(), end = MotionData.ptr(); // We use the ptr as iterator.
			begin = std::next(begin, nb_dimensions * i);
			end = std::next(begin, nb_dimensions);
			std::vector<float> point(begin, end);
			nodes.push_back(Kdtree::KdNode(std::move(point), &db_anim_category[i], i));
		}
		u::prints("Creating kdtree");
		kdt = new Kdtree::KdTree(&nodes, distance_type);

		weights.resize(nb_dimensions);

		auto begin = weights.ptr(), end = weights.ptr(); // We use the ptr as iterator.
		begin = std::next(begin, 0);
		end = std::next(begin, nb_dimensions);
		const std::vector<float> tmp_weight(begin, end);

		kdt->set_distance(distance_type, &tmp_weight);
		u::prints("KDTree Constructed");
	}

	void bake_data() {
		for (auto i = 0; i < motion_features.size(); ++i) {
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[i]);
			ERR_FAIL_COND_EDMSG(!f->has_method("get_dimension"), "Feature # " + u::str(i) + " doesn't have a get_dimension method");
			ERR_FAIL_COND_EDMSG(!f->has_method("setup_bake_init"), "Feature # " + u::str(i) + " doesn't have a setup_bake_init method");
			ERR_FAIL_COND_EDMSG(!f->has_method("setup_bake_animation"), "Feature # " + u::str(i) + " doesn't have a setup_bake_animation method");
			ERR_FAIL_COND_EDMSG(!f->has_method("bake_animation_pose"), "Feature # " + u::str(i) + " doesn't have a bake_animation_pose method");
		}

		ERR_FAIL_COND_EDMSG(time_interval < 0.016f, "Please choose a time inverval higher than 0.016s");
		ERR_FAIL_COND_EDMSG(motion_features.is_empty(), "No Motion Features to extract data");
		ERR_FAIL_COND_EDMSG(skeleton_profile == nullptr, "Skeleton_profile is empty");
		ERR_FAIL_COND_EDMSG(skeleton_profile->get_root_bone().is_empty(), "SkeletonProfile requires a Root Bone");
		u::prints("Preparing Features...");
		int tmp_nb_dim = 0;
		for (auto i = 0; i < motion_features.size(); ++i) {
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[i]);
			ERR_FAIL_NULL_MSG(f, "Features no." + u::str(i) + "is null");
			u::prints("Feature no.", i, f->get_name(), "Dimensions:", (int)f->call("get_dimension"));
			if ((bool)f->call("setup_bake_init", Ref<MMAnimationLibrary>(this)) == false) {
				ERR_FAIL_EDMSG("Motion Feature failed when setting the profile at index " + u::str(i));
			}
			tmp_nb_dim += (int)f->call("get_dimension");
		}
		nb_dimensions = tmp_nb_dim;
		u::prints("Total Dimension", nb_dimensions);

		godot::TypedArray<godot::StringName> anim_names = get_animation_list();
		u::prints("Detecting", anim_names.size(), "animations. Preparing...");

		PackedFloat32Array data = PackedFloat32Array();

		db_anim_category.clear();
		db_anim_index.clear();
		db_anim_timestamp.clear();

		using namespace boost::accumulators;
		using acc_stats = stats<tag::density, tag::max, tag::min, tag::median, tag::skewness, tag::variance>;
		const accumulator_set<float, acc_stats> default_acc(tag::density::num_bins = 10, tag::density::cache_size = 15);
		std::vector<accumulator_set<float, acc_stats>> data_stats(nb_dimensions, default_acc);

		u::prints("Starting animation baking...");
		Rng_Start.resize(anim_names.size());
		Rng_Start.fill(-1);
		Rng_Stop.resize(anim_names.size());
		Rng_Stop.fill(-1);
		size_t range_counter = 0;
		for (auto anim_index = 0; anim_index < anim_names.size(); ++anim_index) {
			auto clock_start = std::chrono::system_clock::now();

			auto anim_name = anim_names[anim_index];
			auto animation = get_animation(anim_name);

			Rng_Start[anim_index] = range_counter;
			Rng_Stop[anim_index] = range_counter;

			auto current_tags = std::vector<TagInfo *>{};
			for (size_t i = 0; i < tags.size(); i++) {
				TagInfo *tag = Object::cast_to<TagInfo>(tags[i]);
				if (tag != nullptr && tag->animation_name == (StringName)anim_name) {
					current_tags.push_back(tag);
				}
			}
			u::prints("Found", current_tags.size(), "Tags associated with current animation");

			int should_continue = -1;
			for (auto features_index = 0; features_index < motion_features.size(); ++features_index) {
				MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
				if ((bool)f->call("setup_bake_animation", animation) == false) {
					u::prints((bool)f->call("setup_bake_animation", animation));
					should_continue = features_index;
					break;
				}
			}
			if (should_continue != -1) {
				WARN_PRINT_ED("Skipping Animation '" + (String)anim_name + "' because of motion feature index :" + u::str(should_continue));
				continue;
			}

			const auto length = animation->get_loop_mode() == Animation::LOOP_NONE ? animation->get_length() - 0.2 : animation->get_length();

			u::prints("Animations setup for", anim_name, "duration", animation->get_length());

			int _limit = animation->get_length() / time_interval;
			IndexSet timed(0, _limit);
			for (TagInfo *tag : current_tags) {
				if (TagJunk *junk = Object::cast_to<TagJunk>(tag); junk) {
					auto _start = godot::CLAMP(int(junk->timestamp / time_interval), 0, _limit);
					auto _end = godot::CLAMP(int((junk->timestamp + junk->duration) / time_interval), 0, _limit);
					timed -= IndexRange(_start, _end);
				}
			}

			auto counter = 0;
			for (IndexRange interval : timed) {
				for (size_t time_index = interval.front(); time_index <= interval.back(); ++time_index) {
					auto time = time_index * time_interval;

					int64_t tmp_category_value = 0;

					// Tags Logic
					// Get all tags at this timestamp
					// If one is Junk, continue
					auto skip = std::find_if(current_tags.begin(), current_tags.end(),
							[time](TagInfo *tag) {
								TagJunk *junk = Object::cast_to<TagJunk>(tag);
								if (junk != nullptr) {
									return junk->timestamp <= time && time <= junk->timestamp + junk->duration;
								}
								return false;
							});
					if (skip != current_tags.end()) {
						continue;
					}
					// If category, OR it
					std::for_each(current_tags.begin(), current_tags.end(),
							[&tmp_category_value, time](TagInfo *tag) {
								TagCategory *category = Object::cast_to<TagCategory>(tag);
								if (category != nullptr && category->timestamp <= time && time <= category->timestamp + category->duration) {
									tmp_category_value |= category->category;
								}
							});

					PackedFloat32Array pose_data{};
					for (size_t features_index = 0; features_index < motion_features.size(); ++features_index) {
						MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
						size_t const expected_dimension = (size_t)f->call("get_dimension");
						PackedFloat32Array feature_data = f->call("bake_animation_pose", animation, time);
						ERR_FAIL_COND_MSG(feature_data.size() != expected_dimension, String("Features no.") + u::str(int(features_index)) + "bake_animation_pose didn't return a array of the correct size:" + u::str(feature_data.size()) + '/' + u::str(expected_dimension));
						pose_data.append_array(feature_data);
					}

					for (int i = 0; i < nb_dimensions; ++i) {
						data_stats[i](pose_data[i]);
					}
					data.append_array(pose_data);
					db_anim_index.append(anim_index);
					db_anim_timestamp.append(time);
					db_anim_category.append(tmp_category_value);

					++counter;
					Rng_Stop[anim_index] = range_counter;
					++range_counter;
				}
			}
			auto clock_end = std::chrono::system_clock::now();
			float duration = float(std::chrono::duration_cast<std::chrono::milliseconds>(clock_end - clock_start).count());
			u::prints("Collecting animation data from ", animation->get_name(), " in ", duration, "ms. PoseCount", counter);
		}

		u::prints("Animation Data Collected. Normalizing... ");

		feature_offset.clear();
		feature_scale.clear();
		feature_offset.resize(nb_dimensions);
		feature_scale.resize(nb_dimensions);
		feature_offset.fill(0.0f);
		feature_scale.fill(1.0f);

		// Normalization
		// There is some amount of logic here that must be taking care when baking and querying.
		// A feature could expect to use the raw values instead of normalizing.
		// I expect this part to be changed feature type get added.
		for (size_t features_index = 0, offset = 0; features_index < motion_features.size(); ++features_index) {
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
			int feature_dimension = (int)f->call("get_dimension");
			if (MotionFeature::NormalizationType::Standard == f->get_normalization_type()) {
				for (auto i = offset; i < feature_dimension; ++i) {
					feature_scale[offset + i] = std::sqrtf(variance(data_stats[offset + i]));
					if (feature_scale[offset + i] < std::numeric_limits<float>::epsilon()) {
						feature_scale[offset + i] = 1.0f;
					}
				}
			} else if (MotionFeature::NormalizationType::RawValue == f->get_normalization_type()) {
				for (auto i = offset; i < feature_dimension; ++i) {
					feature_offset[offset + i] = 0.0f;
					feature_scale[offset + i] = 1.0f;
				}
			}
			offset += (int)f->call("get_dimension");
		}
		// Apply normalization to data. When using RawValue, means and variance are 0 and 1 respectively.
		for (size_t pose = 0; pose < data.size() / nb_dimensions; ++pose) {
			for (int offset = 0; offset < nb_dimensions; ++offset) {
				data[pose * nb_dimensions + offset] = (data[pose * nb_dimensions + offset] - feature_offset[offset]) / feature_scale[offset];
			}
		}

		u::prints("Data Normalized. Copy data to Motion Data property...");
		MotionData = data.duplicate();

		if (weights.size() != nb_dimensions) {
			WARN_PRINT_ED("Weights resized to " + u::str(nb_dimensions) + " and reset to ones.");
			weights.resize(nb_dimensions);
		}

		u::prints("Creating bounds");
		build_bounds();

		u::prints("Finished All Animations");
		u::prints("NbDim", nb_dimensions, "NbPoses:", data.size() / nb_dimensions, "Size", data.size());
		WARN_PRINT_ED("MMAnimationLibrary " + get_name() + " bake operation is finished");
	}

	// Calculate the weights using the features get_weights() functions.
	// Take into consideration the number of dimensions.
	// The calculation might be reconsidered, but it's the best I found.
	void recalculate_weights() {
		PackedFloat32Array tmp_weight{};

		for (auto features_index = 0; features_index < motion_features.size(); ++features_index) {
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
			ERR_FAIL_COND_EDMSG(!f->has_method("get_weights"), "Feature # " + u::str(features_index) + " doesn't have a get_weights method");
			tmp_weight.append_array((PackedFloat32Array)f->call("get_weights"));
		}
		weights.clear();
		weights = tmp_weight;
		u::prints("New Weights Values:", weights);
	}

	/// AABB
	GETSET(PackedInt32Array, Rng_Start);
	GETSET(PackedInt32Array, Rng_Stop);
	GETSET(PackedFloat32Array, SM_MIN);
	GETSET(PackedFloat32Array, SM_MAX);
	GETSET(PackedFloat32Array, LR_MIN);
	GETSET(PackedFloat32Array, LR_MAX);
	GETSET(int, BOUND_SM_SIZE, 16);
	GETSET(int, BOUND_LR_SIZE, 64);
	GETSET(real_t, category_penality);

	void build_bounds() {
		// Compute array size
		const size_t nframe = MotionData.size() / nb_dimensions;
		const size_t nbound_sm = ((nframe + BOUND_SM_SIZE - 1) / BOUND_SM_SIZE);
		const size_t nbound_lr = ((nframe + BOUND_LR_SIZE - 1) / BOUND_LR_SIZE);
		SM_MAX.resize(nbound_sm * nb_dimensions);
		SM_MAX.fill(std::numeric_limits<float>::max());
		SM_MIN.resize(nbound_sm * nb_dimensions);
		SM_MIN.fill(std::numeric_limits<float>::min());
		LR_MAX.resize(nbound_lr * nb_dimensions);
		LR_MAX.fill(std::numeric_limits<float>::max());
		LR_MIN.resize(nbound_lr * nb_dimensions);
		LR_MIN.fill(std::numeric_limits<float>::min());

		for (size_t i = 0; i < nframe; ++i) {
			int i_sm = i / BOUND_SM_SIZE;
			int i_lr = i / BOUND_LR_SIZE;
			for (size_t j = 0; j < nb_dimensions; ++j) {
				const size_t small_index = i_sm * nb_dimensions + j;
				const size_t large_index = i_lr * nb_dimensions + j;
				const size_t db_index = i * nb_dimensions + j;
				SM_MIN[small_index] = fminf(SM_MIN[small_index], MotionData[db_index]);
				SM_MAX[small_index] = fmaxf(SM_MAX[small_index], MotionData[db_index]);
				LR_MIN[large_index] = fminf(LR_MIN[large_index], MotionData[db_index]);
				LR_MAX[large_index] = fmaxf(LR_MAX[large_index], MotionData[db_index]);
			}
		}
	}

	static inline float squaref(float x) {
		return x * x;
	}
	static inline float clampf(float x, float min, float max) {
		return x > max ? max : x < min ? min
									   : x;
	}

	Ref<SetRangeIndex> get_indicies_of_animations() {
		Ref<SetRangeIndex> out{};
		out.instantiate();

		for (size_t i = 0; i < Rng_Start.size(); ++i) {
			out->AddRange(Rng_Start[i], Rng_Stop[i]);
		}

		return out;
	}

	Ref<SetRangeIndex> get_indicies_of_category(int _mask) {
		Ref<SetRangeIndex> out{};
		out.instantiate();

		const std::bitset<32> mask = _mask;

		bool out_active = false;
		int out_i = 0;
		int start = 0;

		for (auto i :
				std::ranges::iota_view{ 1, db_anim_category.size() }) {
			std::bitset<32> bit = db_anim_category[i];
			// Activate output
			if (!out_active && (bit & mask) == mask) {
				start = i;
				out_active = true;
			}
			// Deactivate output
			else if (out_active && (bit & mask) != mask) {
				out->AddRange(start, i);
				out_active = false;
				out_i++;
			}
		}
		if (out_active) {
			out->AddRange(start, db_anim_category.size());
			out_i++;
		}
		return out;
	}

	// The logic is range-based. So it's better to find all the Tags that include the category, and remove the unwanted.
	TypedArray<Dictionary> query_pose_aabb(PackedFloat32Array query, int best_index = -1, int ignore_surrounding = 20, Ref<SetRangeIndex> ranges_search = nullptr) {
		constexpr size_t ignore_range_end = 20;
		const float transition_cost = continuation_bias;
		const size_t nfeatures = nb_dimensions;
		const size_t nranges = ranges_search == nullptr ? Rng_Start.size() : ranges_search->ranges.size();
		float best_cost = 0.0f;
		int curr_index = best_index;

		for (size_t i = 0; i < feature_offset.size(); ++i) {
			query[i] = (query[i] - feature_offset[i]) / feature_scale[i];
		}

		auto query_normalized = [&](size_t i) { return query[i]; };
		auto features = [&](size_t i, size_t j) { return MotionData[i * nb_dimensions + j]; };
		auto range_starts = [&](size_t i) -> int { if (ranges_search == nullptr) return Rng_Start[i]; else return cast_to<RangeIndex>(ranges_search->ranges[i])->from; };
		auto range_stops = [&](size_t i) -> int { if (ranges_search == nullptr) return Rng_Stop[i]; else return cast_to<RangeIndex>(ranges_search->ranges[i])->to; };
		auto bound_lr_min = [&](size_t i, size_t j) { return LR_MIN[i * nb_dimensions + j]; };
		auto bound_lr_max = [&](size_t i, size_t j) { return LR_MAX[i * nb_dimensions + j]; };
		auto bound_sm_min = [&](size_t i, size_t j) { return SM_MIN[i * nb_dimensions + j]; };
		auto bound_sm_max = [&](size_t i, size_t j) { return SM_MAX[i * nb_dimensions + j]; };

		if (best_index >= 0) {
			best_cost = 0.0;
			for (int i = 0; i < nfeatures; i++) {
				// Important to not add transition_cost
				best_cost += weights[i] * squaref(query_normalized(i) - features(best_index, i));
			}
		}

		float curr_cost = 0.0f;

		// Search rest of database
		for (int r = 0; r < nranges; r++) {
			// Exclude end of ranges from search
			int i = range_starts(r);
			int range_end = range_stops(r); // - ignore_range_end;

			while (i < range_end) {
				// Find index of current and next large box
				int i_lr = i / BOUND_LR_SIZE;
				int i_lr_next = (i_lr + 1) * BOUND_LR_SIZE;

				// Find distance to box
				curr_cost = transition_cost;
				for (int j = 0; j < nfeatures; j++) {
					curr_cost += weights[j] * squaref(query_normalized(j) - clampf(query_normalized(j), bound_lr_min(i_lr, j), bound_lr_max(i_lr, j)));

					if (curr_cost >= best_cost) {
						break;
					}
				}

				// If distance is greater than current best jump to next box
				if (curr_cost >= best_cost) {
					i = i_lr_next;
					continue;
				}

				// Check against small box
				while (i < i_lr_next && i < range_end) {
					// Find index of current and next small box
					int i_sm = i / BOUND_SM_SIZE;
					int i_sm_next = (i_sm + 1) * BOUND_SM_SIZE;

					// Find distance to box
					curr_cost = transition_cost;
					for (int j = 0; j < nfeatures; j++) {
						curr_cost += weights[j] * squaref(query_normalized(j) - clampf(query_normalized(j), bound_sm_min(i_sm, j), bound_sm_max(i_sm, j)));

						if (curr_cost >= best_cost) {
							break;
						}
					}

					// If distance is greater than current best jump to next box
					if (curr_cost >= best_cost) {
						i = i_sm_next;
						continue;
					}

					// Search inside small box
					while (i < i_sm_next && i < range_end) {
						// Skip surrounding frames
						if (curr_index != -1 && abs(i - curr_index) < ignore_surrounding) {
							i++;
							continue;
						}

						// Check against each frame inside small box
						curr_cost = transition_cost;
						for (int j = 0; j < nfeatures; j++) {
							curr_cost += weights[j] * squaref(query_normalized(j) - features(i, j));
							if (curr_cost >= best_cost) {
								break;
							}
						}

						// If cost is lower than current best then update best
						if (curr_cost < best_cost) {
							best_index = i;
							best_cost = curr_cost;
						}

						i++;
					}
				}
			}
		}
		TypedArray<Dictionary> result{};
		Dictionary data{};
		if (best_index < 0) {
			data["index"] = -1;
			data["animation"] = "";
			data["timestamp"] = 0.0f;
			data["cost"] = best_cost;
			result.append(data);
			return result;
		}

		const StringName anim_name = get_animation_list()[db_anim_index[best_index]];
		const float anim_time = db_anim_timestamp[best_index];
		data["index"] = best_index;
		data["animation"] = anim_name;
		data["timestamp"] = std::move(anim_time);
		data["cost"] = best_cost;
		result.append(data);
		return result;
	}
	/// AABB

	// Bypass the feature query, and ask directly which poses is the most similar.
	// The query must be of the correct dimension.
	Array check_query_results(PackedFloat32Array query, int64_t nb_result = 1) {
		_cache_kdtree(true);

		auto begin = weights.ptr(), end = weights.ptr(); // We use the ptr as iterator.
		begin = std::next(begin, 0);
		end = std::next(begin, query.size());
		const std::vector<float> tmp_weight(begin, end);

		kdt->set_distance(distance_type, &tmp_weight);

		auto query_data = Kdtree::CoordPoint(query.ptr(), std::next(query.ptr(), query.size()));

		u::prints("query Constructed");

		Kdtree::KdNodeVector re = Kdtree::KdNodeVector{};
		kdt->k_nearest_neighbors(query_data, nb_result, &re);
		u::prints("Results obtained");
		Array result;
		for (auto i : re) {
			const auto anim_name = get_animation_list()[db_anim_index[i.index]];
			const auto anim_time = db_anim_timestamp[i.index];
			const auto anim_cat = db_anim_category[i.index];
			result.append(Array::make(anim_name, anim_time, anim_cat));
		}
		return result;
	}

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

	TypedArray<Dictionary> query_pose(PackedFloat32Array query, unsigned int nb_result = 1, int64_t included_category = std::numeric_limits<int64_t>::max(), int64_t excluded_category = 0) {
		ERR_FAIL_COND_V_MSG(query.size() != nb_dimensions, {}, "Query must the same size as nb_dimensions");
		ERR_FAIL_COND_V_MSG(feature_offset.size() != nb_dimensions, {}, "Feature Offset must the same size as nb_dimensions");
		ERR_FAIL_COND_V_MSG(feature_scale.size() != nb_dimensions, {}, "Feature Scale must the same size as nb_dimensions");

		// Create three if needs be
		_cache_kdtree();

		// Normalization of the query data. It's expected to not be normalized.
		for (size_t i = 0; i < feature_offset.size(); ++i) {
			query[i] = (query[i] - feature_offset[i]) / feature_scale[i];
		}

		{
			Kdtree::KdNodeVector re{};

			auto query_data = Kdtree::CoordPoint(query.ptr(), std::next(query.ptr(), kdt->dimension));
			auto clock_start = std::chrono::system_clock::now();
			if (included_category == std::numeric_limits<int64_t>::max() && excluded_category == 0)
				kdt->k_nearest_neighbors(query_data, nb_result, &re);
			else {
				auto pred = Category_Pred(included_category, excluded_category);
				kdt->k_nearest_neighbors(query_data, nb_result, &re, &pred);
			}

			auto clock_end = std::chrono::system_clock::now();

			float duration = float(std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_start).count());

			TypedArray<Dictionary> results = {};

			for (int i = 0; i < re.size(); ++i) {
				Dictionary data{};
				const StringName anim_name = get_animation_list()[db_anim_index[re[i].index]];
				const float anim_time = db_anim_timestamp[re[i].index];
				PackedFloat32Array data_result{};
				for (auto d : re[i].point) {
					data_result.append(d);
				}
				for (auto i = 0; i < data_result.size(); ++i) {
					data_result[i] *= feature_scale[i];
					data_result[i] += feature_offset[i];
				}
				data["index"] = re[i].index;
				data["animation"] = anim_name;
				data["timestamp"] = std::move(anim_time);
				data["data"] = data_result;
				results.append(data);
			}

			return results;
		}
		return {};
	}

	enum Space {
		Local,
		Model,
		RootMotion,
		Global
	};

	Dictionary sample_bone_global_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_global_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	Dictionary sample_bone_model_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_model_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	Dictionary sample_bone_rootmotion_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_root_model_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	Dictionary sample_bone_local_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_local_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	void prints_dimensions() {
		for (auto features_index = 0; features_index < motion_features.size(); ++features_index) {
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);

			u::prints("Features #", features_index, "nb dimension", f->call("get_dimension"));
			u::prints("Features #", features_index, "hints", f->call("get_hints"));
			u::prints("Features #", features_index, "setup_bake_init", f->call("setup_bake_init", this));
			u::prints("Features #", features_index, "setup_bake_animation", f->call("setup_bake_animation", nullptr));
			u::prints("Features #", features_index, "bake", (PackedFloat32Array)f->call("bake_animation_pose", nullptr, 0.016));
		}
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("prints_dimensions"), &MMAnimationLibrary::prints_dimensions);
		ClassDB::bind_method(D_METHOD("fill_kdtree"), &MMAnimationLibrary::fill_kdtree);
		// Enum
		{
			BIND_ENUM_CONSTANT(Local);
			BIND_ENUM_CONSTANT(Model);
			BIND_ENUM_CONSTANT(RootMotion);
			BIND_ENUM_CONSTANT(Global);
		}
		// Functions
		{
			ClassDB::bind_method(D_METHOD("sample_bone_local_info", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_local_info);
			ClassDB::bind_method(D_METHOD("sample_bone_model_info", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_model_info);
			ClassDB::bind_method(D_METHOD("sample_bone_rootmotion_info", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_rootmotion_info);
			ClassDB::bind_method(D_METHOD("sample_bone_global_info", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_global_info);

			ClassDB::bind_method(D_METHOD("bake_data"), &MMAnimationLibrary::bake_data);
			ClassDB::bind_method(D_METHOD("recalculate_weights"), &MMAnimationLibrary::recalculate_weights);
			ClassDB::bind_method(D_METHOD("check_query_results", "Query", "Result count"), &MMAnimationLibrary::check_query_results);
			ClassDB::bind_method(D_METHOD("query_pose", "serialized_query", "number_result", "include_category", "exclude_category"), &MMAnimationLibrary::query_pose, DEFVAL(1), DEFVAL(std::numeric_limits<int64_t>::max()), DEFVAL(0));
			ClassDB::bind_method(D_METHOD("query_pose_aabb", "serialized_query", "best_index", "ignore_surrounding_indicies", "ranges_search"), &MMAnimationLibrary::query_pose_aabb, DEFVAL(-1), DEFVAL(20), DEFVAL(nullptr));

			// SetRangeIndex
			ClassDB::bind_method(D_METHOD("get_indicies_of_animations"), &MMAnimationLibrary::get_indicies_of_animations);
			ClassDB::bind_method(D_METHOD("get_indicies_of_category", "mask"), &MMAnimationLibrary::get_indicies_of_category);
		}
		// Internal properties
		{
			ClassDB::bind_method(D_METHOD("set_nb_dimensions", "value"), &MMAnimationLibrary::set_nb_dimensions);
			ClassDB::bind_method(D_METHOD("get_nb_dimensions"), &MMAnimationLibrary::get_nb_dimensions);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "nb_dimensions", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_nb_dimensions", "get_nb_dimensions");
			ClassDB::bind_method(D_METHOD("set_db_anim_index", "value"), &MMAnimationLibrary::set_db_anim_index);
			ClassDB::bind_method(D_METHOD("get_db_anim_index"), &MMAnimationLibrary::get_db_anim_index);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_INT32_ARRAY, "db_anim_index", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_db_anim_index", "get_db_anim_index");
			ClassDB::bind_method(D_METHOD("set_db_anim_timestamp", "value"), &MMAnimationLibrary::set_db_anim_timestamp);
			ClassDB::bind_method(D_METHOD("get_db_anim_timestamp"), &MMAnimationLibrary::get_db_anim_timestamp);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "db_anim_timestamp", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_db_anim_timestamp", "get_db_anim_timestamp");
			ClassDB::bind_method(D_METHOD("set_db_anim_category", "value"), &MMAnimationLibrary::set_db_anim_category);
			ClassDB::bind_method(D_METHOD("get_db_anim_category"), &MMAnimationLibrary::get_db_anim_category);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_INT32_ARRAY, "db_anim_category", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_db_anim_category", "get_db_anim_category");
		}
		ClassDB::add_property_group(get_class_static(), "Dependancy resources", "");
		{
			ClassDB::bind_method(D_METHOD("set_time_interval", "value"), &MMAnimationLibrary::set_time_interval, DEFVAL(0.016f));
			ClassDB::bind_method(D_METHOD("get_time_interval"), &MMAnimationLibrary::get_time_interval);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "time_interval", PROPERTY_HINT_RANGE, "0.016, 1, 0.016, or_greater"), "set_time_interval", "get_time_interval");

			ClassDB::bind_method(D_METHOD("set_skeleton_path", "value"), &MMAnimationLibrary::set_skeleton_path);
			ClassDB::bind_method(D_METHOD("get_skeleton_path"), &MMAnimationLibrary::get_skeleton_path);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING_NAME, "skeleton_path"), "set_skeleton_path", "get_skeleton_path");

			ClassDB::bind_method(D_METHOD("set_skeleton_profile", "value"), &MMAnimationLibrary::set_skeleton_profile);
			ClassDB::bind_method(D_METHOD("get_skeleton_profile"), &MMAnimationLibrary::get_skeleton_profile);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT, "skeleton_profile", PROPERTY_HINT_RESOURCE_TYPE, "SkeletonProfile"), "set_skeleton_profile", "get_skeleton_profile");
		}

		{
			ClassDB::bind_method(D_METHOD("set_strategy", "value"), &MMAnimationLibrary::set_strategy);
			ClassDB::bind_method(D_METHOD("get_strategy"), &MMAnimationLibrary::get_strategy);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "strategy", PROPERTY_HINT_ENUM, "NoAcceleration:0,AABBTree:1,KDTree:2"), "set_strategy", "get_strategy");
		}

		ClassDB::add_property_group(get_class_static(), "Features", "");
		{
			ClassDB::bind_method(D_METHOD("set_category_track_names", "value"), &MMAnimationLibrary::set_category_track_names);
			ClassDB::bind_method(D_METHOD("get_category_track_names"), &MMAnimationLibrary::get_category_track_names);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY, "category_track_names", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_DEFAULT), "set_category_track_names", "get_category_track_names");

			// BINDER_PROPERTY_PARAMS(MMAnimationLibrary,Variant::STRING,category_hint_string);
			ClassDB::bind_method(D_METHOD("set_category_hint_string", "value"), &MMAnimationLibrary::set_category_hint_string);
			ClassDB::bind_method(D_METHOD("get_category_hint_string"), &MMAnimationLibrary::get_category_hint_string);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING, "category_hint_string", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), "set_category_hint_string", "get_category_hint_string");

			ClassDB::bind_method(D_METHOD("set_tags", "value"), &MMAnimationLibrary::set_tags);
			ClassDB::bind_method(D_METHOD("get_tags"), &MMAnimationLibrary::get_tags);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "tags", godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":TagInfo", PROPERTY_USAGE_DEFAULT), "set_tags", "get_tags");

			ClassDB::bind_method(D_METHOD("set_motion_features", "value"), &MMAnimationLibrary::set_motion_features);
			ClassDB::bind_method(D_METHOD("get_motion_features"), &MMAnimationLibrary::get_motion_features);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "motion_features", godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":MotionFeature", PROPERTY_USAGE_DEFAULT), "set_motion_features", "get_motion_features");
		}
		ClassDB::add_property_group(get_class_static(), "Database", "");
		{
			ClassDB::bind_method(D_METHOD("set_MotionData", "value"), &MMAnimationLibrary::set_MotionData);
			ClassDB::bind_method(D_METHOD("get_MotionData"), &MMAnimationLibrary::get_MotionData);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "MotionData"), "set_MotionData", "get_MotionData");
			ClassDB::bind_method(D_METHOD("set_weights", "value"), &MMAnimationLibrary::set_weights);
			ClassDB::bind_method(D_METHOD("get_weights"), &MMAnimationLibrary::get_weights);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "weights"), "set_weights", "get_weights");

			ClassDB::bind_method(D_METHOD("set_feature_offset", "value"), &MMAnimationLibrary::set_feature_offset);
			ClassDB::bind_method(D_METHOD("get_feature_offset"), &MMAnimationLibrary::get_feature_offset);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "feature_offset"), "set_feature_offset", "get_feature_offset");
			ClassDB::bind_method(D_METHOD("set_feature_scale", "value"), &MMAnimationLibrary::set_feature_scale);
			ClassDB::bind_method(D_METHOD("get_feature_scale"), &MMAnimationLibrary::get_feature_scale);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "feature_scale"), "set_feature_scale", "get_feature_scale");
		}

		ClassDB::add_property_group(get_class_static(), "KDTree", "");
		{
			ClassDB::bind_method(D_METHOD("set_distance_type", "value"), &MMAnimationLibrary::set_distance_type);
			ClassDB::bind_method(D_METHOD("get_distance_type"), &MMAnimationLibrary::get_distance_type);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "distance_type", PROPERTY_HINT_ENUM, "Manhattan:1,EuclidianSquared:2,Maximum:0"), "set_distance_type", "get_distance_type");
		}
		ClassDB::add_property_group(get_class_static(), "AABB Bounding box", "");
		{
			ClassDB::bind_method(D_METHOD("set_continuation_bias", "value"), &MMAnimationLibrary::set_continuation_bias);
			ClassDB::bind_method(D_METHOD("get_continuation_bias"), &MMAnimationLibrary::get_continuation_bias);
			::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "continuation_bias"), "set_continuation_bias", "get_continuation_bias");

			ClassDB::bind_method(D_METHOD("set_category_penality", "value"), &MMAnimationLibrary::set_category_penality, DEFVAL(2.0));
			ClassDB::bind_method(D_METHOD("get_category_penality"), &MMAnimationLibrary::get_category_penality);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "category_penality", PROPERTY_HINT_RANGE, "1.0, 100.0, 0.1, or_greater"), "set_category_penality", "get_category_penality");

			ClassDB::bind_method(D_METHOD("set_BOUND_LR_SIZE", "value"), &MMAnimationLibrary::set_BOUND_LR_SIZE, DEFVAL(64));
			ClassDB::bind_method(D_METHOD("get_BOUND_LR_SIZE"), &MMAnimationLibrary::get_BOUND_LR_SIZE);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "BOUND_LR_SIZE", PROPERTY_HINT_RANGE, "4, 100, 1, or_greater"), "set_BOUND_LR_SIZE", "get_BOUND_LR_SIZE");
			ClassDB::bind_method(D_METHOD("set_BOUND_SM_SIZE", "value"), &MMAnimationLibrary::set_BOUND_SM_SIZE, DEFVAL(16));
			ClassDB::bind_method(D_METHOD("get_BOUND_SM_SIZE"), &MMAnimationLibrary::get_BOUND_SM_SIZE);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "BOUND_SM_SIZE", PROPERTY_HINT_RANGE, "2, 100, 1, or_greater"), "set_BOUND_SM_SIZE", "get_BOUND_SM_SIZE");

			ClassDB::bind_method(D_METHOD("set_LR_MAX", "value"), &MMAnimationLibrary::set_LR_MAX);
			ClassDB::bind_method(D_METHOD("get_LR_MAX"), &MMAnimationLibrary::get_LR_MAX);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "LR_MAX", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_LR_MAX", "get_LR_MAX");
			ClassDB::bind_method(D_METHOD("set_LR_MIN", "value"), &MMAnimationLibrary::set_LR_MIN);
			ClassDB::bind_method(D_METHOD("get_LR_MIN"), &MMAnimationLibrary::get_LR_MIN);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "LR_MIN", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_LR_MIN", "get_LR_MIN");
			ClassDB::bind_method(D_METHOD("set_SM_MAX", "value"), &MMAnimationLibrary::set_SM_MAX);
			ClassDB::bind_method(D_METHOD("get_SM_MAX"), &MMAnimationLibrary::get_SM_MAX);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "SM_MAX", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_SM_MAX", "get_SM_MAX");
			ClassDB::bind_method(D_METHOD("set_SM_MIN", "value"), &MMAnimationLibrary::set_SM_MIN);
			ClassDB::bind_method(D_METHOD("get_SM_MIN"), &MMAnimationLibrary::get_SM_MIN);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "SM_MIN", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_SM_MIN", "get_SM_MIN");
			ClassDB::bind_method(D_METHOD("set_Rng_Start", "value"), &MMAnimationLibrary::set_Rng_Start);
			ClassDB::bind_method(D_METHOD("get_Rng_Start"), &MMAnimationLibrary::get_Rng_Start);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "Rng_Start", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_Rng_Start", "get_Rng_Start");
			ClassDB::bind_method(D_METHOD("set_Rng_Stop", "value"), &MMAnimationLibrary::set_Rng_Stop);
			ClassDB::bind_method(D_METHOD("get_Rng_Stop"), &MMAnimationLibrary::get_Rng_Stop);
			godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "Rng_Stop", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_Rng_Stop", "get_Rng_Stop");
		}
	}

public:
};

VARIANT_ENUM_CAST(MMAnimationLibrary::Space);
