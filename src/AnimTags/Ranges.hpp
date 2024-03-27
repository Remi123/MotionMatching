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
#include <iterator>

class IndexRange {
   public:
    using value_type = IndexRange;
    size_t FROM = 0;
    size_t TO = 0;
    // member typedefs provided through inheriting from std::iterator
    class iterator
        : public std::iterator<std::input_iterator_tag,  // iterator_category
                               long,                     // value_type
                               long,                     // difference_type
                               const long*,              // pointer
                               long                      // reference
                               > {
        friend class IndexRange;
        size_t num = 0;
        const size_t FROM = 0;
        const size_t TO = 0;

       public:
        explicit iterator(long _num, size_t from, size_t to)
            : num(_num), FROM{from}, TO{to} {}
        iterator& operator++() {
            num = TO >= FROM ? num + 1 : num - 1;
            return *this;
        }
        iterator operator++(int) {
            iterator retval = *this;
            ++(*this);
            return retval;
        }
        iterator& operator=(const iterator& other) = default;
        bool operator==(iterator other) const { return num == other.num; }
        bool operator<=(iterator other) const { return num <= other.num; }
        bool operator<(iterator other) const { return num < other.num; }
        bool operator>=(iterator other) const { return num >= other.num; }
        bool operator>(iterator other) const { return num > other.num; }
        bool operator!=(iterator other) const { return !(*this == other); }
        reference operator*() const { return num; }
    };
    IndexRange(size_t from, size_t to) : FROM{from}, TO{to} {}
    IndexRange(size_t to) : FROM{0}, TO{to} {}
    IndexRange(iterator from, iterator to) : FROM{from.num}, TO{to.num} {}
    iterator begin() { return iterator(FROM, FROM, TO); }
    iterator end() { return iterator(TO, FROM, TO); }
    iterator begin() const { return iterator(FROM, FROM, TO); }
    iterator end() const { return iterator(TO, FROM, TO); }
    size_t front() { return FROM; }
    size_t back() { return TO; }
    size_t size(){return TO - FROM;}

    IndexRange subspan(size_t offset, size_t size) const {
        return IndexRange(offset, offset + size);
    }
    IndexRange subrange(size_t start, size_t end) const {
        return IndexRange(start, end);
    }
    protected:
    static void _bind_methods(){

    }
};

#include <assert.h>

class IndexSet {
    using list_type = Vector<IndexRange>;
    using value_type = IndexRange;
    IndexRange _lhs;
    list_type _subviews;

   public:
    using value_type = IndexRange;
    IndexSet(const size_t start, const size_t end): _lhs{start,end},_subviews{}{

    }
    IndexSet(const IndexRange _v,
             const std::initializer_list<IndexRange>& _sub = {})
        : _lhs{_v}, _subviews{_sub} {}
    IndexSet(const IndexSet& r) = default;

    const IndexRange subspan(size_t offset, size_t count) const {
        return _lhs.subspan(offset, count);
    }
    const IndexRange subrange(size_t index_begin, size_t index_end) const {
        assert(index_begin <= index_end);
        return _lhs.subrange(index_begin, index_end);
    }

    void operator|=(const IndexRange& _rhs) {
        assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
        spans_union(_subviews, list_type{_subviews}, list_type{_rhs});
    }
    void operator|=(const IndexSet& _rhs) {
        spans_union(_subviews, list_type{_subviews},
                    _rhs._subviews);
    }
    IndexSet operator||(const IndexSet& _rhs) const {
        IndexSet result{_lhs};
        spans_union(result._subviews, _subviews, _rhs._subviews);
        return result;
    }

    void operator-=(const IndexRange& _rhs) {
        assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
        spans_difference(_subviews, list_type{_subviews}, {_rhs});
    }
    void operator-=(const IndexSet& _rhs) {
        // assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
        spans_difference(_subviews, list_type{_subviews},
                         _rhs._subviews);
    }
    IndexSet operator-(const IndexSet& _rhs) const {
        IndexSet result{_lhs};
        spans_difference(result._subviews, _subviews, _rhs._subviews);
        return result;
    }

    void operator&=(const IndexRange& _rhs) {
        assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
        spans_intersection(_subviews, list_type{_subviews},
                           {_rhs});
    }
    IndexSet operator&&(const IndexRange& _rhs) const {
        assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
        IndexSet result = *this;
        spans_intersection(result._subviews, list_type{_subviews},
                           {_rhs});
        return result;
    }
    // void operator&=(const std::vector<IndexRange>& _rhs) {
    //     assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
    //     spans_intersection(_subviews, std::vector<IndexRange>{_subviews},
    //                        _rhs._subviews);
    // }
    IndexSet operator&&(const IndexSet& _rhs) const {
        IndexSet result{_lhs};
        spans_intersection(result._subviews, _subviews, _rhs._subviews);
        return result;
    }
    void operator&=(const IndexSet& _rhs) {
        spans_intersection(_subviews, list_type{_subviews},
                           _rhs._subviews);
    }

    // IndexRange operator[](const std::size_t index) { return _subviews.; }
    const IndexRange& operator[](const std::size_t index) const {
        return _subviews[index];
    }

    auto begin() { return _subviews.begin(); }
    auto end() { return _subviews.end(); }
    list_type::ConstIterator cbegin(){return _subviews.ptr();}
    list_type::ConstIterator cend(){return _subviews.ptr() + _subviews.size();}


    // IndexSet RasterizeBit(size_t _mask) {

    //     IndexSet out{_lhs};
    //     out._subviews.clear();

    //     bool out_active = false;
    //     int out_i = 0;
    //     int start = 0;

    //     for (auto i :
    //          std::views::iota(0) | std::ranges::views::take(_lhs.size())) {
    //         std::bitset<sizeof(T)> bit = _lhs[i];
    //         // Activate output
    //         if (!out_active && bit.test(_mask)) {
    //             start = i;
    //             out_active = true;
    //         }
    //         // Deactivate output
    //         else if (out_active && !bit.test(_mask)) {
    //             out._subviews.push_back(_lhs.subspan(start, i - start));
    //             out_active = false;
    //             out_i++;
    //         }
    //     }
    //     if (out_active) {
    //         out._subviews.push_back(_lhs.subspan(start, _lhs.size() -
    //         start)); out_i++;
    //     }
    //     return out;
    // }

    // span_view<T> RasterizeValue(const T& _mask) {
    //     span_view<T> out{_lhs};
    //     out._subviews.clear();

    //     bool out_active = false;
    //     int out_i = 0;
    //     int start = 0;

    //     for (auto i :
    //          std::views::iota(0) | std::ranges::views::take(_lhs.size())) {
    //         auto& bit = _lhs[i];
    //         // Activate output
    //         if (!out_active && bit == _mask) {
    //             start = i;
    //             out_active = true;
    //         }
    //         // Deactivate output
    //         else if (out_active && bit != _mask) {
    //             out._subviews.push_back(_lhs.subspan(start, i - start));
    //             out_active = false;
    //             out_i++;
    //         }
    //     }
    //     if (out_active) {
    //         out._subviews.push_back(_lhs.subspan(start, _lhs.size() -
    //         start)); out_i++;
    //     }
    //     return out;
    // }
};