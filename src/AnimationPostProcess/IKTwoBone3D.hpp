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

struct IKTwoBone3D : godot::Node3D
{
    GDCLASS(IKTwoBone3D,Node3D);
    using u = godot::UtilityFunctions;

    GETSET(AnimationMixer*,mixer,nullptr);
    GETSET(Skeleton3D*,skeleton,nullptr);

    GETSET(String,bone_A); // UpLeg
    GETSET(String,bone_B); // Leg
    GETSET(String,bone_C); // Heel

    GETSET(bool,active);

    enum Bone_Type{
        BONE_ROOT,
        BONE_MIDDLE,
        BONE_REACH,
        BONE_PARENT,

        BONE_COUNT
    };

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
        if(active == false || skeleton == nullptr || mixer == nullptr) return;
        kforms locals{BONE_COUNT},globals{BONE_COUNT};
        if(bone_A.is_empty() || bone_B.is_empty() || bone_C.is_empty())
        {
            return;
        };

        int bone_A_id = skeleton->find_bone(bone_A);
        int bone_B_id = skeleton->find_bone(bone_B);
        int bone_C_id = skeleton->find_bone(bone_C);
        if(bone_A_id == -1 || bone_B_id == -1 || bone_C_id == -1)
        {
            return;
        };
        
        locals.pos[BONE_ROOT] = skeleton->get_bone_pose_position(bone_A_id);
        locals.rot[BONE_ROOT] = skeleton->get_bone_pose_rotation(bone_A_id);
        globals.pos[BONE_ROOT] = skeleton->get_global_position() + skeleton->get_bone_global_pose(bone_A_id).origin;
        globals.rot[BONE_ROOT] = skeleton->get_global_transform().get_basis().get_rotation_quaternion() * skeleton->get_bone_global_pose(bone_A_id).basis.get_rotation_quaternion();
        
        locals.pos[BONE_MIDDLE] = skeleton->get_bone_pose_position(bone_B_id);
        locals.rot[BONE_MIDDLE] = skeleton->get_bone_pose_rotation(bone_B_id);
        globals.pos[BONE_MIDDLE] = skeleton->get_global_position() + skeleton->get_bone_global_pose(bone_B_id).origin;
        globals.rot[BONE_MIDDLE] = skeleton->get_global_transform().get_basis().get_rotation_quaternion() * skeleton->get_bone_global_pose(bone_B_id).basis.get_rotation_quaternion();
     
        locals.pos[BONE_REACH] = skeleton->get_bone_pose_position(bone_C_id);
        locals.rot[BONE_REACH] = skeleton->get_bone_pose_rotation(bone_C_id);
        globals.pos[BONE_REACH] = skeleton->get_global_position() + skeleton->get_bone_global_pose(bone_C_id).origin;
        globals.rot[BONE_REACH] = skeleton->get_global_transform().get_basis().get_rotation_quaternion() * skeleton->get_bone_global_pose(bone_C_id).basis.get_rotation_quaternion();
    
        auto parent_id = skeleton->get_bone_parent(bone_A_id);
        locals.pos[BONE_PARENT] = skeleton->get_bone_pose_position(parent_id);
        locals.rot[BONE_PARENT] = skeleton->get_bone_pose_rotation(parent_id);
        globals.pos[BONE_PARENT] = skeleton->get_global_position() + skeleton->get_bone_global_pose(parent_id).origin;
        globals.rot[BONE_PARENT] = skeleton->get_global_transform().get_basis().get_rotation_quaternion() * skeleton->get_bone_global_pose(parent_id).basis.get_rotation_quaternion();
        
        op_two_bone_ik_static(
            locals,globals,get_global_position(), get_global_basis().get_rotation_quaternion().xform(Vector3(0,0,1))
        );
        skeleton->set_bone_pose_rotation(bone_A_id,locals.rot[BONE_ROOT]);
        skeleton->set_bone_pose_rotation(bone_B_id,locals.rot[BONE_MIDDLE]);
    }

        void op_two_bone_ik_static(
        kforms& local,
        const kforms& global,
        const Vector3 heel_target,
        const Vector3 fwd = Vector3(0.0f, 1.0f, 0.0f),
        const float max_length_buffer = 0.01f)
    {
        float max_extension = 
            (global.pos[BONE_ROOT] - global.pos[BONE_MIDDLE]).length() + 
            (global.pos[BONE_MIDDLE] - global.pos[BONE_REACH]).length() - 
            max_length_buffer;
        
        Vector3 target_clamp = heel_target;
        if ((heel_target - global.pos[BONE_ROOT]).length() > max_extension)
        {
            target_clamp = global.pos[BONE_ROOT] + max_extension * (heel_target - global.pos[BONE_ROOT]).normalized();
        }
        
        Vector3 axis_dwn = (global.pos[BONE_REACH] - global.pos[BONE_ROOT]).normalized();
        Vector3 axis_rot = (axis_dwn.cross(global.rot[BONE_MIDDLE].xform(fwd))).normalized();

        Vector3 a = global.pos[BONE_ROOT];
        Vector3 b = global.pos[BONE_MIDDLE];
        Vector3 c = global.pos[BONE_REACH];
        Vector3 t = target_clamp;
        
        float lab = (b - a).length();
        float lcb = (b - c).length();
        float lat = (t - a).length();

        float ac_ab_0 = acosf(u::clampf((c - a).normalized().dot((b - a).normalized()), -1.0f, 1.0f));
        float ba_bc_0 = acosf(u::clampf((a - b).normalized().dot((c - b).normalized()), -1.0f, 1.0f));

        float ac_ab_1 = acosf(u::clampf((lab * lab + lat * lat - lcb * lcb) / (2.0f * lab * lat), -1.0f, 1.0f));
        float ba_bc_1 = acosf(u::clampf((lab * lab + lcb * lcb - lat * lat) / (2.0f * lab * lcb), -1.0f, 1.0f));


        Quaternion r0 = Quaternion(axis_rot,ac_ab_1 - ac_ab_0);// Spring::quat_from_angle_axis(ac_ab_1 - ac_ab_0, axis_rot);
        Quaternion r1 = Quaternion(axis_rot,ba_bc_1 - ba_bc_0);//Spring::quat_from_angle_axis(ba_bc_1 - ba_bc_0, axis_rot);

        Vector3 c_a = (global.pos[BONE_REACH] - global.pos[BONE_ROOT]).normalized();
        Vector3 t_a = (target_clamp - global.pos[BONE_ROOT]).normalized();

        // Quaternion r2 = Spring::quat_from_angle_axis(
        //     acosf(u::clampf(c_a.dot(t_a), -1.0f, 1.0f)),
        //     (c_a.cross(t_a).normalized()));
        Quaternion r2 = Quaternion(c_a.cross(t_a).normalized(),acosf(u::clampf(c_a.dot(t_a), -1.0f, 1.0f)));
        
        local.rot[BONE_ROOT] = global.rot[BONE_PARENT].inverse() * (r2 * r0 * global.rot[BONE_ROOT]);
        // local.rot[BONE_ROOT] = quat_inv_mul(global.rot[BONE_PARENT], quat_mul(r2, quat_mul(r0, global.rot[BONE_ROOT])));
        local.rot[BONE_MIDDLE] = global.rot[BONE_ROOT]. inverse() * r1 * global.rot[BONE_MIDDLE];
        // local.rot[BONE_MIDDLE] = quat_inv_mul( global.rot[BONE_ROOT], quat_mul(r1, global.rot[BONE_MIDDLE]));

    }

    protected:
    static void _bind_methods()
    {
        ClassDB::bind_method( D_METHOD("set_active" ,"value"), &IKTwoBone3D::set_active); 
        ClassDB::bind_method( D_METHOD("get_active" ), &IKTwoBone3D::get_active); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"active"), "set_active", "get_active");

        ClassDB::bind_method( D_METHOD("set_bone_A" ,"value"), &IKTwoBone3D::set_bone_A); ClassDB::bind_method( D_METHOD("get_bone_A" ), &IKTwoBone3D::get_bone_A); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"bone_A"), "set_bone_A", "get_bone_A");
        ClassDB::bind_method( D_METHOD("set_bone_B" ,"value"), &IKTwoBone3D::set_bone_B); ClassDB::bind_method( D_METHOD("get_bone_B" ), &IKTwoBone3D::get_bone_B); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"bone_B"), "set_bone_B", "get_bone_B");
        ClassDB::bind_method( D_METHOD("set_bone_C" ,"value"), &IKTwoBone3D::set_bone_C); ClassDB::bind_method( D_METHOD("get_bone_C" ), &IKTwoBone3D::get_bone_C); godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::STRING,"bone_C"), "set_bone_C", "get_bone_C");

        ClassDB::bind_method( D_METHOD("set_mixer" ,"value"), &IKTwoBone3D::set_mixer); 
        ClassDB::bind_method( D_METHOD("get_mixer" ), &IKTwoBone3D::get_mixer); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"mixer",PROPERTY_HINT_NODE_TYPE ,"AnimationMixer",PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_DEFAULT), "set_mixer", "get_mixer");
    
        ClassDB::bind_method( D_METHOD("set_skeleton" ,"value"), &IKTwoBone3D::set_skeleton); 
        ClassDB::bind_method( D_METHOD("get_skeleton" ), &IKTwoBone3D::get_skeleton); 
        godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::OBJECT,"skeleton",PROPERTY_HINT_NODE_TYPE ,"Skeleton3D",PROPERTY_USAGE_SCRIPT_VARIABLE | PROPERTY_USAGE_DEFAULT), "set_skeleton", "get_skeleton");
    
    }
};