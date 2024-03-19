#pragma once
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <bitset>
#include <ranges>
#include <span>
#include <unordered_map>


using namespace godot;
using u = godot::UtilityFunctions;

using Range = Vector2i;

auto range_less = [](auto lhs, auto rhs) {
    if (lhs.begin() != rhs.begin())
        return lhs.begin() < rhs.begin();
    else
        return lhs.end() < rhs.end();
};

template <typename R>
void spans_simplify(R& r, bool skip_sorting = true) {
    using range_type = R;
    using span_type = R::value_type;
    if (!skip_sorting) std::ranges::sort(r, range_less);
    size_t i = 0;
    span_type _c = r[0];
    for (auto& _r : r) {
        if (_c.end() < _r.begin()) {
            r[i++] = _c;
            _c = _r;
        } else {
            _c = span_type(std::min(_c.begin(), _r.begin()),
                           std::max(_c.end(), _r.end()));
        }
    }
    r[i++] = _c;
    r.resize(i);
}

template<typename R>
void spans_difference(R& out, const R& lhs,
                      const R& rhs) {
    using span_type = R::value_type;
    out.resize(lhs.size() + rhs.size());
    // Activation state of each list of ranges
    bool out_active = false;
    bool lhs_active = false;
    bool rhs_active = false;

    // Event index for each list of ranges
    int out_i = 0;
    int lhs_i = 0;
    int rhs_i = 0;

    // While both ranges have events to process
    while (lhs_i < lhs.size() * 2 && rhs_i < rhs.size() * 2) {
        // Are the next lhs, and rhs events active or inactive
        bool lhs_active_next = lhs_i % 2 == 0;
        bool rhs_active_next = rhs_i % 2 == 0;

        // Time of the next lhs, and rhs events
        auto lhs_t =
            lhs_active_next ? lhs[lhs_i / 2].begin() : lhs[lhs_i / 2].end();
        auto rhs_t =
            rhs_active_next ? rhs[rhs_i / 2].begin() : rhs[rhs_i / 2].end();

        // Event coming from lhs first
        if (lhs_t < rhs_t) {
            // Activate output
            if (!out_active && !rhs_active && lhs_active_next) {
                out_active = true;
                out[out_i] = span_type(lhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && !lhs_active_next) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), lhs_t);
                out_i++;
            }

            lhs_active = lhs_active_next;
            lhs_i++;
        }
        // Event coming from rhs first
        else if (rhs_t < lhs_t) {
            // Activate output
            if (!out_active && lhs_active && !rhs_active_next) {
                out_active = true;
                out[out_i] = span_type(rhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && rhs_active_next) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), rhs_t);
                out_i++;
            }

            rhs_active = rhs_active_next;
            rhs_i++;
        }
        // Event from lhs and rhs coming at same time
        else {
            // Activate output
            if (!out_active && lhs_active_next && !rhs_active_next) {
                out_active = true;
                out[out_i] = span_type(lhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && rhs_active_next) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), lhs_t);
                out_i++;
            }

            lhs_active = lhs_active_next;
            rhs_active = rhs_active_next;
            lhs_i++;
            rhs_i++;
        }
    }

    // Process any remaining lhs events
    while (lhs_i < lhs.size() * 2) {
        bool lhs_active_next = lhs_i % 2 == 0;
        auto lhs_t =
            lhs_active_next ? lhs[lhs_i / 2].begin() : lhs[lhs_i / 2].end();

        // Activate output
        if (!out_active && lhs_active_next) {
            out_active = true;
            out[out_i] = span_type(lhs_t, out[out_i].end());
        }
        // Deactivate output
        else if (out_active && !lhs_active_next) {
            out_active = false;
            out[out_i] = span_type(out[out_i].begin(), lhs_t);
            out_i++;
        }

        lhs_active = lhs_active_next;
        lhs_i++;
    }
    out.resize(out_i);
}

template< typename R>
void spans_intersection(R& out, const R& lhs,
                        const R& rhs) {
    using span_type = R::value_type;
    out.resize(lhs.size() + rhs.size());
    // Activation state of each list of ranges
    bool out_active = false;
    bool lhs_active = false;
    bool rhs_active = false;

    // Event index for each list of ranges
    int out_i = 0;
    int lhs_i = 0;
    int rhs_i = 0;

    // While both ranges have events to process
    while (lhs_i < lhs.size() * 2 && rhs_i < rhs.size() * 2) {
        // Are the next lhs, and rhs events active or inactive
        bool lhs_active_next = lhs_i % 2 == 0;
        bool rhs_active_next = rhs_i % 2 == 0;

        // Time of the next lhs, and rhs events
        auto lhs_t =
            lhs_active_next ? lhs[lhs_i / 2].begin() : lhs[lhs_i / 2].end();
        auto rhs_t =
            rhs_active_next ? rhs[rhs_i / 2].begin() : rhs[rhs_i / 2].end();

        // Event from lhs coming first
        if (lhs_t < rhs_t) {
            // Activate output
            if (!out_active && rhs_active && lhs_active_next) {
                out_active = true;
                out[out_i] = span_type(lhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && !lhs_active_next) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), lhs_t);
                out_i++;
            }

            lhs_active = lhs_active_next;
            lhs_i++;
        }
        // Event from rhs coming first
        else if (rhs_t < lhs_t) {
            // Activate output
            if (!out_active && lhs_active && rhs_active_next) {
                out_active = true;
                out[out_i] = span_type(rhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && !rhs_active_next) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), rhs_t);
                out_i++;
            }

            rhs_active = rhs_active_next;
            rhs_i++;
        }
        // Event from lhs and rhs coming at same time
        else {
            // Activate output
            if (!out_active && (lhs_active_next && rhs_active_next)) {
                out_active = true;
                out[out_i] = span_type(lhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && (!lhs_active_next || !rhs_active_next)) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), lhs_t);
                out_i++;
            }

            lhs_active = lhs_active_next;
            rhs_active = rhs_active_next;
            lhs_i++;
            rhs_i++;
        }
    }
    out.resize(out_i);
}

template< typename R>
void spans_union(R& out, const R& lhs,
                        const R& rhs) {
    using span_type = R::value_type;
    out.resize(lhs.size() + rhs.size());

    bool out_active = false;
    bool lhs_active = false;
    bool rhs_active = false;

    // Event index for each list of ranges
    int out_i = 0;
    int lhs_i = 0;
    int rhs_i = 0;

    // While both ranges have events to process
    while (lhs_i < lhs.size() * 2 && rhs_i < rhs.size() * 2) {
        // Are the next lhs, and rhs events active or inactive
        bool lhs_active_next = lhs_i % 2 == 0;
        bool rhs_active_next = rhs_i % 2 == 0;

        // Time of the next lhs, and rhs events
        auto lhs_t =
            lhs_active_next ? lhs[lhs_i / 2].begin() : lhs[lhs_i / 2].end();
        auto rhs_t =
            rhs_active_next ? rhs[rhs_i / 2].begin() : rhs[rhs_i / 2].end();

        // Event from lhs is coming first
        if (lhs_t < rhs_t) {
            // Activate output
            if (!out_active && lhs_active_next) {
                out_active = true;
                out[out_i] = span_type(lhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && !lhs_active_next && !rhs_active) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), lhs_t);
                out_i++;
            }

            lhs_active = lhs_active_next;
            lhs_i++;
        }
        // Event from rhs is coming first
        else if (rhs_t < lhs_t) {
            // Activate output
            if (!out_active && rhs_active_next) {
                out_active = true;
                out[out_i] = span_type(rhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && !lhs_active && !rhs_active_next) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), rhs_t);
                out_i++;
            }

            rhs_active = rhs_active_next;
            rhs_i++;
        }
        // Event from lhs and rhs coming at same time
        else {
            // Activate output
            if (!out_active && (lhs_active_next || rhs_active_next)) {
                out_active = true;
                out[out_i] = span_type(lhs_t, out[out_i].end());
            }
            // Deactivate output
            else if (out_active && !(lhs_active_next || rhs_active_next)) {
                out_active = false;
                out[out_i] = span_type(out[out_i].begin(), lhs_t);
                out_i++;
            }

            lhs_active = lhs_active_next;
            rhs_active = rhs_active_next;
            lhs_i++;
            rhs_i++;
        }
    }

    // Process any remaining lhs events
    while (lhs_i < lhs.size() * 2) {
        bool lhs_active_next = lhs_i % 2 == 0;
        auto lhs_t =
            lhs_active_next ? lhs[lhs_i / 2].begin() : lhs[lhs_i / 2].end();

        // Activate output
        if (!out_active && lhs_active_next) {
            out_active = true;
            out[out_i] = span_type(lhs_t, out[out_i].end());
        }
        // Deactivate output
        else if (out_active && !lhs_active_next) {
            out_active = false;
            out[out_i] = span_type(out[out_i].begin(), lhs_t);
            out_i++;
        }

        lhs_active = lhs_active_next;
        lhs_i++;
    }

    // Process any remaining rhs events
    while (rhs_i < rhs.size() * 2) {
        bool rhs_active_next = rhs_i % 2 == 0;
        auto rhs_t =
            rhs_active_next ? rhs[rhs_i / 2].begin() : rhs[rhs_i / 2].end();

        // Activate output
        if (!out_active && rhs_active_next) {
            out_active = true;
            out[out_i] = span_type(rhs_t, out[out_i].end());
        }
        // Deactivate output
        else if (out_active && !rhs_active_next) {
            out_active = false;
            out[out_i] = span_type(out[out_i].begin(), rhs_t);
            out_i++;
        }

        rhs_active = rhs_active_next;
        rhs_i++;
    }
    out.resize(out_i);
}

template<typename R>
void masks_vectorize(R& r){

}

struct RangeSet : Resource {
	GDCLASS(RangeSet, Resource);
	PackedInt32Array anims{};
	TypedArray<Range> anims_submasks{};
	TypedArray<Range> ranges{};

public:
	friend RangeSet Union(const RangeSet lhs, const RangeSet rhs) {
		RangeSet out{};

		return out;
	}

protected:
	static void _bind_methods() {
	}
};

struct MaskSet : Resource {
	GDCLASS(MaskSet, Resource);

	PackedInt32Array anims{};
	TypedArray<Range> anims_submasks{};
	PackedInt32Array masks{};

	void Rasterize(const PackedInt32Array &sorted_animations_index, const PackedInt32Array &mask_index) {
		ERR_FAIL_COND(sorted_animations_index.size() != mask_index.size());
		Clear();
		// find number of anims
		const std::span<const int32_t> sorted_anim_span(sorted_animations_index.ptr(), sorted_animations_index.size());
		const size_t anim_size = sorted_anim_span.back();
		anims.resize(anim_size);
		anims_submasks.resize(anim_size);
		// Assign submasks. Must be correctly sorted;
		size_t offset = 0;
		for (int i : std::views::iota(0) | std::views::take(anim_size)) {
			anims[i] = i;
			size_t size = std::ranges::count(sorted_anim_span, i);
			if (size == 0) {
				anims_submasks[i] = Range{ -1, -1 };
				continue;
			}
			anims_submasks[i] = Range{ offset, offset + size };
			offset += size;
		}
		//
		masks = mask_index;
	}
	PackedVector2Array Vectorize() {
		PackedVector2Array out{};
		return out;
	}

	void Clear() {
		anims.clear();
		anims_submasks.clear();
		masks.clear();
	}

	
	friend MaskSet Intersection(const MaskSet lhs, const MaskSet rhs) { return {}; }
	friend MaskSet Difference(const MaskSet lhs, const MaskSet rhs) { return {}; }

public:
protected:
	static void _bind_methods() {
	}
};
