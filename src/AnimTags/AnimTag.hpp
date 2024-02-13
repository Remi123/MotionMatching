#pragma once
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <ranges>

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
using u = godot::UtilityFunctions;
// Base Class
struct TagInfo : godot::Resource
{
    GDCLASS(TagInfo,Resource);
public:
    GETSET(StringName, animation_name);
    GETSET(int,track_id);
    real_t timestamp{}; 
    real_t get_timestamp(){return timestamp;} 
    void set_timestamp(real_t value){
        const real_t snap = Engine::get_singleton()->get_physics_ticks_per_second();
        timestamp = std::max(real_t(0.0),std::floor(value * snap) / snap) ;
    }
    real_t duration{real_t(0.016)}; 
    real_t get_duration(){return duration;} 
    void set_duration(real_t value){
        const real_t snap = Engine::get_singleton()->get_physics_ticks_per_second();
        duration = std::abs(value);// std::max(real_t(0.0), std::floor(value * snap) / snap );
    }

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_animation_name", "value"), &TagInfo::set_animation_name);
        ClassDB::bind_method(D_METHOD("get_animation_name"), &TagInfo::get_animation_name);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING_NAME, "animation_name"), "set_animation_name", "get_animation_name");
        
        ClassDB::bind_method( D_METHOD("set_track_id" ,"value"), &TagInfo::set_track_id); 
        ClassDB::bind_method( D_METHOD("get_track_id" ), &TagInfo::get_track_id); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT,"track_id",godot::PROPERTY_HINT_NONE, "", godot::PROPERTY_USAGE_DEFAULT), "set_track_id", "get_track_id");
        
        ClassDB::bind_method( D_METHOD("set_timestamp" ,"value"), &TagInfo::set_timestamp);
        ClassDB::bind_method( D_METHOD("get_timestamp" ), &TagInfo::get_timestamp); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"timestamp"), "set_timestamp", "get_timestamp");

        ClassDB::bind_method(D_METHOD("set_duration", "value"), &TagInfo::set_duration, DEFVAL(real_t(0.016)));
        ClassDB::bind_method(D_METHOD("get_duration"), &TagInfo::get_duration);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "duration"), "set_duration", "get_duration");
    }
};

struct TagMotionMatching : TagInfo
{
    GDCLASS(TagMotionMatching,TagInfo);
public:
protected:
    static void _bind_methods()
    {
    }
};

struct TagJunk : TagMotionMatching
{
    GDCLASS(TagJunk,TagMotionMatching);
public:
protected:
    static void _bind_methods()
    {
    }
};

struct TagCategory : TagMotionMatching
{
    GDCLASS(TagCategory,TagMotionMatching);
    public:

    GETSET(int,category);
    GETSET(StringName,property_hint_string,"");


protected:
	 void _get_property_list(List<PropertyInfo> *r_props) const{      // return list of properties
        TagMotionMatching::_get_property_list(r_props);
        PropertyInfo catego{Variant::INT,"category",godot::PROPERTY_HINT_FLAGS,property_hint_string};
        r_props->push_back(catego);

        PropertyInfo hint{Variant::STRING_NAME,"property_hint_string",godot::PROPERTY_HINT_NONE,"",godot::PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED | godot::PROPERTY_USAGE_DEFAULT};
        r_props->push_back(hint);

    }

    bool _get(const StringName &p_name, Variant &r_ret) const {
        if (p_name == StringName("category"))
        {
            r_ret = category;
            return true;
        }
        else if (p_name == StringName("property_hint_string"))
        {
            r_ret = property_hint_string;
            return true;
        }
        else
        {
            return TagMotionMatching::_get(p_name,r_ret);
        }
        return false;
    }

    bool _set(const StringName &p_name, const Variant &p_value){
        if (p_name == StringName("category"))
        {
            category = (int)p_value;
            return true;
        }
        else if (p_name == StringName("property_hint_string"))
        {
            property_hint_string = (StringName)p_value;
            return true;
        }
        else
        {
            return TagMotionMatching::_set(p_name,p_value);
        }
        return false;
    }

protected:
    static void _bind_methods()
    {
    }
};

struct TagMFEvent : TagMotionMatching
{
    GDCLASS(TagMFEvent,TagMotionMatching);
public:
    GETSET(StringName,event_name);
    // GETSET(real_t, reference_time);

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method( D_METHOD("set_event_name" ,"value"), &TagMFEvent::set_event_name); 
        ClassDB::bind_method( D_METHOD("get_event_name" ), &TagMFEvent::get_event_name); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING_NAME,"event_name"), "set_event_name", "get_event_name");

        // ClassDB::bind_method( D_METHOD("set_reference_time" ,"value"), &TagMFEvent::set_reference_time); 
        // ClassDB::bind_method( D_METHOD("get_reference_time" ), &TagMFEvent::get_reference_time); 
        // godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"reference_time"), "set_reference_time", "get_reference_time");
    }
};

struct TagMFDistance : TagMotionMatching
{
    GDCLASS(TagMFDistance,TagMotionMatching);
public:
    enum Strategy{
        RootPos,AnchorPoint,AnchorBone
    };
    GETSET(StringName,event_name);
    GETSET(int,anchor_point_strategy, RootPos);
    GETSET(StringName,reference_bone);
    GETSET(Vector3, reference_position);

    void _get_property_list(List<PropertyInfo> *r_props) const{      // return list of properties
        auto name = PropertyInfo(Variant::STRING_NAME,"event_name");
        r_props->push_back(name);

        auto strat = PropertyInfo(Variant::INT,"anchor_point_strategy"
                    ,PROPERTY_HINT_ENUM,"UseRootPosition,UsePoint,UseBone"
                    ,PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED);
        r_props->push_back(strat);

        auto ref_point = PropertyInfo(Variant::VECTOR3,"anchor_point"
        ,PROPERTY_HINT_NONE,""
        ,anchor_point_strategy == AnchorPoint ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_STORAGE);
        r_props->push_back(ref_point);

        auto ref_bone = PropertyInfo(Variant::STRING_NAME,"bone_name"
        ,PROPERTY_HINT_NONE,""
        ,anchor_point_strategy == AnchorBone ? PROPERTY_USAGE_DEFAULT : PROPERTY_USAGE_STORAGE);
        r_props->push_back(ref_bone);
    }

    bool _get(const StringName &p_name, Variant &r_ret) const {
        if (p_name == StringName("event_name"))
        {
            r_ret = event_name;
            return true;
        }
        else if (p_name == StringName("anchor_point_strategy"))
        {
            r_ret = anchor_point_strategy;
            return true;
        }
        else if (p_name == StringName("bone_name"))
        {
            r_ret = reference_bone;
            return true;
        }
        else if (p_name == StringName("anchor_point"))
        {
            r_ret = reference_position;
            return true;
        }
        else
        {
            return TagMotionMatching::_get(p_name,r_ret);
        }
        return false;
    }

    bool _set(const StringName &p_name, const Variant &p_value){
        if (p_name == StringName("event_name"))
        {
            event_name = (StringName)p_value;
            return true;
        }
        else if (p_name == StringName("anchor_point_strategy"))
        {
            anchor_point_strategy = (int)p_value;
            return true;
        }
        else if (p_name == StringName("bone_name"))
        {
            reference_bone = (StringName)p_value;
            return true;
        }
        else if (p_name == StringName("anchor_point"))
        {
            reference_position = (Vector3)p_value;
            return true;
        }
        else
        {
            return TagMotionMatching::_set(p_name,p_value);
        }
        return false;
    }
protected:
    static void _bind_methods()
    {}
};


struct TagAnimation : TagInfo
{
    GDCLASS(TagAnimation,TagInfo);
public:
protected:
    static void _bind_methods()
    {
    }
};


struct TagRootWarp : TagAnimation
{
    GDCLASS(TagRootWarp,TagAnimation);
    
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
        ClassDB::bind_method(D_METHOD("set_warp_rule", "value"), &TagRootWarp::set_warp_rule, DEFVAL(XYZr));
        ClassDB::bind_method(D_METHOD("get_warp_rule"), &TagRootWarp::get_warp_rule);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "warp_rule", godot::PROPERTY_HINT_ENUM, "Z,Zr,XY,XYr,XYZ,XYZr,R"), "set_warp_rule", "get_warp_rule");
        BIND_ENUM_CONSTANT(Align);
        BIND_ENUM_CONSTANT(Facing);
        ClassDB::bind_method(D_METHOD("set_rotation_type", "value"), &TagRootWarp::set_rotation_type, DEFVAL(Align));
        ClassDB::bind_method(D_METHOD("get_rotation_type"), &TagRootWarp::get_rotation_type);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "rotation_type", godot::PROPERTY_HINT_ENUM, "Align,Facing"), "set_rotation_type", "get_rotation_type");
    }
};
VARIANT_ENUM_CAST(TagRootWarp::warp_rule_t);
VARIANT_ENUM_CAST(TagRootWarp::rotation_type_t);


