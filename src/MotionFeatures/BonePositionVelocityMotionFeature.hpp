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
struct BonePositionVelocityMotionFeature : public MotionFeature {
    GDCLASS(BonePositionVelocityMotionFeature,MotionFeature)

    virtual ~BonePositionVelocityMotionFeature() = default;

    // Skeleton
    Skeleton3D* _skeleton = nullptr;



    PackedStringArray bone_names{};
    void set_bone_names(PackedStringArray value){
        bone_names = value;
    }
    PackedStringArray get_bone_names(){return bone_names;}

    PackedInt32Array bones_id{};

    HashMap<uint32_t,PackedInt32Array> bone_tracks{};

    virtual int get_dimension()override{
        return bone_names.size() * 3 * 2;
    }
    virtual void setup_nodes(Variant main_node, Skeleton3D* skeleton) override{
        bones_id.clear();
        if(skeleton!=nullptr)
        {
            _skeleton = skeleton;
            for(size_t i = 0; i < bone_names.size();++i)
            {
                const size_t id = _skeleton->find_bone(bone_names[i]);
                if (id >= 0)
                    bones_id.push_back(id);
            }
            u::prints("Bones id",bone_names,bones_id);
        }
        
        last_known_positions.resize(bones_id.size());
        last_known_positions.fill({});
        last_known_velocities.resize(bones_id.size());
        last_known_velocities.fill({});
        last_known_result.resize(bones_id.size()*2*3);
        last_known_result.fill({});

    }
    virtual void setup_for_animation(Ref<Animation> animation)override{
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
            bone_tracks[bone_id] = Array::make(-1,-1);
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
        }
    }

    void set_skeleton_to_animation_timestamp(Ref<Animation> anim, float time){
        if (anim == nullptr || _skeleton == nullptr)
        {
            return;
        }
        for(size_t bone_id = 0; bone_id < _skeleton->get_bone_count() ; ++bone_id)
        {
            if (!bone_tracks.has(bone_id)) 
                continue;
            const auto pos = bone_tracks[bone_id][0];
            const auto quat = bone_tracks[bone_id][1];
            // const auto scale = bone_tracks[bone_id][2];
            if (pos >= 0 )
            {
                const Vector3 position = anim->position_track_interpolate(pos,time);
                _skeleton->set_bone_pose_position(bone_id,position);
            }

            if (quat >= 0 )
            {
                const Quaternion rotation = anim->rotation_track_interpolate(quat,time);
                _skeleton->set_bone_pose_rotation(bone_id,rotation);
            }
        }
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time)override{
        
        PackedVector3Array prev_pos{},curr_pos{};
        PackedFloat32Array result{};
        set_skeleton_to_animation_timestamp(animation,time-0.1);
        for(size_t index = 0; index < bones_id.size(); ++index)
        {
            const auto bone_id = bones_id[index];
            prev_pos.push_back(_skeleton->get_bone_global_pose(bone_id).get_origin() * _skeleton->get_motion_scale()) ;
        }
        set_skeleton_to_animation_timestamp(animation,time);
        for(size_t index = 0; index < bones_id.size(); ++index)
        {
            const auto bone_id = bones_id[index];
            curr_pos.push_back(_skeleton->get_bone_global_pose(bone_id).get_origin() * _skeleton->get_motion_scale());
        }
        const size_t root_id = _skeleton->find_bone(root_bone_name);
        const Transform3D root = _skeleton->get_bone_global_pose(root_id) * _skeleton->get_motion_scale();


        for(size_t index = 0; index < bones_id.size(); ++index)
        {
            const auto pos = root.xform_inv(curr_pos[index]);
            result.push_back(pos.x);result.push_back(pos.y);result.push_back(pos.z);
            const auto vel = root.basis.xform_inv(curr_pos[index] - prev_pos[index])/0.1;
            result.push_back(vel.x);result.push_back(vel.y);result.push_back(vel.z);
        }
        return result;
    }

    PackedVector3Array last_known_positions{};
    PackedVector3Array last_known_velocities{};
    PackedFloat32Array last_known_result{};
    float last_time_queried = 0.0f;

    GETSET(PackedVector3Array,bones_pos);
    GETSET(PackedVector3Array,bones_vel);

    virtual void physics_update(double delta) override {
        last_known_result.resize(bones_id.size()*2*3);
        bones_pos.resize(bones_id.size());
        bones_vel.resize(bones_id.size());
        for(size_t index = 0; index < bones_id.size(); ++index)
        {
            Vector3 pos = _skeleton->get_bone_global_pose(bones_id[index]).origin; // Bad naming : Not global pose, but model pose.
            Vector3 vel = (pos - bones_pos[index] ) / delta;
            bones_pos[index] = pos;
            bones_vel[index] = vel; 
        } 
    }

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard,float delta) override{
        // PackedVector3Array current_positions{}, current_velocities{};
        last_known_result.resize(bones_id.size()*2*3);
        bones_pos.resize(bones_id.size());
        bones_vel.resize(bones_id.size());

        constexpr size_t size = 3;
        for(size_t i = 0; i < bones_id.size(); ++i)
        {
            Vector3 pos = bones_pos[i], vel = bones_vel[i];

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
    virtual PackedFloat32Array get_weights() override{
        PackedFloat32Array result{};
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

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_weight_bone_pos", "value"), &BonePositionVelocityMotionFeature::set_weight_bone_pos);
        ClassDB::bind_method(D_METHOD("get_weight_bone_pos"), &BonePositionVelocityMotionFeature::get_weight_bone_pos);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_pos"), "set_weight_bone_pos", "get_weight_bone_pos");
        ClassDB::bind_method(D_METHOD("set_weight_bone_vel", "value"), &BonePositionVelocityMotionFeature::set_weight_bone_vel);
        ClassDB::bind_method(D_METHOD("get_weight_bone_vel"), &BonePositionVelocityMotionFeature::get_weight_bone_vel);
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_vel"), "set_weight_bone_vel", "get_weight_bone_vel");


        ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
        {
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
        // ClassDB::add_property_group(get_class_static(), "Queries to fill", "query");
        // {
            // Nothing to fill.
        // }

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