#pragma once

#include <climits>
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

#include <godot_cpp/classes/time.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/skeleton_profile.hpp>

#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>


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

class MMAnimationLibrary;

struct MotionFeature : public Resource {
    GDCLASS(MotionFeature,Resource)
    friend class MMAnimationLibrary;
    public:
    enum NormalizationType{
        Standard,
        RawValue
    };
    GETSET(NormalizationType,normalization_type,Standard);
    GETSET(real_t,norm_clamp_min,std::numeric_limits<float>::min());
    GETSET(real_t,norm_clamp_max,std::numeric_limits<float>::max());

    static constexpr float delta = 0.016f;

    virtual int get_dimension(){return 0;}

    virtual PackedStringArray get_hints()const {return {};}
    
    virtual PackedFloat32Array get_weights(){ return {};}

    virtual bool setup_bake_init(Ref<MMAnimationLibrary> mmal){
        // returning false will abort the process.
        // feel free to print more details
        return true;
    }

    virtual bool setup_bake_animation(Ref<Animation> animation){
        // returning false will skip this animation and print a warning
        // feel free to print more details
        return true;
    }
    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time){return {};}

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}){return;}

    
    static void _bind_methods() {

        BIND_ENUM_CONSTANT(Standard);
        BIND_ENUM_CONSTANT(RawValue);

        ClassDB::bind_method( D_METHOD("get_dimension"), &MotionFeature::get_dimension);
        ClassDB::bind_method( D_METHOD("get_weights"), &MotionFeature::get_weights);
        ClassDB::bind_method( D_METHOD("get_hints"), &MotionFeature::get_hints);

        ClassDB::bind_method( D_METHOD("set_normalization_type" ,"value"), &MotionFeature::set_normalization_type,DEFVAL(NormalizationType::Standard)); 
        ClassDB::bind_method( D_METHOD("get_normalization_type" ), &MotionFeature::get_normalization_type); 
        ADD_PROPERTY(PropertyInfo(Variant::INT,"normalization_type",godot::PROPERTY_HINT_ENUM,"Standard,RawValue"), "set_normalization_type", "get_normalization_type");
        
        ClassDB::bind_method( D_METHOD("setup_bake_init","mm_animation_library"),   &MotionFeature::setup_bake_init);        
        ClassDB::bind_method( D_METHOD("setup_bake_animation","animation"),         &MotionFeature::setup_bake_animation);        
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"),   &MotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &MotionFeature::debug_pose_gizmo);
        
    }

    static void embed_variant(Variant& v,PackedFloat32Array& result)
    {
        using namespace godot;
        if(v.get_type() == Variant::BOOL)
        {
            result.append((bool)v);
        }
        else if(v.get_type() == Variant::INT)
        {
            result.append((int)v);
        }
        else if(v.get_type() == Variant::FLOAT)
        {
            result.append((real_t)v);
        }
        else if(v.get_type() == Variant::VECTOR2)
        {
            godot::Vector2 v2 = (Vector2)v;
            result.append(v2.x);
            result.append(v2.y);
        }
        else if(v.get_type() == Variant::VECTOR3)
        {
            Vector3 v3 = (Vector3)v;
            result.append(v3.x);
            result.append(v3.y);
            result.append(v3.z);
        }
        else if(v.get_type() == Variant::QUATERNION)
        {
            Quaternion q = (Quaternion)v;
            result.append(q.x);
            result.append(q.y);
            result.append(q.z);
            result.append(q.w);
        }
    }
    
};

VARIANT_ENUM_CAST(MotionFeature::NormalizationType);
