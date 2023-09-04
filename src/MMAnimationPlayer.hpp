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

struct kforms
{
    std::vector<Vector3> pos; // Position
    std::vector<Quaternion> rot;  // Rotation
    std::vector<Vector3> scl; // Scale
    std::vector<Vector3> vel; // Linear Velocity
    std::vector<Vector3> ang; // Angular Velocity
    std::vector<Vector3> svl; // Scalar Velocity

    kforms(std::size_t N): pos(N,Vector3()),rot(N,Quaternion()),scl(N,Vector3(1,1,1)),vel(N,Vector3()),ang(N,Vector3()),svl(N,Vector3())
    {}

    void reserve(std::size_t N){
        pos.reserve(N); rot.reserve(N); scl.reserve(N);vel.reserve(N); ang.reserve(N); svl.reserve(N);
    }

    struct proxy{
        Vector3& pos; 
        Quaternion& rot ;
        Vector3& scl ;
        Vector3& vel ;
        Vector3& ang ;
        Vector3& svl ;
        proxy(kforms& k, std::size_t N):
            pos{k.pos[N]},rot{k.rot[N]},scl{k.scl[N]},vel{k.vel[N]},ang{k.ang[N]},svl{k.svl[N]}
        {}
    };

    inline const proxy operator[](const std::size_t N) noexcept{
        return proxy(*this,N);
    }

    void reset(const std::size_t N){
        pos[N] = Vector3() ;rot[N] = Quaternion(); scl[N] = Vector3(1,1,1) ;vel[N] = Vector3();ang[N] = Vector3();svl[N] = Vector3();
    }




};

/// @brief This animation node is for Motion Matching.
/// It was made to get request for a pose from the list of animations,
/// make a transition, then play the animation from then.
/// It can handle transition while 
struct MMAnimationPlayer : godot::AnimationPlayer
{
    GDCLASS(MMAnimationPlayer,AnimationPlayer);
    using u = godot::UtilityFunctions;

    kforms bones_kform{0}, bones_offset{0};

    GETSET(float,halflife,0.1);
    NodePath skeleton_path{};
    NodePath get_skeleton_path() { return skeleton_path; }
    void set_skeleton_path(NodePath value)
    {
        skeleton_path = value;
    }
    Skeleton3D* _skeleton = nullptr;

    String root_bone_name{}; int32_t root_bone_id = -1;
    String get_root_bone_name(){return root_bone_name;} 
    void set_root_bone_name(String value){
        root_bone_name = value;      
    }

    Ref<Animation> pending_desired_anim = nullptr;
    float pending_desired_time = 0.0f;

    virtual bool request_animation(StringName p_animation_name, float p_time,float new_halflife = -1.0f, float time_diff = 0.0f)
    {
        _skeleton = get_node<Skeleton3D>(skeleton_path);
        ERR_FAIL_NULL_V(_skeleton,false);
        auto p_animation = get_animation(p_animation_name);
        ERR_FAIL_NULL_V(p_animation,false);

        //
        if(p_animation_name == get_current_animation() && abs(p_time - get_current_animation_position()) < time_diff)
        {
            // We are already playing
            return false;
        }
        if ( new_halflife > 0.0f)
        {
            set_halflife(new_halflife);
        }

        bones_kform.reserve(_skeleton->get_bone_count());
        bones_offset.reserve(_skeleton->get_bone_count());

        const double delta = 0.016;
        const String skeleton_path = _skeleton->is_unique_name_in_owner() ? '%' + _skeleton->get_name() : String(_skeleton->get_owner()->get_path_to(this, true));

        p_time = u::clampf(p_time,0.0,p_animation->get_length()-halflife);
        const auto future_time = u::clampf(p_time+delta,0.0,p_animation->get_length());

        for (auto bone_id = 0; bone_id < _skeleton->get_bone_count(); ++bone_id)
        {
            
            const String bone_path = skeleton_path + String(":") + _skeleton->get_bone_name(bone_id);  

            if (auto track_pos = p_animation->find_track(bone_path, Animation::TrackType::TYPE_POSITION_3D); track_pos != -1)
            {
                Vector3 fut_bone_pos = p_animation->position_track_interpolate(track_pos, p_time);
                Vector3 fut_bone_vel = (p_animation->position_track_interpolate(track_pos, future_time) - fut_bone_pos) / abs(future_time - p_time);

                if(bone_id == root_bone_id)
                {
                    fut_bone_pos = Vector3();
                }

                // Offset are calculated Between current pos of the bone and the desired pose
                CritDampSpring::inertialize_transition(bones_offset[bone_id].pos, bones_offset[bone_id].vel,
                                                       bones_kform[bone_id].pos, bones_kform[bone_id].vel,
                                                       fut_bone_pos, fut_bone_vel);
            }

            if (auto track_rot = p_animation->find_track(bone_path, Animation::TrackType::TYPE_ROTATION_3D); track_rot != -1)
            {

                Quaternion fut_bone_rot = p_animation->rotation_track_interpolate(track_rot, p_time);
                Vector3 fut_bone_ang = CritDampSpring::quat_to_scaled_angle_axis(
                                           CritDampSpring::quat_abs(p_animation->rotation_track_interpolate(track_rot, future_time) * fut_bone_rot.inverse())) /
                                       abs(future_time - p_time);

                if(bone_id == root_bone_id)
                {
                    fut_bone_rot = Quaternion();
                }

                // At this point the animation desired changed
                CritDampSpring::inertialize_transition(bones_offset[bone_id].rot, bones_offset[bone_id].ang, // Offset are calculated...
                                                       bones_kform[bone_id].rot, bones_kform[bone_id].ang,   // Between current rot of the bone...
                                                       fut_bone_rot, fut_bone_ang);                          // and the desired pose
            }
        }

        play(p_animation_name);
        seek(p_time,true);

        return true;
    }

    virtual void _ready() override
    {
        if (Engine::get_singleton()->is_editor_hint())
        {
            return;
        }
        _skeleton = get_node<Skeleton3D>(skeleton_path);
        ERR_FAIL_NULL(_skeleton);
        root_bone_id = _skeleton->find_bone(root_bone_name);


        inertialize_reset();
    }

    void inertialize_reset(bool skeleton_to_rest = false)
    {
        ERR_FAIL_NULL(_skeleton);
        if(skeleton_to_rest)
        {
            _skeleton->reset_bone_poses();
        }

        const auto bone_count = _skeleton->get_bone_count();
        bones_kform.reserve(bone_count);
        bones_offset.reserve(bone_count);
        for (int b = 0; b < bone_count; ++b)
        {
            bones_kform.reset(b);
            bones_kform[b].pos = _skeleton->get_bone_pose_position(b);
            bones_kform[b].rot = _skeleton->get_bone_pose_rotation(b);

            bones_offset.reset(b);
        }
    }



    virtual Variant _post_process_key_value(const Ref<Animation> &animation, int32_t track, const Variant &value, Object *object, int32_t bone_id) const
    {
        if(Engine::get_singleton()->is_editor_hint())
        {
            return value;
        }
        if(animation == nullptr)
        {
            return value;
        }
        auto* s = cast_to<Skeleton3D>(object);
        // We need to modify some internal values, so const_cast it is because this member function is const.
        // According to C++ Standard section 7.1.6.1 para 4 : "Except that any class member declared mutable (7.1.1) can be modified, 
        // any attempt to modify a const object during its lifetime (3.8) results in undefined behavior"
        // In other word, as long as the AnimationPlayer object isn't declared const, we are not in UB
        auto *_self = const_cast<MMAnimationPlayer *>(this);
        const auto delta = get_process_callback() == AnimationProcessCallback::ANIMATION_PROCESS_PHYSICS ? get_physics_process_delta_time() : get_process_delta_time();
        const auto track_type = animation->track_get_type(track);

        const auto p_time = u::clampf(get_current_animation_position(), 0.0, animation->get_length());
        const auto future_time = u::clampf(p_time + delta, 0.0, animation->get_length());
        const auto delta_diff = abs(future_time-p_time);

        switch(track_type)
        {
            // Position
            case Animation::TYPE_POSITION_3D:
            {
                Vector3 fut_bone_pos = value;
                Vector3 fut_bone_vel = u::is_zero_approx(delta_diff) ? Vector3() : (animation->position_track_interpolate(track, future_time) - fut_bone_pos) / delta_diff;                
                
                // Root bone have a special process
                if (bone_id == root_bone_id)
                {
                    fut_bone_pos = Vector3();
                }

                CritDampSpring::inertialize_update(_self->bones_kform[bone_id].pos, _self->bones_kform[bone_id].vel,   // Current pos of the bone
                                                _self->bones_offset[bone_id].pos, _self->bones_offset[bone_id].vel, // Current Offset pos, get reduced every frame
                                                fut_bone_pos, fut_bone_vel,                                         // Desired position from the animation
                                                halflife,                                                           // Stats on how the offset decay
                                                delta * get_speed_scale());                                         // delta time between frames
                return (bone_id == root_bone_id) ? Vector3() :  _self->bones_kform[bone_id].pos * _skeleton->get_motion_scale();                                       // Set the bone position with motion_scale
            
            }   
            break;

            // Rotation
            case Animation::TYPE_ROTATION_3D:
            {
                Quaternion fut_bone_rot = value;
                Vector3 fut_bone_ang = u::is_zero_approx(delta_diff) ? Vector3() : CritDampSpring::quat_to_scaled_angle_axis(CritDampSpring::quat_abs(animation->rotation_track_interpolate(track, future_time) * fut_bone_rot.inverse())) / delta_diff;

                if (bone_id == root_bone_id)
                {
                    fut_bone_rot = Quaternion();
                }

                CritDampSpring::inertialize_update(_self->bones_kform[bone_id].rot, _self->bones_kform[bone_id].ang,   // Current rot of the bone
                                                   _self->bones_offset[bone_id].rot, _self->bones_offset[bone_id].ang, // Current Offset rot, get reduced every frame
                                                   fut_bone_rot, fut_bone_ang,                                         // Desired rotation from the animation
                                                   halflife,                                                           // Stats on how the offset decay
                                                   delta * get_speed_scale());                                         // delta time between frames
                return bone_id == root_bone_id ? Quaternion() : _self->bones_kform[bone_id].rot;                                                                       // Set the bone rotation
            }
            break;

            // We only manage position and rotation for now
            default:
                break;
            }
            return value;
    }

    // The root bone have some special process
    // However, this is not ready
    /*
    void inertialize_root_transition(const Ref<Animation> &animation, int32_t track, const Variant &value, Object *object, int32_t bone_id, double delta)
    {


        transition_src_pos  = root_bone_id != -1 ? _skeleton->get_bone_pose_position(root_bone_id) : Vector3();
        transition_src_rot  = root_bone_id != -1 ? _skeleton->get_bone_pose_rotation(root_bone_id) : Quaternion();
        transition_dst_pos = Vector3();
        transition_dst_rot = Quaternion();
    }

    Vector3 transition_src_pos;
    Quaternion transition_src_rot;
    Vector3 transition_dst_pos;
    Quaternion transition_dst_rot;

    void root_bone_process(double delta)
    {
            using vec3 = Vector3;
            using quat = Quaternion;

            pending_desired_anim = get_animation(get_current_animation());

            String root_path = is_unique_name_in_owner() ? "%" + get_name() : get_owner()->get_path_to(this, true);
            root_path += String(":") + String(root_bone_name);

            auto t_pos = pending_desired_anim->find_track(root_path, Animation::TrackType::TYPE_POSITION_3D);
            auto t_rot = pending_desired_anim->find_track(root_path, Animation::TrackType::TYPE_ROTATION_3D);

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

            vec3 world_space_angular_velocity = transition_dst_rotation.xform(transition_src_rotation.xform_inv(bones_kform[root_bone_id].ang));

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
    */

    Vector3 get_root_motion_velocity()
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        return bones_kform[root_bone_id].rot.xform_inv(bones_kform[root_bone_id].vel * get_speed_scale());
    }
    Quaternion get_root_motion_angular()
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        return CritDampSpring::quat_from_scaled_angle_axis(bones_kform[root_bone_id].ang * get_speed_scale());
    }

    protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_skeleton_path", "value"), &MMAnimationPlayer::set_skeleton_path);
        ClassDB::bind_method(D_METHOD("get_skeleton_path"), &MMAnimationPlayer::get_skeleton_path);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::NODE_PATH, "skeleton_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_skeleton_path", "get_skeleton_path");
        
        ClassDB::bind_method(D_METHOD("request_animation", "animation", "timestamp", "new_halflife","skip_same_anim_difference"), &MMAnimationPlayer::request_animation, (-1.0f),(0.0f));

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