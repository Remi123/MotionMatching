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
#define GETSET(type,variable,...) type variable{__VA_ARGS__}; type get_##variable(){return  variable;} void set_##variable(type value){variable = value;}
#define STR(x) #x
#define STRING_PREFIX(prefix,s) STR(prefix##s)
#define BINDER_PROPERTY_PARAMS(type,variant_type,variable,...)\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(set_,variable) ,"value"), &type::set_##variable);\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(get_,variable) ), &type::get_##variable);\
        ADD_PROPERTY(PropertyInfo(variant_type,#variable,__VA_ARGS__),STRING_PREFIX(set_,variable),STRING_PREFIX(get_,variable));

/// @brief This animation node is for Motion Matching.
/// It was made to get request for a pose from the list of animations,
/// make a transition, then play the animation from then.
/// It can handle transition while 
struct AnimationNodeInertialization : godot::AnimationRootNode
{
    GDCLASS(AnimationNodeInertialization,AnimationNode);
    using u = godot::UtilityFunctions;

    GETSET(Skeleton3D*,skeleton,nullptr);
    GETSET(AnimationPlayer*,ap,nullptr);

    GETSET(float,blend_time, 0.3f);

private:
    double delta_time = 0.0;

public:
    virtual String _get_caption() const {
        return "Inertialization";
    }




/*
    virtual double _old_process(double p_time, bool p_seek, bool p_is_external_seeking, bool p_test_only)
    {
        if(skeleton == nullptr || ap == nullptr)
        {
            return 0.0;
        }

        double cur_time = get_parameter("timestamp");

        if (!ap->has_animation(get_parameter("animation")))
        {
            return 0.0;
        }

        Ref<Animation> anim = ap->get_animation((StringName)(get_parameter("animation")));
        double anim_size = (double)anim->get_length();
        double step = 0.0;
        double prev_time = cur_time;
        Animation::LoopedFlag looped_flag = Animation::LOOPED_FLAG_NONE;
        bool node_backward = play_mode == PLAY_MODE_BACKWARD;

        if (p_seek)
        {
            step = p_time - cur_time;
            cur_time = p_time;
        }
        else
        {
            p_time *= backward ? -1.0 : 1.0;
            cur_time = cur_time + p_time;
            step = p_time;
        }

        bool is_looping = false;
        if (anim->get_loop_mode() == Animation::LOOP_PINGPONG)
        {
            if (!Math::is_zero_approx(anim_size))
            {
                if (prev_time >= 0 && cur_time < 0)
                {
                    backward = !backward;
                    looped_flag = node_backward ? Animation::LOOPED_FLAG_END : Animation::LOOPED_FLAG_START;
                }
                if (prev_time <= anim_size && cur_time > anim_size)
                {
                    backward = !backward;
                    looped_flag = node_backward ? Animation::LOOPED_FLAG_START : Animation::LOOPED_FLAG_END;
                }
                cur_time = Math::pingpong(cur_time, anim_size);
            }
            is_looping = true;
        }
        else if (anim->get_loop_mode() == Animation::LOOP_LINEAR)
        {
            if (!Math::is_zero_approx(anim_size))
            {
                if (prev_time >= 0 && cur_time < 0)
                {
                    looped_flag = node_backward ? Animation::LOOPED_FLAG_END : Animation::LOOPED_FLAG_START;
                }
                if (prev_time <= anim_size && cur_time > anim_size)
                {
                    looped_flag = node_backward ? Animation::LOOPED_FLAG_START : Animation::LOOPED_FLAG_END;
                }
                cur_time = Math::fposmod(cur_time, anim_size);
            }
            backward = false;
            is_looping = true;
        }
        else
        {
            if (cur_time < 0)
            {
                step += cur_time;
                cur_time = 0;
            }
            else if (cur_time > anim_size)
            {
                step += anim_size - cur_time;
                cur_time = anim_size;
            }
            backward = false;

            // If ended, don't progress animation. So set delta to 0.
            if (p_time > 0)
            {
                if (play_mode == PLAY_MODE_FORWARD)
                {
                    if (prev_time >= anim_size)
                    {
                        step = 0;
                    }
                }
                else
                {
                    if (prev_time <= 0)
                    {
                        step = 0;
                    }
                }
            }

            // Emit start & finish signal. Internally, the detections are the same for backward.
            // We should use call_deferred since the track keys are still being prosessed.
            // if (state->tree)
            {
                // // AnimationTree uses seek to 0 "internally" to process the first key of the animation, which is used as the start detection.
                // if (p_seek && !p_is_external_seeking && cur_time == 0)
                // {
                //     state->tree->call_deferred(SNAME("emit_signal"), "animation_started", animation);
                // }
                // // Finished.
                // if (prev_time < anim_size && cur_time >= anim_size)
                // {
                //     state->tree->call_deferred(SNAME("emit_signal"), "animation_finished", animation);
                // }
            }
        }

        if (!p_test_only)
        {
            if (play_mode == PLAY_MODE_FORWARD)
            {
                blend_animation(get_parameter("animation"), cur_time, step, p_seek, p_is_external_seeking, 1.0, looped_flag);
            }
            else
            {
                blend_animation(get_parameter("animation"), anim_size - cur_time, -step, p_seek, p_is_external_seeking, 1.0, looped_flag);
            }
        }
        set_parameter(get_parameter("timestamp"), cur_time);

        return is_looping ? 31540000 : anim_size - cur_time;
    }
    */

    std::vector<Vector3> bones_pos{};
    std::vector<Vector3> bones_vel{};
    std::vector<Quaternion> bones_rot{};
    std::vector<Vector3> bones_ang{};

    template<typename T>
    static inline T clampf(T x, T min, T max)
    {
        static_assert(std::is_arithmetic_v<T>,"Must be arithmetic");
        return x > max ? max : x < min ? min : x;
    }

    static inline Quaternion quat_abs(Quaternion q){
        return q.w < 0.0 ? -q : q;
    }

    static inline Vector3 quat_log(Quaternion q, float eps=1e-8f){
        real_t length = sqrt(q.x*q.x + q.y*q.y + q.z*q.z);
        if (length < eps)
        {
            return Vector3(q.x, q.y, q.z);
        }
        else
        {
            float halfangle = acosf(clampf(q.w, -1.0f, 1.0f));
            return halfangle * (Vector3(q.x, q.y, q.z) / length);
        }
    }

    static inline Vector3 quat_to_scaled_angle_axis(Quaternion q, float eps=1e-8f)
    {
        return 2.0f * quat_log(q, eps);
    }

    String pending_animation = "";    
    double pending_timestamp = 0.0;
    String current_animation = "";
    double current_time = 0.0;

        /// @brief Processing. Should manage blending to a new animation pose.
    /// @param p_time 
    /// @param p_seek 
    /// @param p_is_external_seeking 
    /// @param p_test_only 
    /// @return
    virtual double _process(double p_time, bool p_seek, bool p_is_external_seeking, bool p_test_only)
    {
        if(skeleton == nullptr || ap == nullptr)
        {
            return 0.0;
        }

        //u::prints("Velocity update");
        // Update velocities
        bones_pos.resize(skeleton->get_bone_count());
        bones_vel.resize(skeleton->get_bone_count());
        bones_rot.resize(skeleton->get_bone_count());
        bones_ang.resize(skeleton->get_bone_count());
        for(size_t index = 0; index < skeleton->get_bone_count(); ++index)
        {
            // Local pos
            Vector3 pos = skeleton->get_bone_pose_position(index) / skeleton->get_motion_scale(); 
            Quaternion rot = skeleton->get_bone_pose_rotation(index);
            Vector3 vel = (pos - bones_pos[index] ) / p_time;
            Vector3 ang = quat_to_scaled_angle_axis(
                quat_abs(rot * bones_rot[index].inverse())
            ) / p_time;
            bones_pos[index] = std::move(pos);
            bones_vel[index] = std::move(vel);
            bones_rot[index] = std::move(rot);
            bones_ang[index] = std::move(ang);
        }
        //u::prints("Velocity updated");

        if(!ap->has_animation(current_animation))
        {
            return 0.0;
        }
        Ref<Animation> anim = ap->get_animation(current_animation);
	    double anim_size = (double)anim->get_length();

        auto loop_flag = Animation::LoopedFlag::LOOPED_FLAG_NONE;

        current_time += p_time;
        if(pending)
        {
            if (current_time >= anim_size)
            {
                u::prints("Changing", current_animation, pending_animation);
                current_animation = pending_animation;
                pending_animation = "";
                current_time = pending_timestamp;
                pending_timestamp = 0.0f;
                anim = ap->get_animation(current_animation);
                anim_size = anim->get_length();
                pending = false;
            }
        }
        else {
            if (current_time >= anim_size && anim->get_loop_mode() == Animation::LoopMode::LOOP_LINEAR)
            {
                loop_flag = Animation::LoopedFlag::LOOPED_FLAG_END;
                current_time = Math::fposmod(current_time, anim_size);
            }
        }

        blend_animation(current_animation, current_time, p_time, p_seek, p_is_external_seeking, 1.0, loop_flag);
        return anim_size - current_time;
    }

    void request_pose(StringName p_next_anim, float p_next_timestamp)
    {
        ERR_FAIL_NULL(ap);
        ERR_FAIL_NULL(skeleton);

        if (pending == true && p_next_anim == pending_animation && abs(p_next_timestamp - current_time) < blend_time)
        {
            // Already transitioning to this poses.
            return;
        }
        if (pending == false && p_next_anim == current_animation && abs(p_next_timestamp - current_time) < blend_time)
        {
            // Already playing this poses.
            return;
        }

        //  States 
        //  Normal Pending
        //  -Normal should behave like animationnodeanimation
        //  -Pending check the current skeleton and the desired pose, and inertialize between the two.
        //   During the transition, it should be possible to switch the desired pose without problem.

        pending = true;

        // create transition animation if necessary
        Ref<AnimationLibrary> animlib = nullptr;
        if (! ap->has_animation_library("Inertialization")){
            animlib.instantiate();
            ap->add_animation_library("Inertialization",animlib);
            u::prints("Creating animation library for inertialization");
        }
        else {
            animlib = ap->get_animation_library("Inertialization");
        }
        ERR_FAIL_NULL(animlib);
        ERR_FAIL_COND(!ap->has_animation_library("Inertialization"));

        Ref<Animation> anim = nullptr;
        StringName anim_name = String("inert_") +  u::str(reinterpret_cast<std::uintptr_t>(skeleton));

        current_animation = String("Inertialization/") +anim_name;
        current_time = 0.0;
        pending_animation = p_next_anim;
        pending_timestamp = p_next_timestamp;

        String skel_path = skeleton->is_unique_name_in_owner() ? "%" + skeleton->get_name() : skeleton->get_owner()->get_path_to(skeleton,true);

        if(! animlib->has_animation(anim_name) ){
            anim.instantiate();
            animlib->add_animation(anim_name,anim);
            anim->set_length(blend_time);
            for (int b = 0; b < skeleton->get_bone_count(); ++b)
            {
                auto bone_path = skel_path + String(":") + skeleton->get_bone_name(b);
                auto local_tr = skeleton->get_bone_pose(b);
                
                int pos_track = anim->add_track(Animation::TrackType::TYPE_POSITION_3D);                
                int rot_track = anim->add_track(Animation::TrackType::TYPE_ROTATION_3D);
                
                anim->track_set_path(pos_track,bone_path);
                {
                    anim->track_insert_key(pos_track,0.0,Vector3());
                    anim->track_insert_key(pos_track,blend_time,Vector3());
                }
                anim->track_set_path(rot_track,bone_path);
                {
                    anim->track_insert_key(rot_track,0.0,Quaternion());
                    anim->track_insert_key(rot_track,blend_time,Quaternion());
                }
            }
            u::prints("Created animation for Skeleton :",anim_name,anim->get_track_count());
        }
        else {
            anim = animlib->get_animation(anim_name);
        }
        ERR_FAIL_NULL(anim);
        ERR_FAIL_COND(!animlib->has_animation(anim_name));

        // set transition animation to the current poses of the skeleton
        auto pending_anim = ap->get_animation(p_next_anim);
        for(int t = 0; t < anim->get_track_count(); ++t){
            const auto track_path = anim->track_get_path(t);
            const auto bone_name = track_path.get_subname(0);

            auto b = skeleton->find_bone(bone_name);
            if (b == -1){
                u::prints("Didn't find bone",bone_name,"path",track_path);
                continue;
            }
            else if (anim->track_get_type(t) == Animation::TrackType::TYPE_POSITION_3D)
            {
                anim->track_set_key_time(t,0,0.0);
                anim->track_set_key_value(t,0,skeleton->get_bone_pose_position(b) / skeleton->get_motion_scale());
                anim->track_set_key_time(t,1,blend_time);
                const auto future_pos_track = pending_anim->find_track(track_path, Animation::TrackType::TYPE_POSITION_3D);
                if (future_pos_track != -1)
                {
                    const auto future_pos = pending_anim->position_track_interpolate(future_pos_track, p_next_timestamp);
                    anim->track_set_key_value(t,1,future_pos);
                }
                else
                {
                    anim->track_set_key_value(t,1,skeleton->get_bone_pose_position(b)/ skeleton->get_motion_scale());
                }

                

            }
            else if (anim->track_get_type(t) == Animation::TrackType::TYPE_ROTATION_3D)
            {
                anim->track_set_key_time(t,0,0.0);
                anim->track_set_key_value(t,0,skeleton->get_bone_pose_rotation(b));
                anim->track_set_key_time(t,1,blend_time);
                const auto future_pos_track = pending_anim->find_track(track_path, Animation::TrackType::TYPE_ROTATION_3D);
                if (future_pos_track != -1)
                {
                    const auto future_rot = pending_anim->rotation_track_interpolate(future_pos_track, p_next_timestamp);
                    anim->track_set_key_value(t,1,future_rot.normalized());
                }
                else
                {
                    anim->track_set_key_value(t,1,skeleton->get_bone_pose_rotation(b).normalized());
                }
            }
        }
        anim->set_length(blend_time);
        return;
        // DEBUG. Remove after
        auto parentless = skeleton->get_parentless_bones();
        for(int i = 0; i < parentless.size();++i)
        {
            auto bone = parentless[i];
            auto bone_path = skel_path + String(":") + skeleton->get_bone_name(bone);
            auto pos_track = anim->find_track(bone_path,Animation::TrackType::TYPE_POSITION_3D);
            auto rot_track = anim->find_track(bone_path,Animation::TrackType::TYPE_ROTATION_3D);
            Array positions{};
            Array rotations{};
            Array keys{};
            if ( pos_track != -1)
                for(int j = 0; j < anim->track_get_key_count(pos_track);++j)
                {
                    positions.append(anim->track_get_key_value(pos_track,j));
                    keys.append(j);
                }
            if ( rot_track != -1)
                for(int j = 0; j < anim->track_get_key_count(rot_track);++j)
                {
                    rotations.append(anim->track_get_key_value(rot_track,j));
                }
            u::prints(bone_path,positions,rotations,keys);
        }
        // calculate and save the end pose

        // During the processing, the differences from the current pose to the desired is reduced using spring inertialization.

    }
    bool pending = false;

    virtual Array _get_parameter_list() const {
        Dictionary _skeleton {};
        _skeleton["name"] = "skeleton";
        _skeleton["class_name"] = StringName("Skeleton3D");
        _skeleton["type"] = Variant::OBJECT;
        _skeleton["hint"] = PROPERTY_HINT_NONE;
        _skeleton["hint_string"] = "";
        _skeleton["usage"] = PROPERTY_USAGE_DEFAULT;

        Dictionary _animplayer {};
        _animplayer["name"] = "animation_player";
        _animplayer["class_name"] = StringName("AnimationPlayer");
        _animplayer["type"] = Variant::OBJECT;
        _animplayer["hint"] = PROPERTY_HINT_NONE;
        _animplayer["hint_string"] = "";
        _animplayer["usage"] = PROPERTY_USAGE_DEFAULT;

        Dictionary requested_anim {};
        requested_anim["name"] = "animation";
        requested_anim["class_name"] = StringName();
        requested_anim["type"] = Variant::STRING;
        requested_anim["hint"] = PROPERTY_HINT_NONE;
        requested_anim["hint_string"] = String();
        requested_anim["usage"] = PROPERTY_USAGE_DEFAULT;

        Dictionary requested_timestamp {};
        requested_timestamp["name"] = "timestamp";
        requested_timestamp["class_name"] = StringName();
        requested_timestamp["type"] = Variant::FLOAT;
        requested_timestamp["hint"] = PROPERTY_HINT_NONE;
        requested_timestamp["hint_string"] = String();
        requested_timestamp["usage"] = PROPERTY_USAGE_DEFAULT;

        return Array::make(_skeleton,_animplayer,requested_anim,requested_timestamp);
    }

    virtual Variant _get_parameter_default_value(StringName param) const {
        if (param == StringName("animation"))
            return "";
        else if (param == StringName("timestamp"))
            return 0.0;
        else
            return Variant();
    }

    virtual bool _is_parameter_read_only(StringName param)const {
        return false;
    }


protected:
    static void _bind_methods()
    {
        ClassDB::bind_method( D_METHOD("set_blend_time" ,"value"), &AnimationNodeInertialization::set_blend_time); 
        ClassDB::bind_method( D_METHOD("get_blend_time" ), &AnimationNodeInertialization::get_blend_time); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"blend_time"), "set_blend_time", "get_blend_time");

        ClassDB::bind_method( D_METHOD("set_ap" ,"value"), &AnimationNodeInertialization::set_ap);
        ClassDB::bind_method( D_METHOD("get_ap" ), &AnimationNodeInertialization::get_ap);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"ap"), "set_ap", "get_ap");

        ClassDB::bind_method( D_METHOD("set_skeleton" ,"value"), &AnimationNodeInertialization::set_skeleton); 
        ClassDB::bind_method( D_METHOD("get_skeleton" ), &AnimationNodeInertialization::get_skeleton); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"skeleton"), "set_skeleton", "get_skeleton");
        // Functions

        ClassDB::bind_method(D_METHOD("request_pose","pending_anim_name","pending_time"),&AnimationNodeInertialization::request_pose);

    }
};