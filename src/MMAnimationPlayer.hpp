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

#include <KForm.hpp>
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

    inline const kform operator[](const std::size_t N) noexcept{
        kform out{};
        out.pos = pos[N];
        out.rot = {rot[N]};
        out.scl = {scl[N]};
        out.vel = {vel[N]};
        out.ang = {ang[N]};
        out.svl = {svl[N]};
        return out;
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
    int root_track_pos = -1;
    int root_track_rot = -1;

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
            auto track_pos = p_animation->find_track(bone_path, Animation::TrackType::TYPE_POSITION_3D);
            auto track_rot = p_animation->find_track(bone_path, Animation::TrackType::TYPE_ROTATION_3D);
            // Root bone have a special process
            if (bone_id == root_bone_id)
            {
                root_track_pos = track_pos;
                root_track_rot = track_rot;
                continue;
            }
            

            if (track_pos != -1)
            {
                Vector3 fut_bone_pos = p_animation->position_track_interpolate(track_pos, p_time);
                Vector3 fut_bone_vel = (p_animation->position_track_interpolate(track_pos, future_time) - fut_bone_pos) / abs(future_time - p_time);

                // Offset are calculated Between current pos of the bone and the desired pose
                CritDampSpring::inertialize_transition(bones_offset.pos[bone_id], bones_offset.vel[bone_id],
                                                       bones_kform.pos[bone_id], bones_kform.vel[bone_id],
                                                       fut_bone_pos, fut_bone_vel);
            }

            if ( track_rot != -1)
            {
                Quaternion curr_rot = p_animation->rotation_track_interpolate(track_rot, p_time);
                Quaternion fut_rot = p_animation->rotation_track_interpolate(track_rot, future_time);
                Vector3 curr_ang = CritDampSpring::quat_differentiate_angular_velocity(fut_rot,curr_rot,abs(future_time - p_time));

                // At this point the animation desired changed
                CritDampSpring::inertialize_transition(bones_offset.rot[bone_id], bones_offset.ang[bone_id], // Offset are calculated...
                                                       bones_kform.rot[bone_id], bones_kform.ang[bone_id],   // Between current rot of the bone...
                                                       curr_rot, curr_ang);                          // and the desired pose
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
        u::prints("Root Bone Id",root_bone_id);

        _skeleton->reset_bone_poses();
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
            bones_kform.pos[b] = _skeleton->get_bone_pose_position(b);
            bones_kform.rot[b] = _skeleton->get_bone_pose_rotation(b);

            bones_offset.reset(b);
        }
    }

    enum root_motion_type {
        ROOT_FIX = 0,
        ROOT_LOOP = 1,
        ROOT_ACCUMULATE=2
    } root_motion = ROOT_ACCUMULATE;

    virtual void _physics_process(double delta) override{
        if (Engine::get_singleton()->is_editor_hint())
        {
            return;
        }
        if(get_current_animation().is_empty()) return;

        auto anim = get_animation(get_current_animation());
        auto time = get_current_animation_position();
        if(get_playing_speed() < 0.0) delta *= -1.0;

        if(root_motion == ROOT_FIX)
        {
            bones_kform.pos[root_bone_id] = Vector3();
            bones_kform.rot[root_bone_id] = Quaternion();
        }
        else if(root_motion == ROOT_LOOP)
        {
            auto p0 = root_track_pos != -1 ? anim->position_track_interpolate(root_track_pos,0.0) : _skeleton->get_bone_rest(root_bone_id).origin;
            auto r0 = root_track_pos != -1 ? anim->rotation_track_interpolate(root_track_rot,0.0) : _skeleton->get_bone_rest(root_bone_id).basis.get_quaternion();
            auto p1 = root_track_pos != -1 ? anim->position_track_interpolate(root_track_pos,time) : _skeleton->get_bone_rest(root_bone_id).origin;
            auto r1 = root_track_pos != -1 ? anim->rotation_track_interpolate(root_track_rot,time) : _skeleton->get_bone_rest(root_bone_id).basis.get_quaternion();
            
            bones_kform.pos[root_bone_id] = r0.xform_inv(p1-p0);
            bones_kform.rot[root_bone_id] = r0.inverse() * r1;
        }
        else if(root_motion == ROOT_ACCUMULATE)
        {
            auto p0 = root_track_pos != -1 ? anim->position_track_interpolate(root_track_pos,time) : _skeleton->get_bone_rest(root_bone_id).origin;
            auto r0 = root_track_pos != -1 ? anim->rotation_track_interpolate(root_track_rot,time) : _skeleton->get_bone_rest(root_bone_id).basis.get_quaternion();
            auto p1 = root_track_pos != -1 ? anim->position_track_interpolate(root_track_pos,time+delta) : _skeleton->get_bone_rest(root_bone_id).origin;
            auto r1 = root_track_pos != -1 ? anim->rotation_track_interpolate(root_track_rot,time+delta) : _skeleton->get_bone_rest(root_bone_id).basis.get_quaternion();
            
            bones_kform.vel[root_bone_id] = time+delta > get_current_animation_length() ? bones_kform.vel[root_bone_id] : r0.xform_inv(p1-p0)/delta;
            bones_kform.ang[root_bone_id] = time+delta > get_current_animation_length() ? bones_kform.ang[root_bone_id]:
               r0.xform_inv(CritDampSpring::quat_differentiate_angular_velocity(r1,r0,delta));
            bones_kform.pos[root_bone_id] += bones_kform.rot[root_bone_id].xform(bones_kform.vel[root_bone_id]*delta*get_playing_speed());
            bones_kform.rot[root_bone_id] = CritDampSpring::quat_from_scaled_angle_axis(bones_kform.rot[root_bone_id].xform(bones_kform.ang[root_bone_id]*delta*get_playing_speed())) * bones_kform.rot[root_bone_id];
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
                    return _self->bones_kform.pos[bone_id] * _skeleton->get_motion_scale();
                }


                CritDampSpring::inertialize_update(_self->bones_kform.pos[bone_id], _self->bones_kform.vel[bone_id],   // Current pos of the bone
                                                _self->bones_offset.pos[bone_id], _self->bones_offset.vel[bone_id], // Current Offset pos, get reduced every frame
                                                fut_bone_pos, fut_bone_vel,                                         // Desired position from the animation
                                                halflife,                                                           // Stats on how the offset decay
                                                delta * get_speed_scale());                                         // delta time between frames
                return _self->bones_kform.pos[bone_id] * _skeleton->get_motion_scale();                                       // Set the bone position with motion_scale
            
            }   
            break;

            // Rotation
            case Animation::TYPE_ROTATION_3D:
            {
                Quaternion curr_rot = value;
                Quaternion fut_rot = animation->rotation_track_interpolate(track, future_time);
                Vector3 curr_ang = u::is_zero_approx(delta_diff) ? Vector3() : CritDampSpring::quat_differentiate_angular_velocity(fut_rot,curr_rot,delta_diff);

                // Root bone have a special process
                if (bone_id == root_bone_id)
                {
                    return _self->bones_kform.rot[bone_id];
                }

                CritDampSpring::inertialize_update(_self->bones_kform.rot[bone_id], _self->bones_kform.ang[bone_id],   // Current rot of the bone
                                                   _self->bones_offset.rot[bone_id], _self->bones_offset.ang[bone_id], // Current Offset rot, get reduced every frame
                                                   curr_rot, curr_ang,                                         // Desired rotation from the animation
                                                   halflife,                                                           // Stats on how the offset decay
                                                   delta * get_speed_scale());                                         // delta time between frames
                return _self->bones_kform.rot[bone_id];                                                                       // Set the bone rotation
            }
            break;

            // We only manage position and rotation for now
            default:
                break;
            }
            return value;
    }

   Dictionary get_local_bone_info(StringName bone_name)
   {
        ERR_FAIL_COND_V(_skeleton == nullptr,{});
        auto id = _skeleton->find_bone(bone_name);
        ERR_FAIL_COND_V_MSG(id == -1,{},"Bone " +bone_name + " doesn't exist in skeleton");
        const auto kin = bones_kform[id];
        Dictionary result = Dictionary{};
        result["position"] = kin.pos;
        result["linear_vel"] = kin.vel;
        result["rotation"] = kin.rot;
        result["angular_vel"] = kin.ang;
        result["scale"] = kin.scl;
        result["scalar_vel"] = kin.svl;
        return result;
   }

   Dictionary get_global_bone_info(StringName bone_name)
   {
        using vec3 = Vector3;
        using quat = Quaternion;

        ERR_FAIL_COND_V(_skeleton == nullptr, {});
        auto id = _skeleton->find_bone(bone_name);
        ERR_FAIL_COND_V_MSG(id == -1,{},"Bone " +bone_name + " doesn't exist in skeleton");
        std::vector<int> parents_id{id};
        auto tmp_p = id;
        while( _skeleton->get_bone_parent(tmp_p) != -1)
        {
            auto new_parent = _skeleton->get_bone_parent(tmp_p);
            parents_id.push_back(new_parent);
            tmp_p = new_parent;
        }
        std::reverse(parents_id.begin(),parents_id.end());
        kform global{};
        kform parent{};
        for(auto& i = parents_id.begin(); i != parents_id.end(); ++i)
        {
            const auto local = bones_kform[*i];
            // Position
            global.pos = parent.rot.xform(local.pos * parent.scl) + parent.pos;
            // Rotation
            global.rot = (parent.rot * local.rot);
            // Scale
            global.scl = local.scl * parent.scl;
            // Linear Velocity
            global.vel = parent.rot.xform(local.vel*parent.scl)+
                        parent.vel +
                        parent.ang.cross(parent.rot.xform(local.pos * parent.scl)) +
                        parent.rot.xform(local.pos * parent.scl * parent.svl);
            
            // Angular Velocity
            global.ang = parent.rot.xform(local.ang) + parent.ang;
            
            // Scalar Velocity
            global.svl = local.svl + parent.svl;

            parent = global;
        }
        global = bones_kform[root_bone_id] / global;
        

        Dictionary result = Dictionary{};
        result["position"] = global.pos;
        result["linear_vel"] = global.vel;
        result["rotation"] = global.rot;
        result["angular_vel"] = global.ang;
        result["scale"] = global.scl;
        result["scalar_vel"] = global.svl;
        return result;
   }

    Vector3 get_root_motion_velocity()
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        return bones_kform.vel[root_bone_id] * get_speed_scale();
        return bones_kform.rot[root_bone_id].xform_inv(bones_kform.vel[root_bone_id] * get_speed_scale());
    }
    Quaternion get_root_motion_angular(float delta)
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        return bones_kform.rot[root_bone_id].inverse() * CritDampSpring::quat_from_scaled_angle_axis(bones_kform.ang[root_bone_id] * get_speed_scale() * delta);
    }

    protected:
    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("set_skeleton_path", "value"), &MMAnimationPlayer::set_skeleton_path);
        ClassDB::bind_method(D_METHOD("get_skeleton_path"), &MMAnimationPlayer::get_skeleton_path);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::NODE_PATH, "skeleton_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton3D"), "set_skeleton_path", "get_skeleton_path");
        
        ClassDB::bind_method(D_METHOD("request_animation", "animation", "timestamp", "new_halflife","skip_same_anim_difference"), &MMAnimationPlayer::request_animation, (-1.0f),(0.0f));

        ClassDB::bind_method(D_METHOD("get_local_bone_info","bone_name"),&MMAnimationPlayer::get_local_bone_info);
        ClassDB::bind_method(D_METHOD("get_global_bone_info","bone_name"),&MMAnimationPlayer::get_global_bone_info);

        ClassDB::bind_method(D_METHOD("get_root_motion_velocity"), &MMAnimationPlayer::get_root_motion_velocity);
        ClassDB::bind_method(D_METHOD("get_root_motion_angular","delta_time"),&MMAnimationPlayer::get_root_motion_angular);

        ClassDB::bind_method( D_METHOD("set_halflife" ,"value"), &MMAnimationPlayer::set_halflife); 
        ClassDB::bind_method( D_METHOD("get_halflife" ), &MMAnimationPlayer::get_halflife); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"halflife"
        , PROPERTY_HINT_RANGE, "0.0,1,0.01,or_greater"), "set_halflife", "get_halflife");

        ClassDB::bind_method( D_METHOD("set_root_bone_name" ,"value"), &MMAnimationPlayer::set_root_bone_name); 
        ClassDB::bind_method( D_METHOD("get_root_bone_name" ), &MMAnimationPlayer::get_root_bone_name); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"root_bone_name"), "set_root_bone_name", "get_root_bone_name");
    }
};