#pragma once

#include <MotionFeatures/MotionFeatures.hpp>

using namespace godot;
using u = godot::UtilityFunctions;

#define GETSET(type,variable,...) type variable{__VA_ARGS__}; type get_##variable(){return  variable;} void set_##variable(type value){variable = value;}

struct LocalDistanceToPointMotionFeature : MotionFeature{
    GDCLASS(LocalDistanceToPointMotionFeature,MotionFeature)

    virtual int get_dimension() override{
        return 3;
    }
    virtual PackedFloat32Array get_weights() override {
        return Array::make(axis_weights.x,axis_weights.y,axis_weights.z);
    }
    GETSET(Vector3,axis_weights,Vector3(1.0f,1.0f,1.0f))
    GETSET(String,point_track_name);
    GETSET(String,root_bone_name);

    GETSET(Vector3, query_local_distance);

    int root_track_pos = -1, root_track_quat = -1, point_track_id = -1;

    virtual void setup_for_animation(Ref<Animation> animation)override
    {
        point_track_id = animation->find_track(NodePath(point_track_name),Animation::TrackType::TYPE_POSITION_3D);
        root_track_pos = animation->find_track(NodePath(root_bone_name),Animation::TrackType::TYPE_POSITION_3D);
        root_track_quat = animation->find_track(NodePath(root_bone_name),Animation::TrackType::TYPE_ROTATION_3D);
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time) override 
    {
        if(point_track_id == -1 || root_track_pos == -1 || root_track_quat == -1 ) 
            return {};

        Vector3 position = animation->position_track_interpolate(root_track_pos,time);
        Quaternion rotation = animation->rotation_track_interpolate(root_track_quat,time).normalized();
        Vector3 point = animation->position_track_interpolate(point_track_id,time);

        auto distance = rotation.xform_inv(point - position);

        return Array::make(distance.x,distance.y,distance.z);
    }

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float delta)override { 
        return Array::make(query_local_distance.x,query_local_distance.y,query_local_distance.z);
    }

    protected:
    static void _bind_methods()
    {
        ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
        {
            ClassDB::bind_method(D_METHOD("set_axis_weights", "value"), &LocalDistanceToPointMotionFeature::set_axis_weights, DEFVAL(Vector3(1.0f, 1.0f, 1.0f)));
            ClassDB::bind_method(D_METHOD("get_axis_weights"), &LocalDistanceToPointMotionFeature::get_axis_weights);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::VECTOR3, "axis_weights"), "set_axis_weights", "get_axis_weights");

            ClassDB::bind_method(D_METHOD("set_root_bone_name", "value"), &LocalDistanceToPointMotionFeature::set_root_bone_name, DEFVAL("%GeneralSkeleton:Root"));
            ClassDB::bind_method(D_METHOD("get_root_bone_name"), &LocalDistanceToPointMotionFeature::get_root_bone_name);
            ADD_PROPERTY(PropertyInfo(Variant::STRING, "root_bone_name"), "set_root_bone_name", "get_root_bone_name");

            ClassDB::bind_method(D_METHOD("set_point_track_name", "value"), &LocalDistanceToPointMotionFeature::set_point_track_name);
            ClassDB::bind_method(D_METHOD("get_point_track_name"), &LocalDistanceToPointMotionFeature::get_point_track_name);
            ADD_PROPERTY(PropertyInfo(Variant::STRING, "point_track_name"), "set_point_track_name", "get_point_track_name");
        }
        ClassDB::add_property_group(get_class_static(), "Query to fills", "query");
        {
            ClassDB::bind_method(D_METHOD("set_query_local_distance", "value"), &LocalDistanceToPointMotionFeature::set_query_local_distance);
            ClassDB::bind_method(D_METHOD("get_query_local_distance"), &LocalDistanceToPointMotionFeature::get_query_local_distance);
            ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "query_local_distance"), "set_query_local_distance", "get_query_local_distance");
        }
        ClassDB::add_property_group(get_class_static(), "", "");
        ClassDB::bind_method( D_METHOD("get_weights"), &LocalDistanceToPointMotionFeature::get_weights);
        ClassDB::bind_method( D_METHOD("get_dimension"), &LocalDistanceToPointMotionFeature::get_dimension);
        ClassDB::bind_method( D_METHOD("setup_nodes","character"), &LocalDistanceToPointMotionFeature::setup_nodes);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &LocalDistanceToPointMotionFeature::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &LocalDistanceToPointMotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("broadphase_query_pose","blackboard","delta"), &LocalDistanceToPointMotionFeature::broadphase_query_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &LocalDistanceToPointMotionFeature::debug_pose_gizmo);
    }
};