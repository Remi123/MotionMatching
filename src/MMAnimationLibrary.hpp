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

    MMAnimationLibrary()
    {
        u::prints("MMAL","Constructor");
        fill_kdtree();
    }
    ~MMAnimationLibrary(){
        u::prints("MMAL", "Destructor");
        if (kdt != nullptr)
        {
            delete kdt;
        }
    }
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
                if(kdt != nullptr){
                    delete kdt;
                }
            }
            break;
            default:
                u::prints("MMAL Default notification",what);
        }
    }

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


    // How the kdtree calculate the distance.
    // 0 (L0) : Maximum of each difference in all dimensions.
    // 1 (L1) : Manhattan distance (default)
    // 2 (L2) : Distance squared.
    int distance_type = 1; int get_distance_type(){return distance_type;} 
    void set_distance_type(int value){
        distance_type = value;
        if(kdt != nullptr && 0 <= distance_type && distance_type <= 2)
            kdt->set_distance(distance_type);
    }

    void fill_kdtree()
    {
        ERR_FAIL_COND_EDMSG(motion_features.is_empty(),"No Motion Features to extract data");
        ERR_FAIL_COND_EDMSG(MotionData.is_empty(),"Motion Data is Empty");
        int nb_dimensions = 0;
        for(auto i = 0; i < motion_features.size(); ++i )
        {
            MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[i]);
            if(f != nullptr)
            {
                auto dim = (int64_t)f->call("get_dimension");
                ERR_FAIL_COND_MSG(dim <= 0,"Motions Feature index " + u::str(i) +" can't have dimensions be zero or less");
                nb_dimensions += dim;
            }
        }
        

        u::prints("Total Dimension", nb_dimensions);
        if(kdt != nullptr)
            delete kdt;

        // Now we bake all the data
        u::prints("Preparing kdtree");
        Kdtree::KdNodeVector nodes{};
        for(size_t i = 0; i < MotionData.size()/nb_dimensions; ++i)
        {
            auto begin = MotionData.ptr(), end = MotionData.ptr(); // We use the ptr as iterator.
            begin = std::next(begin,nb_dimensions * i);
            end = std::next(begin,nb_dimensions);
            std::vector<float> point(begin,end);
            nodes.push_back(Kdtree::KdNode(std::move(point),&db_anim_category[i],i));
        }
        kdt = new Kdtree::KdTree(&nodes,distance_type);

        auto begin = weights.ptr(), end = weights.ptr(); // We use the ptr as iterator.
        begin = std::next(begin,0);
        end =   std::next(begin,nb_dimensions);
        const std::vector<float> tmp_weight(begin,end);

        kdt->set_distance(distance_type,&tmp_weight);
        u::prints("KDTree Constructed");
    }





    void bake_data()
    {
        ERR_FAIL_COND_EDMSG(motion_features.is_empty(),"No Motion Features to extract data");
        ERR_FAIL_COND_EDMSG(skeleton_profile == nullptr,"Skeleton_profile is empty");

        if(kdt != nullptr)
        {
            delete kdt;
        }
        u::prints("Preparing Features...");
        for(auto i = 0; i < motion_features.size(); ++i )
        {
            MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[i]);
            ERR_FAIL_NULL_MSG(f,"Features no."+u::str(i) + "is null");
            u::prints("Feature no.",i,f->get_name(),"Dimensions:", f->get_dimension());
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

        for(auto anim_index = 0; anim_index < anim_names.size(); ++anim_index)
        {
            auto clock_start = std::chrono::system_clock::now();

            auto anim_name = anim_names[anim_index];
            auto animation = animation_library->get_animation(anim_name);


            std::vector<int32_t> category_tracks{};
            for(auto i = 0 ; i<category_track_names.size();++i)
            {
                 auto category_track = animation->find_track((String)category_track_names[i],Animation::TrackType::TYPE_VALUE);
                 animation->value_track_set_update_mode(category_track,Animation::UpdateMode::UPDATE_DISCRETE);
                 animation->track_set_interpolation_type(category_track,Animation::InterpolationType::INTERPOLATION_NEAREST);
                 if (category_track != -1)
                 {
                    category_tracks.push_back(category_track);                    
                 }
                 u::prints("Checking Category Track",category_track_names[i], "result:",category_track != -1);
            }

            for(auto features_index = 0; features_index < motion_features.size(); ++features_index )
            {
                MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[features_index]);
                f->setup_for_animation(animation);
            }
            const auto length = animation->get_loop_mode() == Animation::LOOP_NONE ? animation->get_length() - 0.2 : animation->get_length() ;

            u::prints("Animations setup for",anim_name,"duration",length);

            auto counter = 0;
            for(auto time = 0.1f; time < length; time += 0.1f)
            {
                int64_t tmp_category_value = 0;
                for(const auto& category:category_tracks)
                {
                    tmp_category_value = tmp_category_value | (int64_t)animation->value_track_interpolate(category,time);
                }
                // If the reserved category value contain DONOTUSE (31th bit set to true), then we skip.
                if (std::bitset<64>(tmp_category_value).test(31))
                {
                    continue;
                }

                PackedFloat32Array pose_data{};
                for(size_t features_index = 0; features_index < motion_features.size(); ++features_index )
                {
                    MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[features_index]);
                    PackedFloat32Array feature_data = f->bake_animation_pose(animation,time);
                    ERR_FAIL_COND_MSG(feature_data.size() != f->get_dimension(),"Features no." + u::str(features_index)+"bake_animation_pose didn't return a array of the correct size:" +u::str(feature_data.size())+ "!=" + u::(f->get_dimension()) );
                    pose_data.append_array(feature_data);                    
                }

                
                for(int i = 0; i<nb_dimensions;++i)
                {
                    data_stats[i](pose_data[i]);
                }
                data.append_array(pose_data);
                db_anim_index.append(anim_index);
                db_anim_timestamp.append(time);
                db_anim_category.append(tmp_category_value);

                ++counter;
            }
            auto clock_end = std::chrono::system_clock::now();
            float duration = float(std::chrono::duration_cast <std::chrono::milliseconds> (clock_end - clock_start).count());
            u::prints("Collecting animation data from ",animation->get_name(), " in ", duration, "ms. PoseCount",counter);
        }

        u::prints("Animation Data Collected. Normalizing... ");

        for(auto i = 0; i< nb_dimensions;++i)
        {
            means[i] = mean(data_stats[i]);
            variances[i] = variance(data_stats[i]);
            Array arr{};
            for(const auto& d : density(data_stats[i]))
            {
                arr.append(Array::make(d.first,d.second) ); 
            }
            densities[i] = std::move(arr);
        }

        for(auto& v : variances )
        {
            if (v <= std::numeric_limits<float>::epsilon() )
            {
                v = 1.0f;
            }
        }
        
        // // Normalization
        for(size_t pose = 0; pose < data.size()/nb_dimensions; ++pose)
        {
            for(int offset = 0; offset<nb_dimensions;++offset)
            {
                data[pose*nb_dimensions + offset] = (data[pose*nb_dimensions + offset] - means[offset])/variances[offset]; 
            }
        }
        u::prints("Data Normalized. Copy data to Motion Data property...");
        MotionData = data.duplicate();

        u::prints("Finished All Animations");
        u::prints("NbDim",nb_dimensions,"NbPoses:",data.size()/nb_dimensions,"Size",data.size());
    }

protected:
    static void _bind_methods()
    {
        // Functions
        {
            // ClassDB::bind_method(D_METHOD("_notification","what"), &MMAnimationLibrary::_notification);
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
            ClassDB::bind_method(D_METHOD("set_distance_type", "value"), &MMAnimationLibrary::set_distance_type);
            ClassDB::bind_method(D_METHOD("get_distance_type"), &MMAnimationLibrary::get_distance_type);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "distance_type", PROPERTY_HINT_ENUM, "Manhattan:1,EuclidianSquared:2,Maximum:0"), "set_distance_type", "get_distance_type");
            ClassDB::bind_method(D_METHOD("set_weights", "value"), &MMAnimationLibrary::set_weights);
            ClassDB::bind_method(D_METHOD("get_weights"), &MMAnimationLibrary::get_weights);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "weights"), "set_weights", "get_weights");
        }
    }
};