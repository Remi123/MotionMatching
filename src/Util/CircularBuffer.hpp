#pragma once

#include <boost/circular_buffer.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>


using namespace godot;

struct CircularBuffer : public godot::RefCounted {
	GDCLASS(CircularBuffer, RefCounted);

public:
	using capacity_t = typename boost::circular_buffer_space_optimized<godot::Variant>::capacity_type;
	using value_type = typename boost::circular_buffer_space_optimized<godot::Variant>::value_type;

	void set_capacity(int i) {
		if (i < 0)
			return;
		buffer.resize(i);
	}
	int get_capacity() {
		return buffer.capacity().capacity();
	}

	void push_back(Variant v) {
		buffer.push_back(v);
	}
	void push_front(Variant v) {
		buffer.push_front(v);
	}
	void pop_back(Variant v) {
		buffer.push_back(v);
	}
	void pop_front(Variant v) {
		buffer.push_front(v);
	}

	void insert(int index, Variant v) {
		if (index > 0)
			buffer.insert(std::next(buffer.begin(), index), v);
	}
	void erase(int index) {
		if (index > 0)
			buffer.erase(std::next(buffer.begin(), index));
	}
	void clear() {
		buffer.clear();
	}

	Variant get(int index) {
		return buffer[godot::Math::posmod(index, buffer.size())];
	}

	using iter_t = typename boost::circular_buffer_space_optimized<godot::Variant>::iterator;
	iter_t iterator;
	bool _iter_init(Variant arg) {
		iterator = buffer.begin();
		return iterator != buffer.end();
	}
	bool _iter_next(Variant arg) {
		++iterator;
		return iterator != buffer.end();
	}
	Variant _iter_get(Variant arg) {
		return *iterator;
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_capacity", "new_capacity"), &CircularBuffer::set_capacity, DEFVAL(1));
		ClassDB::bind_method(D_METHOD("get_capacity"), &CircularBuffer::get_capacity);
		ClassDB::add_property("CircularBuffer", PropertyInfo(Variant::INT, "capacity", PROPERTY_HINT_RANGE, "1,100,or_greater"), "set_capacity", "get_capacity");

		ClassDB::bind_method(D_METHOD("push_back", "value"), &CircularBuffer::push_back);
		ClassDB::bind_method(D_METHOD("push_front", "value"), &CircularBuffer::push_front);
		ClassDB::bind_method(D_METHOD("pop_back"), &CircularBuffer::pop_back);
		ClassDB::bind_method(D_METHOD("pop_front"), &CircularBuffer::pop_front);

		ClassDB::bind_method(D_METHOD("insert", "index", "value"), &CircularBuffer::insert);
		ClassDB::bind_method(D_METHOD("erase", "index"), &CircularBuffer::erase);
		ClassDB::bind_method(D_METHOD("clear"), &CircularBuffer::clear);

		ClassDB::bind_method(D_METHOD("get", "index"), &CircularBuffer::get);

		ClassDB::bind_method(D_METHOD("_iter_init", "arg"), &CircularBuffer::_iter_init);
		ClassDB::bind_method(D_METHOD("_iter_next", "arg"), &CircularBuffer::_iter_next);
		ClassDB::bind_method(D_METHOD("_iter_get", "arg"), &CircularBuffer::_iter_get);
	}
	boost::circular_buffer_space_optimized<godot::Variant> buffer{ capacity_t{ 1, 1 } };
};