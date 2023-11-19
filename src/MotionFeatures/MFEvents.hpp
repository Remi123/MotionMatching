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

#include <algorithm>

#include <MotionFeatures/MotionFeatures.hpp>


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

    virtual ~MFEvents() = default;

    GETSET(bool,embed_as_frames);
    GETSET(bool,embed_time_since_last_event);
    GETSET(godot::PackedStringArray,events_tracks);
    GETSET(godot::PackedStringArray,events_names);

    static constexpr float delta = 0.016f;

    virtual int get_dimension()override{return events_names.size();}
    
    virtual PackedFloat32Array get_weights()override{ return Array::make(1.0f);}

    virtual bool setup_profile(NodePath skeleton_path,Ref<SkeletonProfile> skel_profile)override{
        // returning false will abort the process.
        // feel free to print more details

        return true;
    }

    virtual bool setup_for_animation(Ref<Animation> animation)override{
        // returning false will skip this animation and print a warning
        // feel free to print more details

        bool has_tracks = false;
        for(auto index_track = 0;index_track < events_names.size(); ++index_track)
        {
            const auto track_name = events_tracks[index_track];
            auto track_id = animation->find_track(track_name,Animation::TrackType::TYPE_METHOD);
            if(track_id == -1) continue;
            has_tracks = true;
        }
        if(has_tracks == false)
        {
            u::prints("No tracks found the animation",animation->get_path());
        }

        return has_tracks;
    }
    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time) override {
        PackedFloat32Array result = {};
        for(auto event_i = 0;event_i < events_names.size(); ++event_i)
        {
            String event_name = events_names[event_i];
            float closest_left = 0.0,closest_right = animation->get_length();
            for(auto index_track = 0;index_track < events_names.size(); ++index_track)
            {
                const auto track_name = events_tracks[index_track];
                auto track_id = animation->find_track(track_name,Animation::TrackType::TYPE_METHOD);
                if(track_id == -1) continue;

                for(auto index_key=0;index_key < animation->track_get_key_count(track_id); ++index_key )
                {
                    auto method_name = animation->method_track_get_name(track_id,index_key);
                    auto method_args = animation->method_track_get_params(track_id,index_key); //0.1
                    float method_time = (float)animation->track_get_key_time(track_id,index_key); // 0.33
                    if(method_name == event_name || 
                     (method_name == String("emit_signal") && method_args[0] == (StringName)event_name ))
                    {
                        if                      (time <= method_time )
                        {
                            closest_right = std::min(closest_right,method_time);                                                        
                        }
                        else if   (method_time < time)
                        {
                            closest_left = std::max(closest_left,method_time);
                        }
                    }
                }
            }
            auto time_until = closest_right - time;
            if(embed_as_frames)
            {
                time_until = sec_to_frame(time_until);
            }
            result.append(time_until);

        }
        return result;
    }

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}){return;}

    
    static void _bind_methods() {

        ClassDB::bind_method( D_METHOD("set_events_tracks" ,"value"), &MFEvents::set_events_tracks); 
        ClassDB::bind_method( D_METHOD("get_events_tracks" ), &MFEvents::get_events_tracks); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY,"events_tracks"), "set_events_tracks", "get_events_tracks");

        ClassDB::bind_method( D_METHOD("set_events_names" ,"value"), &MFEvents::set_events_names); 
        ClassDB::bind_method( D_METHOD("get_events_names" ), &MFEvents::get_events_names); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY,"events_names"), "set_events_names", "get_events_names");

        ClassDB::bind_method( D_METHOD("set_embed_as_frames" ,"value"), &MFEvents::set_embed_as_frames); 
        ClassDB::bind_method( D_METHOD("get_embed_as_frames" ), &MFEvents::get_embed_as_frames); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"embed_as_frames"), "set_embed_as_frames", "get_embed_as_frames");
        
        ClassDB::bind_method( D_METHOD("set_embed_time_since_last_event" ,"value"), &MFEvents::set_embed_time_since_last_event); 
        ClassDB::bind_method( D_METHOD("get_embed_time_since_last_event" ), &MFEvents::get_embed_time_since_last_event); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"embed_time_since_last_event"), "set_embed_time_since_last_event", "get_embed_time_since_last_event");

        ClassDB::bind_method( D_METHOD("get_dimension"), &MFEvents::get_dimension);

        ClassDB::bind_method( D_METHOD("get_weights"), &MFEvents::get_weights);
        
        ClassDB::bind_method( D_METHOD("setup_profile","skeleton_path","skeleton_profile"), &MFEvents::setup_profile);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &MFEvents::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &MFEvents::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &MFEvents::debug_pose_gizmo);
        
    }
};
