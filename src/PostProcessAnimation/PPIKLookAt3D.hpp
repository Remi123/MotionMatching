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

struct PPIKLookAt3D : godot::Node3D
{
    GDCLASS(PPIKLookAt3D,Node3D);
    public:
    using u = godot::UtilityFunctions;

    GETSET(AnimationMixer*,mixer,nullptr);
    GETSET(Skeleton3D*,skeleton,nullptr);

    GETSET(String,bone);
    GETSET(bool,active);

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
        if( active == false || bone.is_empty() || skeleton == nullptr) return;
        int bone_id = skeleton->find_bone(bone);
        if (bone_id == -1) return;

        Vector3 target_pos = get_global_position();
        Vector3 bone_global_pos = skeleton->get_global_position() + skeleton->get_bone_global_pose(bone_id).origin;
        Quaternion bone_global_rot = skeleton->get_global_transform().get_basis().get_rotation_quaternion() * skeleton->get_bone_global_pose(bone_id).basis.get_rotation_quaternion();
        
        Vector3 tar_pos = bone_global_rot.xform_inv(target_pos - bone_global_pos);
    
        Quaternion diff = Quaternion(get_global_basis().get_rotation_quaternion().xform(Vector3(0,0,1)),tar_pos.normalized());            

        // Rotate the head to face toward the target
        Quaternion bone_local_rot = skeleton->get_bone_pose_rotation(bone_id);
        skeleton->set_bone_pose_rotation(bone_id,bone_local_rot * diff);
    }
    

    protected:
    static void _bind_methods()
    {
        ClassDB::bind_method( D_METHOD("set_active" ,"value"), &PPIKLookAt3D::set_active); 
        ClassDB::bind_method( D_METHOD("get_active" ), &PPIKLookAt3D::get_active); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"active"), "set_active", "get_active");

        ClassDB::bind_method( D_METHOD("set_bone" ,"value"), &PPIKLookAt3D::set_bone); 
        ClassDB::bind_method( D_METHOD("get_bone" ), &PPIKLookAt3D::get_bone); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"bone"), "set_bone", "get_bone");

        ClassDB::bind_method( D_METHOD("set_mixer" ,"value"), &PPIKLookAt3D::set_mixer); 
        ClassDB::bind_method( D_METHOD("get_mixer" ), &PPIKLookAt3D::get_mixer); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"mixer",PROPERTY_HINT_NODE_TYPE ,"AnimationMixer",PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_DEFAULT), "set_mixer", "get_mixer");
    
        ClassDB::bind_method( D_METHOD("set_skeleton" ,"value"), &PPIKLookAt3D::set_skeleton); 
        ClassDB::bind_method( D_METHOD("get_skeleton" ), &PPIKLookAt3D::get_skeleton); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"skeleton",PROPERTY_HINT_NODE_TYPE ,"Skeleton3D",PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_DEFAULT), "set_skeleton", "get_skeleton");
    
    }
};