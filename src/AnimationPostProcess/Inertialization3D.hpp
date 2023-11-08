
#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/animation_mixer.hpp>

#include <KForm.hpp>

#define GETSET(type,variable,...) type variable{__VA_ARGS__};\
    type get_##variable(){return  variable;}  \
    void set_##variable(type value){variable = value;}
#define STR(x) #x
#define STRING_PREFIX(prefix,s) STR(prefix##s)
#define BINDER_PROPERTY_PARAMS(type,variant_type,variable,...)\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(set_,variable) ,"value"), &type::set_##variable);\
        ClassDB::bind_method( D_METHOD(STRING_PREFIX(get_,variable) ), &type::get_##variable); \
        ADD_PROPERTY(PropertyInfo(variant_type,#variable,__VA_ARGS__),STRING_PREFIX(set_,variable),STRING_PREFIX(get_,variable));

using namespace godot;

struct Inertialization3D : godot::Node
{
    GDCLASS(Inertialization3D,Node);
    using u = godot::UtilityFunctions;

    kforms offsets = {0};
    kforms bones = {0};

    GETSET(AnimationMixer*,mixer,nullptr);
    Skeleton3D* skeleton{nullptr};
    Skeleton3D* get_skeleton(){return skeleton;} 
    void set_skeleton(Skeleton3D* value)
    {   skeleton = value;
        inertialize_reset();
    }

    GETSET(float,halflife,0.1f);
    GETSET(float,ratio,1.0f);

    bool active{true}; 
    bool get_active(){return active;} 
    void set_active(bool value){
        active = value;
        inertialize_reset();
        }
    

    virtual void _ready() override{
        inertialize_reset();
    }

    void inertialize_reset(){
        if (skeleton == nullptr ) return;
        const auto bone_count = skeleton->get_bone_count();
        bones.reserve(bone_count);
        offsets.reserve(bone_count);
        for (int b = 0; b < bone_count; ++b)
        {
            bones.reset(b);
            bones.pos[b] = skeleton->get_bone_pose_position(b);
            bones.rot[b] = skeleton->get_bone_pose_rotation(b);
            bones.scl[b] = skeleton->get_bone_pose_scale(b);

            offsets.reset(b);
        }
    }

    virtual void _process(double delta) override{
        if( mixer != nullptr && mixer->get_callback_mode_process() == AnimationMixer::AnimationCallbackModeProcess::ANIMATION_CALLBACK_MODE_PROCESS_IDLE)
            advance(delta);
    }

    virtual void _physics_process(double delta) override{
        if(mixer != nullptr && mixer->get_callback_mode_process() == AnimationMixer::AnimationCallbackModeProcess::ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS)
            advance(delta);
    }

    void advance(double delta)
    {
        if(active == false || skeleton == nullptr) return;

        bones.reserve(skeleton->get_bone_count());
        offsets.reserve(skeleton->get_bone_count());

        for(auto bone_id = 0; bone_id < skeleton->get_bone_count();++bone_id)
        {
            kform desired {};
            desired.pos = skeleton->get_bone_pose_position(bone_id);
            desired.rot = skeleton->get_bone_pose_rotation(bone_id);
            desired.vel = (desired.pos - bones.pos[bone_id])/delta;
            desired.ang =  Spring::quat_differentiate_angular_velocity(desired.rot, bones.rot[bone_id],delta);

            Spring::_simple_spring_damper_exact(
                bones.pos[bone_id],bones.vel[bone_id]
                ,desired.pos,halflife,delta
            );
            Spring::_simple_spring_damper_exact(
                bones.rot[bone_id],bones.ang[bone_id]
                ,desired.rot,halflife,delta
            );

            skeleton->set_bone_pose_position(bone_id,bones.pos[bone_id]);
            skeleton->set_bone_pose_rotation(bone_id,bones.rot[bone_id]);
        }
        
    }

    protected:
    static void _bind_methods()
    {
        ClassDB::bind_method( D_METHOD("set_active" ,"value"), &Inertialization3D::set_active,DEFVAL(true)); 
        ClassDB::bind_method( D_METHOD("get_active" ), &Inertialization3D::get_active); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"active"), "set_active", "get_active");


        ClassDB::bind_method( D_METHOD("set_halflife" ,"value"), &Inertialization3D::set_halflife); 
        ClassDB::bind_method( D_METHOD("get_halflife" ), &Inertialization3D::get_halflife); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT,"halflife"
        , PROPERTY_HINT_RANGE, "0.0,1.0,0.01,or_greater"), "set_halflife", "get_halflife");

        ClassDB::bind_method( D_METHOD("set_mixer" ,"value"), &Inertialization3D::set_mixer); 
        ClassDB::bind_method( D_METHOD("get_mixer" ), &Inertialization3D::get_mixer); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"mixer",PROPERTY_HINT_NODE_TYPE ,"AnimationMixer",PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_DEFAULT), "set_mixer", "get_mixer");
    
        ClassDB::bind_method( D_METHOD("set_skeleton" ,"value"), &Inertialization3D::set_skeleton); 
        ClassDB::bind_method( D_METHOD("get_skeleton" ), &Inertialization3D::get_skeleton); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"skeleton",PROPERTY_HINT_NODE_TYPE ,"Skeleton3D",PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_DEFAULT), "set_skeleton", "get_skeleton");
    }
};