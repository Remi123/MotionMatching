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
#define STR(x) #x
#define STRING_PREFIX(prefix,s) STR(prefix##s) 
#define BINDER_PROPERTY_PARAMS(type,variant_type,variable,...)\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(set_,variable) ,"value"), &type::set_##variable);\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(get_,variable) ), &type::get_##variable);\
        ADD_PROPERTY(PropertyInfo(variant_type,#variable,__VA_ARGS__),STRING_PREFIX(set_,variable),STRING_PREFIX(get_,variable));

struct RootVelocityMotionFeature : public MotionFeature {
    GDCLASS(RootVelocityMotionFeature,MotionFeature);


    virtual ~RootVelocityMotionFeature() = default;

    int root_track_pos =-1, root_track_quat = -1;//, root_track_scale = -1;

    GETSET(Vector3,query_velocity);

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

    virtual void setup_nodes(Variant main_node, Skeleton3D* skeleton) override{

    }
    virtual void setup_for_animation(Ref<Animation> animation)override{
        root_track_pos = animation->find_track(NodePath(root_bone_name),Animation::TrackType::TYPE_POSITION_3D);
        root_track_quat = animation->find_track(NodePath(root_bone_name),Animation::TrackType::TYPE_ROTATION_3D);
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time)override{
        auto pos = animation->position_track_interpolate(root_track_pos,time + 0.05);
        auto prev_pos = animation->position_track_interpolate(root_track_pos,time);
        Quaternion rotation = animation->rotation_track_interpolate(root_track_quat,time).normalized();

        Vector3 vel = rotation.xform_inv(pos-prev_pos) / 0.05;

        PackedFloat32Array result{};
        result.push_back(vel.x);
        result.push_back(vel.y);
        result.push_back(vel.z);
        return result;
    }

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard,float delta) override{ 
        PackedFloat32Array result{};
        result.push_back(query_velocity.x);
        result.push_back(query_velocity.y);
        result.push_back(query_velocity.z);
        return result;
    }

    PackedFloat32Array serialize_charbody3d(CharacterBody3D * body)
    {
        PackedFloat32Array result{};
        auto vel = body->get_global_transform().basis.get_quaternion().xform_inv(body->get_velocity());
        result.push_back(vel.x);
        result.push_back(vel.y);
        result.push_back(vel.z);
        return result;
    }
    PackedFloat32Array serialize_vec3(Vector3 local_vel)
    {
        PackedFloat32Array result{};
        result.push_back(local_vel.x);
        result.push_back(local_vel.y);
        result.push_back(local_vel.z);
        return result;
    }


protected:
    static void _bind_methods() {

        {
            ClassDB::bind_method(D_METHOD("serialize_CharacterBody3d", "body"), &RootVelocityMotionFeature::serialize_charbody3d);
            ClassDB::bind_method(D_METHOD("serialize_Local_Velocity", "local_velocity"), &RootVelocityMotionFeature::serialize_vec3);
        }

        ClassDB::bind_method(D_METHOD("set_weight", "value"), &RootVelocityMotionFeature::set_weight, DEFVAL(1.0f));
        ClassDB::bind_method(D_METHOD("get_weight"), &RootVelocityMotionFeature::get_weight);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");

        ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
        {
            ClassDB::bind_method(D_METHOD("set_root_bone_name", "value"), &RootVelocityMotionFeature::set_root_bone_name, DEFVAL("%GeneralSkeleton:Root"));
            ClassDB::bind_method(D_METHOD("get_root_bone_name"), &RootVelocityMotionFeature::get_root_bone_name);
            ADD_PROPERTY(PropertyInfo(Variant::STRING, "Root Bone"), "set_root_bone_name", "get_root_bone_name");

            ClassDB::bind_method(D_METHOD("set_debug_color", "value"), &RootVelocityMotionFeature::set_debug_color);
            ClassDB::bind_method(D_METHOD("get_debug_color"), &RootVelocityMotionFeature::get_debug_color);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color"), "set_debug_color", "get_debug_color");
        }
        ClassDB::add_property_group(get_class_static(), "Query to fills", "query");
        {
            ClassDB::bind_method( D_METHOD("set_query_velocity" ,"value"), &RootVelocityMotionFeature::set_query_velocity); 
            ClassDB::bind_method( D_METHOD("get_query_velocity" ), &RootVelocityMotionFeature::get_query_velocity); 
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::VECTOR3,"query_local_velocity",PROPERTY_HINT_NONE,"",PROPERTY_USAGE_EDITOR), "set_query_velocity", "get_query_velocity");
        }
        ClassDB::add_property_group(get_class_static(), "", "");

        ClassDB::bind_method( D_METHOD("get_weights"), &RootVelocityMotionFeature::get_weights);
        ClassDB::bind_method( D_METHOD("get_dimension"), &RootVelocityMotionFeature::get_dimension);
        ClassDB::bind_method( D_METHOD("setup_nodes","character"), &RootVelocityMotionFeature::setup_nodes);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &RootVelocityMotionFeature::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &RootVelocityMotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("broadphase_query_pose","blackboard","delta"), &RootVelocityMotionFeature::broadphase_query_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &RootVelocityMotionFeature::debug_pose_gizmo);
    }

    GETSET(Color,debug_color,godot::Color(1.0f,1.0f,1.0f));


    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}) override
    {
        const auto material_name = "rootvel" + get_path();
        if(gizmo->get_plugin()->get_material(material_name) == nullptr)
        {
            gizmo->get_plugin()->create_material(material_name,debug_color);
        }
        if (data.size() == get_dimension())
        {
            Vector3 vel = tr.xform( Vector3(data[0], data[1] ,data[2]) );
            auto mat = gizmo->get_plugin()->get_material(material_name,gizmo);
            mat->set_albedo(debug_color);
            gizmo->add_lines(Array::make(tr.origin, tr.origin + vel), mat);
        }
    }
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX