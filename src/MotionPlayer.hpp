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

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

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
#define GETSET(type,variable,...) type variable{__VA_ARGS__}; type get_##variable(){return  variable;} void set_##variable(type value){variable = value;}
#define STR(x) #x
#define STRING_PREFIX(prefix,s) STR(prefix##s) 
// #define BINDER(type,variable,...)\
//         ClassDB::bind_method( D_METHOD(STRING_PREFIX(set_,variable),"value"), &type::set_##variable,__VA_ARGS__);\
//         ClassDB::bind_method( D_METHOD(STRING_PREFIX(get_,variable)), &type::get_##variable);
// #define BINDER_PROPERTY(type,variant_type,variable,...)\
//         BINDER(type,variable,__VA_ARGS__)\
//         ADD_PROPERTY(PropertyInfo(variant_type,#variable),STRING_PREFIX(set_,variable),STRING_PREFIX(get_,variable));
// #define BINDER_PROPERTY_PARAMS(type,variant_type,variable,...)\
//         BINDER(type,variable)\
//         ADD_PROPERTY(PropertyInfo(variant_type,#variable,__VA_ARGS__),STRING_PREFIX(set_,variable),STRING_PREFIX(get_,variable));

struct MotionPlayer : public Node {
    GDCLASS(MotionPlayer,Node)

    static constexpr float interval = 0.1;
    static constexpr float time_delta = 1.f / 30.f;

    MotionPlayer() = default;
    ~MotionPlayer() = default;

    // Properties and variables

    // The array. Will be sent to the KDtree.
    GETSET(PackedFloat32Array,MotionData);

    // Useful for passing data at runtime.
    GETSET(Dictionary,blackboard)

    // The track names inside the animations that define the categories, usually a value track to a int.
    GETSET(TypedArray<String>,category_track_names)

    // Skeleton
    Skeleton3D* skeleton = nullptr;
    // NodePath skeleton_path;
    void set_skeleton(Object* path){
        // skeleton_path = path;
        skeleton = Object::cast_to<Skeleton3D>(path);
        }
    Object* get_skeleton(){return cast_to<Object>(skeleton);}

    GETSET(NodePath,main_node)

    // Animation Library. Each one will be analysed
    GETSET(Ref<AnimationLibrary>,animation_library);

    // Array of the motion features.
    GETSET(TypedArray<MotionFeature>,motion_features);

    // Dimensional Stats.
    GETSET(PackedFloat32Array,weights)
    GETSET(PackedFloat32Array,means)
    GETSET(PackedFloat32Array,variances)
    GETSET(Array,densities) 

    // The KdTree.
    Kdtree::KdTree *  kdt = nullptr;

    // Database. A pose is just the index of a row in the kdtree.
    // Usage : db_anim_*[result.index] = 
    GETSET(PackedInt32Array,    db_anim_index);     // Index of the animation name in the animation library
    GETSET(PackedFloat32Array,  db_anim_timestamp); // timestamp of the pose in the animation
    GETSET(PackedInt32Array,    db_anim_category);  // Category of the pose in the animation

    // Simple helper. Might be removed from this class
    GETSET(int,category_value);

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

    // Functions

    virtual void _ready() override {
        if(godot::Engine::get_singleton()->is_editor_hint())
            return;

        Node* character;
        character = get_node<Node>(main_node);
        int nb_dimensions = 0;
        for(auto i = 0; i < motion_features.size(); ++i )
        {
            MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[i]);
            if(f != nullptr)
            {
                u::prints(f->get_name(), f->call("get_dimension").operator int64_t());
                f->setup_nodes(character);
                nb_dimensions += (int64_t)f->call("get_dimension");
            }

        }
        u::prints("Total Dimension", nb_dimensions);
        u::prints("Constructing kdtree");
                Kdtree::KdNodeVector nodes{};
        for(size_t i = 0; i < MotionData.size()/nb_dimensions; ++i)
        {
            auto begin = MotionData.ptr(), end = MotionData.ptr(); // We use the ptr as iterator.
            begin = std::next(begin,nb_dimensions * i);
            end = std::next(begin,nb_dimensions);
            std::vector<float> point(begin,end);
            nodes.push_back(Kdtree::KdNode(point,&db_anim_category[i],i));
        }

        

        // Now we bake all the data
        if(kdt != nullptr)
            delete kdt;
        kdt = new Kdtree::KdTree(&nodes,distance_type);

        auto begin = weights.ptr(), end = weights.ptr(); // We use the ptr as iterator.
        begin = std::next(begin,0);
        end =   std::next(begin,nb_dimensions);
        const std::vector<float> tmp_weight(begin,end);

        kdt->set_distance(distance_type,&tmp_weight);
        u::prints("Nb poses", (int64_t)nodes.size());
        u::prints("MotionPlayer Ready");
    }

    virtual void _physics_process(double delta)override{
        for(auto i = 0; i < motion_features.size(); ++i )
        {
            MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[i]);
            f->physics_update(delta);
        }
    }

    // Useful while baking data and in editor.
    void set_skeleton_to_pose(Ref<Animation> animation,double time)
    {
        auto skeleton_bones_path = Node::get_path_to(skeleton,true).get_concatenated_names();
        for(auto bone_id = 0; bone_id < skeleton->get_bone_count(); ++bone_id)
        {            
            const auto bone_name = skeleton_bones_path + String(":") +skeleton->get_bone_name(bone_id);
            
            const auto pos_track = animation->find_track(NodePath(bone_name),Animation::TrackType::TYPE_POSITION_3D);
            const auto rot_track = animation->find_track(NodePath(bone_name),Animation::TrackType::TYPE_ROTATION_3D);
            if ( pos_track >= 0 )
            {
                const Vector3 position = animation->position_track_interpolate(pos_track,time);
                skeleton->set_bone_pose_position(bone_id,position * skeleton->get_motion_scale());
            }            
            if (rot_track >= 0 )
            {
                const Quaternion rotation = animation->rotation_track_interpolate(rot_track,time);
                skeleton->set_bone_pose_rotation(bone_id,rotation);
            }
        }
    }

    // Reset the skeleton poses.
    void reset_skeleton_poses(){
        if (skeleton == nullptr)
        {
            u::print("Skeleton is empty");
            return;
        }
        UtilityFunctions::print((skeleton == nullptr)?"Skeleton error, path not found":"Skeleton set");
        
        UtilityFunctions::print("Resetting the skeleton");
        skeleton->reset_bone_poses();
        UtilityFunctions::print("Skeleton reset");
    }

    // Bake the data into the KdTree.
    // Goes through all the animations and construct the kdtree with each features at the interval.
    virtual void baking_data() {
        using namespace godot;
        using u = godot::UtilityFunctions;

        // skeleton = get_node<Skeleton3D>(skeleton_path);

        if (motion_features.size() == 0)
        {
            u::prints("Motions Features is empty");
            return;
        }
        else if(skeleton == nullptr)
        {
            u::prints("Skeleton isn't properly set");
            return;
        }
        Node* character;
        character = get_node<Node>(main_node);

        if(kdt != nullptr)
        {
            delete kdt;
        }

        int nb_dimensions = 0;
        // Setup the nodes for all features
        for(auto i = 0; i < motion_features.size(); ++i )
        {
            MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[i]);
            if(f == nullptr)
            {
                u::prints("Features no.",i,"is null");
            }
            else{
                u::prints(f->get_name(), f->get_dimension());
            }
            f->setup_nodes(character);
            nb_dimensions += (int)(f->get_dimension());
        }

        u::prints("Total Dimension", nb_dimensions);

        godot::TypedArray<godot::StringName> anim_names = animation_library->get_animation_list();
        u::print(anim_names);
        
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
                if (std::bitset<64>(tmp_category_value).test(31))
                {
                    continue;
                }

                PackedFloat32Array pose_data{};
                for(size_t features_index = 0; features_index < motion_features.size(); ++features_index )
                {
                    
                    MotionFeature* f = Object::cast_to<MotionFeature>(motion_features[features_index]);
                    PackedFloat32Array feature_data = f->bake_animation_pose(animation,time);
                    if (feature_data.size()  == f->get_dimension())
                    {
                        pose_data.append_array(feature_data);
                    }                    
                }

                
                for(int i = 0; i<nb_dimensions;++i)
                {
                    data_stats[i](pose_data[i]);
                }
                data.append_array(pose_data);
                // TODO Float to integer conversion ... needs better logic
                db_anim_index.append(anim_index);
                db_anim_timestamp.append(time);
                db_anim_category.append(tmp_category_value);

                ++counter;
            }
            auto clock_end = std::chrono::system_clock::now();
            float duration = float(std::chrono::duration_cast <std::chrono::milliseconds> (clock_end - clock_start).count());
            u::prints("Collecting animation data from ",animation->get_name(), " in ", duration, "ms. PoseCount",counter);
        }


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
        MotionData = data.duplicate();


        u::prints("Finished all animations");
        u::prints("NbDim",nb_dimensions,"NbPoses:",data.size()/nb_dimensions,"Size",data.size());

        Kdtree::KdNodeVector nodes{};
        for(size_t i = 0; i < data.size()/nb_dimensions; ++i)
        {
            auto begin = data.ptr(), end = data.ptr(); // We use the ptr as iterator.
            begin = std::next(begin,nb_dimensions * i);
            end = std::next(begin,nb_dimensions);
            std::vector<float> point(begin,end);
            nodes.push_back(Kdtree::KdNode(point,&db_anim_category[i],i));
        }

        u::prints("Nb poses", (int64_t)nodes.size());

        skeleton->reset_bone_poses();
    }

    // A predicate for searching animation categories.
    // included_category_bitfield : The animations must at least contains those categories
    // excluded_category_bitfield : Exclude every animations with any of those categories
    struct Category_Pred : Kdtree::KdNodePredicate
    {
        const std::bitset<64> m_desired_category;
        const std::bitset<64> m_exclude_category;
        Category_Pred(int64_t included_category_bitfield, int64_t excluded_category_bitfield = 0):
            m_desired_category{static_cast<uint64_t>(included_category_bitfield)}
            ,m_exclude_category{static_cast<uint64_t>(excluded_category_bitfield)}
        {}

        virtual bool operator()(const Kdtree::KdNode& node) const {
            static constexpr std::bitset<64> zero = {};
            const std::bitset<64> node_category = *((int32_t*)node.data);
            const bool include = (m_desired_category & node_category) == node_category;
            const bool exclude = (m_exclude_category & node_category) == zero;
            return include && exclude;
        }
    };

    // Calculate the weights using the features get_weights() functions.
    // Take into consideration the number of dimensions.
    // The calculation might be reconsidered, but it's the best I found.
    void recalculate_weights()
    {
        weights.clear();
        using namespace boost::accumulators;
        accumulator_set<double, stats<tag::min,tag::max,tag::sum,tag::count> > weight_stats, dim_stats, total;
        for (auto features_index = 0; features_index < motion_features.size(); ++features_index)
        {   
            MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
            
            weights.append_array(f->get_weights() );
            u::prints(f->get_name(),f->get_weights());
        }
        u::prints("Total:",weights);
        for(auto i = 0; i < weights.size(); ++i)
        {
            weight_stats(weights[i]);
        }
        u::prints("Sum weight:",(double)sum(weight_stats), "Count:",(double)count(weight_stats));
        for (auto features_index = 0; features_index < motion_features.size(); ++features_index)
        {
            MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);            
            dim_stats(f->get_dimension());
        }
        u::prints("Sum stats",sum(dim_stats));
        for (auto features_index = 0, offset = 0; features_index < motion_features.size(); ++features_index)
        {
            MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
            for(auto i = offset; i < offset + f->get_dimension();++i)
            {
                weights[i] = abs(weights[i]) / sum(weight_stats) / f->get_dimension();
                total(weights[i]);
            }
            offset += f->get_dimension();
        }
        u::prints("Sum",sum(weight_stats));
        if (min(total) < 1.0f && min(total) > 0.0f)
        {
            for(auto i = 0; i < weights.size(); ++i)
            {
                weights[i] *= 1 / min(total);
            }
        }
    }

    // query the kdtree.
    // Can include or exclude categories.
    TypedArray<Dictionary> query_pose(int64_t included_category = std::numeric_limits<int64_t>::max(), int64_t exclude = 0)
    {
        PackedFloat32Array query{};
        for (size_t features_index = 0; features_index < motion_features.size(); ++features_index)
        {
            MotionFeature *f = Object::cast_to<MotionFeature>(motion_features[features_index]);
            auto f_query = f->broadphase_query_pose(blackboard,0.016);
            if(f_query.size() == f->get_dimension())
                query.append_array(f_query);
        }
        // Normalization
        for (size_t i = 0; i < means.size();++i)
        {
            query[i] = (query[i] - means[i])/variances[i]; 
        }
        if(kdt == nullptr)
        {
            u::print("kdtree not init");
        }
        else{
            Kdtree::KdNodeVector re{};

            auto query_data = Kdtree::CoordPoint(query.ptr(),std::next(query.ptr(),kdt->dimension));
            auto clock_start = std::chrono::system_clock::now();
            if(included_category == std::numeric_limits<int64_t>::max())
                kdt->k_nearest_neighbors(query_data,1,&re);
            else
            {
                auto pred = Category_Pred(included_category,exclude);
                kdt->k_nearest_neighbors(query_data,1,&re,&pred);
            }

            auto clock_end = std::chrono::system_clock::now();
            
            float duration = float(std::chrono::duration_cast <std::chrono::microseconds> (clock_end - clock_start).count());




            TypedArray<Dictionary> results = {};

            String debug = "[";
            for(auto i = 0; i< re.size();++i)
            {
                Dictionary subresult{};
                
                const StringName anim_name = animation_library->get_animation_list()[db_anim_index[re[i].index]];
                const float anim_time = db_anim_timestamp[re[i].index];

                float cost = 0.0f;
                for(auto j = 0; j< query.size();++j)
                {
                    cost += weights[i] * abs(re[i].point[j] - query[j]);                    
                }

                subresult["animation"] = anim_name;
                subresult["timestamp"] = std::move(anim_time);
                subresult["cost"] = cost;

                results.append(subresult);
            }
            debug += "]";
            return results;
        }
        return {};

    }


    // Bypass the feature query, and ask directly which poses is the most similar.
    // The query must be of the correct dimension.
    Array check_query_results(PackedFloat32Array query,int64_t nb_result = 1)
    {
        if(kdt == nullptr)
        {
            auto nb_dimensions = query.size();
            Kdtree::KdNodeVector nodes{};
            for(size_t i = 0; i < MotionData.size()/nb_dimensions; ++i)
            {
                auto begin = MotionData.ptr(), end = MotionData.ptr(); // We use the ptr as iterator.
                begin = std::next(begin,nb_dimensions * i);
                end = std::next(begin,nb_dimensions);
                std::vector<float> point(begin,end);
                nodes.push_back(Kdtree::KdNode(point,nullptr,i));
            }       
            u::prints("KdTree Constructed 0 ");
            // Now we bake all the data
            // kdt->bake_nodes(data,nb_dimensions);
            if(kdt != nullptr)
            {    u::prints("KdTree Was occupy, deleting ");
                delete kdt;
            }
            kdt = new Kdtree::KdTree(&nodes,distance_type);
            u::prints("KdTree Constructed End");
        }

        auto begin = weights.ptr(), end = weights.ptr(); // We use the ptr as iterator.
        begin = std::next(begin,0);
        end =   std::next(begin,query.size());
        const std::vector<float> tmp_weight(begin,end);

        kdt->set_distance(distance_type,&tmp_weight);

        auto query_data = Kdtree::CoordPoint(query.ptr(),std::next(query.ptr(),query.size()));

        u::prints("query Constructed");

        Kdtree::KdNodeVector re{};
        kdt->k_nearest_neighbors(query_data,nb_result,&re);
        u::prints("Results obtained");
        Array result;
        for(auto i : re)
        {
            const auto anim_name = animation_library->get_animation_list()[db_anim_index[i.index]];
            const auto anim_time = db_anim_timestamp[i.index];
            const auto anim_cat = db_anim_category[i.index];
            result.append(Array::make(anim_name,anim_time,anim_cat));
        }
        return result;
    }



    protected:
    // Binding.
        static void _bind_methods()
        {
        // Private section
        {
            ClassDB::bind_method(D_METHOD("set_category_value", "value"), &MotionPlayer::set_category_value);
            ClassDB::bind_method(D_METHOD("get_category_value"), &MotionPlayer::get_category_value);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "category_value", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_CLASS_IS_BITFIELD | PROPERTY_USAGE_DEFAULT), "set_category_value", "get_category_value");
            // Stats
            ClassDB::bind_method(D_METHOD("set_means", "value"), &MotionPlayer::set_means);
            ClassDB::bind_method(D_METHOD("get_means"), &MotionPlayer::get_means);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "means", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_means", "get_means");
            ClassDB::bind_method(D_METHOD("set_variances", "value"), &MotionPlayer::set_variances);
            ClassDB::bind_method(D_METHOD("get_variances"), &MotionPlayer::get_variances);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "variances", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_variances", "get_variances");
            ClassDB::bind_method(D_METHOD("set_densities", "value"), &MotionPlayer::set_densities);
            ClassDB::bind_method(D_METHOD("get_densities"), &MotionPlayer::get_densities);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "densities", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_densities", "get_densities");
            // For retrieving the anim name and timestamp and category

            ClassDB::bind_method(D_METHOD("set_db_anim_index", "value"), &MotionPlayer::set_db_anim_index);
            ClassDB::bind_method(D_METHOD("get_db_anim_index"), &MotionPlayer::get_db_anim_index);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_INT32_ARRAY, "db_anim_index", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_db_anim_index", "get_db_anim_index");
            ClassDB::bind_method(D_METHOD("set_db_anim_timestamp", "value"), &MotionPlayer::set_db_anim_timestamp);
            ClassDB::bind_method(D_METHOD("get_db_anim_timestamp"), &MotionPlayer::get_db_anim_timestamp);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "db_anim_timestamp", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_db_anim_timestamp", "get_db_anim_timestamp");
            ClassDB::bind_method(D_METHOD("set_db_anim_category", "value"), &MotionPlayer::set_db_anim_category);
            ClassDB::bind_method(D_METHOD("get_db_anim_category"), &MotionPlayer::get_db_anim_category);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_INT32_ARRAY, "db_anim_category", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_STORAGE), "set_db_anim_category", "get_db_anim_category");
        }

        ClassDB::add_property_group(get_class_static(), "Nodes & Resources Sources", "");
        {
            ClassDB::bind_method(D_METHOD("set_main_node", "path"), &MotionPlayer::set_main_node);
            ClassDB::bind_method(D_METHOD("get_main_node"), &MotionPlayer::get_main_node);
            ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "main_node"), "set_main_node", "get_main_node");
            ClassDB::bind_method(D_METHOD("set_skeleton_path", "skeleton path"), &MotionPlayer::set_skeleton);
            ClassDB::bind_method(D_METHOD("get_skeleton"), &MotionPlayer::get_skeleton);
            ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "skeleton_node_path",PROPERTY_HINT_NODE_TYPE,"Skeleton3D",PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE ), "set_skeleton_path", "get_skeleton");

            ClassDB::bind_method(D_METHOD("set_animation_library", "value"), &MotionPlayer::set_animation_library);
            ClassDB::bind_method(D_METHOD("get_animation_library"), &MotionPlayer::get_animation_library);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT, "animation_library", PROPERTY_HINT_RESOURCE_TYPE, "AnimationLibrary"), "set_animation_library", "get_animation_library");
            ClassDB::bind_method(D_METHOD("set_category_track_names", "value"), &MotionPlayer::set_category_track_names);
            ClassDB::bind_method(D_METHOD("get_category_track_names"), &MotionPlayer::get_category_track_names);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY, "category_track_names", PROPERTY_HINT_NONE, "", PropertyUsageFlags::PROPERTY_USAGE_DEFAULT), "set_category_track_names", "get_category_track_names");
        }

        ClassDB::add_property_group(get_class_static(), "Data & KdTree params", "");
        {
            ClassDB::bind_method(D_METHOD("set_MotionData", "value"), &MotionPlayer::set_MotionData);
            ClassDB::bind_method(D_METHOD("get_MotionData"), &MotionPlayer::get_MotionData);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "MotionData"), "set_MotionData", "get_MotionData");
            ClassDB::bind_method(D_METHOD("set_distance_type", "value"), &MotionPlayer::set_distance_type);
            ClassDB::bind_method(D_METHOD("get_distance_type"), &MotionPlayer::get_distance_type);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::INT, "distance_type", PROPERTY_HINT_ENUM, "Manhattan:1,EuclidianSquared:2,Maximum:0"), "set_distance_type", "get_distance_type");
            ClassDB::bind_method(D_METHOD("set_weights", "value"), &MotionPlayer::set_weights);
            ClassDB::bind_method(D_METHOD("get_weights"), &MotionPlayer::get_weights);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "weights"), "set_weights", "get_weights");
        }

        ClassDB::add_property_group(get_class_static(), "Features", "");
        {
            ClassDB::bind_method(D_METHOD("set_motion_features", "value"), &MotionPlayer::set_motion_features);
            ClassDB::bind_method(D_METHOD("get_motion_features"), &MotionPlayer::get_motion_features);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::ARRAY, "motion_features", godot::PROPERTY_HINT_TYPE_STRING, "24/17:MotionFeature", PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_DEFAULT), "set_motion_features", "get_motion_features");
            ClassDB::bind_method(D_METHOD("set_blackboard", "value"), &MotionPlayer::set_blackboard);
            ClassDB::bind_method(D_METHOD("get_blackboard"), &MotionPlayer::get_blackboard);
            godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::DICTIONARY, "blackboard"), "set_blackboard", "get_blackboard");
        }

        ClassDB::add_property_group(get_class_static(), "", "");

        // Functions
        ClassDB::bind_method(D_METHOD("reset_skeleton_poses"), &MotionPlayer::reset_skeleton_poses);
        ClassDB::bind_method(D_METHOD("set_skeleton_to_pose", "animation", "time"), &MotionPlayer::set_skeleton_to_pose);

        ClassDB::bind_method(D_METHOD("recalculate_weights"), &MotionPlayer::recalculate_weights);
        ClassDB::bind_method(D_METHOD("baking_data"), &MotionPlayer::baking_data);
        ClassDB::bind_method(D_METHOD("query_pose", "include_category", "exclude_category"), &MotionPlayer::query_pose, DEFVAL(std::numeric_limits<int64_t>::max()), DEFVAL(0));
        ClassDB::bind_method(D_METHOD("check_query_results", "Query", "Result count"), &MotionPlayer::check_query_results);
        }
};

#undef MAKE_RESOURCE_TYPE_HINT
#undef GETSET
#undef STR
#undef STRING_PREFIX
// #undef BINDER
// #undef BINDER_PROPERTY
// #undef BINDER_PROPERTY_PARAMS