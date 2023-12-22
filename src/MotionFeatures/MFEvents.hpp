#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/variant.hpp>

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

#include <limits>
#include <algorithm>

#include <MotionFeatures/MotionFeatures.hpp>
#include <MMAnimationLibrary.hpp>


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

int sec_to_frame(float seconds, int fps = -1)
{
    if (fps <= 0 )
    {
        fps = Engine::get_singleton()->get_physics_ticks_per_second();
    }
    return (int)ceil(seconds * Engine::get_singleton()->get_physics_ticks_per_second());
}

struct MFEvents : public MotionFeature {
    GDCLASS(MFEvents,MotionFeature)

    public:
    enum EventType{
        Timing,
        EmbedValue
    };

    GETSET(EventType,event_type);


    GETSET(bool,embed_as_frames);
    GETSET(godot::PackedStringArray,events_tracks);
    GETSET(godot::PackedStringArray,events_names);

    static constexpr float delta = 0.016f;

    virtual int get_dimension()override{return events_names.size();}
    
    virtual PackedFloat32Array get_weights()override{ return Array::make(1.0f);}

    virtual bool setup_bake_init(Ref<MMAnimationLibrary> animlib)override{
        // returning false will abort the process.
        // feel free to print more details

        return true;
    }

    virtual bool setup_for_animation(Ref<Animation> animation)override{
        return true;
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time) override {
        PackedFloat32Array result = {};
        Vector<Ref<TagInfo>> all_tags;
        // for(auto tag : tags)
        {
            // if (auto* event = Object::cast_to<TagMFEvent>(tag); event != nullptr && event->tag_name ==)
            {

            }
        }
        

        return result;
    }

    void property_track(int track_id,Ref<Animation> animation,float time,PackedFloat32Array& result)
    {
        switch (event_type)
        {
            case RawValue:
            {
                Variant value = animation->value_track_interpolate(track_id,time);
                embed_variant(value,result);
            }
            case Timing:
            {
                
            }
            default :
            {

            }
        }
    }



    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}){return;}

    
    static void _bind_methods() {

        BIND_ENUM_CONSTANT(Timing);        
        BIND_ENUM_CONSTANT(EmbedValue);

        // Override Default Value
        ClassDB::bind_method( D_METHOD("set_normalization_type" ,"value"), &MFEvents::set_normalization_type,DEFVAL(NormalizationType::RawValue)); 
        

        ClassDB::bind_method( D_METHOD("set_events_tracks" ,"value"), &MFEvents::set_events_tracks); 
        ClassDB::bind_method( D_METHOD("get_events_tracks" ), &MFEvents::get_events_tracks); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY,"events_tracks"), "set_events_tracks", "get_events_tracks");

        ClassDB::bind_method( D_METHOD("set_events_names" ,"value"), &MFEvents::set_events_names); 
        ClassDB::bind_method( D_METHOD("get_events_names" ), &MFEvents::get_events_names); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY,"events_names"), "set_events_names", "get_events_names");

        ClassDB::bind_method( D_METHOD("set_embed_as_frames" ,"value"), &MFEvents::set_embed_as_frames); 
        ClassDB::bind_method( D_METHOD("get_embed_as_frames" ), &MFEvents::get_embed_as_frames); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"embed_as_frames"), "set_embed_as_frames", "get_embed_as_frames");
        
        ClassDB::bind_method( D_METHOD("get_dimension"), &MFEvents::get_dimension);

        ClassDB::bind_method( D_METHOD("get_weights"), &MFEvents::get_weights);
        
        
        ClassDB::bind_method( D_METHOD("setup_bake_init","mm_animation_library"), &MFEvents::setup_bake_init);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &MFEvents::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &MFEvents::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &MFEvents::debug_pose_gizmo);
        
    }
};

VARIANT_ENUM_CAST(MFEvents::EventType);