#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/variant.hpp>

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

#include <limits>
#include <algorithm>

#include <MotionFeatures/MFEvents.hpp>
#include <MMAnimationLibrary.hpp>


using namespace godot;

struct MFDistance : public MotionFeature {
    GDCLASS(MFDistance,MotionFeature)
    Ref<MMAnimationLibrary> mmlib = nullptr;
    std::vector<Ref<TagMFDistance>> animation_events{};

    public:

    GETSET(bool,relative_rotation,false);
    enum EmbeddedAxis{
        X,Y,Z
    };
    std::bitset<3> embedded_axis{}; 
    int get_embedded_axis(){return embedded_axis.to_ulong();} 
    void set_embedded_axis(int value){embedded_axis = std::bitset<3>(value);}

    GETSET(float,default_value, (1<<30));
    GETSET(bool,embed_as_frames);
    GETSET(bool,use_only_start,false);
    GETSET(godot::PackedStringArray,events_names);

    static constexpr float delta = 0.016f;

    virtual int get_dimension()override{return events_names.size() * 3;}
    
    virtual PackedFloat32Array get_weights()override{ return Array::make(1.0f,1.0f,1.0f);}

    String root_bone_track = "";
    Transform3D rest_pose = Transform3D();
    virtual bool setup_bake_init(Ref<MMAnimationLibrary> animlib)override{
        ERR_FAIL_COND_V_EDMSG(animlib->skeleton_path.is_empty(), false,"SkeletonPath is Empty");
        ERR_FAIL_COND_V_EDMSG(animlib->skeleton_profile == nullptr, false,"SkeletonProfile is null");
        ERR_FAIL_COND_V_EDMSG(animlib->skeleton_profile->get_root_bone().is_empty(),false,"No Root bone to extract data");
        // returning false will abort the process.
        // feel free to print more details
        mmlib = animlib;
        root_bone_track = u::str(animlib->skeleton_path) + ":" + animlib->skeleton_profile->get_root_bone();
        rest_pose = animlib->skeleton_profile->get_reference_pose(animlib->skeleton_profile->find_bone(animlib->skeleton_profile->get_root_bone()));
        return true;
    }
    virtual PackedStringArray get_hints()const {
        PackedStringArray hints = {};
        for(auto e : events_names)
        {
            hints.append(e + ":x");
            hints.append(e + ":y");
            hints.append(e + ":z");
        }
        return hints;}

    int root_track_pos = -1;
    int root_track_quat = -1;
    virtual bool setup_bake_animation(Ref<Animation> animation)override{
        auto tags = mmlib->tags;
        animation_events.clear();
        for(auto i = 0;i < mmlib->tags.size();++i)
        {
            if(TagMFDistance* event = Object::cast_to<TagMFDistance>(tags[i]); event != nullptr && event->animation_name == animation->get_name())
            {
                animation_events.push_back(event);
            }
        }
        root_track_pos = animation->find_track(NodePath(root_bone_track),Animation::TrackType::TYPE_POSITION_3D);
        root_track_quat = animation->find_track(NodePath(root_bone_track),Animation::TrackType::TYPE_ROTATION_3D);

        return true;
    }

    // the current logic is this : Take the first event
    virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation,float time) override {
        PackedFloat32Array result = {};
        std::vector<Ref<TagMFEvent>> current_events{};
        const float time_offset = 1.0f / Engine::get_singleton()->get_physics_ticks_per_second();
        // Get current events tags.
        for(Ref<TagMFDistance> tag : animation_events)
        {
            if(tag->timestamp <= time && time < tag->timestamp + tag->duration + time_offset)
            {
                current_events.push_back(tag);
            }
        }

        for(auto i = 0; i < events_names.size();++i)
        {
            const auto event_name = events_names[i];
            auto it = std::find_if(animation_events.begin(), animation_events.end(),[event_name](Ref<TagMFDistance> event){return event->event_name == event_name;});
            if(it == animation_events.end())
            {
                result.append(default_value);
                continue;
            }
            const auto event = *it;
            Vector3 value{};

            // Find distance from Root bone to the point depending on the tag.
            Vector3 root_pos{},anchor_pos{};
            Quaternion root_rot{};
            // Step 1 : Get root bone pos
            if(root_track_pos >= 0)
            {
                root_pos = animation->position_track_interpolate(root_track_pos,time);
            } else {
                root_pos = rest_pose.get_origin();
            }
            if(root_track_quat >= 0)
            {
                root_rot = animation->rotation_track_interpolate(root_track_quat,time);
            } else {
                root_rot = rest_pose.get_basis().get_rotation_quaternion();
            }
            // Step 2 : Get anchor point pos
            if (event->anchor_point_strategy == TagMFDistance::Strategy::RootPos)
            {
                kform const anchor_bone = kform::get_global(mmlib->skeleton_profile,animation,event->timestamp,NodePath(mmlib->skeleton_profile->get_root_bone()));
                anchor_pos = anchor_bone.pos;
            }
            else if(event->anchor_point_strategy == TagMFDistance::Strategy::AnchorPoint)
            {
                anchor_pos = event->reference_position;
            }
            else if (event->anchor_point_strategy == TagMFDistance::Strategy::AnchorBone)
            {
                kform const anchor_bone = kform::get_global(mmlib->skeleton_profile,animation,event->timestamp,NodePath(event->reference_bone));
                anchor_pos = anchor_bone.pos;
            }

            value = root_pos - (anchor_pos);

            result.append(value.x);
            result.append(value.y);
            result.append(value.z);
        }

        return result;
    }

    virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data,godot::Transform3D tr = godot::Transform3D{}){return;}

    static void _bind_methods() {
        
        ClassDB::bind_method( D_METHOD("set_events_names" ,"value"), &MFDistance::set_events_names); 
        ClassDB::bind_method( D_METHOD("get_events_names" ), &MFDistance::get_events_names); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY,"events_names"), "set_events_names", "get_events_names");

        ClassDB::bind_method( D_METHOD("set_relative_rotation" ,"value"), &MFDistance::set_relative_rotation,DEFVAL(false)); 
        ClassDB::bind_method( D_METHOD("get_relative_rotation" ), &MFDistance::get_relative_rotation); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"relative_rotation"), "set_relative_rotation", "get_relative_rotation");

        // auto prop_axis = PropertyInfo(Variant::INT,"embedded_axis"
        //             ,PROPERTY_HINT_ENUM,"Magnitude,X,Y,Z"
        //             ,PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED);
        // ClassDB::bind_method( D_METHOD("set_embedded_axis" ,"value"), &MFDistance::set_embedded_axis); 
        // ClassDB::bind_method( D_METHOD("get_embedded_axis" ), &MFDistance::get_embedded_axis); 
        // godot::ClassDB::add_property(get_class_static(), prop_axis, "set_embedded_axis", "get_embedded_axis");


        ClassDB::bind_method( D_METHOD("set_default_value" ,"value"), &MFDistance::set_default_value,DEFVAL(real_t(int32_t(1<<30)))); 
        ClassDB::bind_method( D_METHOD("get_default_value" ), &MFDistance::get_default_value); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"default_value"), "set_default_value", "get_default_value");
        
        ClassDB::bind_method( D_METHOD("get_dimension"), &MFDistance::get_dimension);

        ClassDB::bind_method( D_METHOD("get_weights"), &MFDistance::get_weights);

        ClassDB::bind_method( D_METHOD("get_hints"), &MFDistance::get_hints);
        
        
        ClassDB::bind_method( D_METHOD("setup_bake_init","mm_animation_library"), &MFDistance::setup_bake_init);        
        ClassDB::bind_method( D_METHOD("setup_bake_animation","animation"), &MFDistance::setup_bake_animation);

        ClassDB::bind_method( D_METHOD("bake_animation_pose","animation","time"), &MFDistance::bake_animation_pose);

        ClassDB::bind_method( D_METHOD("debug_pose_gizmo","gizmo","data","root_transform"), &MFDistance::debug_pose_gizmo);
        
    }
};
