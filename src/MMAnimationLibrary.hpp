#pragma once

#include <Util/Util.hpp>

#include <godot_cpp/core/gdvirtual.gen.inc>
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
#include <limits>
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
	}

	Callable anim_added = Callable(this, "_event_on_anim_added");
	Callable anim_removed = Callable(this, "_event_on_anim_removed");

	void _notification(int what) {
		switch (what) {
			case NOTIFICATION_POSTINITIALIZE: // Constructor
			{
				u::prints("MMAL NOTIFICATION_POSTINITIALIZE", "InEditor:", godot::Engine::get_singleton()->is_editor_hint(), MotionData.size());
				connect("animation_added", anim_added);
				connect("animation_removed", anim_removed);
			} break;
			case NOTIFICATION_PREDELETE: // Destructor
			{
				u::prints("MMAL NOTIFICATION_PREDELETE", "InEditor:", godot::Engine::get_singleton()->is_editor_hint(), MotionData.size());
				disconnect("animation_added", anim_added);
				disconnect("animation_removed", anim_removed);
			} break;
			default:
				u::prints("MMAL Default notification", what);
		}
	}

	void _event_on_anim_added(StringName animname) {
		Curve *c = new Curve();
		Ref<Animation> anim = get_animation(animname);
		c->set_min_value(-1.0);
		c->set_max_value(1.0);
		c->add_point(Vector2{ 0.0, 0.0 });
		// if (c->has_method("set_max_domain")) // Waiting for PR https://github.com/godotengine/godot/pull/67857
		// {
		// 	c->call("set_min_domain", 0.0);
		// 	c->call("set_max_domain", anim->get_length());
		// 	c->add_point(Vector2(anim->get_length(), 0.0));
		// } else {
		c->add_point(Vector2{ 1.0, 0.0 });
		// }

		curvecost[animname] = c;
	}
	void _event_on_anim_removed(StringName animname) {
		curvecost.erase(animname);
	}
	Ref<Curve> get_curvecost_animname(StringName animname) {
		return curvecost.get_or_add(animname, new Curve{});
	}

	TypedArray<TagInfo> get_pose_tags(String animation_name, float time) {
		TypedArray<TagInfo> result{};
		for (auto i = 0; i < tags.size(); ++i) {
			Ref<TagInfo> tag = cast_to<TagInfo>(tags[i]);
			if (tag->animation_name == animation_name && time >= tag->timestamp && (tag->timestamp + tag->duration) >= time) {
				result.append(tag);
			}
		}
		return result;
	}

	void bake_data() {
		for (auto i = 0; i < motion_features.size(); ++i) {
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[i]);
			ERR_FAIL_COND_EDMSG(!f->has_method("get_dimension"), "Feature # " + u::str(i) + " doesn't have a get_dimension method");
			// ERR_FAIL_COND_EDMSG(!f->has_method("setup_bake_init"), "Feature # " + u::str(i) + " doesn't have a setup_bake_init method");
			// ERR_FAIL_COND_EDMSG(!f->has_method("setup_bake_animation"), "Feature # " + u::str(i) + " doesn't have a setup_bake_animation method");
			// ERR_FAIL_COND_EDMSG(!f->has_method("bake_animation_pose"), "Feature # " + u::str(i) + " doesn't have a bake_animation_pose method");
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
			int feature_dim = 0;
			if (!GDVIRTUAL_CALL_PTR(f, get_dimension, feature_dim)) {
				feature_dim = f->call("get_dimension");
			}
			tmp_nb_dim += feature_dim;
		}
		nb_dimensions = tmp_nb_dim;
		u::prints("Total Dimension", nb_dimensions);

		godot::TypedArray<godot::StringName> anim_names = get_animation_list();
		u::prints("Detecting", anim_names.size(), "animations. Preparing...");

		PackedFloat32Array data = PackedFloat32Array();

		biases.clear();
		db_anim_category.clear();
		db_anim_index.clear();
		db_anim_timestamp.clear();
		Rng_Start.resize(anim_names.size());
		Rng_Start.fill(-1);
		Rng_Stop.resize(anim_names.size());
		Rng_Stop.fill(-1);

		using namespace boost::accumulators;
		using acc_stats = stats<tag::density, tag::max, tag::min, tag::median, tag::skewness, tag::variance>;
		const accumulator_set<float, acc_stats> default_acc(tag::density::num_bins = 10, tag::density::cache_size = 15);
		std::vector<accumulator_set<float, acc_stats>> data_stats(nb_dimensions, default_acc);

		u::prints("Starting animation baking...");

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

			u::prints("Animations setup for", anim_name, "duration", animation->get_length(), "found", current_tags.size(), "tags for this animation");

			const int _limit = animation->get_length() / time_interval;
			IndexSet timed(0, _limit);
			for (auto t = 0; t < current_tags.size(); ++t) {
				if (TagJunk *junk = Object::cast_to<TagJunk>(current_tags[t]); junk) {
					auto _start = godot::CLAMP(int(junk->timestamp / time_interval), 0, _limit);
					auto _end = godot::CLAMP(int((junk->timestamp + junk->duration) / time_interval), 0, _limit);
					timed -= IndexRange(_start, _end);
				}
			}

			nb_poses = 0;
			for (IndexRange interval : timed) {
				for (size_t time_index = interval.front(); time_index <= interval.back(); ++time_index) {
					auto time = time_index * time_interval;

					// If category, OR it
					int64_t tmp_category_value = 0;
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
						PackedFloat32Array feature_data{};
						if (!GDVIRTUAL_CALL_PTR(f, bake_pose, this, anim_name, time, feature_data)) {
							feature_data = f->call("bake_pose", this, anim_name, time);
						}
						// f->GDVIRTUAL_CALL(bake_pose,this, anim_name, time, feature_data);
						ERR_FAIL_COND_MSG(feature_data.size() != expected_dimension, String("Features no.") + u::str(int(features_index)) + " bake_pose didn't return a array of the correct size:" + u::str(feature_data.size()) + '/' + u::str(expected_dimension));
						pose_data.append_array(feature_data);
					}
					// Discover Biases
					float _biases = 0.0f;
					Ref<Curve> anim_bias = curvecost.get_or_add(anim_name, new Curve{});
					float curvecost_time = time;
					// TODO : This calculation will be change when Curve have modifiable domain instead of just 0 to 1.
					// A PR is ready, but isn't merge yet.
					_biases = anim_bias->sample_baked(curvecost_time / animation->get_length());
					biases.append(_biases);

					for (int i = 0; i < nb_dimensions; ++i) {
						data_stats[i](pose_data[i]);
					}
					data.append_array(pose_data);
					db_anim_index.append(anim_index);
					db_anim_timestamp.append(time);
					db_anim_category.append(tmp_category_value);

					++nb_poses;
					Rng_Stop[anim_index] = range_counter;
					++range_counter;
				}
			}
			auto clock_end = std::chrono::system_clock::now();
			float duration = float(std::chrono::duration_cast<std::chrono::milliseconds>(clock_end - clock_start).count());
			u::prints("Collecting animation data from ", animation->get_name(), " in ", duration, "ms. PoseCount", nb_poses);
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
			if (MotionFeature::NormalizationType::Standardized == f->get_normalization_type()) {
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
			int feature_dim = 0;
			GDVIRTUAL_REQUIRED_CALL_PTR(f, get_dimension, feature_dim);
			offset += feature_dim;
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
		PackedFloat32Array all_weight{};

		for (auto features_index = 0; features_index < motion_features.size(); ++features_index) {
			PackedFloat32Array feature_weight{};
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
			ERR_FAIL_COND_EDMSG(!f->has_method("get_weights"), "Feature # " + u::str(features_index) + " doesn't have a get_weights method");
			GDVIRTUAL_CALL_PTR(f, get_weights, feature_weight);
			all_weight.append_array(feature_weight);
		}
		weights.clear();
		weights = MMUtil::softmax(all_weight);

		u::prints("New Weights Values:", weights);
	}

	Array get_stats() {
		if (MotionData.size() < get_nb_dimensions())
			return {};

		Array result{};
		{
			using namespace boost::accumulators;
			using acc_stats = stats<tag::density, tag::max, tag::min, tag::median, tag::skewness, tag::variance>;
			const accumulator_set<float, acc_stats> default_acc(tag::density::num_bins = 11, tag::density::cache_size = 15);
			std::vector<accumulator_set<float, acc_stats>> data_stats(nb_dimensions, default_acc);

			for (size_t p = 0; p < get_nb_poses(); ++p) {
				for (size_t d = 0; d < get_nb_dimensions(); ++d) {
					auto data = MotionData[p * get_nb_dimensions() + d] * feature_scale[d] + feature_offset[d];
					data_stats[d](data);
				}
			}

			for (auto &&s : data_stats) {
				Dictionary dimension_stats{};

				dimension_stats["maximum"] = max(s);
				dimension_stats["minimum"] = min(s);
				dimension_stats["skewness"] = skewness(s);
				dimension_stats["median"] = median(s);
				dimension_stats["variance"] = variance(s);
				auto hist = density(s);
				PackedFloat32Array lower_bounds, values{};
				for (auto &&h : hist) {
					lower_bounds.append(h.first);
					values.append(h.second);
				}

				dimension_stats["density_hist_bounds"] = lower_bounds;
				dimension_stats["density_hist_values"] = values;
				result.append(dimension_stats);
			}
		}
		return result;
	}

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

		BIAS_SM_MAX.resize(nbound_sm);
		BIAS_SM_MAX.fill(std::numeric_limits<float>::max());
		BIAS_SM_MIN.resize(nbound_sm);
		BIAS_SM_MIN.fill(std::numeric_limits<float>::min());
		BIAS_LR_MAX.resize(nbound_lr);
		BIAS_LR_MAX.fill(std::numeric_limits<float>::max());
		BIAS_LR_MIN.resize(nbound_lr);
		BIAS_LR_MIN.fill(std::numeric_limits<float>::min());

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
			BIAS_SM_MIN[i_sm] = fminf(BIAS_SM_MIN[i_sm], biases[i]);
			BIAS_SM_MAX[i_sm] = fmaxf(BIAS_SM_MAX[i_sm], biases[i]);
			BIAS_LR_MIN[i_lr] = fminf(BIAS_LR_MIN[i_lr], biases[i]);
			BIAS_LR_MAX[i_lr] = fmaxf(BIAS_LR_MAX[i_lr], biases[i]);
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

	TypedArray<Dictionary> query_pose_noacceleration(PackedFloat32Array query, int best_index = -1, int ignore_surrounding = 20, Ref<SetRangeIndex> ranges_search = nullptr, PackedFloat32Array custom_weights = {}) {
		constexpr size_t ignore_range_end = 20;
		const float transition_cost = continuation_bias;
		const size_t nfeatures = nb_dimensions;
		const size_t nranges = ranges_search == nullptr ? Rng_Start.size() : ranges_search->ranges.size();
		float best_cost = std::numeric_limits<float>::max();
		int curr_index = best_index;

		// Normalize query
		for (size_t i = 0; i < feature_offset.size(); ++i) {
			query[i] = (query[i] - feature_offset[i]) / feature_scale[i];
		}

		auto get_weights = [&](size_t i) { return custom_weights.size() > 0 ? custom_weights[i] : weights[i]; };
		auto query_normalized = [&](size_t i) { return query[i]; };
		auto features = [&](size_t i, size_t j) { return MotionData[i * nb_dimensions + j]; };
		auto range_starts = [&](size_t i) -> int { if (ranges_search == nullptr) return Rng_Start[i]; else return cast_to<RangeIndex>(ranges_search->ranges[i])->from; };
		auto range_stops = [&](size_t i) -> int { if (ranges_search == nullptr) return Rng_Stop[i]; else return cast_to<RangeIndex>(ranges_search->ranges[i])->to; };
		auto bound_lr_min = [&](size_t i, size_t j) { return LR_MIN[i * nb_dimensions + j]; };
		auto bound_lr_max = [&](size_t i, size_t j) { return LR_MAX[i * nb_dimensions + j]; };
		auto bound_sm_min = [&](size_t i, size_t j) { return SM_MIN[i * nb_dimensions + j]; };
		auto bound_sm_max = [&](size_t i, size_t j) { return SM_MAX[i * nb_dimensions + j]; };

		if (best_index >= 0) {
			best_cost = 0.0 + biases[best_index]; // Important to not add transition_cost
			for (int f = 0; f < nfeatures; f++) {
				best_cost += get_weights(f) * squaref(query_normalized(f) - features(best_index, f));
			}
		}

		for (int r = 0; r < nranges; ++r) {
			int range_begin = range_starts(r);
			int range_end = range_stops(r);

			for (int i = range_begin; i <= range_begin; ++i) {
				if (curr_index != -1 && abs(i - curr_index) < ignore_surrounding) {
					continue;
				}
				// Check against each frame
				auto curr_cost = transition_cost + biases[i];
				for (int j = 0; j < nfeatures; j++) {
					curr_cost += get_weights(j) * squaref(query_normalized(j) - features(i, j));
					if (curr_cost >= best_cost) {
						break;
					}
				}

				// If cost is lower than current best then update best
				if (curr_cost < best_cost) {
					best_index = i;
					best_cost = curr_cost;
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

	// The logic is range-based. So it's better to find all the Tags that include the category, and remove the unwanted.
	TypedArray<Dictionary> query_pose_aabb(PackedFloat32Array query, int best_index = -1, int ignore_surrounding = 20, Ref<SetRangeIndex> ranges_search = nullptr, PackedFloat32Array custom_weights = {}) {
		constexpr size_t ignore_range_end = 20;
		const float transition_cost = continuation_bias;
		const size_t nfeatures = nb_dimensions;
		const size_t nranges = ranges_search == nullptr ? Rng_Start.size() : ranges_search->ranges.size();
		float best_cost = std::numeric_limits<float>::max();
		int curr_index = best_index;

		for (size_t i = 0; i < feature_offset.size(); ++i) {
			query[i] = (query[i] - feature_offset[i]) / feature_scale[i];
		}

		auto get_weights = [&](size_t i) { return custom_weights.size() > 0 ? custom_weights[i] : weights[i]; };
		auto query_normalized = [&](size_t i) { return query[i]; };
		auto features = [&](size_t i, size_t j) { return MotionData[i * nb_dimensions + j]; };
		auto range_starts = [&](size_t i) -> int { if (ranges_search == nullptr) return Rng_Start[i]; else return cast_to<RangeIndex>(ranges_search->ranges[i])->from; };
		auto range_stops = [&](size_t i) -> int { if (ranges_search == nullptr) return Rng_Stop[i]; else return cast_to<RangeIndex>(ranges_search->ranges[i])->to; };
		auto bound_lr_min = [&](size_t i, size_t j) { return LR_MIN[i * nb_dimensions + j]; };
		auto bound_lr_max = [&](size_t i, size_t j) { return LR_MAX[i * nb_dimensions + j]; };
		auto bound_sm_min = [&](size_t i, size_t j) { return SM_MIN[i * nb_dimensions + j]; };
		auto bound_sm_max = [&](size_t i, size_t j) { return SM_MAX[i * nb_dimensions + j]; };

		if (best_index >= 0) {
			best_cost = 0.0 + biases[best_index];
			for (int f = 0; f < nfeatures; ++f) {
				// Important to not add transition_cost
				best_cost += get_weights(f) * squaref(query_normalized(f) - features(best_index, f));
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
				curr_cost = transition_cost + clampf(0.0, BIAS_LR_MIN[i_lr], BIAS_LR_MAX[i_lr]);
				for (int j = 0; j < nfeatures; j++) {
					curr_cost += get_weights(j) * squaref(query_normalized(j) - clampf(query_normalized(j), bound_lr_min(i_lr, j), bound_lr_max(i_lr, j)));

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
					curr_cost = transition_cost + clampf(0.0, BIAS_SM_MIN[i_sm], BIAS_SM_MAX[i_sm]);
					for (int j = 0; j < nfeatures; j++) {
						curr_cost += get_weights(j) * squaref(query_normalized(j) - clampf(query_normalized(j), bound_sm_min(i_sm, j), bound_sm_max(i_sm, j)));

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
						curr_cost = transition_cost + biases[i];
						for (int j = 0; j < nfeatures; j++) {
							curr_cost += get_weights(j) * squaref(query_normalized(j) - features(i, j));
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

	Dictionary sample_bone_global_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_global_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	Ref<Kform> sample_bone_global_kform(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		Ref<Kform> result = new Kform{};
		result.instantiate();
		result->k = get_global_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
		return result;
	}

	Dictionary sample_bone_model_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_model_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	Ref<Kform> sample_bone_model_kform(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		Ref<Kform> result = new Kform{};
		result.instantiate();
		result->k = get_model_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
		return result;
	}

	Dictionary sample_bone_rootmotion_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_root_model_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	Ref<Kform> sample_bone_rootmotion_kform(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		Ref<Kform> result = new Kform{};
		result.instantiate();
		result->k = get_root_model_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
		return result;
	}

	Dictionary sample_bone_local_info(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		return (Dictionary)get_local_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
	}

	Ref<Kform> sample_bone_local_kform(StringName animation_name, double time, NodePath bone_path) {
		ERR_FAIL_COND_V(skeleton_profile == nullptr, {});
		ERR_FAIL_COND_V(!has_animation(animation_name), {});
		Ref<Kform> result = new Kform{};
		result.instantiate();
		result->k = get_local_kform(skeleton_profile, get_animation(animation_name), time, bone_path);
		return result;
	}

	void prints_dimensions() {
		for (auto features_index = 0; features_index < motion_features.size(); ++features_index) {
			MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);

			u::prints("Features #", features_index, "nb dimension", f->call("get_dimension"));
			u::prints("Features #", features_index, "hints", f->call("get_hints"));
			// u::prints("Features #", features_index, "setup_bake_init", f->call("setup_bake_init", this));
			// u::prints("Features #", features_index, "setup_bake_animation", f->call("setup_bake_animation", nullptr));
			// u::prints("Features #", features_index, "bake", (PackedFloat32Array)f->call("bake_animation_pose", nullptr, 0.016));
		}
	}

	// All the GETSET
	GETSET(StringName, skeleton_path);
	GETSET(Ref<SkeletonProfile>, skeleton_profile)
	GETSET(float, time_interval, 0.1);

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
		emit_changed();
	}
	GETSET(TypedArray<TagInfo>, tags);
	GETSET(Dictionary, curvecost); // map<StringName,Curve> -> map<animation_name, cost>

	// Array of the motion features.
	GETSET(TypedArray<MotionFeature>, motion_features);
	// The data
	GETSET(PackedFloat32Array, MotionData);

	// Dimensional Stats.
	GETSET(int, nb_dimensions)
	GETSET(int, nb_poses)
	GETSET(PackedFloat32Array, weights)
	GETSET(PackedFloat32Array, biases)

	GETSET(PackedFloat32Array, feature_offset);
	GETSET(PackedFloat32Array, feature_scale);

	// Database.
	// Usage : db_anim_*[result.index] =
	GETSET(PackedInt32Array, db_anim_index); // Index of the animation name in the animation library
	GETSET(PackedFloat32Array, db_anim_timestamp); // timestamp of the pose in the animation
	GETSET(PackedInt32Array, db_anim_category); // Category of the pose in the animation

	/// AABB
	GETSET(PackedInt32Array, Rng_Start);
	GETSET(PackedInt32Array, Rng_Stop);
	GETSET(PackedFloat32Array, SM_MIN);
	GETSET(PackedFloat32Array, SM_MAX);
	GETSET(PackedFloat32Array, LR_MIN);
	GETSET(PackedFloat32Array, LR_MAX);
	GETSET(int, BOUND_SM_SIZE, 16);
	GETSET(int, BOUND_LR_SIZE, 64);

	GETSET(PackedFloat32Array, BIAS_SM_MIN);
	GETSET(PackedFloat32Array, BIAS_SM_MAX);
	GETSET(PackedFloat32Array, BIAS_LR_MIN);
	GETSET(PackedFloat32Array, BIAS_LR_MAX);

protected:
	static void _bind_methods() {
		// Refactored Default Inspector
		ClassDB::add_property_group(get_class_static(), "Core", "");
		{
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::FLOAT, time_interval, PROPERTY_HINT_RANGE, "0.016, 1, 0.016, or_greater");
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::STRING_NAME, skeleton_path);
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::OBJECT, skeleton_profile);

			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::INT, nb_dimensions, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY);
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::INT, nb_poses, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY);
		}

		ClassDB::add_property_group(get_class_static(), "Features", "");
		{
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::ARRAY, motion_features, godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":MotionFeature", PROPERTY_USAGE_DEFAULT);
		}

		ClassDB::add_property_group(get_class_static(), "Tags", "");
		{
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::STRING, category_hint_string, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED);
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::ARRAY, tags, godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":TagInfo", PROPERTY_USAGE_STORAGE);
		}

		ClassDB::add_property_group(get_class_static(), "Query Options", "");
		{
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::PACKED_FLOAT32_ARRAY, weights);
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::FLOAT, continuation_bias);
		}

		ClassDB::add_property_group(get_class_static(), "Database", "");
		{
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::PACKED_FLOAT32_ARRAY, MotionData);
			

			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::PACKED_FLOAT32_ARRAY, feature_scale,PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY);
			BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::PACKED_FLOAT32_ARRAY, feature_offset,PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY);

			ClassDB::add_property_subgroup(get_class_static(), "Acceleration Options", "");
			{
				
				BINDER_PROPERTY_PARAMS(MMAnimationLibrary, Variant::PACKED_FLOAT32_ARRAY, biases, PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY);
				

				ClassDB::bind_method(D_METHOD("set_BOUND_LR_SIZE", "value"), &MMAnimationLibrary::set_BOUND_LR_SIZE, DEFVAL(64));
				ClassDB::bind_method(D_METHOD("get_BOUND_LR_SIZE"), &MMAnimationLibrary::get_BOUND_LR_SIZE);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "BOUND_LR_SIZE", PROPERTY_HINT_RANGE, "4, 100, 1, or_greater"), "set_BOUND_LR_SIZE", "get_BOUND_LR_SIZE");
				ClassDB::bind_method(D_METHOD("set_BOUND_SM_SIZE", "value"), &MMAnimationLibrary::set_BOUND_SM_SIZE, DEFVAL(16));
				ClassDB::bind_method(D_METHOD("get_BOUND_SM_SIZE"), &MMAnimationLibrary::get_BOUND_SM_SIZE);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "BOUND_SM_SIZE", PROPERTY_HINT_RANGE, "2, 100, 1, or_greater"), "set_BOUND_SM_SIZE", "get_BOUND_SM_SIZE");
				ClassDB::bind_method(D_METHOD("set_curvecost", "value"), &MMAnimationLibrary::set_curvecost);
				ClassDB::bind_method(D_METHOD("get_curvecost"), &MMAnimationLibrary::get_curvecost);
				::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::DICTIONARY, "curvecost", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL), "set_curvecost", "get_curvecost");
			}
			ClassDB::add_property_subgroup(get_class_static(), "", "");
			{
				ClassDB::bind_method(D_METHOD("set_db_anim_index", "value"), &MMAnimationLibrary::set_db_anim_index);
				ClassDB::bind_method(D_METHOD("get_db_anim_index"), &MMAnimationLibrary::get_db_anim_index);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_INT32_ARRAY, "db_anim_index", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_db_anim_index", "get_db_anim_index");
				ClassDB::bind_method(D_METHOD("set_db_anim_timestamp", "value"), &MMAnimationLibrary::set_db_anim_timestamp);
				ClassDB::bind_method(D_METHOD("get_db_anim_timestamp"), &MMAnimationLibrary::get_db_anim_timestamp);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "db_anim_timestamp", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_db_anim_timestamp", "get_db_anim_timestamp");
				ClassDB::bind_method(D_METHOD("set_db_anim_category", "value"), &MMAnimationLibrary::set_db_anim_category);
				ClassDB::bind_method(D_METHOD("get_db_anim_category"), &MMAnimationLibrary::get_db_anim_category);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_INT32_ARRAY, "db_anim_category", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_db_anim_category", "get_db_anim_category");

				ClassDB::bind_method(D_METHOD("set_LR_MAX", "value"), &MMAnimationLibrary::set_LR_MAX);
				ClassDB::bind_method(D_METHOD("get_LR_MAX"), &MMAnimationLibrary::get_LR_MAX);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "LR_MAX", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY), "set_LR_MAX", "get_LR_MAX");
				ClassDB::bind_method(D_METHOD("set_LR_MIN", "value"), &MMAnimationLibrary::set_LR_MIN);
				ClassDB::bind_method(D_METHOD("get_LR_MIN"), &MMAnimationLibrary::get_LR_MIN);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "LR_MIN", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_LR_MIN", "get_LR_MIN");
				ClassDB::bind_method(D_METHOD("set_SM_MAX", "value"), &MMAnimationLibrary::set_SM_MAX);
				ClassDB::bind_method(D_METHOD("get_SM_MAX"), &MMAnimationLibrary::get_SM_MAX);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "SM_MAX", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_SM_MAX", "get_SM_MAX");
				ClassDB::bind_method(D_METHOD("set_SM_MIN", "value"), &MMAnimationLibrary::set_SM_MIN);
				ClassDB::bind_method(D_METHOD("get_SM_MIN"), &MMAnimationLibrary::get_SM_MIN);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "SM_MIN", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_SM_MIN", "get_SM_MIN");

				ClassDB::bind_method(D_METHOD("set_BIAS_LR_MAX", "value"), &MMAnimationLibrary::set_BIAS_LR_MAX);
				ClassDB::bind_method(D_METHOD("get_BIAS_LR_MAX"), &MMAnimationLibrary::get_BIAS_LR_MAX);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "BIAS_LR_MAX", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_BIAS_LR_MAX", "get_BIAS_LR_MAX");
				ClassDB::bind_method(D_METHOD("set_BIAS_LR_MIN", "value"), &MMAnimationLibrary::set_BIAS_LR_MIN);
				ClassDB::bind_method(D_METHOD("get_BIAS_LR_MIN"), &MMAnimationLibrary::get_BIAS_LR_MIN);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "BIAS_LR_MIN", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_BIAS_LR_MIN", "get_BIAS_LR_MIN");
				ClassDB::bind_method(D_METHOD("set_BIAS_SM_MAX", "value"), &MMAnimationLibrary::set_BIAS_SM_MAX);
				ClassDB::bind_method(D_METHOD("get_BIAS_SM_MAX"), &MMAnimationLibrary::get_BIAS_SM_MAX);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "BIAS_SM_MAX", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_BIAS_SM_MAX", "get_BIAS_SM_MAX");
				ClassDB::bind_method(D_METHOD("set_BIAS_SM_MIN", "value"), &MMAnimationLibrary::set_BIAS_SM_MIN);
				ClassDB::bind_method(D_METHOD("get_BIAS_SM_MIN"), &MMAnimationLibrary::get_BIAS_SM_MIN);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "BIAS_SM_MIN", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_BIAS_SM_MIN", "get_BIAS_SM_MIN");

				ClassDB::bind_method(D_METHOD("set_Rng_Start", "value"), &MMAnimationLibrary::set_Rng_Start);
				ClassDB::bind_method(D_METHOD("get_Rng_Start"), &MMAnimationLibrary::get_Rng_Start);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "Rng_Start", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_Rng_Start", "get_Rng_Start");

				ClassDB::bind_method(D_METHOD("set_Rng_Stop", "value"), &MMAnimationLibrary::set_Rng_Stop);
				ClassDB::bind_method(D_METHOD("get_Rng_Stop"), &MMAnimationLibrary::get_Rng_Stop);
				godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "Rng_Stop", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_INTERNAL), "set_Rng_Stop", "get_Rng_Stop");
			}
		}

		ClassDB::bind_method(D_METHOD("prints_dimensions"), &MMAnimationLibrary::prints_dimensions);
		ClassDB::bind_method(D_METHOD("get_stats"), &MMAnimationLibrary::get_stats);
		// Functions
		{
			ClassDB::bind_method(D_METHOD("sample_bone_local", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_local_kform);
			ClassDB::bind_method(D_METHOD("sample_bone_model", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_model_kform);
			ClassDB::bind_method(D_METHOD("sample_bone_rootmotion", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_rootmotion_kform);
			ClassDB::bind_method(D_METHOD("sample_bone_global", "animation_name", "time", "bone_path"), &MMAnimationLibrary::sample_bone_global_kform);

			ClassDB::bind_method(D_METHOD("get_pose_tags", "animation_name", "time"), &MMAnimationLibrary::get_pose_tags);

			ClassDB::bind_method(D_METHOD("bake_data"), &MMAnimationLibrary::bake_data);
			ClassDB::bind_method(D_METHOD("recalculate_weights"), &MMAnimationLibrary::recalculate_weights);
			ClassDB::bind_method(D_METHOD("query_pose_aabb", "serialized_query", "best_index", "ignore_surrounding_indicies", "ranges_search", "custom_weights"), &MMAnimationLibrary::query_pose_aabb, DEFVAL(-1), DEFVAL(20), DEFVAL(nullptr), DEFVAL(PackedFloat32Array{}));
			ClassDB::bind_method(D_METHOD("query_pose_noacceleration", "serialized_query", "best_index", "ignore_surrounding_indicies", "ranges_search", "custom_weights"), &MMAnimationLibrary::query_pose_noacceleration, DEFVAL(-1), DEFVAL(20), DEFVAL(nullptr), DEFVAL(PackedFloat32Array{}));

			// SetRangeIndex
			ClassDB::bind_method(D_METHOD("get_indicies_of_animations"), &MMAnimationLibrary::get_indicies_of_animations);
			ClassDB::bind_method(D_METHOD("get_indicies_of_category", "mask"), &MMAnimationLibrary::get_indicies_of_category);
		}
		// Internal functions
		{
			ClassDB::bind_method(D_METHOD("get_curvecost_animname", "anim"), &MMAnimationLibrary::get_curvecost_animname);
			ClassDB::bind_method(D_METHOD("_event_on_anim_added", "anim"), &MMAnimationLibrary::_event_on_anim_added);
			ClassDB::bind_method(D_METHOD("_event_on_anim_removed", "anim"), &MMAnimationLibrary::_event_on_anim_removed);
		}
	}

public:
};

struct QueryOptions : public godot::RefCounted {
	GDCLASS(QueryOptions, RefCounted)
	using u = godot::UtilityFunctions;

public:
	GETSET(PackedFloat32Array, query);
	GETSET(PackedFloat32Array, custom_weights);
	GETSET(PackedFloat32Array, custom_ranges);
	GETSET(int, ignore_surrounding_frames, 10);
	GETSET(int, best_index, -1);

	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_query", "value"), &QueryOptions::set_query);
		ClassDB::bind_method(D_METHOD("get_query"), &QueryOptions::get_query);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "query"), "set_query", "get_query");
		ClassDB::bind_method(D_METHOD("set_custom_weights", "value"), &QueryOptions::set_custom_weights);
		ClassDB::bind_method(D_METHOD("get_custom_weights"), &QueryOptions::get_custom_weights);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "custom_weights"), "set_custom_weights", "get_custom_weights");
		ClassDB::bind_method(D_METHOD("set_custom_weights", "value"), &QueryOptions::set_custom_weights);
		ClassDB::bind_method(D_METHOD("get_custom_weights"), &QueryOptions::get_custom_weights);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "custom_weights"), "set_custom_weights", "get_custom_weights");

		ClassDB::bind_method(D_METHOD("set_best_index", "value"), &QueryOptions::set_best_index);
		ClassDB::bind_method(D_METHOD("get_best_index"), &QueryOptions::get_best_index);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "best_index"), "set_best_index", "get_best_index");
		ClassDB::bind_method(D_METHOD("set_ignore_surrounding_frames", "value"), &QueryOptions::set_ignore_surrounding_frames);
		ClassDB::bind_method(D_METHOD("get_ignore_surrounding_frames"), &QueryOptions::get_ignore_surrounding_frames);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "ignore_surrounding_frames"), "set_ignore_surrounding_frames", "get_ignore_surrounding_frames");
	}
};
