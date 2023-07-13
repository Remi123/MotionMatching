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

#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>

#include <godot_cpp/classes/character_body3d.hpp>

#include <MotionFeatures/MotionFeatures.hpp>

using namespace godot;
using u = godot::UtilityFunctions;

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type,variable,...) type variable{__VA_ARGS__}; type get_##variable(){return  variable;} void set_##variable(type value){variable = value;}

struct TrajectoryMotionFeature : public MotionFeature{
    GDCLASS(TrajectoryMotionFeature,MotionFeature)


    virtual ~TrajectoryMotionFeature() = default;

    Skeleton3D* skeleton{nullptr}; Skeleton3D* get_skeleton(){return skeleton;} void set_skeleton(Skeleton3D* value){skeleton = value;}
    GETSET(String,root_bone_name,"%GeneralSkeleton:Root")

    GETSET(NodePath,character_path);

    GETSET(float,halflife_velocity,0.2);
    GETSET(float,halflife_angular_velocity,0.13);

    GETSET(PackedFloat32Array,past_time_dt);
    GETSET(PackedFloat32Array,future_time_dt);
    GETSET(int,past_count,4);
    GETSET(float,past_delta,0.7f/past_count);
    GETSET(int,future_count,6);
    GETSET(float,future_delta,1.2f/future_count);

    GETSET(float,weight_history_pos,1.0f);
    GETSET(float,weight_prediction_pos,1.0f);
    GETSET(float,weight_prediction_angle,1.0f);
    virtual PackedFloat32Array get_weights() override{
        PackedFloat32Array result{};
        for(auto i =0; i < 2 * past_time_dt.size(); ++i)
        {
            result.append(weight_history_pos);
        }
        for(auto i =0; i < 2 * future_time_dt.size(); ++i)
        {
            result.append(weight_prediction_pos);
        }
        for(auto i =0; i < 1 * future_time_dt.size(); ++i)
        {
            result.append(weight_prediction_angle);
        }
        return result;
    }


private:
    void create_default_dt(){
        past_time_dt.clear();
        future_time_dt.clear();
        float time = past_delta;
        for (int count = 0; count < past_count; ++count, time+=past_delta)
        {
            past_time_dt.push_back(-time);
        }
        time = future_delta;
        for (int count = 0; count < future_count; ++count, time+=future_delta)
        {
            future_time_dt.push_back(time);
        }
    }
public:
    virtual int get_dimension() override
    {
        // Offset for each
        const size_t past_pos =  2 * past_time_dt.size();
        const size_t future_pos = 2 * future_time_dt.size();
        const size_t future_rot_angle = future_time_dt.size();
        return past_pos + future_pos + future_rot_angle ;
    }

    CharacterBody3D* body;
    virtual void setup_nodes(Variant character) override{
        auto n = Object::cast_to<CharacterBody3D>(character);
        skeleton = n->get_node<Skeleton3D>("Armature/GeneralSkeleton");
    }

    int root_tracks[3] = {0,0,0};
    Vector3 start_pos,start_vel,end_pos,end_vel;
    Quaternion start_rot,end_rot, end_ang_vel;
    float start_time = 0.0f, end_time = 0.0f;

    virtual void setup_for_animation(Ref<Animation> animation) override
    {
        start_time = 0.1f;
        end_time = std::floor(animation->get_length() * 10)/10.0f;
        root_tracks[0] = animation->find_track(root_bone_name, Animation::TrackType::TYPE_POSITION_3D);
        root_tracks[1] = animation->find_track(root_bone_name, Animation::TrackType::TYPE_ROTATION_3D);
        root_tracks[2] = animation->find_track(root_bone_name, Animation::TrackType::TYPE_SCALE_3D);
        {
            start_pos = animation->position_track_interpolate(root_tracks[0], 0.0);
            start_rot = animation->rotation_track_interpolate(root_tracks[1], 0.0);
            start_vel = (animation->position_track_interpolate(root_tracks[0], 0.1) - start_pos) / 0.1;
        }
        {
            end_pos = animation->position_track_interpolate(root_tracks[0], end_time);
            end_rot = animation->rotation_track_interpolate(root_tracks[1], end_time);
            end_vel = (end_pos - animation->position_track_interpolate(root_tracks[0], end_time - 0.1)) / 0.1;

            end_ang_vel = animation->rotation_track_interpolate(root_tracks[1], animation->get_length() - delta - 0.1).inverse() * animation->rotation_track_interpolate(root_tracks[1], animation->get_length() - delta);
        }
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time)override 
    {
        PackedFloat32Array result{};
        Vector3 curr_pos = animation->position_track_interpolate(root_tracks[0],time);
        Quaternion curr_rot = animation->rotation_track_interpolate(root_tracks[1],time);

        for (size_t index = 0; index < past_time_dt.size(); ++index)
        {
            const float t = time - abs(past_time_dt[index]);
            Vector3 pos{};
            Quaternion rot{};
            if (t >= 0.0f)
            { // The offset can be accessed through the anim data
                pos = animation->position_track_interpolate(root_tracks[0], t) - curr_pos;
                rot = animation->rotation_track_interpolate(root_tracks[1], t);
                pos = curr_rot.xform_inv(pos);
            }
            else
            { // The offset must be calculated using the starting velocity and extrapoling
                pos = start_pos + (start_vel * t) - curr_pos;
                pos = curr_rot.xform_inv(pos);
            }
            result.push_back(pos.x);
            result.push_back(pos.z);
        }
        for (size_t index = 0; index < future_time_dt.size(); ++index)
        {
            const float t = time + abs(future_time_dt[index]);
            Vector3 pos{};
            Quaternion rot{};
            if (t <= end_time)
            { // The offset can be accessed through the anim data
                pos = animation->position_track_interpolate(root_tracks[0], t) - curr_pos;
                rot = animation->rotation_track_interpolate(root_tracks[1], t);
                pos = curr_rot.xform_inv(pos);
            }
            else
            { // The offset must be calculated using the end velocity and extrapoling
                pos = end_pos + end_vel * (t - end_time) - curr_pos;
                pos = curr_rot.xform_inv(pos);
            }


            result.push_back(pos.x);
            result.push_back(pos.z);
        }
        for (size_t index = 0; index < future_time_dt.size(); ++index)
        {
            const float t = time + abs(future_time_dt[index]);
            Vector3 pos{};
            Quaternion rot{};
            if (t <= end_time)
            { // The offset can be accessed through the anim data
                rot = animation->rotation_track_interpolate(root_tracks[1], t) * curr_rot.inverse();
            }
            else
            { // The offset must be calculated using the end velocity and extrapoling
                rot = animation->rotation_track_interpolate(root_tracks[1], animation->get_length() - delta) * curr_rot.inverse();
            }
            
            result.push_back(rot.get_euler_xyz().y);
        }
        return result;
    }

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard,float delta) override{
        PackedFloat32Array result{};
        if(!blackboard.has_all(Array::make("history","prediction","pred_dir"))) return result;

        PackedVector3Array history = PackedVector3Array(blackboard["history"]);
        PackedVector3Array prediction = PackedVector3Array(blackboard["prediction"]);
        PackedFloat32Array direction = PackedFloat32Array(blackboard["pred_dir"]);
        bool valid = false;
        {
            for(auto elem: history)
            {
                result.append(elem.x);
                result.append(elem.z);
            }
            for(auto elem: prediction)
            {
                result.append(elem.x);
                result.append(elem.z);
            }
            for(auto elem: direction)
            {
                result.append(elem);
            }
        }
        return result;
    }

    virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert)override{return 0.0;}


    protected:
    static void _bind_methods() {
        return;
        ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
        {
            ClassDB::bind_method( D_METHOD("set_weight_history_pos","value"), &TrajectoryMotionFeature::set_weight_history_pos ); ClassDB::bind_method( D_METHOD("get_weight_history_pos"), &TrajectoryMotionFeature::get_weight_history_pos); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"weight_history_pos"), "set_weight_history_pos", "get_weight_history_pos");
            ClassDB::bind_method( D_METHOD("set_weight_prediction_pos","value"), &TrajectoryMotionFeature::set_weight_prediction_pos ); ClassDB::bind_method( D_METHOD("get_weight_prediction_pos"), &TrajectoryMotionFeature::get_weight_prediction_pos); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"weight_prediction_pos"), "set_weight_prediction_pos", "get_weight_prediction_pos");
            ClassDB::bind_method( D_METHOD("set_weight_prediction_angle","value"), &TrajectoryMotionFeature::set_weight_prediction_angle ); ClassDB::bind_method( D_METHOD("get_weight_prediction_angle"), &TrajectoryMotionFeature::get_weight_prediction_angle); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"weight_prediction_angle"), "set_weight_prediction_angle", "get_weight_prediction_angle");

            PackedFloat32Array m_default{};
            m_default.push_back(0.2);m_default.push_back(0.4);
            ClassDB::bind_method( D_METHOD("set_root_bone_name","value"), &TrajectoryMotionFeature::set_root_bone_name,("%GeneralSkeleton:Root")); ClassDB::bind_method( D_METHOD("get_root_bone_name"), &TrajectoryMotionFeature::get_root_bone_name); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"root_bone_name"), "set_root_bone_name", "get_root_bone_name");
            ClassDB::bind_method( D_METHOD("set_past_time_dt","value"), &TrajectoryMotionFeature::set_past_time_dt,(m_default)); ClassDB::bind_method( D_METHOD("get_past_time_dt"), &TrajectoryMotionFeature::get_past_time_dt); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY,"past_time_dt"), "set_past_time_dt", "get_past_time_dt");
            ClassDB::bind_method( D_METHOD("set_future_time_dt","value"), &TrajectoryMotionFeature::set_future_time_dt ); ClassDB::bind_method( D_METHOD("get_future_time_dt"), &TrajectoryMotionFeature::get_future_time_dt); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY,"future_time_dt"), "set_future_time_dt", "get_future_time_dt");
            
            ClassDB::bind_method(D_METHOD("set_debug_color_history", "value"), &TrajectoryMotionFeature::set_debug_color_history);
            ClassDB::bind_method(D_METHOD("get_debug_color_history"), &TrajectoryMotionFeature::get_debug_color_history);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_history"), "set_debug_color_history", "get_debug_color_history");

            ClassDB::bind_method(D_METHOD("set_debug_color_future", "value"), &TrajectoryMotionFeature::set_debug_color_future);
            ClassDB::bind_method(D_METHOD("get_debug_color_future"), &TrajectoryMotionFeature::get_debug_color_future);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_future"), "set_debug_color_future", "get_debug_color_future");
        
        }
        ClassDB::bind_method( D_METHOD("get_dimension"), &TrajectoryMotionFeature::get_dimension);
        // BIND_VIRTUAL_METHOD(MotionFeature,get_dimension);

        ClassDB::bind_method( D_METHOD("setup_nodes","character"), &TrajectoryMotionFeature::setup_nodes);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &TrajectoryMotionFeature::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &TrajectoryMotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("broadphase_query_pose","blackboard","delta"), &TrajectoryMotionFeature::broadphase_query_pose);
        ClassDB::bind_method( D_METHOD("narrowphase_evaluate_cost","data_to_evaluate"), &TrajectoryMotionFeature::narrowphase_evaluate_cost);
        
        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &TrajectoryMotionFeature::debug_pose_gizmo);
    }
    GETSET(Color,debug_color_history,godot::Color(1.0f,1.0f,1.0f));
    GETSET(Color,debug_color_future,godot::Color(0.0f,0.0f,0.0f));

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{})override
    {
        const auto mat_name_history = "history" + get_path();
        const auto mat_name_future = "future" + get_path();
        if(gizmo->get_plugin()->get_material(mat_name_history,gizmo) == nullptr)
        {
            gizmo->get_plugin()->create_material(mat_name_history,debug_color_history);
        }
        if(gizmo->get_plugin()->get_material(mat_name_future,gizmo) == nullptr)
        {
            gizmo->get_plugin()->create_material(mat_name_future,debug_color_future);
        }
        // if (data.size() == get_dimension())
        {
            constexpr int s = 3;
            auto history = gizmo->get_plugin()->get_material(mat_name_history,gizmo);
            history->set_albedo(debug_color_history);
            auto future = gizmo->get_plugin()->get_material(mat_name_future,gizmo);
            future->set_albedo(debug_color_future);
            for(size_t i = 0; i < past_time_dt.size(); ++i)
            {
                const size_t offset = i * 2;
                Vector3 pos = Vector3(data[offset + 0],0,data[offset + 1]); 
                pos = tr.xform(pos);
                gizmo->add_lines(Array::make(pos, pos + Vector3(0,1,0)), history);               
            }
            const size_t pos_offset = past_time_dt.size();
            const size_t traj_offset = past_time_dt.size() * 2 + future_time_dt.size() * 2;
            for(size_t i = 0; i < future_time_dt.size(); ++i)
            {
                const size_t offset = (pos_offset + i) * 2;
                Vector3 pos = Vector3(data[offset + 0],0,data[offset + 1]); 
                Vector3 traj = tr.xform(Vector3(0,0,1)).rotated(Vector3(0,1,0),data[traj_offset + i]);
                pos = tr.xform(pos);
                // traj = tr.xform(traj);
                gizmo->add_lines(Array::make(pos, pos + traj), future);          
            }

        }
    }
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX