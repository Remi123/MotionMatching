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

    Skeleton3D* skeleton = nullptr;
    AnimationPlayer* ap = nullptr;

    GETSET(AnimationPlayer*, animationplayer);

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

    virtual double _process(double p_time, bool p_seek, bool p_is_external_seeking, bool p_test_only)
    {
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

    virtual void _init(){
        
    }

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
        if (param == StringName("request_animation"))
            return "";
        else if (param == StringName("request_timestamp"))
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

        // Functions

    }
};