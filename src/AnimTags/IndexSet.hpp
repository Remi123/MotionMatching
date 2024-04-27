#pragma once
#include <algorithm>
#include <bitset>
#include <format>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

auto range_less = [](auto lhs, auto rhs) {
	if (lhs.begin() != rhs.begin())
		return lhs.begin() < rhs.begin();
	else
		return lhs.end() < rhs.end();
};

template <typename R>
void spans_simplify(R &r, bool skip_sorting = true) {
	using range_type = R;
	using span_type = R::value_type;
	if (!skip_sorting)
		std::ranges::sort(r, range_less);
	size_t i = 0;
	span_type _c = r[0];
	for (auto &_r : r) {
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

template <typename R>
void spans_difference(R &out, const R &lhs, const R &rhs) {
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

template <typename R>
void spans_intersection(R &out, const R &lhs, const R &rhs) {
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

template <typename R, typename R1, typename R2>
void spans_union(R &out, const R1 &lhs, const R2 &rhs) {
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
			: public std::iterator<std::input_iterator_tag, // iterator_category
					  long, // value_type
					  long, // difference_type
					  const long *, // pointer
					  long // reference
					  > {
		friend class IndexRange;
		size_t num = 0;
		size_t FROM = 0;
		size_t TO = 0;

	public:
		iterator() :
				FROM{ 0 }, TO{ 0 } {}
		iterator(const iterator &) = default;
		explicit iterator(long _num, size_t from, size_t to) :
				num(_num), FROM{ from }, TO{ to } {}
		iterator &operator++() {
			num = TO >= FROM ? num + 1 : num - 1;
			return *this;
		}
		iterator operator++(int) {
			iterator retval = *this;
			++(*this);
			return retval;
		}
		iterator &operator=(iterator &other) = default;
		iterator &operator=(const iterator &other) = default;

		bool operator==(iterator other) const { return num == other.num; }
		bool operator<=(iterator other) const { return num <= other.num; }
		bool operator<(iterator other) const { return num < other.num; }
		bool operator>=(iterator other) const { return num >= other.num; }
		bool operator>(iterator other) const { return num > other.num; }
		bool operator!=(iterator other) const { return !(*this == other); }
		reference operator*() const { return num; }
	};
	IndexRange() = default;
	IndexRange(size_t from, size_t to) :
			FROM{ from }, TO{ to } {}
	IndexRange(size_t to) :
			FROM{ 0 }, TO{ to } {}
	IndexRange(iterator from, iterator to) :
			FROM{ from.num }, TO{ to.num } {}
	IndexRange(const IndexRange &other) = default;
	IndexRange &operator=(const IndexRange &other) = default;
	iterator begin() { return iterator(FROM, FROM, TO); }
	iterator end() { return iterator(TO, FROM, TO); }
	iterator begin() const { return iterator(FROM, FROM, TO); }
	iterator end() const { return iterator(TO, FROM, TO); }
	size_t front() { return FROM; }
	size_t back() { return TO; }
	size_t size() { return TO - FROM; }

	IndexRange subspan(size_t offset, size_t size) const {
		return IndexRange(offset, offset + size);
	}
	IndexRange subrange(size_t start, size_t end) const {
		return IndexRange(start, end);
	}
};

#include <assert.h>

class IndexSet {
	IndexRange _lhs;
	std::vector<IndexRange> _subviews;

public:
	using value_type = IndexRange;
	IndexSet() = default;
	IndexSet(const size_t start, const size_t end) :
			_lhs{ start, end }, _subviews{ IndexRange{ start, end } } {
	}
	IndexSet(const IndexRange _v,
			const std::initializer_list<IndexRange> &_sub = {}) :
			_lhs{ _v }, _subviews{ _sub } {}
	IndexSet(const IndexSet &r) = default;

	const IndexRange subspan(size_t offset, size_t count) const {
		return _lhs.subspan(offset, count);
	}
	const IndexRange subrange(size_t index_begin, size_t index_end) const {
		assert(index_begin <= index_end);
		return _lhs.subrange(index_begin, index_end);
	}

	void operator|=(const IndexRange &_rhs) {
		assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
		spans_union(_subviews, std::vector<IndexRange>{ _subviews }, std::vector<IndexRange>{ _rhs });
	}
	void operator|=(const IndexSet &_rhs) {
		spans_union(_subviews, std::vector<IndexRange>{ _subviews },
				_rhs._subviews);
	}
	IndexSet operator||(const IndexSet &_rhs) const {
		IndexSet result{ _lhs };
		spans_union(result._subviews, _subviews, _rhs._subviews);
		return result;
	}

	void operator-=(const IndexRange &_rhs) {
		assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
		spans_difference(_subviews, std::vector<IndexRange>{ _subviews }, { _rhs });
	}
	void operator-=(const IndexSet &_rhs) {
		// assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
		spans_difference(_subviews, std::vector<IndexRange>{ _subviews },
				_rhs._subviews);
	}
	IndexSet operator-(const IndexSet &_rhs) const {
		IndexSet result{ _lhs };
		spans_difference(result._subviews, _subviews, _rhs._subviews);
		return result;
	}

	void operator&=(const IndexRange &_rhs) {
		assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
		spans_intersection(_subviews, std::vector<IndexRange>{ _subviews },
				{ _rhs });
	}
	IndexSet operator&&(const IndexRange &_rhs) const {
		assert(_lhs.begin() <= _rhs.begin() && _rhs.end() <= _lhs.end());
		IndexSet result = *this;
		spans_intersection(result._subviews, std::vector<IndexRange>{ _subviews },
				{ _rhs });
		return result;
	}

	IndexSet operator&&(const IndexSet &_rhs) const {
		IndexSet result{ _lhs };
		spans_intersection(result._subviews, _subviews, _rhs._subviews);
		return result;
	}
	void operator&=(const IndexSet &_rhs) {
		spans_intersection(_subviews, std::vector<IndexRange>{ _subviews },
				_rhs._subviews);
	}

	IndexRange &operator[](const std::size_t index) { return _subviews[index]; }
	const IndexRange &operator[](const std::size_t index) const {
		return _subviews[index];
	}

	auto begin() { return _subviews.begin(); }
	auto end() { return _subviews.end(); }
	auto cbegin() { return _subviews.cbegin(); }
	auto cend() { return _subviews.cend(); }

	using iter_t = decltype(_subviews)::iterator;
	iter_t iterator;
	typename IndexRange::iterator sub_iterator;
	bool _iter_init(Variant arg) {
		iterator = _subviews.begin();
		sub_iterator = iterator->begin();
		return iterator != _subviews.end() && sub_iterator != iterator->end();
	}
	bool _iter_next(Variant arg) {
		if (iterator == _subviews.end())
			return false;
		++sub_iterator;
		if (sub_iterator == iterator->end()) {
			++iterator;
			sub_iterator = iterator->begin();
		}
		return iterator != _subviews.end();
		// return iterator != buffer.end();
	}
	Variant _iter_get(Variant arg) {
		return (int)*sub_iterator;
	}

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

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>

struct RangeIndex : godot::RefCounted {
public:
	GDCLASS(RangeIndex, RefCounted);

public:
	void _init(int _f, int _t) {
		from = _f;
		to = _t;
	}
	GETSET(int, from, 0);
	GETSET(int, to, 1);

	int begin() { return from; }
	int end() { return to; }

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("_init", "from", "to"), &RangeIndex::_init);

		ClassDB::bind_method(D_METHOD("set_from", "value"), &RangeIndex::set_from);
		ClassDB::bind_method(D_METHOD("get_from"), &RangeIndex::get_from);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "from"), "set_from", "get_from");

		ClassDB::bind_method(D_METHOD("set_to", "value"), &RangeIndex::set_to);
		ClassDB::bind_method(D_METHOD("get_to"), &RangeIndex::get_to);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "to"), "set_to", "get_to");
	}
};

struct SetRangeIndex : godot::RefCounted {
public:
	GDCLASS(SetRangeIndex, RefCounted)
public:
	GETSET(TypedArray<RangeIndex>, ranges);

	void AddRange(int from, int to) {
		if (ranges.size() == 0) {
			Ref<RangeIndex> _r{};
			_r.instantiate();
			_r->from = from;
			_r->to = to;
			ranges.append(_r);
			return;
		}

		Ref<SetRangeIndex> _tmp{};
		_tmp.instantiate();
		Ref<RangeIndex> _r{};
		_r.instantiate();
		_r->from = from;
		_r->to = to;
		_tmp->ranges.append(_r);
		Union(_tmp);
	}

	void Union(Ref<SetRangeIndex> other) {
		std::vector<IndexRange> _tmp_lhs{}, _tmp_rhs{}, _tmp_out{};

		for (size_t i = 0; i < ranges.size(); ++i) {
			auto _rg = *cast_to<RangeIndex>(ranges[i]);
			_tmp_lhs.push_back({ (size_t)_rg.from, (size_t)_rg.to });
		}
		for (size_t i = 0; i < other->ranges.size(); ++i) {
			auto _rg = *cast_to<RangeIndex>(other->ranges[i]);
			_tmp_rhs.push_back({ (size_t)_rg.from, (size_t)_rg.to });
		}

		spans_union(_tmp_out, _tmp_lhs, _tmp_rhs);

		ranges.clear();
		for (size_t i = 0; i < _tmp_out.size(); ++i) {
			Ref<RangeIndex> _ri{};
			_ri.instantiate();
			_ri->from = _tmp_out[i].front();
			_ri->to = _tmp_out[i].back();
			ranges.append(_ri);
		}
	}
	void Difference(Ref<SetRangeIndex> other) {
		std::vector<IndexRange> _tmp_lhs{}, _tmp_rhs{}, _tmp_out{};

        u::prints("Diff0",ranges.size(),other->ranges.size());
		for (size_t i = 0; i < ranges.size(); ++i) {
			auto _rg = *cast_to<RangeIndex>(ranges[i]);
			_tmp_lhs.push_back({ (size_t)_rg.from, (size_t)_rg.to });
		}
        u::prints("Diff",_tmp_lhs.size(),_tmp_rhs.size());
		for (size_t i = 0; i < other->ranges.size(); ++i) {
			auto _rg = *cast_to<RangeIndex>(other->ranges[i]);
			_tmp_rhs.push_back({ (size_t)_rg.from, (size_t)_rg.to });
		}
        u::prints("Diff",_tmp_lhs.size(),_tmp_rhs.size());
		spans_difference(_tmp_out, _tmp_lhs, _tmp_rhs);
        u::prints("Diff",_tmp_out.size());

		ranges.clear();
		for (size_t i = 0; i < _tmp_out.size(); ++i) {
			Ref<RangeIndex> _ri{};
			_ri.instantiate();
			_ri->from = _tmp_out[i].front();
			_ri->to = _tmp_out[i].back();
			ranges.append(_ri);
		}
	}
	void Intersection(Ref<SetRangeIndex> other) {
		std::vector<IndexRange> _tmp_lhs{}, _tmp_rhs{}, _tmp_out{};

		for (size_t i = 0; i < ranges.size(); ++i) {
			auto _rg = *cast_to<RangeIndex>(ranges[i]);
			_tmp_lhs.push_back({ (size_t)_rg.from, (size_t)_rg.to });
		}
		for (size_t i = 0; i < other->ranges.size(); ++i) {
			auto _rg = *cast_to<RangeIndex>(other->ranges[i]);
			_tmp_rhs.push_back({ (size_t)_rg.from, (size_t)_rg.to });
		}

		spans_intersection(_tmp_out, _tmp_lhs, _tmp_rhs);

		ranges.clear();
		for (size_t i = 0; i < _tmp_out.size(); ++i) {
			Ref<RangeIndex> _ri{};
			_ri.instantiate();
			_ri->from = _tmp_out[i].front();
			_ri->to = _tmp_out[i].back();
			ranges.append(_ri);
		}
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_ranges", "value"), &SetRangeIndex::set_ranges);
		ClassDB::bind_method(D_METHOD("get_ranges"), &SetRangeIndex::get_ranges);
		::godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "ranges", godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":RangeIndex", PROPERTY_USAGE_DEFAULT), "set_ranges", "get_ranges");

		ClassDB::bind_method(D_METHOD("AddRange", "from", "to"), &SetRangeIndex::AddRange);

		ClassDB::bind_method(D_METHOD("Union", "other"), &SetRangeIndex::Union);

		ClassDB::bind_method(D_METHOD("Difference", "other"), &SetRangeIndex::Difference);

		ClassDB::bind_method(D_METHOD("Intersection", "other"), &SetRangeIndex::Intersection);
	}
};