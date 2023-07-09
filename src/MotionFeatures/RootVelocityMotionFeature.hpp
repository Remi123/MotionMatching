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


struct RootVelocityMotionFeature : public MotionFeature {
    GDCLASS(RootVelocityMotionFeature,MotionFeature)
    CharacterBody3D* body;
    CharacterBody3D* get_body(){return body;} void set_body(CharacterBody3D* value){body = value;}
    int root_track_pos =-1, root_track_quat = -1;//, root_track_scale = -1;

    String root_bone_name = "%GeneralSkeleton:Root";
    void set_root_bone_name(String value){
        root_bone_name = value;
    }
    String get_root_bone_name(){return root_bone_name;}

    virtual int get_dimension()override{
        return 3;
    }

    GETSET(float,weight,1.0f);
    virtual PackedFloat32Array get_weights() override{
        return Array::make(weight,weight,weight);
    }

    virtual void setup_nodes(Variant character) override{
        body = Object::cast_to<CharacterBody3D>(character);  

    }
    virtual void setup_for_animation(Ref<Animation> animation)override{
        root_track_pos = animation->find_track(NodePath(root_bone_name),Animation::TrackType::TYPE_POSITION_3D);
        root_track_quat = animation->find_track(NodePath(root_bone_name),Animation::TrackType::TYPE_ROTATION_3D);
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time)override{
        auto pos = animation->position_track_interpolate(root_track_pos,time);
        auto prev_pos = animation->position_track_interpolate(root_track_pos,time - 0.1);
        Quaternion rotation = animation->rotation_track_interpolate(root_track_quat,time).normalized();

        Vector3 vel = rotation.xform_inv(pos-prev_pos) / 0.1;

        PackedFloat32Array result{};
        result.push_back(vel.x);
        result.push_back(vel.y);
        result.push_back(vel.z);
        return result;
    }

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard,float delta) override{ 
        auto vel = body->get_quaternion().xform_inv(body->get_velocity());
        PackedFloat32Array result{};
        result.push_back(vel.x);
        result.push_back(vel.y);
        result.push_back(vel.z);
        return result;
    }

    virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert)override{
        Vector3 data_vel = {to_convert[0],to_convert[1],to_convert[2]};
        return (body->get_velocity() - data_vel).length_squared();
    }


protected:
    static void _bind_methods() {
        ClassDB::bind_method( D_METHOD("set_weight","value"), &RootVelocityMotionFeature::set_weight, DEFVAL(1.0f)); 
        ClassDB::bind_method( D_METHOD("get_weight"), &RootVelocityMotionFeature::get_weight); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"weight"), "set_weight", "get_weight");

        ClassDB::bind_method( D_METHOD("set_root_bone_name","value"), &RootVelocityMotionFeature::set_root_bone_name,DEFVAL("%GeneralSkeleton:Root"));
        ClassDB::bind_method( D_METHOD("get_root_bone_name"), &RootVelocityMotionFeature::get_root_bone_name);
        ADD_PROPERTY(PropertyInfo(Variant::STRING,"Root Bone"),"set_root_bone_name","get_root_bone_name");
        

        ClassDB::bind_method( D_METHOD("get_dimension"), &RootVelocityMotionFeature::get_dimension);
        ClassDB::bind_method( D_METHOD("setup_nodes","character"), &RootVelocityMotionFeature::setup_nodes);
        
        // ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &RootVelocityMotionFeature::setup_for_animation);
        // ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &RootVelocityMotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("broadphase_query_pose","blackboard","delta"), &RootVelocityMotionFeature::broadphase_query_pose);
        // ClassDB::bind_method( D_METHOD("narrowphase_evaluate_cost","data_to_evaluate"), &RootVelocityMotionFeature::narrowphase_evaluate_cost);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &RootVelocityMotionFeature::debug_pose_gizmo);
    }

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}) override
    {
        if (data.size() == get_dimension())
        {
            Vector3 vel = tr.xform( Vector3(data[0], data[1] ,data[2]) );
            auto mat = gizmo->get_plugin()->get_material("white",gizmo);
            gizmo->add_lines(Array::make(tr.origin, tr.origin + vel), mat);
        }
    }
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX