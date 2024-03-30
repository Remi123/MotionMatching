#pragma once
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

using u = godot::UtilityFunctions;

struct BaseVirtual : godot::Resource {
	GDCLASS(BaseVirtual, Resource)
public:
	GDVIRTUAL0R(int, get_int);
	GDVIRTUAL1(print_msg, String);

protected:
	static void _bind_methods() {
		GDVIRTUAL_BIND(get_int)
		GDVIRTUAL_BIND(print_msg, "msg")
	}
};

struct CppVirtual : BaseVirtual {
	GDCLASS(CppVirtual, BaseVirtual)

	int cpp_get_int() {
		return 42;
	}
	void cpp_print_msg(String msg) {
		u::prints("Cpp", msg);
	}

	void test_virtual_call() {
		int x = 0;
		GDVIRTUAL_CALL(get_int, x);
		u::prints("Cpp virtual", x);
		GDVIRTUAL_CALL(print_msg, "Hello");
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("get_int"), &CppVirtual::cpp_get_int); //Attempt to overwrite in binding
		ClassDB::bind_method(D_METHOD("print_msg", "msg"), &CppVirtual::cpp_print_msg); //Attempt to overwrite in binding

		ClassDB::bind_method(D_METHOD("test_virtual_call"), &CppVirtual::test_virtual_call);
	}
};

#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }

struct VirtualContainer : godot::Resource {
	GDCLASS(VirtualContainer, Resource)
public:
	GETSET(TypedArray<BaseVirtual>, motion_features);

	void test_cpp_call() {
        for(auto i = 0; i < motion_features.size();++i)
        {
            u::prints("Casting to BaseVirtual");
            BaseVirtual* f = Object::cast_to<BaseVirtual>(motion_features[i]);
            f->call("print_msg","Hello");
            int value = 0;
            value = f->call("get_int");
            u::prints(i,value);
        }
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("test_cpp_call"), &VirtualContainer::test_cpp_call);

		ClassDB::bind_method(D_METHOD("set_motion_features", "value"), &VirtualContainer::set_motion_features);
		ClassDB::bind_method(D_METHOD("get_motion_features"), &VirtualContainer::get_motion_features);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "motion_features", godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":BaseVirtual", PROPERTY_USAGE_DEFAULT), "set_motion_features", "get_motion_features");
	}
};

#undef GETSET