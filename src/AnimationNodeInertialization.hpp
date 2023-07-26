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

    bool backward = false;
	enum PlayMode {
		PLAY_MODE_FORWARD,
		PLAY_MODE_BACKWARD
	};
    PlayMode play_mode = PLAY_MODE_FORWARD;

    /// @brief Processing. Should manage blending to a new animation pose.
    /// @param p_time 
    /// @param p_seek 
    /// @param p_is_external_seeking 
    /// @param p_test_only 
    /// @return 
    virtual double _process(double p_time, bool p_seek, bool p_is_external_seeking, bool p_test_only)
    {
        return 0.0f;
        if ( ap == nullptr && get_parameter("animation_player") != Variant())
        {
            ap = Object::cast_to<AnimationPlayer>(get_parameter("animation_player"));
            u::prints(ap->get_path());
        }
        if ( skeleton == nullptr && get_parameter("skeleton") != Variant())
        {
            skeleton = Object::cast_to<Skeleton3D>(get_parameter("skeleton"));
            u::prints(skeleton->get_path());
        }
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

    String current_animation = String("");

    void request_pose(StringName pending_anim_name, float pending_time)
    {
        // ERR_FAIL_COND_MSG(get_parameter("animation_player") != Variant(), "Animation Player isn't set");
        // ap = Object::cast_to<AnimationPlayer>((Object*)get_parameter("animation_player"));
        ERR_FAIL_NULL(ap);
        // ERR_FAIL_COND_MSG(get_parameter("skeleton") != Variant(), "Skeleton isn't set");
        // skeleton = Object::cast_to<Skeleton3D>(get_parameter("skeleton"));
        ERR_FAIL_NULL(skeleton);


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
        }
        else {
            animlib = ap->get_animation_library("Inertialization");
        }
        ERR_FAIL_NULL(animlib);
        ERR_FAIL_COND(!ap->has_animation_library("Inertialization"));
        u::prints("Animation Library set");

        Ref<Animation> anim = nullptr;
        StringName anim_name = String("inert_") +  u::str(reinterpret_cast<std::uintptr_t>(skeleton));
        u::prints(anim_name);

        String skel_path = skeleton->get_owner()->get_path_to(skeleton,true);
        if (skeleton->is_unique_name_in_owner())
        {
            skel_path = "%" + skeleton->get_name();
        }

        if(! animlib->has_animation(anim_name) ){
            anim.instantiate();
            animlib->add_animation(anim_name,anim);

            u::prints("SkeletonPath",skel_path);
            for (int b = 0; b < skeleton->get_bone_count(); ++b)
            {
                auto bone_path = skel_path + String(":") + skeleton->get_bone_name(b);
                auto local_tr = skeleton->get_bone_pose(b);
                
                int pos_track = anim->add_track(Animation::TrackType::TYPE_POSITION_3D);                
                int rot_track = anim->add_track(Animation::TrackType::TYPE_ROTATION_3D);

                anim->track_set_path(pos_track,bone_path);
                anim->track_set_path(rot_track,bone_path);
            }
        }
        else {
            anim = animlib->get_animation(anim_name);
        }
        ERR_FAIL_NULL(anim);
        ERR_FAIL_COND(!animlib->has_animation(anim_name));
        u::prints(anim_name,"tracks",anim->get_track_count());

        // set transition animation to the current poses of the skeleton
        auto pending_anim = ap->get_animation(pending_anim_name);
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
                anim->track_insert_key(t, 0.0, skeleton->get_bone_pose_position(b));
                const auto future_pos_track = pending_anim->find_track(track_path, Animation::TrackType::TYPE_POSITION_3D);
                if (future_pos_track != -1)
                {
                    const auto future_pos = pending_anim->position_track_interpolate(future_pos_track, pending_time);
                    anim->position_track_insert_key(t, blend_time, future_pos);
                }
            }
            else if (anim->track_get_type(t) == Animation::TrackType::TYPE_ROTATION_3D)
            {
                anim->track_insert_key(t, 0.0, skeleton->get_bone_pose_rotation(b));
                const auto future_rot_track = pending_anim->find_track(track_path, Animation::TrackType::TYPE_ROTATION_3D);
                if (future_rot_track != -1)
                {
                    const auto future_rot = pending_anim->rotation_track_interpolate(future_rot_track, pending_time);
                    anim->rotation_track_insert_key(t,blend_time,future_rot);
                }
            }
        }

        auto parentless = skeleton->get_parentless_bones();
        for(int i = 0; i < parentless.size();++i)
        {
            auto bone = parentless[i];
            auto bone_name = skeleton->get_bone_name(bone);
            auto bone_path = skel_path + String(":") + bone_name;
            auto pos_track = anim->find_track(bone_path,Animation::TrackType::TYPE_POSITION_3D);
            auto rot_track = anim->find_track(bone_path,Animation::TrackType::TYPE_ROTATION_3D);
            Array positions{};
            Array rotations{};
            if ( pos_track != -1)
                for(int j = 0; j < anim->track_get_key_count(pos_track);++j)
                {
                    positions.append(anim->track_get_key_value(pos_track,j));
                }
            if ( rot_track != -1)
                for(int j = 0; j < anim->track_get_key_count(rot_track);++j)
                {
                    rotations.append(anim->track_get_key_value(rot_track,j));
                }
            u::prints(bone_path,positions,rotations);
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