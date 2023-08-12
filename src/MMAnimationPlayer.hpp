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
#include "godot_cpp/variant/vector3.hpp"

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
struct MMAnimationPlayer : godot::AnimationPlayer
{
    GDCLASS(MMAnimationPlayer,AnimationPlayer);
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
    NodePath skeleton_path{};
    NodePath get_skeleton_path() { return skeleton_path; }
    void set_skeleton_path(NodePath value)
    {
        skeleton_path = value;
        // _skeleton = get_node<Skeleton3D>(value);
        // if (_skeleton != nullptr)
        //     skeleton_path = value;
    }
    Skeleton3D* _skeleton = nullptr;

    String root_bone_name{}; uint32_t root_bone_id = -1;
    String get_root_bone_name(){return root_bone_name;} 
    void set_root_bone_name(String value){
        root_bone_name = value; 
        // auto id = _skeleton->find_bone(value);
        // if (id < 0)
        // {
        //     u::prints("Didn't find bone ",value);
        //     root_bone_id = -1;
        //     //root_bone_name = "";
        // }
        // u::prints("Root bone name:",value,"Id:",id);
        // root_bone_id = id;
       
    }

    Ref<Animation> pending_desired_anim = nullptr;
    float pending_desired_time = 0.0f;

    bool new_request = false;

    virtual void request_animation(StringName p_animation_name, float p_time,float new_halflife = -1.0f)
    {
        _skeleton = get_node<Skeleton3D>(skeleton_path);
        ERR_FAIL_NULL(_skeleton);
        auto p_animation = get_animation(p_animation_name);
        ERR_FAIL_NULL(p_animation);
        // TODO : This is a test to see if preventing going to the same animation give better result
        // Maybe it's not the job of the skeleton to prevent such conditions ?
        if(p_animation == pending_desired_anim && abs(p_time - current_time) < 0.5)
        {
            return;
        }
        if ( new_halflife > 0.0f)
        {
            set_halflife(new_halflife);
        }
        new_request = true;
        bones_kform.reserve(_skeleton->get_bone_count());
        bones_offset.reserve(_skeleton->get_bone_count());

        pending_desired_anim = p_animation;
        current_time = p_time;
        play(p_animation_name);
        seek(p_time);
        new_request = true;
        return;
    }

    double current_time = 0.0;

    virtual void _ready() override
    {
        if (Engine::get_singleton()->is_editor_hint())
        {
            return;
        }
        _skeleton = get_node<Skeleton3D>(skeleton_path);
        ERR_FAIL_NULL(_skeleton);
        root_bone_id = _skeleton->find_bone(root_bone_name);


        const auto bone_count = _skeleton->get_bone_count();
        bones_kform.reserve(bone_count);
        bones_offset.reserve(bone_count);
        for (int b = 0; b < bone_count; ++b)
        {
            bones_kform[b].pos = _skeleton->get_bone_pose_position(b);
        }
    }

    virtual void _physics_process(double delta) override
    {
        if(Engine::get_singleton()->is_editor_hint())
        {
            return;
        }
        //AnimationPlayer::_physics_process(delta);

        current_time += delta;
        if(new_request == true)
            new_request = false;
    }

    virtual Variant _post_process_key_value(const Ref<Animation> &animation, int32_t track, const Variant &value, Object *object, int32_t bone_id) const
    {
        if(Engine::get_singleton()->is_editor_hint())
        {
            return value;
        }
        auto* s = cast_to<Skeleton3D>(object);
        // We need to modify some internal values, so const_cast it is.
        // According to C++ Standard section 7.1.6.1 para 4 : "Except that any class member declared mutable (7.1.1) can be modified, 
        // any attempt to modify a const object during its lifetime (3.8) results in undefined behavior"
        // In other word, as long as the AnimationPlayer object isn't declared const, we are not in UB
        auto *_self = const_cast<MMAnimationPlayer *>(this);
        auto delta = get_process_callback() == AnimationProcessCallback::ANIMATION_PROCESS_PHYSICS ? get_physics_process_delta_time() : get_process_delta_time();
        auto track_type = animation->track_get_type(track);
        // Root bone process
        // Due to how this function is called, we only return 0 and identity.
        if (bone_id == root_bone_id)
        {
            if (track_type == Animation::TYPE_POSITION_3D)
                return Vector3();
            else if (track_type == Animation::TYPE_ROTATION_3D)
                return Quaternion();
            else
                return value;
        }

        switch(track_type)
        {
            // Position
            case Animation::TYPE_POSITION_3D:{
                
                Vector3 fut_bone_pos = pending_desired_anim->position_track_interpolate(track, current_time);
                Vector3 fut_bone_vel = (pending_desired_anim->position_track_interpolate(track, current_time + 0.16) - fut_bone_pos) / 0.016;
                if (new_request)
                {
                    // At this point the animation desired changed
                    CritDampSpring::inertialize_transition(_self->bones_offset[bone_id].pos, _self->bones_offset[bone_id].vel, // Offset are calculated...
                                                           bones_kform[bone_id].pos, bones_kform[bone_id].vel,   // Between current pos of the bone...
                                                           fut_bone_pos, fut_bone_vel);                          // and the desired pose
                }

                CritDampSpring::inertialize_update(_self->bones_kform[bone_id].pos, _self->bones_kform[bone_id].vel,   // Current pos of the bone
                                                   _self->bones_offset[bone_id].pos, _self->bones_offset[bone_id].vel, // Current Offset pos, get reduced every frame
                                                   fut_bone_pos, fut_bone_vel,                           // Desired position from the animation
                                                   halflife,                                             // Stats on how the offset decay
                                                   delta);                                               // delta time between frames
                return bones_kform[bone_id].pos * _skeleton->get_motion_scale();                         // Set the bone position with motion_scale
            }   break;

            // Rotation
            case Animation::TYPE_ROTATION_3D:
            {
                auto fut_bone_rot = pending_desired_anim->rotation_track_interpolate(track, current_time);
                auto fut_bone_ang = CritDampSpring::quat_to_scaled_angle_axis(
                                        CritDampSpring::quat_abs(pending_desired_anim->rotation_track_interpolate(track, current_time + 0.16f) * fut_bone_rot.inverse())) /
                                    0.16f;

                if (new_request)
                {
                    // At this point the animation desired changed
                    CritDampSpring::inertialize_transition(_self->bones_offset[bone_id].rot, _self->bones_offset[bone_id].ang, // Offset are calculated...
                                                           bones_kform[bone_id].rot, bones_kform[bone_id].ang,   // Between current rot of the bone...
                                                           fut_bone_rot, fut_bone_ang);                          // and the desired pose
                }

                CritDampSpring::inertialize_update(_self->bones_kform[bone_id].rot, _self->bones_kform[bone_id].ang,   // Current rot of the bone
                                                   _self->bones_offset[bone_id].rot, _self->bones_offset[bone_id].ang, // Current Offset rot, get reduced every frame
                                                   fut_bone_rot, fut_bone_ang,                           // Desired rotation from the animation
                                                   halflife,                                             // Stats on how the offset decay
                                                   delta);                                               // delta time between frames
                return bones_kform[bone_id].rot;                               // Set the bone rotation
            }
            break;

            // We only manage position and rotation for now
            default:
                break;
            }
            return value;
    }
// The root bone have some special process
void root_bone_process(double delta)
    {
        using vec3 = Vector3;
        using quat = Quaternion;

        pending_desired_anim = get_animation(get_current_animation());

        String root_path = is_unique_name_in_owner() ? "%" + get_name() : get_owner()->get_path_to(this,true);
        root_path += String(":") + String(root_bone_name);

        auto t_pos = pending_desired_anim->find_track(root_path,Animation::TrackType::TYPE_POSITION_3D);
        auto t_rot = pending_desired_anim->find_track(root_path,Animation::TrackType::TYPE_ROTATION_3D);

        auto fut_bone_pos = pending_desired_anim->position_track_interpolate(t_pos, current_time);
        auto fut_bone_vel = (pending_desired_anim->position_track_interpolate(t_pos, current_time + 0.16) - fut_bone_pos) / 0.016;
        auto fut_bone_rot = pending_desired_anim->rotation_track_interpolate(t_rot, current_time);
        auto fut_bone_ang = CritDampSpring::quat_to_scaled_angle_axis(
                                CritDampSpring::quat_abs(pending_desired_anim->rotation_track_interpolate(t_rot, current_time + 0.16f) * fut_bone_rot.inverse())) /
                            0.16f;

        vec3 transition_dst_position = bones_kform[root_bone_id].pos;
        quat transition_dst_rotation = bones_kform[root_bone_id].rot;
        vec3 transition_src_position = fut_bone_pos;
        quat transition_src_rotation = fut_bone_rot;

        // We then find the velocities so we can transition the
        // root inertiaizers
        vec3 world_space_dst_velocity = transition_dst_rotation.xform(
                                                      transition_src_rotation.xform_inv(fut_bone_vel));

        vec3 world_space_dst_angular_velocity = transition_dst_rotation.xform(transition_src_rotation.xform_inv(fut_bone_ang));

        // Transition inertializers recording the offsets for
        // the root joint
        CritDampSpring::inertialize_transition(
            bones_offset[root_bone_id].pos,
            bones_offset[root_bone_id].vel,
            bones_kform[root_bone_id].pos,
            bones_kform[root_bone_id].vel,
            bones_kform[root_bone_id].pos,
            world_space_dst_velocity);

        CritDampSpring::inertialize_transition(
            bones_offset[root_bone_id].rot,
            bones_offset[root_bone_id].ang,
            bones_kform[root_bone_id].rot,
            bones_kform[root_bone_id].ang,
            bones_kform[root_bone_id].rot,
            world_space_dst_angular_velocity);

        vec3 world_space_position = transition_dst_rotation.xform(transition_src_rotation.xform_inv(bones_kform[root_bone_id].pos - transition_src_position)) + transition_dst_position;
    
        vec3 world_space_velocity = transition_dst_rotation.xform(transition_src_rotation.xform_inv(bones_kform[root_bone_id].vel));

        quat world_space_rotation = (transition_dst_rotation * (transition_src_rotation.inverse() * bones_kform[root_bone_id].rot)).normalized();

        vec3 world_space_angular_velocity = transition_dst_rotation.xform(transition_src_rotation.xform_inv( bones_kform[root_bone_id].ang ));

        CritDampSpring::inertialize_update(
            bones_kform[root_bone_id].pos,
            bones_kform[root_bone_id].vel,
            bones_offset[root_bone_id].pos,
            bones_offset[root_bone_id].vel,
            world_space_position,
            world_space_velocity,
            halflife,
            delta);
        CritDampSpring::inertialize_update(
            bones_kform[root_bone_id].rot,
            bones_kform[root_bone_id].ang,
            bones_offset[root_bone_id].rot,
            bones_offset[root_bone_id].ang,
            world_space_rotation,
            world_space_angular_velocity,
            halflife,
            delta);
    }


    Vector3 get_root_motion_velocity()
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        return bones_kform[root_bone_id].rot.xform_inv(bones_kform[root_bone_id].vel);
    }
    Quaternion get_root_motion_angular()
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        return CritDampSpring::quat_from_scaled_angle_axis(bones_kform[root_bone_id].ang);
    }

    protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_skeleton_path", "value"), &MMAnimationPlayer::set_skeleton_path);
        ClassDB::bind_method(D_METHOD("get_skeleton_path"), &MMAnimationPlayer::get_skeleton_path);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::NODE_PATH, "skeleton_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_skeleton_path", "get_skeleton_path");
        
        ClassDB::bind_method(D_METHOD("request_animation", "animation", "timestamp", "new_halflife"), &MMAnimationPlayer::request_animation, (-1.0f));

        ClassDB::bind_method(D_METHOD("get_root_motion_velocity"), &MMAnimationPlayer::get_root_motion_velocity);
        ClassDB::bind_method(D_METHOD("get_root_motion_angular"),&MMAnimationPlayer::get_root_motion_angular);

        ClassDB::bind_method( D_METHOD("set_halflife" ,"value"), &MMAnimationPlayer::set_halflife); 
        ClassDB::bind_method( D_METHOD("get_halflife" ), &MMAnimationPlayer::get_halflife); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"halflife"
        , PROPERTY_HINT_RANGE, "0.0,1,0.01,or_greater"), "set_halflife", "get_halflife");

        ClassDB::bind_method( D_METHOD("set_root_bone_name" ,"value"), &MMAnimationPlayer::set_root_bone_name); 
        ClassDB::bind_method( D_METHOD("get_root_bone_name" ), &MMAnimationPlayer::get_root_bone_name); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"root_bone_name"), "set_root_bone_name", "get_root_bone_name");
    }
};