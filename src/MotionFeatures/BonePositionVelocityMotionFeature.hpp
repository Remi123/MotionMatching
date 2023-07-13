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
    Skeleton3D* skeleton = nullptr;
    // NodePath skeleton_path;
    void set_skeleton(Object* path){
        // skeleton_path = path;
        skeleton = Object::cast_to<Skeleton3D>(path);
        }
    Object* get_skeleton(){return cast_to<Object>(skeleton);}


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
    virtual void setup_nodes(Variant character) override{
        bones_id.clear();
        if(skeleton!=nullptr)
        {
            for(size_t i = 0; i < bone_names.size();++i)
            {
                const size_t id = skeleton->find_bone(bone_names[i]);
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
        last_known_velocities.fill({});

    }
    virtual void setup_for_animation(Ref<Animation> animation)override{
        skeleton->reset_bone_poses();
        bone_tracks.clear();
        bones_id.clear();
        for(size_t i = 0; i < bone_names.size();++i)
        {
            const size_t id = skeleton->find_bone(bone_names[i]);
            if (id >= 0)
                bones_id.push_back(id);
        }
        for(auto bone_id = 0; bone_id < skeleton->get_bone_count(); ++bone_id)
        {
            const auto bone_name = "%GeneralSkeleton:" + skeleton->get_bone_name(bone_id);
            PackedInt32Array tracks{};
            tracks.push_back(animation->find_track(NodePath(bone_name),Animation::TrackType::TYPE_POSITION_3D));
            tracks.push_back(animation->find_track(NodePath(bone_name),Animation::TrackType::TYPE_ROTATION_3D));
            //tracks.push_back(animation->find_track(NodePath(bone_name),Animation::TrackType::TYPE_SCALE_3D));
            bone_tracks[bone_id] = tracks;
        }

    }

    void set_skeleton_to_animation_timestamp(Ref<Animation> anim, float time){
        // UtilityFunctions::print((skeleton == nullptr)?"Skeleton error, path not found":"Skeleton set");
        if (anim == nullptr || skeleton == nullptr)
        {
            return;
        }
        for(size_t bone_id = 0; bone_id < skeleton->get_bone_count() ; ++bone_id)
        {
            if (!bone_tracks.has(bone_id)) 
                continue;
            const auto pos = bone_tracks[bone_id][0];
            const auto quat = bone_tracks[bone_id][1];
            // const auto scale = bone_tracks[bone_id][2];
            if (pos >= 0 )
            {
                const Vector3 position = anim->position_track_interpolate(pos,time);
                skeleton->set_bone_pose_position(bone_id,position);
            }

            if (quat >= 0 )
            {
                const Quaternion rotation = anim->rotation_track_interpolate(quat,time);
                skeleton->set_bone_pose_rotation(bone_id,rotation);
            }
                
            // const Vector3 scaling = anim->scale_track_interpolate(scale,time);

            
            // skeleton->set_bone_pose_scale(bone_id,scaling);
        }
    }

    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time)override{
        
        PackedVector3Array prev_pos{},curr_pos{};
        PackedFloat32Array result{};
        set_skeleton_to_animation_timestamp(animation,time-0.1);
        for(size_t index = 0; index < bones_id.size(); ++index)
        {
            const auto bone_id = bones_id[index];
            prev_pos.push_back(skeleton->get_bone_global_pose(bone_id).get_origin() * skeleton->get_motion_scale()) ;
        }
        set_skeleton_to_animation_timestamp(animation,time);
        for(size_t index = 0; index < bones_id.size(); ++index)
        {
            const auto bone_id = bones_id[index];
            curr_pos.push_back(skeleton->get_bone_global_pose(bone_id).get_origin() * skeleton->get_motion_scale());
        }
        const size_t root_id = skeleton->find_bone(root_bone_name);
        const Transform3D root = skeleton->get_bone_global_pose(root_id) * skeleton->get_motion_scale();


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

    virtual PackedFloat32Array broadphase_query_pose(Dictionary blackboard,float delta) override{
        PackedVector3Array current_positions{}, current_velocities{};
        last_known_result.resize(bones_id.size()*2*3);
        current_positions.resize(bones_id.size());
        current_velocities.resize(bones_id.size());

        float curr_time = Time::get_singleton()->get_ticks_msec();
        
        for(size_t index = 0; index < bones_id.size(); ++index)
        {
            Vector3 pos = skeleton->get_bone_global_pose(bones_id[index]).origin;            
            Vector3 vel = (pos - last_known_positions[index])/delta;
            current_positions[index] = pos;
            current_velocities[index] = vel;
        }

        last_time_queried = curr_time;
        last_known_positions = current_positions.duplicate();
        last_known_velocities = current_velocities.duplicate();

        const size_t size = 3;
        for(size_t i = 0; i < bones_id.size(); ++i)
        {
            Vector3 pos = current_positions[i], vel = current_velocities[i]; 

            last_known_result[i*size*2] = pos.x;last_known_result[i*size*2+1] = pos.y;last_known_result[i*size*2+2] = pos.z;
            last_known_result[i*size*2+size] = vel.x; last_known_result[i*size*2+size+1] = vel.y; last_known_result[i*size*2+size+2] = vel.z;
        }
        return last_known_result;
    }

    virtual float narrowphase_evaluate_cost(PackedFloat32Array to_convert)override{
        if (to_convert.size() != last_known_result.size())
        {
            return std::numeric_limits<float>::max();
        }
        float cost = 0.0;
        for(auto i = 0; i < to_convert.size();++i)
        {
            cost += abs(to_convert[i]*to_convert[i] - last_known_result[i]*last_known_result[i]);            
        }
        
        return cost;
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
        ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
        {
            ClassDB::bind_method(D_METHOD("set_weight_bone_pos", "value"), &BonePositionVelocityMotionFeature::set_weight_bone_pos);
            ClassDB::bind_method(D_METHOD("get_weight_bone_pos"), &BonePositionVelocityMotionFeature::get_weight_bone_pos);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_pos"), "set_weight_bone_pos", "get_weight_bone_pos");
            ClassDB::bind_method(D_METHOD("set_weight_bone_vel", "value"), &BonePositionVelocityMotionFeature::set_weight_bone_vel);
            ClassDB::bind_method(D_METHOD("get_weight_bone_vel"), &BonePositionVelocityMotionFeature::get_weight_bone_vel);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "weight_bone_vel"), "set_weight_bone_vel", "get_weight_bone_vel");

            ClassDB::bind_method(D_METHOD("set_skeleton", "skeleton_path"), &BonePositionVelocityMotionFeature::set_skeleton);
            ClassDB::bind_method(D_METHOD("get_skeleton"), &BonePositionVelocityMotionFeature::get_skeleton);
            ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "skeleton",PROPERTY_HINT_NODE_TYPE,"Skeleton3D",PROPERTY_USAGE_DEFAULT ), "set_skeleton", "get_skeleton");

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

        // }

        ClassDB::add_property_group(get_class_static(), "", "");

        ClassDB::bind_method( D_METHOD("get_dimension"), &BonePositionVelocityMotionFeature::get_dimension);
        ClassDB::bind_method( D_METHOD("setup_nodes","character"), &BonePositionVelocityMotionFeature::setup_nodes);
        
        ClassDB::bind_method( D_METHOD("setup_for_animation","animation"), &BonePositionVelocityMotionFeature::setup_for_animation);
        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &BonePositionVelocityMotionFeature::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("broadphase_query_pose","blackboard","delta"), &BonePositionVelocityMotionFeature::broadphase_query_pose);
        // ClassDB::bind_method( D_METHOD("narrowphase_evaluate_cost","data_to_evaluate"), &RootVelocityMotionFeature::narrowphase_evaluate_cost);
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