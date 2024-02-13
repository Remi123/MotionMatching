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

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/curve3d.hpp>

#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/vector3.hpp"

#include <godot_cpp/classes/animation_root_node.hpp>

#include <KForm.hpp>
#include <Spring.hpp>
#include <numeric>

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
    public:
    using u = godot::UtilityFunctions;

    enum Motion_Tags{
        NoTag = 0,
        StartUp,
        Active,
        Recovery        
    };


    GETSET(Motion_Tags,motion_tag);
    GETSET(TypedArray<Transform3D>,transforms_queue);

    kforms bones_kform{0}, bones_offset{0};

    float default_halflife = 0.1f;
    GETSET(float,halflife,0.1f);
    NodePath skeleton_path{};
    Skeleton3D* _skeleton = nullptr;

    int32_t root_bone_id = -1;

    Ref<Animation> pending_desired_anim = nullptr;
    float pending_desired_time = 0.0f;

    virtual void _ready() override
    {
        AnimationPlayer::_ready();
        u::prints("MMAnimationPlayer Init",u::str(skeleton_path));
        
        if(Engine::get_singleton()->is_editor_hint())
        {
            return;
        }
        warpXYZ = Ref<Curve3D>();
        warpR = Ref<Curve3D>();

        default_halflife = halflife;
        skeleton_path = NodePath(get_root_motion_track().get_concatenated_names());
        _skeleton = get_node<Skeleton3D>(NodePath(skeleton_path));
        ERR_FAIL_NULL(_skeleton);

        _skeleton->reset_bone_poses();
        inertialize_reset();
        root_bone_id = _skeleton->find_bone(get_root_motion_track().get_concatenated_subnames());
        connect("animation_finished",Callable(this,"_on_anim_finish"));
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
            bones_kform.scl[b] = _skeleton->get_bone_pose_scale(b);

            bones_offset.reset(b);
        }
    }

    void request_pose(StringName p_animation_name, float p_time = 0.0f,float new_halflife = -1.0f)
    {
        if ( new_halflife > 0.0f)
        {
            set_halflife(new_halflife);
        }
        else {
            set_halflife(default_halflife);
        }
        last_anim = p_animation_name;
        last_timestamp = p_time;
        stop();
    }

#pragma region  MotionWarping
    GETSET(Signal,mw_target_update);
    GETSET(PackedStringArray,section_tracks);

    /* Motion Warping
    MW is a kind of additive animation, but is mostly applied on the root bone.
    Look into Esoterica, the goal is to create a series of curves
    The offset position is calculated from the current position to the target position, relative to a bone (default is the root bone).
    Step 0: 
        Verify that Root Track, Root node and other dependancies are ok
    Step 1: 
        Calculate Total displacement
    Step 2:
        Analyze the animation for sections of time where you can modify the alignment and displacement.
    */

    bool generate_warping_info(Ref<Animation> anim,real_t start_time, String bone_name , std::vector<godot::Transform3D>& targets)
    {
        for(int i = 0; i < section_tracks.size();++i)
        {

        }
        trs_warping.clear(); trs_root.clear();
        trs_warping.reserve(anim->get_length() / anim->get_step());
        trs_root.reserve(anim->get_length() / anim->get_step());

        for(double t = 0.0; t < anim->get_length(); t += anim->get_step() )
        {
            trs_root.push_back((Transform3D)kform(_skeleton,anim,t,bone_name,get_root_motion_track().get_concatenated_subnames()));
        }

        return true;
    }

    std::vector<Transform3D> trs_warping{},trs_root{};
    virtual void request_motion_warping(StringName p_animation_name, real_t start_time = real_t(0.0), String bone_name = "", TypedArray<godot::Transform3D> global_transforms = {}, float new_halflife = -1.0f)
    {
        // if global_transforms is empty, this means that we don't create offsets. Just play the anim.
        if (global_transforms.is_empty())
        {
            request_animation(p_animation_name,start_time,new_halflife);
            return;
        }
        // const String mmplayer_path = is_unique_name_in_owner() ? String("%") + get_name() : get_name();
        // const String mw_path = mmplayer_path + String(":motion_tag");
        // Step 0
        auto* root_node = get_node<Node3D>(get_root());
        _skeleton = get_node<Skeleton3D>(NodePath(skeleton_path));
        Ref<Animation> p_animation = get_animation(p_animation_name);
        ERR_FAIL_NULL_MSG(p_animation,"Animation name isn't in the list of animations");
        ERR_FAIL_NULL_MSG(root_node,"RootNode Not Setup or not a Node3D");
        ERR_FAIL_NULL_MSG(_skeleton,"Skeleton Not Setup or not a Skeleton3D");
        const auto motion_scale = _skeleton->get_motion_scale();

        // Step 1
        // Get root node global position
        Transform3D const start_tr = root_node->get_global_transform();
        // Change targets to be relative to the global position of the root node.
        std::vector<Transform3D> targets{};
        for(int i = 0; i < global_transforms.size();++i)
        {
            targets.push_back(start_tr.inverse() * (Transform3D)global_transforms[i]);
        }

        if(generate_warping_info(p_animation,start_time,bone_name,targets))
        {

        }
        


        int const root_id = _skeleton->find_bone(get_root_motion_track().get_concatenated_subnames());
        Transform3D const bone_rest = _skeleton->get_bone_rest(root_id);
        Vector3 pos{bone_rest.origin},scl(bone_rest.basis.get_scale()); Quaternion rot{bone_rest.get_basis().get_rotation_quaternion()};       



        // Get root motion transforms
        for(double t = 0.0; t < p_animation->get_length(); t += p_animation->get_step() )
        {
            Transform3D tr{bone_rest};
            if(int pos_track=p_animation->find_track(get_root_motion_track(),Animation::TrackType::TYPE_POSITION_3D); pos_track != -1)
            {
                tr.origin = p_animation->position_track_interpolate(pos_track,t);
            }
            if(int rot_track=p_animation->find_track(get_root_motion_track(),Animation::TrackType::TYPE_ROTATION_3D); rot_track != -1)
            {
                tr.basis.set_quaternion(p_animation->rotation_track_interpolate(rot_track,t));
            }
            trs_root.emplace_back(std::move(tr));
        }
    }

#pragma endregion

    virtual bool request_animation(StringName p_animation_name, float p_time = 0.0f,float new_halflife = -1.0f)
    {
        _skeleton = get_node<Skeleton3D>(NodePath(skeleton_path));
        ERR_FAIL_NULL_V(_skeleton,false);
        const auto motion_scale = _skeleton->get_motion_scale();
        auto p_animation = get_animation(p_animation_name);

        ERR_FAIL_NULL_V(p_animation,false);
        bones_kform.reserve(_skeleton->get_bone_count());
        bones_offset.reserve(_skeleton->get_bone_count());
        
        if ( new_halflife > 0.0f)
        {
            set_halflife(new_halflife);
        }
        else {
            set_halflife(default_halflife);
        }
        last_anim = p_animation_name;
        last_timestamp = p_time;

        auto root_id = -1;

        const double delta = 0.016;

        p_time = u::clampf(p_time,0.0,p_animation->get_length()-halflife);
        const auto future_time = u::clampf(p_time+delta,0.0,p_animation->get_length());
        for (auto bone_id = 0; bone_id < _skeleton->get_bone_count(); ++bone_id)
        {
            const Transform3D bone_rest = _skeleton->get_bone_rest(bone_id);
            const String bone_path = u::str(skeleton_path) + String(":") + _skeleton->get_bone_name(bone_id);

            auto track_pos = p_animation->find_track(bone_path, Animation::TrackType::TYPE_POSITION_3D);
            auto track_rot = p_animation->find_track(bone_path, Animation::TrackType::TYPE_ROTATION_3D);

            //POSITION 3D
                Vector3 desired_position = bones_kform.pos[bone_id] ,// _skeleton->get_bone_pose_position(bone_id),
                        desired_linear_vel = bones_kform.vel[bone_id]; // Vector3();
                if (track_pos != -1)
                {
                    desired_position = p_animation->position_track_interpolate(track_pos, p_time)* motion_scale ;
                    desired_linear_vel = ((p_animation->position_track_interpolate(track_pos, future_time)* motion_scale)- desired_position) / abs(future_time - p_time);
                }

            //ROTATION 3D
                Quaternion desired_rotation = bones_kform.rot[bone_id] ;//_skeleton->get_bone_pose_rotation(bone_id);
                Vector3 desired_angular_vel = bones_kform.ang[bone_id] ;//Vector3{};
                if ( track_rot != -1)
                {
                    desired_rotation = p_animation->rotation_track_interpolate(track_rot, p_time).normalized();
                    Quaternion r1 = p_animation->rotation_track_interpolate(track_rot, future_time).normalized();
                    desired_angular_vel = Spring::quat_differentiate_angular_velocity(r1,desired_rotation,abs(future_time - p_time)).normalized();
                }


            // ROOT BONE
            // Root bone have a special process
            if (NodePath(bone_path) ==  get_root_motion_track())
            {
                auto rooting = desired_rotation.inverse();
                desired_position = Vector3();
                desired_linear_vel = rooting.xform(desired_linear_vel);
                desired_rotation = Quaternion();
                //desired_angular_vel doesn't change
            }
            
            // Offset are calculated Between current pos of the bone and the desired pose
            Spring::inertialize_transition(bones_offset.pos[bone_id], bones_offset.vel[bone_id],
                                        bones_kform.pos[bone_id], bones_kform.vel[bone_id],
                                        desired_position, desired_linear_vel);
            Spring::inertialize_transition(bones_offset.rot[bone_id], bones_offset.ang[bone_id], // Offset are calculated...
                                                    bones_kform.rot[bone_id], bones_kform.ang[bone_id],   // Between current rot of the bone...
                                                    desired_rotation, desired_angular_vel);                          // and the desired pose
        }
 

        play(p_animation_name);
        seek(p_time,false);

        return true;
    }


    void _on_anim_finish(StringName p_animation_name)
    {
        set_halflife(default_halflife);
        auto p_animation = get_animation(p_animation_name);
        auto p_time = p_animation->get_length();

        last_anim = p_animation_name;
        last_timestamp = p_time;

        return;
    }

    StringName last_anim = "";
    double last_timestamp = 0.0;

    virtual void _physics_process(double _delta) override
    {
        if(Engine::get_singleton()->is_editor_hint() || last_anim.is_empty())
        {
            return;
        }


        // Let's hope this is done after AnimationMixer's _notification
        Ref<Animation> animation = get_current_animation().is_empty() ? nullptr : get_animation(get_current_animation());

        String skel_path = get_root_motion_track().get_concatenated_names();
        const auto motion_scale = _skeleton->get_motion_scale();

        const auto current_time = animation == nullptr ? 0.0 : u::clampf(get_current_animation_position(), 0.0, animation->get_length());
        const auto future_time =  animation == nullptr ? _delta : u::clampf(current_time + _delta, 0.0, animation->get_length());
        const auto delta_diff = abs(future_time-current_time);

        for (auto bone_id = 0; bone_id < _skeleton->get_bone_count(); ++bone_id)
        {
            const String bone_path = skel_path + String(":") + _skeleton->get_bone_name(bone_id);

            kform desired {};

            if(get_current_animation().is_empty())
            {
                animation = get_animation(last_anim);

                desired = bones_kform[bone_id];
                
                const Transform3D bone_rest = _skeleton->get_bone_rest(bone_id).scaled_local(Vector3(1,1,1) * motion_scale);
                const String bone_path = u::str(skeleton_path) + String(":") + _skeleton->get_bone_name(bone_id);

                auto track_pos = animation->find_track(bone_path, Animation::TrackType::TYPE_POSITION_3D);
                if(track_pos != -1)
                {
                    desired.pos = animation->position_track_interpolate(track_pos, last_timestamp) * motion_scale;
                }
                auto track_rot = animation->find_track(bone_path, Animation::TrackType::TYPE_ROTATION_3D);
                if(track_rot != -1)
                {
                    desired.rot = animation->rotation_track_interpolate(track_rot, last_timestamp);
                }

                if (bone_id == root_bone_id)
                {
                    desired.pos = Vector3();
                    desired.rot = Quaternion();
                }

                Spring::_simple_spring_damper_exact(
                    bones_kform.pos[bone_id],bones_kform.vel[bone_id]
                    ,desired.pos,halflife,_delta
                );
                Spring::_simple_spring_damper_exact(
                    bones_kform.rot[bone_id],bones_kform.ang[bone_id]
                    ,desired.rot,halflife,_delta
                );
                Spring::_decay_spring_damper_exact(
                    bones_offset.pos[bone_id],bones_offset.vel[bone_id],
                    halflife,_delta
                );
                Spring::_decay_spring_damper_exact(
                    bones_offset.rot[bone_id],bones_offset.ang[bone_id],
                    halflife,_delta
                );
            }
            else
            {
                desired.pos = _skeleton->get_bone_pose_position(bone_id); // Have MotionScale
                desired.vel = Vector3();
                desired.rot = _skeleton->get_bone_pose_rotation(bone_id);
                desired.ang = Vector3();


                const int track_pos = animation->find_track(bone_path,Animation::TrackType::TYPE_POSITION_3D);
                const int track_rot = animation->find_track(bone_path,Animation::TrackType::TYPE_ROTATION_3D);
                if(track_pos != -1)
                {
                    desired.pos = animation->position_track_interpolate(track_pos, current_time) * motion_scale;
                    desired.vel = u::is_zero_approx(delta_diff) ? Vector3() : ((animation->position_track_interpolate(track_pos, future_time)* motion_scale) - desired.pos) / delta_diff;                
                }
                if(track_pos != -1)
                {
                    desired.rot = animation->rotation_track_interpolate(track_rot, current_time);
                    desired.ang = u::is_zero_approx(delta_diff) ? Vector3() : Spring::quat_differentiate_angular_velocity( animation->rotation_track_interpolate(track_rot, future_time), desired.rot,delta_diff);
                }
 
                if (bone_id == root_bone_id)
                {
                    desired.vel = desired.rot.xform_inv(desired.vel); 
                    // desired_angular = desired_rotation.xform_inv(desired_angular);
                    desired.pos = Vector3();
                    desired.rot = Quaternion();
                }
                Spring::inertialize_update(
                    bones_kform.pos[bone_id], bones_kform.vel[bone_id],   // Current pos of the bone
                    bones_offset.pos[bone_id],bones_offset.vel[bone_id], // Current Offset pos, get reduced every frame
                    desired.pos, desired.vel,                                         // Desired position from the animation
                    halflife,                                                           // Stats on how the offset decay
                    _delta * get_speed_scale());                                         // delta time between frames
                Spring::inertialize_update(
                    bones_kform.rot[bone_id],bones_kform.ang[bone_id],   // Current rot of the bone
                    bones_offset.rot[bone_id],bones_offset.ang[bone_id], // Current Offset rot, get reduced every frame
                    desired.rot, desired.ang,                                         // Desired rotation from the animation
                    halflife,                                                           // Stats on how the offset decay
                    _delta * get_speed_scale());                                         // delta time between frames
            }

            _skeleton->set_bone_pose_position(bone_id,bones_kform.pos[bone_id]);
            _skeleton->set_bone_pose_rotation(bone_id,bones_kform.rot[bone_id]);

        }
    }

    Dictionary get_local_bone_info(StringName bone_name)
    {
        ERR_FAIL_COND_V(_skeleton == nullptr, {});
        auto id = _skeleton->find_bone(bone_name);
        ERR_FAIL_COND_V_MSG(id == -1, {}, "Bone " + bone_name + " doesn't exist in skeleton");
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

    // Assume bone_id is correct
    kform get_bone_global_kform(int bone_id)
    {
        std::vector<int> parents_id{bone_id};
        auto tmp_p = bone_id;
        while (_skeleton->get_bone_parent(tmp_p) != -1)
        {
            auto new_parent = _skeleton->get_bone_parent(tmp_p);
            parents_id.push_back(new_parent);
            tmp_p = new_parent;
        }
        const auto motion_scale = _skeleton->get_motion_scale();
        return std::accumulate(parents_id.rbegin(), parents_id.rend(), kform{},
                               [this, motion_scale](const kform &acc, int i)
                               {
                                   auto info = bones_kform[i];
                                   //    info.pos *= motion_scale;
                                   return acc * info;
                               });
    }

    kform get_bone_model_kform(int bone_id)
    {
        ERR_FAIL_COND_V(_skeleton == nullptr, {});
        if (bone_id == root_bone_id)
        {
            return kform{};
        }
        std::vector<int> parents_id{};
        auto tmp_p = bone_id;

        do
        {
            parents_id.push_back(tmp_p);
            tmp_p = _skeleton->get_bone_parent(tmp_p);
        } while (tmp_p != -1 && tmp_p != root_bone_id);

        const auto motion_scale = _skeleton->get_motion_scale();
        return std::accumulate(parents_id.rbegin(), parents_id.rend(), kform{},
                               [this, motion_scale](const kform &acc, int i)
                               {
                                   auto info = bones_kform[i];
                                   //    info.pos *= motion_scale;
                                   return acc * info;
                               });
    }

    kform get_bone_info(StringName bone_name, kform::Space space)
    {
        ERR_FAIL_COND_V(_skeleton == nullptr, {});
        auto id = _skeleton->find_bone(bone_name);
        ERR_FAIL_COND_V_MSG(id == -1, {}, "Bone " + bone_name + " doesn't exist in skeleton");
        if (space == kform::Space::Local)
        {
            return bones_kform[id];
        }
        else if (space == kform::Space::Global)
        {
            return get_bone_global_kform(id);
        }
        else if (space == kform::Space::RootMotion)
        {
            kform global = get_bone_global_kform(id);
            return bones_kform[root_bone_id] / global;
        }
        else if (space == kform::Space::Model)
        {
            return get_bone_model_kform(id);
        }
        return kform{};
    }

    Dictionary get_global_bone_info(StringName bone_name)
    {
        ERR_FAIL_COND_V(_skeleton == nullptr, {});
        auto id = _skeleton->find_bone(bone_name);
        ERR_FAIL_COND_V_MSG(id == -1, {}, "Bone " + bone_name + " doesn't exist in skeleton");
        kform global = get_bone_info(bone_name, kform::Space::Global);

        Dictionary result = Dictionary{};
        result["position"] = global.pos;
        result["linear_vel"] = global.vel;
        result["rotation"] = global.rot;
        result["angular_vel"] = global.ang;
        result["scale"] = global.scl;
        result["scalar_vel"] = global.svl;
        return result;
    }

    Dictionary get_model_bone_info(StringName bone_name)
    {
        ERR_FAIL_COND_V(_skeleton == nullptr, {});
        auto id = _skeleton->find_bone(bone_name);
        ERR_FAIL_COND_V_MSG(id == -1, {}, "Bone " + bone_name + " doesn't exist in skeleton");
        kform global = get_bone_info(bone_name, kform::Space::Model);

        Dictionary result = Dictionary{};
        result["position"] = global.pos;
        result["linear_vel"] = global.vel;
        result["rotation"] = global.rot;
        result["angular_vel"] = global.ang;
        result["scale"] = global.scl;
        result["scalar_vel"] = global.svl;
        return result;
    }

    Ref<Curve3D> warpXYZ;
    Ref<Curve3D> warpR; // ScaledAngleAxis

    Vector3 get_root_motion_velocity()
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        else if(get_current_animation().is_empty())
        {
            return bones_kform.vel[root_bone_id] * get_speed_scale();
        }
        auto warp_vel = warpXYZ->sample_baked(get_current_animation_position()/get_current_animation_length() * warpXYZ->get_baked_length())
        - warpXYZ->sample_baked((get_current_animation_position() - 0.016f)/get_current_animation_length() * warpXYZ->get_baked_length())
        / 0.016f;
        return bones_kform.vel[root_bone_id] * get_speed_scale() + warp_vel;
    }
    Quaternion get_root_motion_angular(float delta)
    {
        if (root_bone_id < 0)
        {
            return {};
        }
        return Spring::quat_from_scaled_angle_axis(bones_kform.ang[root_bone_id]*delta*get_playing_speed());
    }


    protected:
    static void _bind_methods()
    {
        BIND_ENUM_CONSTANT(NoTag);
        BIND_ENUM_CONSTANT(StartUp);
        BIND_ENUM_CONSTANT(Active);
        BIND_ENUM_CONSTANT(Recovery);
        ClassDB::bind_method( D_METHOD("set_motion_tag" ,"value"), &MMAnimationPlayer::set_motion_tag,DEFVAL(MMAnimationPlayer::Motion_Tags::NoTag)); 
        ClassDB::bind_method( D_METHOD("get_motion_tag" ), &MMAnimationPlayer::get_motion_tag); 
        ADD_PROPERTY(PropertyInfo(Variant::INT,"motion_tag",godot::PROPERTY_HINT_ENUM,"NoTag,StartUp,Active,Recovery",godot::PROPERTY_USAGE_DEFAULT | godot::PROPERTY_USAGE_ALWAYS_DUPLICATE ), "set_motion_tag", "get_motion_tag");


        ClassDB::bind_method(D_METHOD("_on_anim_finish","anim"),&MMAnimationPlayer::_on_anim_finish);

        ClassDB::bind_method(D_METHOD("request_animation", "animation", "timestamp", "new_halflife"), &MMAnimationPlayer::request_animation, (0.0f),(-1.0f));
        ClassDB::bind_method(D_METHOD("request_pose", "animation", "timestamp", "new_halflife"), &MMAnimationPlayer::request_pose, (0.0f),(-1.0f));
        

        ClassDB::bind_method(D_METHOD("get_local_bone_info","bone_name"),&MMAnimationPlayer::get_local_bone_info);
        ClassDB::bind_method(D_METHOD("get_model_bone_info","bone_name"),&MMAnimationPlayer::get_model_bone_info);
        ClassDB::bind_method(D_METHOD("get_raw_bone_info","bone_name"),&MMAnimationPlayer::get_global_bone_info);

        ClassDB::bind_method(D_METHOD("get_inertialized_root_motion_velocity"), &MMAnimationPlayer::get_root_motion_velocity);
        ClassDB::bind_method(D_METHOD("get_inertialized_root_motion_angular","delta_time"),&MMAnimationPlayer::get_root_motion_angular);

        ClassDB::bind_method( D_METHOD("set_halflife" ,"value"), &MMAnimationPlayer::set_halflife); 
        ClassDB::bind_method( D_METHOD("get_halflife" ), &MMAnimationPlayer::get_halflife); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"halflife"
        , PROPERTY_HINT_RANGE, "0.0,1.0,0.01,or_greater"), "set_halflife", "get_halflife");

    }
};

VARIANT_ENUM_CAST(MMAnimationPlayer::Motion_Tags);