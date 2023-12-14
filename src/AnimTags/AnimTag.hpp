#pragma once
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type,variable,...) type variable{__VA_ARGS__};\
    type get_##variable(){return  variable;}  \
    void set_##variable(type value){variable = value;}
#define STR(x) #x
#define STRING_PREFIX(prefix,s) STR(prefix##s)
#define BINDER_PROPERTY_PARAMS(type,variant_type,variable,...)\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(set_,variable) ,"value"), &type::set_##variable);\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(get_,variable) ), &type::get_##variable); \
        ADD_PROPERTY(PropertyInfo(variant_type,#variable,__VA_ARGS__),STRING_PREFIX(set_,variable),STRING_PREFIX(get_,variable));
using namespace godot;
// Base Class
struct AnimTag : godot::Resource
{
    GDCLASS(AnimTag,Resource);
public:
    GETSET(StringName, tag_name);
    GETSET(int,track_id);
    GETSET(real_t, timestamp);
    GETSET(real_t, duration,real_t(0));

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_tag_name", "value"), &AnimTag::set_tag_name);
        ClassDB::bind_method(D_METHOD("get_tag_name"), &AnimTag::get_tag_name);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING_NAME, "tag_name"), "set_tag_name", "get_tag_name");
        
        ClassDB::bind_method( D_METHOD("set_track_id" ,"value"), &AnimTag::set_track_id); 
        ClassDB::bind_method( D_METHOD("get_track_id" ), &AnimTag::get_track_id); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT,"track_id",godot::PROPERTY_HINT_NONE, "", godot::PROPERTY_USAGE_NO_EDITOR), "set_track_id", "get_track_id");
        
        ClassDB::bind_method( D_METHOD("set_timestamp" ,"value"), &AnimTag::set_timestamp);
        ClassDB::bind_method( D_METHOD("get_timestamp" ), &AnimTag::get_timestamp); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"timestamp"), "set_timestamp", "get_timestamp");

        ClassDB::bind_method(D_METHOD("set_duration", "value"), &AnimTag::set_duration, DEFVAL(real_t(0)));
        ClassDB::bind_method(D_METHOD("get_duration"), &AnimTag::get_duration);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");
    }
};

struct WarpRootTag : AnimTag
{
    GDCLASS(WarpRootTag,AnimTag);
    
public:
    enum warp_rule_t
    {
        Z = 0,
        Zr,
        XY,
        XYr,
        XYZ,
        XYZr,
        R
    };
    enum rotation_type_t
    {
        Align = 0,
        Facing
    };

    GETSET(warp_rule_t, warp_rule);
    GETSET(rotation_type_t, rotation_type);

protected:
    static void _bind_methods()
    {
        BIND_ENUM_CONSTANT(Z);
        BIND_ENUM_CONSTANT(Zr);
        BIND_ENUM_CONSTANT(XY);
        BIND_ENUM_CONSTANT(XYr);
        BIND_ENUM_CONSTANT(XYZ);
        BIND_ENUM_CONSTANT(XYZr);
        BIND_ENUM_CONSTANT(R);
        ClassDB::bind_method(D_METHOD("set_warp_rule", "value"), &WarpRootTag::set_warp_rule, DEFVAL(XYZr));
        ClassDB::bind_method(D_METHOD("get_warp_rule"), &WarpRootTag::get_warp_rule);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "warp_rule", godot::PROPERTY_HINT_ENUM, "Z,Zr,XY,XYr,XYZ,XYZr,R"), "set_warp_rule", "get_warp_rule");
        BIND_ENUM_CONSTANT(Align);
        BIND_ENUM_CONSTANT(Facing);
        ClassDB::bind_method(D_METHOD("set_rotation_type", "value"), &WarpRootTag::set_rotation_type, DEFVAL(Align));
        ClassDB::bind_method(D_METHOD("get_rotation_type"), &WarpRootTag::get_rotation_type);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "rotation_type", godot::PROPERTY_HINT_ENUM, "Align,Facing"), "set_rotation_type", "get_rotation_type");
    }
};
VARIANT_ENUM_CAST(WarpRootTag::warp_rule_t);
VARIANT_ENUM_CAST(WarpRootTag::rotation_type_t);


struct MFTimingTag : AnimTag
{
    GDCLASS(MFTimingTag,AnimTag);
public:
    GETSET(real_t, reference_time);

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method( D_METHOD("set_reference_time" ,"value"), &MFTimingTag::set_reference_time); 
        ClassDB::bind_method( D_METHOD("get_reference_time" ), &MFTimingTag::get_reference_time); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"reference_time"), "set_reference_time", "get_reference_time");
    }
};