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
#include <MMAnimationPlayer.hpp>

#include <MotionFeatures/MotionFeatures.hpp>
#include <KForm.hpp>
#include <MMAnimationLibrary.hpp>
#include <algorithm>

using namespace godot;

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
struct BonePositionVelocityMotionFeature : public MotionFeature {
    GDCLASS(BonePositionVelocityMotionFeature,MotionFeature)

    virtual ~BonePositionVelocityMotionFeature() = default;

    // Skeleton
    Skeleton3D* _skeleton = nullptr;
    Ref<SkeletonProfile> _skel = nullptr;



    PackedStringArray bone_names{};
    void set_bone_names(PackedStringArray value){
        bone_names = value;
    }
    PackedStringArray get_bone_names(){return bone_names;}

    PackedInt32Array bones_id{};

    HashMap<uint32_t,PackedInt32Array> bone_tracks{};

    virtual int get_dimension()override{
        if(use_inertialization)
        {
            return bone_names.size() * 3;
        }
        return bone_names.size() * 3 * 2;
    }
    virtual void setup_nodes(Variant main_node, Skeleton3D* skeleton) override{
        // bones_id.clear();
        // if(skeleton!=nullptr)
        // {
        //     _skeleton = skeleton;
        //     for(size_t i = 0; i < bone_names.size();++i)
        //     {
        //         const size_t id = _skeleton->find_bone(bone_names[i]);
        //         if (id >= 0)
        //             bones_id.push_back(id);
        //     }
        //     u::prints("Bones id",bone_names,bones_id);
        // }
        
        // last_known_positions.resize(bones_id.size());
        // last_known_positions.fill({});
        // last_known_velocities.resize(bones_id.size());
        // last_known_velocities.fill({});
        // if (use_inertialization)
        // {
        //     last_known_result.resize(bones_id.size() * 3);
        //     last_known_result.fill({});
        //     return;
        // }
        // last_known_result.resize(bones_id.size() * 2 * 3);
        // last_known_result.fill({});
    }
    virtual void setup_for_animation(Ref<Animation> animation)override{
        return;
        _skeleton->reset_bone_poses();
        bone_tracks.clear();
        bones_id.clear();
        for(size_t i = 0; i < bone_names.size();++i)
        {
            const size_t id = _skeleton->find_bone(bone_names[i]);
            if (id >= 0)
                bones_id.push_back(id);
        }
        for (auto bone_id = 0; bone_id < _skeleton->get_bone_count(); ++bone_id)
        {
            bone_tracks[bone_id] = Array::make(-1,-1,-1);
        }
        for (int track_id = 0; track_id < animation->get_track_count(); ++track_id)
        {
            // Easier to find bone with the track name than vice-versa
            const auto bone_idx = _skeleton->find_bone(animation->track_get_path(track_id).get_subname(0));
            if (bone_idx == -1)
                continue;

            if (animation->track_get_type(track_id) == Animation::TYPE_POSITION_3D)
            {
                bone_tracks[bone_idx][0] = track_id;
            }
            else if (animation->track_get_type(track_id) == Animation::TYPE_ROTATION_3D)
            {
                bone_tracks[bone_idx][1] = track_id;
            }
            else if (animation->track_get_type(track_id) == Animation::TYPE_SCALE_3D)
            {
                bone_tracks[bone_idx][2] = track_id;
            }
        }
    }


    NodePath _skel_path;

    virtual void setup_profile(NodePath skeleton_path,Ref<SkeletonProfile> skeleton_profile) override{
        _skel = skeleton_profile;
        _skel_path = skeleton_path;
        bones_id.clear();
        if(_skel!=nullptr)
        {
            for(size_t i = 0; i < bone_names.size();++i)
            {
                const size_t id = _skel->find_bone(bone_names[i]);
                if (id >= 0)
                    bones_id.push_back(id);
            }
            u::prints("Bones id",bone_names,bones_id);
        }
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time)override{
        constexpr float dt = 0.05f;

        PackedFloat32Array result{};
        kform kbone{};
 
        for (size_t index = 0; index < bone_names.size(); ++index)
        {
            auto path = u::str(_skel_path)+u::str(":")+bone_names[index];
            kbone = MMAnimationLibrary::sample_bone_rootmotion_kform(animation,time,_skel,path);
            // kbone = kform(_skel,animation,time,path,kform::Space::Model);
            
            // Serialize
            if (use_inertialization)
            {
                const auto cost = inertialization_cost_function(kbone.pos, kbone.vel, inertialization_halflife);
                result.push_back(cost.x);
                result.push_back(cost.y);
                result.push_back(cost.z);
            }
            else
            {
                result.push_back(kbone.pos.x);
                result.push_back(kbone.pos.y);
                result.push_back(kbone.pos.z);
                result.push_back(kbone.vel.x);
                result.push_back(kbone.vel.y);
                result.push_back(kbone.vel.z);
            }
        }


        return result;
    }

    Vector3 inertialization_cost_function(Vector3 pos, Vector3 vel, float halflife)
    {
        const auto halfdamp =  CritDampSpring::halflife_to_damping(halflife) / 2.0;
        return (2*pos) / halfdamp + vel / (halfdamp * halfdamp);
    }

    GETSET(PackedVector3Array,bones_pos);
    GETSET(PackedVector3Array,bones_vel);

    GETSET(MMAnimationPlayer*, mm_player);

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard,float delta) override{        
        ERR_FAIL_NULL_V_MSG(mm_player,{},"animation player not set");
        bones_pos.resize(bones_id.size());
        bones_vel.resize(bones_id.size());
        constexpr size_t size = 3;

        PackedFloat32Array last_known_result{};


        if(use_inertialization)
        {
            last_known_result.resize(bones_id.size() * 3);
            for (size_t i = 0; i < bones_id.size(); ++i)
            {
                Vector3 pos = bones_pos[i], vel = bones_vel[i];
                auto cost = inertialization_cost_function(pos, vel, inertialization_halflife);
                last_known_result[i * size] = cost.x;
                last_known_result[i * size + 1] = cost.y;
                last_known_result[i * size + 2] = cost.z;
            }
            return last_known_result;
        }

        last_known_result.resize(bones_id.size()*2*3);


        for(size_t i = 0; i < bones_id.size(); ++i)
        {
            kform b = mm_player->get_bone_info(bone_names[i],kform::Space::Global);
            Vector3 pos = b.pos, vel = b.vel;

            last_known_result[i * size * 2] = pos.x;
            last_known_result[i * size * 2 + 1] = pos.y;
            last_known_result[i * size * 2 + 2] = pos.z;
            last_known_result[i * size * 2 + size] = vel.x;
            last_known_result[i * size * 2 + size + 1] = vel.y;
            last_known_result[i * size * 2 + size + 2] = vel.z;
        }
        return last_known_result;
    }

    float weight_bone_pos{1.0f}; float get_weight_bone_pos(){return weight_bone_pos;} void set_weight_bone_pos(float value){weight_bone_pos = value;}
    float weight_bone_vel{1.0f}; float get_weight_bone_vel(){return weight_bone_vel;} void set_weight_bone_vel(float value){weight_bone_vel = value;}
    float weight_inertialization{1.0f}; float get_weight_inertialization(){return weight_inertialization;} void set_weight_inertialization(float value){weight_inertialization = value;}
    
    virtual PackedFloat32Array get_weights() override{
        PackedFloat32Array result{};

        if (use_inertialization)
        {
            for (auto i = 0; i < 3 * bone_names.size(); ++i)
            {
                result.append(weight_inertialization);
            }
            return result;
        }

        for(auto i =0; i < 3 * bone_names.size(); ++i)
        {
            result.append(weight_bone_pos);
        }
        for(auto i =0; i < 3 * bone_names.size(); ++i)
        {
            result.append(weight_bone_vel);
        }
        return result;
    }

    String root_bone_name = "Root";
    void set_root_bone_name(String value){
        root_bone_name = value;
    }
    String get_root_bone_name(){return root_bone_name;}

    GETSET(bool,use_inertialization)
    GETSET(float,inertialization_halflife,0.01)

    PackedFloat32Array serialize_mmplayer(MMAnimationPlayer* mm_player){
        ERR_FAIL_NULL_V_MSG(mm_player,{},"MMAnimationPlayer is null");
        constexpr size_t size = 3;
        PackedFloat32Array result{};
        if (use_inertialization)
        {
            result.resize(bone_names.size() * 3);
            for (size_t i = 0; i < bone_names.size(); ++i)
            {
                // _skel isn't init
                kform b = mm_player->get_bone_global_kform(_skel->find_bone(bone_names[i]));
                Vector3 pos = b.pos, vel = b.vel;
                auto cost = inertialization_cost_function(pos, vel, inertialization_halflife);
                result[i * size] = cost.x;
                result[i * size + 1] = cost.y;
                result[i * size + 2] = cost.z;
            }
            return result;
        }
        else
        {
            result.resize(bone_names.size() * 3 * 2);
            for (size_t i = 0; i < bone_names.size(); ++i)
            {
                kform b = mm_player->get_bone_global_kform(_skel->find_bone(bone_names[i]));
                Vector3 pos = b.pos, vel = b.vel;

                result[i * size * 2] = pos.x;
                result[i * size * 2 + 1] = pos.y;
                result[i * size * 2 + 2] = pos.z;
                result[i * size * 2 + size] = vel.x;
                result[i * size * 2 + size + 1] = vel.y;
                result[i * size * 2 + size + 2] = vel.z;
            }
            return result;
        }
        return result;
    }

protected:
    static void _bind_methods() {

        {
            ClassDB::bind_method(D_METHOD("serialize_MMAnimationPlayer", "body"), &BonePositionVelocityMotionFeature::serialize_mmplayer);
        }

        ClassDB::bind_method(D_METHOD("set_weight_bone_pos", "value"), &BonePositionVelocityMotionFeature::set_weight_bone_pos);
        ClassDB::bind_method(D_METHOD("get_weight_bone_pos"), &BonePositionVelocityMotionFeature::get_weight_bone_pos);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_pos"), "set_weight_bone_pos", "get_weight_bone_pos");
        ClassDB::bind_method(D_METHOD("set_weight_bone_vel", "value"), &BonePositionVelocityMotionFeature::set_weight_bone_vel);
        ClassDB::bind_method(D_METHOD("get_weight_bone_vel"), &BonePositionVelocityMotionFeature::get_weight_bone_vel);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_vel"), "set_weight_bone_vel", "get_weight_bone_vel");
        ClassDB::bind_method(D_METHOD("set_weight_inertialization", "value"), &BonePositionVelocityMotionFeature::set_weight_inertialization);
        ClassDB::bind_method(D_METHOD("get_weight_inertialization"), &BonePositionVelocityMotionFeature::get_weight_inertialization);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_inertialization"), "set_weight_inertialization", "get_weight_inertialization");
        


        ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
        {
            ClassDB::bind_method( D_METHOD("set_use_inertialization" ,"value"), &BonePositionVelocityMotionFeature::set_use_inertialization); 
            ClassDB::bind_method( D_METHOD("get_use_inertialization" ), &BonePositionVelocityMotionFeature::get_use_inertialization); 
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"use_inertialization"), "set_use_inertialization", "get_use_inertialization");

            ClassDB::bind_method( D_METHOD("set_inertialization_halflife" ,"value"), &BonePositionVelocityMotionFeature::set_inertialization_halflife); 
            ClassDB::bind_method( D_METHOD("get_inertialization_halflife" ), &BonePositionVelocityMotionFeature::get_inertialization_halflife); 
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"inertialization_halflife"), "set_inertialization_halflife", "get_inertialization_halflife");

            ClassDB::bind_method(D_METHOD("set_root_bone_name", "value"), &BonePositionVelocityMotionFeature::set_root_bone_name, DEFVAL("Root"));
            ClassDB::bind_method(D_METHOD("get_root_bone_name"), &BonePositionVelocityMotionFeature::get_root_bone_name);
            ADD_PROPERTY(PropertyInfo(Variant::STRING, "Root Bone"), "set_root_bone_name", "get_root_bone_name");

            ClassDB::bind_method(D_METHOD("set_bone_names", "value"), &BonePositionVelocityMotionFeature::set_bone_names);
            ClassDB::bind_method(D_METHOD("get_bone_names"), &BonePositionVelocityMotionFeature::get_bone_names);
            ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "Bones"), "set_bone_names", "get_bone_names");

            ClassDB::bind_method(D_METHOD("set_debug_color_position", "value"), &BonePositionVelocityMotionFeature::set_debug_color_position);
            ClassDB::bind_method(D_METHOD("get_debug_color_position"), &BonePositionVelocityMotionFeature::get_debug_color_position);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_position"), "set_debug_color_position", "get_debug_color_position");

            ClassDB::bind_method(D_METHOD("set_debug_color_velocity", "value"), &BonePositionVelocityMotionFeature::set_debug_color_velocity);
            ClassDB::bind_method(D_METHOD("get_debug_color_velocity"), &BonePositionVelocityMotionFeature::get_debug_color_velocity);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::COLOR, "debug_color_velocity"), "set_debug_color_velocity", "get_debug_color_velocity");
        }
        ClassDB::add_property_group(get_class_static(), "Queries to fill", "query");
        {
            ClassDB::bind_method(D_METHOD("set_mm_player", "animation_player"), &BonePositionVelocityMotionFeature::set_mm_player);
        }

        ClassDB::add_property_group(get_class_static(), "", "");

        //BINDER_PROPERTY_PARAMS(BonePositionVelocityMotionFeature,Variant::PACKED_VECTOR3_ARRAY,bones_pos,PROPERTY_HINT_NONE,"",PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY);
        ClassDB::bind_method( D_METHOD("set_bones_pos" ,"value"), &BonePositionVelocityMotionFeature::set_bones_pos);
        ClassDB::bind_method( D_METHOD("get_bones_pos" ), &BonePositionVelocityMotionFeature::get_bones_pos);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_VECTOR3_ARRAY,"bones_pos",PROPERTY_HINT_NONE,"",PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY), "set_bones_pos", "get_bones_pos");
        
        //BINDER_PROPERTY_PARAMS(BonePositionVelocityMotionFeature,Variant::PACKED_VECTOR3_ARRAY,bones_vel,PROPERTY_HINT_NONE,"",PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY);
        ClassDB::bind_method( D_METHOD("set_bones_vel" ,"value"), &BonePositionVelocityMotionFeature::set_bones_vel);
        ClassDB::bind_method( D_METHOD("get_bones_vel" ), &BonePositionVelocityMotionFeature::get_bones_vel);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_VECTOR3_ARRAY,"bones_vel",PROPERTY_HINT_NONE,"",PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_READ_ONLY), "set_bones_vel", "get_bones_vel");

        ClassDB::bind_method( D_METHOD("get_weights"), &BonePositionVelocityMotionFeature::get_weights);
        ClassDB::bind_method( D_METHOD("get_dimension"), &BonePositionVelocityMotionFeature::get_dimension);
        ClassDB::bind_method( D_METHOD("setup_nodes","character"), &BonePositionVelocityMotionFeature::setup_nodes);
        ClassDB::bind_method( D_METHOD("setup_profile","skeleton_path","skeleton_profile"), &BonePositionVelocityMotionFeature::setup_profile);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &BonePositionVelocityMotionFeature::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &BonePositionVelocityMotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("broadphase_query_pose","blackboard","delta"), &BonePositionVelocityMotionFeature::broadphase_query_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &BonePositionVelocityMotionFeature::debug_pose_gizmo);
    }

    GETSET(Color,debug_color_position,godot::Color(1.0f,1.0f,1.0f));
    GETSET(Color,debug_color_velocity,godot::Color(0.0f,0.0f,0.0f));

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}) override
    {
        const auto mat_name_pos = "pos" + get_path();
        const auto mat_name_vel = "vel" + get_path();
        if(gizmo->get_plugin()->get_material(mat_name_pos,gizmo) == nullptr)
        {
            gizmo->get_plugin()->create_material(mat_name_pos,debug_color_position);
        }
        if(gizmo->get_plugin()->get_material(mat_name_vel,gizmo) == nullptr)
        {
            gizmo->get_plugin()->create_material(mat_name_vel,debug_color_velocity);
        }

        auto position_color = gizmo->get_plugin()->get_material(mat_name_pos,gizmo);
        auto velocity_color = gizmo->get_plugin()->get_material(mat_name_vel,gizmo);
        position_color->set_albedo(debug_color_position);
        velocity_color->set_albedo(debug_color_velocity);

        if(use_inertialization)
        {
            return;
        }

        constexpr int s = 3;
        for(size_t index = 0; index < bone_names.size(); ++index)
        {
            //i*size*2+size+2
            Vector3 pos = Vector3(data[index * s * 2 + 0], data[index * s * 2 + 1], data[index * s * 2 + 2]);
            Vector3 vel = Vector3(data[index * s * 2 + s + 0], data[index * s * 2 + s + 1], data[index * s * 2 + s + 2]);
            pos = tr.xform(pos);
            vel = tr.xform(vel);

            gizmo->add_lines(Array::make(pos, pos + vel), velocity_color);
            auto box = Ref<BoxMesh>();
            box.instantiate();
            box->set_size(Vector3(0.05f,0.05f,0.05f));
            Transform3D tr = Transform3D(Basis(),pos);
            gizmo->add_mesh(box,position_color,tr);
        }
    }
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX