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

    struct kform
    {
        Vector3 pos = Vector3(0, 0, 0); // Position
        Quaternion rot = Quaternion();  // Rotation
        Vector3 scl = Vector3(1, 1, 1); // Scale
        Vector3 vel = Vector3(0, 0, 0); // Linear Velocity
        Vector3 ang = Vector3(0, 0, 0); // Angular Velocity
        Vector3 svl = Vector3(0, 0, 0); // Scalar Velocity
    };

    HashMap<NodePath,kform> bones_kform{}, bones_offset{};

void create_animation_if_required(){
        // create transition animation if necessary
        if (! ap->has_animation_library(inertialization_animation_library_name)){
            inertialization_animation_library.instantiate();
            ap->add_animation_library(inertialization_animation_library_name,inertialization_animation_library);
            u::prints("Creating animation library",inertialization_animation_library_name);
        }

        ERR_FAIL_NULL(inertialization_animation_library);
        ERR_FAIL_COND(!ap->has_animation_library(inertialization_animation_library_name));

        String skel_path = skeleton->is_unique_name_in_owner() ? "%" + skeleton->get_name() : skeleton->get_owner()->get_path_to(skeleton,true);
        inertialization_animation_name = String("inert_") +  u::str(reinterpret_cast<std::uintptr_t>(skeleton));

        if(! inertialization_animation_library->has_animation(inertialization_animation_name) ){
            inertialization_animation.instantiate();
            inertialization_animation->set_length(0.16f);
            for (int b = 0; b < skeleton->get_bone_count(); ++b)
            {
                NodePath bone_path = skel_path + String(":") + skeleton->get_bone_name(b);
                auto local_tr = skeleton->get_bone_pose(b);
                
                int pos_track = inertialization_animation->add_track(Animation::TrackType::TYPE_POSITION_3D);                
                int rot_track = inertialization_animation->add_track(Animation::TrackType::TYPE_ROTATION_3D);
                
                inertialization_animation->track_set_path(pos_track,bone_path);
                {
                    inertialization_animation->position_track_insert_key(pos_track,0.0,Vector3());
                    inertialization_animation->track_set_interpolation_type(pos_track,Animation::InterpolationType::INTERPOLATION_NEAREST);
                }
                inertialization_animation->track_set_path(rot_track,bone_path);
                {
                    inertialization_animation->rotation_track_insert_key(rot_track,0.0,Quaternion());
                    inertialization_animation->track_set_interpolation_type(rot_track,Animation::InterpolationType::INTERPOLATION_NEAREST);

                }
                bones_kform[bone_path] = kform();
                bones_offset[bone_path] = kform();
            }
            u::prints("Created animation for Skeleton :",inertialization_animation_name,inertialization_animation->get_track_count());
            inertialization_animation_library->add_animation(inertialization_animation_name,inertialization_animation);
        }
        ERR_FAIL_NULL(inertialization_animation);
        auto anim_name = String(inertialization_animation_library_name) +'/'+inertialization_animation_name;
        ERR_FAIL_COND_MSG(!ap->has_animation(anim_name),"No animation named " + anim_name);
    }



    //--------------------------------------
    virtual double _attemped_process(double p_time, bool p_seek, bool p_is_external_seeking, bool p_test_only)
    {
        if(skeleton == nullptr || ap == nullptr )
        {
            return 0.0;
        }
        create_animation_if_required();
        if (pending_desired_anim == nullptr || inertialization_animation == nullptr)
        {
            return 0.0f;
        }
        auto anim_name = String(inertialization_animation_library_name) + '/' + inertialization_animation_name;
        inertialization_animation = ap->get_animation(anim_name);
        // update the currents kforms informations
        // inertialize
        pending_desired_time += p_time;
        for(int t = 0; t < inertialization_animation->get_track_count(); ++t )
        {
            // TODO Store track path in the pending to not always search for tracks
            if( inertialization_animation->track_get_type(t) == Animation::TrackType::TYPE_POSITION_3D)
            {                 
                auto desired_track = pending_desired_anim->find_track(inertialization_animation->track_get_path(t),Animation::TrackType::TYPE_POSITION_3D);
                if(desired_track != -1)
                {
                    Vector3 pos = pending_desired_anim->position_track_interpolate(desired_track,pending_desired_time);
                    // TODO pos must be inertialized
                    //inertialization_animation->track_set_key_value(t,0,pos);
                    inertialization_animation->position_track_insert_key(t,0.0,pos);
                }
                else {
                }
            }
            else if( inertialization_animation->track_get_type(t) == Animation::TrackType::TYPE_ROTATION_3D)
            {                 
                auto desired_track = pending_desired_anim->find_track(inertialization_animation->track_get_path(t),Animation::TrackType::TYPE_ROTATION_3D);
                if(desired_track != -1)
                {
                    Quaternion rot = pending_desired_anim->rotation_track_interpolate(desired_track,pending_desired_time);
                    // // TODO rot must be inertialized

                    inertialization_animation->rotation_track_insert_key(t,0.0,rot);
                   // Quaternion cur = inertialization_animation->rotation_track_interpolate(t,0.0f);
                   // inertialization_animation->rotation_track_insert_key(t,0.0,(cur * Quaternion(Vector3(0,1,0),u::deg_to_rad(20)).normalized()));
                } else {
                    // Quaternion cur = inertialization_animation->rotation_track_interpolate(t,0.0f);
                    // inertialization_animation->rotation_track_insert_key(t,0.0,(cur * Quaternion(Vector3(0,1,0),u::deg_to_rad(20)).normalized()));
                    //u::prints("not found rot track for",inertialization_animation->track_get_path(t));
                }           
            }
        }
        auto loop_flag =  Animation::LoopedFlag::LOOPED_FLAG_NONE;

        if(yes)
        {
            blend_animation(anim_name, 0.01, p_time, false, false, 1.0, loop_flag);
        }
        else {
            blend_animation(anim_name, 0.01, p_time, false, false, 1.0, loop_flag);
        }
        yes = !yes;
        inertialization_animation->emit_changed();
        inertialization_animation_library->emit_changed();
        return 0.016; pending_desired_anim->get_length() - pending_desired_time;
    }
    bool yes = true;
    String inertialization_animation_library_name = String("Inertialization");
    Ref<AnimationLibrary> inertialization_animation_library = nullptr;
    String inertialization_animation_name = "";
    Ref<Animation> inertialization_animation = nullptr;
    Ref<Animation> pending_desired_anim = nullptr;
    float pending_desired_time = 0.0f;


    

    String pending_animation = "";    
    double pending_timestamp = 0.0;
    String transition_animation = "";
    double transition_time = 0.0;

        /// @brief Processing. Should manage blending to a new animation pose.
    /// @param p_time 
    /// @param p_seek 
    /// @param p_is_external_seeking 
    /// @param p_test_only 
    /// @return
    virtual double _old_process(double p_time, bool p_seek, bool p_is_external_seeking, bool p_test_only)
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
            Vector3 ang = CritDampSpring::quat_to_scaled_angle_axis(
                CritDampSpring::quat_abs(rot * bones_rot[index].inverse())
            ) / p_time;
            bones_pos[index] = std::move(pos);
            bones_vel[index] = std::move(vel);
            bones_rot[index] = std::move(rot);
            bones_ang[index] = std::move(ang);
        }
        //u::prints("Velocity updated");

        if(!ap->has_animation(transition_animation))
        {
            return 0.0;
        }


        auto loop_flag = Animation::LoopedFlag::LOOPED_FLAG_NONE;

        
        if(pending)
        {
            transition_time += p_time;
            Ref<Animation> anim = ap->get_animation(transition_animation);
	        double anim_size = (double)anim->get_length();
            blend_animation(transition_animation, transition_time, p_time, p_seek, p_is_external_seeking, 1.0, loop_flag);
            if (transition_time >= anim_size)
            {
                u::prints("Changing", transition_animation, pending_animation);
                // TODO REWORK how we transition
                transition_time = 0.0f;
                pending = false;
                return 0.0f;
            }
            return anim_size - transition_time;
        }
        else {
            pending_timestamp += p_time;
            Ref<Animation> anim = ap->get_animation(pending_animation);
	        double anim_size = (double)anim->get_length();
            if (pending_timestamp >= anim_size && anim->get_loop_mode() == Animation::LoopMode::LOOP_LINEAR)
            {
                loop_flag = Animation::LoopedFlag::LOOPED_FLAG_END;
                pending_timestamp = Math::fposmod(pending_timestamp, anim_size);
            }
            blend_animation(pending_animation, pending_timestamp, p_time, p_seek, p_is_external_seeking, 1.0, loop_flag);
            return anim_size - pending_timestamp;
        }


    }

    void request_pose(StringName p_next_anim, float p_next_timestamp)
    {
        ERR_FAIL_NULL(ap);
        ERR_FAIL_NULL(skeleton);
        ERR_FAIL_COND_MSG(!ap->has_animation(p_next_anim),"No animation named " + p_next_anim);

        // if (pending == true && p_next_anim == pending_animation && abs(p_next_timestamp - transition_time) < 0.25)
        // {
        //     // Already transitioning to this poses.
        //     return;
        // }
        // if (pending == false && p_next_anim == transition_animation && abs(p_next_timestamp - transition_time) < 0.25)
        // {
        //     // Already playing this poses.
        //     return;
        // }

        //  States 
        //  Normal Pending
        //  -Normal should behave like animationnodeanimation
        //  -Pending check the current skeleton and the desired pose, and inertialize between the two.
        //   During the transition, it should be possible to switch the desired pose without problem.

        pending = true;
        pending_desired_anim = ap->get_animation(p_next_anim);
        pending_desired_time = p_next_timestamp;
        // TODO : Should inertialize_transition and update offset
        auto a = ap->get_animation(inertialization_animation_library_name + '/' + inertialization_animation_name);
        auto t = a->find_track("%GeneralSkeleton:Hips",Animation::TrackType::TYPE_ROTATION_3D);
        auto rot = a->rotation_track_interpolate(t,0.0);
        u::prints("Root rot",rot);
        return;

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

        transition_animation = String("Inertialization/") +anim_name;
        transition_time = 0.0;
        pending_animation = p_next_anim;
        pending_timestamp = p_next_timestamp;

        String skel_path = skeleton->is_unique_name_in_owner() ? "%" + skeleton->get_name() : skeleton->get_owner()->get_path_to(skeleton,true);
        auto duration = CritDampSpring::halflife_to_duration(halflife) + 0.16f;


        if(! animlib->has_animation(anim_name) ){
            anim.instantiate();
            animlib->add_animation(anim_name,anim);
            anim->set_length(duration);
            for (int b = 0; b < skeleton->get_bone_count(); ++b)
            {
                auto bone_path = skel_path + String(":") + skeleton->get_bone_name(b);
                auto local_tr = skeleton->get_bone_pose(b);
                
                int pos_track = anim->add_track(Animation::TrackType::TYPE_POSITION_3D);                
                int rot_track = anim->add_track(Animation::TrackType::TYPE_ROTATION_3D);
                
                anim->track_set_path(pos_track,bone_path);
                {
                    // anim->track_insert_key(pos_track,0.0,Vector3());
                    // anim->track_insert_key(pos_track,blend_time,Vector3());
                }
                anim->track_set_path(rot_track,bone_path);
                {
                    // anim->track_insert_key(rot_track,0.0,Quaternion());
                    // anim->track_insert_key(rot_track,blend_time,Quaternion());
                }
            }
            u::prints("Created animation for Skeleton :",anim_name,anim->get_track_count());
        }
        else {
            anim = animlib->get_animation(anim_name);
            for(auto i = 0; i < anim->get_track_count(); ++i)
            {                
                for(auto j = 0; j < anim->track_get_key_count(i); ++j )
                {
                    anim->track_remove_key(i,j);
                }
            }
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
                Vector3 curr_bone_pos = skeleton->get_bone_pose_position(b) / skeleton->get_motion_scale();
                Vector3 fut_bone_pos = curr_bone_pos;
                Vector3 curr_bone_vel = bones_vel[b];
                Vector3 fut_bone_vel = Vector3();
                
                const auto future_pos_track = pending_anim->find_track(track_path, Animation::TrackType::TYPE_POSITION_3D);
                if (future_pos_track != -1)
                {
                    fut_bone_pos = pending_anim->position_track_interpolate(future_pos_track, p_next_timestamp);
                    fut_bone_vel = (pending_anim->position_track_interpolate(future_pos_track, p_next_timestamp + 0.16f) - fut_bone_pos) / 0.016f;
                    Vector3 offset_x{}, offset_v{};
                    Vector3 in_x{fut_bone_pos}, in_v{fut_bone_vel};
                    Vector3 out_x, out_v;
                    CritDampSpring::inertialize_transition(offset_x,offset_v,curr_bone_pos,curr_bone_vel,fut_bone_pos,fut_bone_vel);
                    // Let's inertialize

                    for(auto delta = 0.0f; delta < duration  -  0.16f; delta += 0.16f)
                    {
                        CritDampSpring::inertialize_update(out_x,out_v,offset_x,offset_v,in_x,in_v,halflife,0.16f);
                        anim->position_track_insert_key(t,delta,out_x);
                    }
                    anim->position_track_insert_key(t,duration,fut_bone_pos);
                }
                else
                {
                    anim->position_track_insert_key(t,0.0,curr_bone_pos);
                    anim->position_track_insert_key(t,duration,curr_bone_pos);
                }
                anim->track_set_interpolation_type(t,Animation::InterpolationType::INTERPOLATION_LINEAR);


            }
            else if (anim->track_get_type(t) == Animation::TrackType::TYPE_ROTATION_3D)
            {
                Quaternion curr_bone_rot = skeleton->get_bone_pose_rotation(b);
                Quaternion fut_bone_rot = curr_bone_rot;
                Vector3 curr_bone_ang = bones_ang[b];
                Vector3 fut_bone_ang = Vector3();
                const auto future_pos_track = pending_anim->find_track(track_path, Animation::TrackType::TYPE_ROTATION_3D);
                if (future_pos_track != -1)
                {
                    fut_bone_rot = pending_anim->rotation_track_interpolate(future_pos_track, p_next_timestamp);
                    fut_bone_ang = CritDampSpring::quat_to_scaled_angle_axis(
                        CritDampSpring::quat_abs(pending_anim->rotation_track_interpolate(future_pos_track, p_next_timestamp + 0.16f) * fut_bone_rot.inverse())
                    ) / 0.16f;
                    
                    Quaternion offset_x{}; Vector3 offset_v{};
                    Quaternion in_x{fut_bone_rot};Vector3 in_v{fut_bone_ang}; 
                    Quaternion out_x; Vector3 out_v;
                    CritDampSpring::inertialize_transition(offset_x,offset_v,curr_bone_rot,curr_bone_ang,fut_bone_rot,fut_bone_ang);
                    // Let's inertialize
                    for(auto delta = 0.0f; delta < duration -  0.16f; delta += 0.16f)
                    {
                        CritDampSpring::inertialize_update(out_x,out_v,offset_x,offset_v,in_x,in_v,halflife,0.16f);
                        anim->rotation_track_insert_key(t,delta,out_x);
                    }
                    anim->rotation_track_insert_key(t,duration,fut_bone_rot);
                }
                else
                {
                    anim->rotation_track_insert_key(t,0.0,curr_bone_rot);
                    anim->rotation_track_insert_key(t,duration,curr_bone_rot);
                }
                anim->track_set_interpolation_type(t,Animation::InterpolationType::INTERPOLATION_LINEAR);

            }
        }
        anim->set_length(duration);
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

    GETSET(float,halflife,0.2);

protected:
    static void _bind_methods()
    {
        ClassDB::bind_method( D_METHOD("set_halflife" ,"value"), &AnimationNodeInertialization::set_halflife); 
        ClassDB::bind_method( D_METHOD("get_halflife" ), &AnimationNodeInertialization::get_halflife); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"halflife"), "set_halflife", "get_halflife");

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