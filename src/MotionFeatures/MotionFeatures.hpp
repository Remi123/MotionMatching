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
#include <godot_cpp/classes/skeleton_profile.hpp>

#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/box_mesh.hpp>


struct MotionFeature : public Resource {
    GDCLASS(MotionFeature,Resource)

    virtual ~MotionFeature() = default;

    static constexpr float delta = 0.016f;

    virtual void physics_update(double delta){}

    virtual int get_dimension(){return 0;}
    
    virtual PackedFloat32Array get_weights(){ return {};}

    virtual void setup_nodes(Variant main_node, Skeleton3D* skeleton){}
    virtual void setup_profile(Ref<SkeletonProfile> skel_profile){}

    virtual void setup_for_animation(Ref<Animation> animation){}
    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time){return {};}

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard, float delta){ return {};}

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}){return;}

    
    static void _bind_methods() {

        ClassDB::bind_method( D_METHOD("get_dimension"), &MotionFeature::get_dimension);

        ClassDB::bind_method( D_METHOD("get_weights"), &MotionFeature::get_weights);

        ClassDB::bind_method( D_METHOD("setup_nodes","main_node","skeleton"), &MotionFeature::setup_nodes);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &MotionFeature::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &MotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("broadphase_query_pose","blackboard","delta"), &MotionFeature::broadphase_query_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &MotionFeature::debug_pose_gizmo);
        
    }
};
