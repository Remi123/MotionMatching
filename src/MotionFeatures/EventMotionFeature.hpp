#pragma once

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

struct EventMotionFeature : public Resource {
    GDCLASS(EventMotionFeature,Resource)

    virtual ~EventMotionFeature() = default;

    GETSET(bool,embed_time_since_last_event);
    GETSET(godot::PackedStringArray,events_tracks);

    static constexpr float delta = 0.016f;

    virtual int get_dimension()override{return events_tracks.count();}
    
    virtual PackedFloat32Array get_weights()override{ return {};}

    virtual bool setup_profile(NodePath skeleton_path,Ref<SkeletonProfile> skel_profile)override{
        // returning false will abort the process.
        // feel free to print more details
        
        return true;
    }

    virtual bool setup_for_animation(Ref<Animation> animation)override{
        // returning false will skip this animation and print a warning
        // feel free to print more details
        return true;
    }
    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time) override {return {};}

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}){return;}

    
    static void _bind_methods() {

        ClassDB::bind_method( D_METHOD("get_dimension"), &MotionFeature::get_dimension);

        ClassDB::bind_method( D_METHOD("get_weights"), &MotionFeature::get_weights);
        
        ClassDB::bind_method( D_METHOD("setup_profile","skeleton_path","skeleton_profile"), &MotionFeature::setup_profile);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &MotionFeature::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &MotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &MotionFeature::debug_pose_gizmo);
        
    }
};
