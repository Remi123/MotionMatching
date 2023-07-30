#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/variant/node_path.hpp>



#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

#include "godot_cpp/core/math.hpp"

#include <godot_cpp/classes/animation_root_node.hpp>

#include <CritSpringDamper.hpp>

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

/// @brief This animation node is for Motion Matching.
/// It was made to get request for a pose from the list of animations,
/// make a transition, then play the animation from then.
/// It can handle transition while 
struct MMSkeleton3D : godot::Skeleton3D
{
    GDCLASS(MMSkeleton3D,Skeleton3D);
    using u = godot::UtilityFunctions;

    struct kform
    {
        Vector3 pos = Vector3(0, 0, 0); // Position
        Quaternion rot = Quaternion();  // Rotation
        Vector3 scl = Vector3(1, 1, 1); // Scale
        Vector3 vel = Vector3(0, 0, 0); // Linear Velocity
        Vector3 ang = Vector3(0, 0, 0); // Angular Velocity
        Vector3 svl = Vector3(0, 0, 0); // Scalar Velocity
    };

    HashMap<int32_t,kform> bones_kform{}, bones_offset{};

    GETSET(float,halflife,0.1);

    String root_bone_name{}; uint32_t root_bone_id = -1;
    String get_root_bone_name(){return root_bone_name;} 
    void set_root_bone_name(String value){
        root_bone_id = find_bone(value);
        if (root_bone_id < 0)
        {
            u::prints("Didn't find bone ",value);
            root_bone_id = -1;
            root_bone_name = "";
        }
        root_bone_name = value;        
    }

    Ref<Animation> pending_desired_anim = nullptr;
    float pending_desired_time = 0.0f;

    bool new_request = false;

    virtual void request_animation(Ref<Animation> p_animation, float p_time,float new_halflife = -1.0f)
    {
        // TODO : This is a test to see if preventing going to the same animation give better result
        // Maybe it's not the job of the skeleton to prevent such conditions ?
        if(p_animation == pending_desired_anim && abs(p_time - current_time) < 1.0)
        {
            u::prints("Same animation");
            return;
        }
        if ( new_halflife > 0.0f)
        {
            set_halflife(new_halflife);
        }
        new_request = true;
        bones_kform.reserve(get_bone_count());
        bones_offset.reserve(get_bone_count());
        pending_desired_anim = p_animation;
        current_time = p_time;
        new_request = true;
        return;
    }

    double current_time = 0.0;

    virtual void _physics_process(double delta) {
        Skeleton3D::_physics_process(delta);
        if(pending_desired_anim == nullptr)
        {
            return;
        }
        bones_kform.reserve(get_bone_count());
        bones_offset.reserve(get_bone_count());
        String skel_path = is_unique_name_in_owner() ? "%" + get_name() : get_owner()->get_path_to(this,true);

        for (auto t = 0; t < pending_desired_anim->get_track_count(); ++t)
        {
            auto bone_name = pending_desired_anim->track_get_path(t).get_concatenated_subnames();
            auto bone_id = find_bone(bone_name);
            if (bone_id == -1)
            {
                continue;
            }
            else if (pending_desired_anim->track_get_type(t) == Animation::TrackType::TYPE_POSITION_3D)
            {
                auto fut_bone_pos = pending_desired_anim->position_track_interpolate(t, current_time);
                auto fut_bone_vel = (pending_desired_anim->position_track_interpolate(t, current_time + 0.16) - fut_bone_pos) / 0.016;
                if (new_request)
                {
                    // At this point the animation desired changed
                    CritDampSpring::inertialize_transition(bones_offset[bone_id].pos, bones_offset[bone_id].vel, // Offset are calculated...
                                                           bones_kform[bone_id].pos, bones_kform[bone_id].vel,   // Between current pos of the bone...
                                                           fut_bone_pos, fut_bone_vel);                          // and the desired pose
                }

                CritDampSpring::inertialize_update(bones_kform[bone_id].pos, bones_kform[bone_id].vel,   // Current pos of the bone
                                                   bones_offset[bone_id].pos, bones_offset[bone_id].vel, // Current Offset pos, get reduced every frame
                                                   fut_bone_pos, fut_bone_vel,                           // Desired position from the animation
                                                   halflife,                                             // Stats on how the offset decay
                                                   delta);                                               // delta time between frames
                set_bone_pose_position(bone_id, bones_kform[bone_id].pos * get_motion_scale());          // Set the bone position with motion_scale
            }
            else if (pending_desired_anim->track_get_type(t) == Animation::TrackType::TYPE_ROTATION_3D)
            {
                auto fut_bone_rot = pending_desired_anim->rotation_track_interpolate(t, current_time);
                auto fut_bone_ang = CritDampSpring::quat_to_scaled_angle_axis(
                                        CritDampSpring::quat_abs(pending_desired_anim->rotation_track_interpolate(t, current_time + 0.16f) * fut_bone_rot.inverse())) /
                                    0.16f;

                if (new_request)
                {
                    // At this point the animation desired changed
                    CritDampSpring::inertialize_transition(bones_offset[bone_id].rot, bones_offset[bone_id].ang, // Offset are calculated...
                                                           bones_kform[bone_id].rot, bones_kform[bone_id].ang,   // Between current rot of the bone...
                                                           fut_bone_rot, fut_bone_ang);                          // and the desired pose
                }

                CritDampSpring::inertialize_update(bones_kform[bone_id].rot, bones_kform[bone_id].ang,   // Current rot of the bone
                                                   bones_offset[bone_id].rot, bones_offset[bone_id].ang, // Current Offset rot, get reduced every frame
                                                   fut_bone_rot, fut_bone_ang,                           // Desired rotation from the animation
                                                   halflife,                                             // Stats on how the offset decay
                                                   delta);                                               // delta time between frames
                set_bone_pose_rotation(bone_id, bones_kform[bone_id].rot);                               // Set the bone rotation
            }
        }

        if(new_request)
        {
            new_request = false;
        }


        current_time += delta;
    }

    protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("request_animation","animation","timestamp","new_halflife"),&MMSkeleton3D::request_animation,(-1.0f));

        ClassDB::bind_method( D_METHOD("set_halflife" ,"value"), &MMSkeleton3D::set_halflife); 
        ClassDB::bind_method( D_METHOD("get_halflife" ), &MMSkeleton3D::get_halflife); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"halflife"
        , PROPERTY_HINT_RANGE, "0.0,1,0.01,or_greater"), "set_halflife", "get_halflife");

        ClassDB::bind_method( D_METHOD("set_root_bone_name" ,"value"), &MMSkeleton3D::set_root_bone_name); 
        ClassDB::bind_method( D_METHOD("get_root_bone_name" ), &MMSkeleton3D::get_root_bone_name); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"root_bone_name"), "set_root_bone_name", "get_root_bone_name");
    }
};