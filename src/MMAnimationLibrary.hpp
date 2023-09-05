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

#include <godot_cpp/classes/v_box_container.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/bone_map.hpp>

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skeleton_profile.hpp>

#include <bitset>
#include <vector>
#include <chrono>
#include <algorithm>
#include <numeric>

#include "godot_cpp/core/math.hpp"

#include "kdtree-cpp/kdtree.hpp"
#include "MotionFeatures/MotionFeatures.hpp"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>


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

// TODO : Save the array in a hashed data structure, so that multiple object doesn't add a new array for each schema.
struct MMAnimationLibrary : public AnimationLibrary {
    using u = godot::UtilityFunctions;
    GDCLASS(MMAnimationLibrary,AnimationLibrary)

    GETSET(Ref<SkeletonProfile>,skeleton_profile)

    // Category tracks
    GETSET(TypedArray<String>,category_track_names)
    // Array of the motion features.
    GETSET(TypedArray<MotionFeature>, motion_features);
    // The data
    GETSET(PackedFloat32Array,MotionData);

    // Dimensional Stats.
    GETSET(int,nb_dimensions)
    GETSET(PackedFloat32Array,weights)
    GETSET(PackedFloat32Array,means)
    GETSET(PackedFloat32Array,variances)
    GETSET(Array,densities) 

    // Database. A pose is just the index of a row in the kdtree.
    // Usage : db_anim_*[result.index] = 
    GETSET(PackedInt32Array,    db_anim_index);     // Index of the animation name in the animation library
    GETSET(PackedFloat32Array,  db_anim_timestamp); // timestamp of the pose in the animation
    GETSET(PackedInt32Array,    db_anim_category);  // Category of the pose in the animation

    // The KdTree.
    Kdtree::KdTree *  kdt = nullptr;

    void _notification(int what){
        switch ( what ){
            case NOTIFICATION_POSTINITIALIZE: // Constructor
            {
                u::prints("MMAL NOTIFICATION_POSTINITIALIZE","InEditor:",godot::Engine::get_singleton()->is_editor_hint());
            }
            break;
            case NOTIFICATION_PREDELETE: // Destructor
            {
                u::prints("MMAL NOTIFICATION_PREDELETE","InEditor:",godot::Engine::get_singleton()->is_editor_hint());
            }
            break;
        }
    }

    void bake_data()
    {
        if (motion_features.is_empty())
        {
            u::prints("Motions Features is empty");
            return;
        }
        else if(skeleton_profile == nullptr)
        {
            u::prints("Skeleton_profile isn't properly set");
            return;
        }

        if(kdt != nullptr)
        {
            delete kdt;
        }

        for(auto i = 0; i < motion_features.size(); ++i )
        {
            MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[i]);
            if(f == nullptr)
            {
                u::prints("Features no.",i,"is null");
                return;
            }
            else{
                u::prints(f->get_name(), f->get_dimension());
            }
            f->setup_profile(skeleton_profile);
            nb_dimensions += (int)(f->get_dimension());
        }
        u::prints("Total Dimension", nb_dimensions);

        godot::TypedArray<godot::StringName> anim_names = get_animation_list();
        u::prints("Detecting",anim_names.size(),"animations. Preparing...");
        
        means.clear(); means.resize(nb_dimensions); means.fill(0.0f);
        variances.clear();variances.resize(nb_dimensions); variances.fill(0.0f);
        densities.clear();densities.resize(nb_dimensions); densities.fill(Array::make(0.0,0.0));

        PackedFloat32Array data = PackedFloat32Array();
        MotionData.clear();

        db_anim_category.clear();db_anim_index.clear();db_anim_timestamp.clear();

        using namespace boost::accumulators;
        using acc_stats = stats<tag::density,tag::max,tag::min,tag::median,tag::skewness,tag::variance>;
        const accumulator_set<float,acc_stats> default_acc (tag::density::num_bins = 10, tag::density::cache_size = 15);
        std::vector<accumulator_set<float,acc_stats>> data_stats(nb_dimensions,default_acc);


    }

protected:
    static void _bind_methods()
    {
        // Functions
        {
            ClassDB::bind_method(D_METHOD("_notification","what"), &MMAnimationLibrary::_notification);
            ClassDB::bind_method(D_METHOD("bake_data"), &MMAnimationLibrary::bake_data);
        }
        ClassDB::add_property_group(get_class_static(), "Dependancy resources", "");
        {
                ClassDB::bind_method(D_METHOD("set_skeleton_profile", "value"), &MMAnimationLibrary::set_skeleton_profile);
                ClassDB::bind_method(D_METHOD("get_skeleton_profile"), &MMAnimationLibrary::get_skeleton_profile);
                godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT, "skeleton_profile", PROPERTY_HINT_RESOURCE_TYPE, "SkeletonProfile"), "set_skeleton_profile", "get_skeleton_profile");
        }
        ClassDB::add_property_group(get_class_static(), "Features", "");
        {
                ClassDB::bind_method(D_METHOD("set_category_track_names", "value"), &MMAnimationLibrary::set_category_track_names);
                ClassDB::bind_method(D_METHOD("get_category_track_names"), &MMAnimationLibrary::get_category_track_names);
                godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY, "category_track_names", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_DEFAULT), "set_category_track_names", "get_category_track_names");

                ClassDB::bind_method(D_METHOD("set_motion_features", "value"), &MMAnimationLibrary::set_motion_features);
                ClassDB::bind_method(D_METHOD("get_motion_features"), &MMAnimationLibrary::get_motion_features);
                godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "motion_features", godot::PROPERTY_HINT_TYPE_STRING, u::str(Variant::OBJECT) + '/' + u::str(Variant::BASIS) + ":MotionFeature", PROPERTY_USAGE_DEFAULT), "set_motion_features", "get_motion_features");
        }
        ClassDB::add_property_group(get_class_static(), "Data & KdTree params", "");
        {
            ClassDB::bind_method(D_METHOD("set_MotionData", "value"), &MMAnimationLibrary::set_MotionData);
            ClassDB::bind_method(D_METHOD("get_MotionData"), &MMAnimationLibrary::get_MotionData);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "MotionData"), "set_MotionData", "get_MotionData");
            // ClassDB::bind_method(D_METHOD("set_distance_type", "value"), &MMAnimationLibrary::set_distance_type);
            // ClassDB::bind_method(D_METHOD("get_distance_type"), &MMAnimationLibrary::get_distance_type);
            // godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "distance_type", PROPERTY_HINT_ENUM, "Manhattan:1,EuclidianSquared:2,Maximum:0"), "set_distance_type", "get_distance_type");
            ClassDB::bind_method(D_METHOD("set_weights", "value"), &MMAnimationLibrary::set_weights);
            ClassDB::bind_method(D_METHOD("get_weights"), &MMAnimationLibrary::get_weights);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "weights"), "set_weights", "get_weights");
        }
    }
};