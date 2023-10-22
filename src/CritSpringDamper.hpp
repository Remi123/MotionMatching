#pragma once

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <cmath>
#include <math.h>

using namespace godot;

struct CritDampSpring : public RefCounted
{
    GDCLASS(CritDampSpring,RefCounted)

    static constexpr float Ln2 = 0.69314718056;

    static float inline square(float x) { 
        return x * x;
    }

    static Vector3 damp_adjustment_exact(Vector3 g, float halflife, float dt, float eps=1e-8) {
        float factor = 1.0 - fast_negexp((CritDampSpring::Ln2 * dt) / (halflife + eps));
        return g * factor;
    }

    static Quaternion damp_adjustment_exact_quat(Quaternion g, float halflife, float dt, float eps=1e-8) {
        float factor = 1.0 - fast_negexp((CritDampSpring::Ln2 * dt) / (halflife + eps));
        return Quaternion().slerp(g, factor).normalized();
    }
    static Variant damper_exponential(Variant variable, Variant goal, float damping, float dt) {
        float ft = 1.0f / (float)ProjectSettings::get_singleton()->get("physics/common/physics_ticks_per_second");
        float factor = 1.0f - pow(1.0f / (1.0f - ft * damping), -dt / ft);
        return Math::lerp(variable, goal, factor);
    }

    static inline float fast_negexp(float x) {
        return 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    }

    static inline Variant damper_exact(Variant variable, Variant goal, float halflife, float dt, float eps = 1e-5f) {
        return Math::lerp(variable, goal, 1.0f - fast_negexp((CritDampSpring::Ln2 * dt) / (halflife + eps)));
    }

    static inline float halflife_to_damping(float halflife, float eps = 1e-5f) {
        return (4.0f * Ln2) / (halflife + eps);
    }

    static inline float halflife_to_duration(float halflife, float initial_value = 1.0f, float eps = 1e-5f){
        return halflife * (log(eps/initial_value) / log(0.5));
    }

    static inline float damping_to_halflife(float damping, float eps = 1e-5f) {
        return (4.0f * Ln2) / (damping + eps);
    }

    static inline float frequency_to_stiffness(float frequency) {
        return square(2.0f * Math_PI * frequency);
    }

    static inline float stiffness_to_frequency(float stiffness) {
        return sqrt(stiffness) / (2.0f * Math_PI);
    }

    static inline float critical_halflife(float frequency) {
        return damping_to_halflife(sqrt(frequency_to_stiffness(frequency) * 4.0f));
    }

    static inline float critical_frequency(float halflife) {
        return stiffness_to_frequency(square(halflife_to_damping(halflife)) / 4.0f);
    }

    static inline float damping_ratio_to_stiffness(float ratio, float damping) {
        return square(damping / (ratio * 2.0f));
    }

    static inline float damping_ratio_to_damping(float ratio, float stiffness) {
        return ratio * 2.0f * sqrt(stiffness);
    }


    static inline float maximum_spring_velocity_to_halflife(float x,float x_goal,float v_max){
        return damping_to_halflife(2.0 * ((v_max / (x_goal - x)) * exp(1.0f)));
    }

    static inline Quaternion quat_exp(Vector3 v, float eps=1e-8f)
    {
        float halfangle = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
        
        if (halfangle < eps)
        {
            Quaternion q{};
            q.w = 1.0f; q.x = v.x; q.y = v.y; q.z = v.z;
            return q.normalized();
        }
        else
        {
            float c = cosf(halfangle);
            float s = sinf(halfangle) / halfangle;
            Quaternion q{};
            q.w = c; q.x = s * v.x; q.y = s * v.y; q.z = s * v.z;
            return q.normalized();
        }
    }
        template<typename T>
    static inline T clampf(T x, T min, T max)
    {
        static_assert(std::is_arithmetic_v<T>,"Must be arithmetic");
        return x > max ? max : x < min ? min : x;
    }

    static inline Quaternion quat_abs(Quaternion q){
        return (q.w < 0.0 ? -q : q).normalized();
    }

    static inline Vector3 quat_log(Quaternion q, float eps=1e-8f){
        real_t length = sqrt(q.x*q.x + q.y*q.y + q.z*q.z);
        if (length < eps)
        {
            return Vector3(q.x, q.y, q.z);
        }
        else
        {
            float halfangle = acosf(clampf(q.w, -1.0f, 1.0f));
            return halfangle * (Vector3(q.x, q.y, q.z) / length);
        }
    }

    static inline Quaternion quat_from_scaled_angle_axis(Vector3 v, float eps = 1e-8f)
    {
        return quat_exp(v / 2.0f, eps).normalized();
    }

    static inline Vector3 quat_to_scaled_angle_axis(Quaternion q, float eps = 1e-8f)
    {
        return 2.0f * quat_log(q, eps);
    }

    static inline Vector3 quat_differentiate_angular_velocity(Quaternion next, Quaternion curr, float dt, float eps = 1e-8f)
    {
        return quat_to_scaled_angle_axis(quat_abs(next * curr.inverse()), eps) / dt;
    }

    static void _spring_damper_exact(
        float& x,
        float& v,
        float x_goal,
        float v_goal,
        float damping_ratio,
        float halflife,
        float dt,
        float eps = 1e-5f)
    {
        float g = x_goal;
        float q = v_goal;
        float d = halflife_to_damping(halflife);
        float s = damping_ratio_to_stiffness(damping_ratio, d);
        float c = g + (d * q) / (s + eps);
        float y = d / 2.0;

        if (std::abs(s - (d * d) / 4.0) < eps) { // Critically Damped
            float j0 = x - c;
            float j1 = v + j0 * y;
            float eydt = std::exp(-y * dt);
            x = j0 * eydt + dt * j1 * eydt + c;
            v = -y * j0 * eydt - y * dt * j1 * eydt + j1 * eydt;
        }
        else if (s - (d * d) / 4.0 > 0.0) { // Under Damped
            float w = std::sqrt(s - (d * d) / 4.0f);
            float j = std::sqrt(std::pow(v + y * (x - c), 2) / (std::pow(w, 2) + eps) + std::pow(x - c, 2));
            float p = std::atan((v + (x - c) * y) / (-(x - c) * w + eps));

            // j = (x - c) > 0.0 ? j : -j;
            j = (x - c) > 0.0 ? j : -j;

            float eydt = std::exp(-y * dt);

            x = j * eydt * std::cos(w * dt + p) + c;
            v = -y * j * eydt * std::cos(w * dt + p) - w * j * eydt * std::sin(w * dt + p);
        }
        else if (s - (d * d) / 4.0 < 0.0) { // Over Damped
            float y0 = (d + std::sqrt(std::pow(d, 2) - 4.0f * s)) / 2.0f;
            float y1 = (d - std::sqrt(std::pow(d, 2) - 4.0f * s)) / 2.0f;
            float j1 = (c * y0 - x * y0 - v) / (y1 - y0);
            float j0 = x - j1 - c;

            float ey0dt = std::exp(-y0 * dt);
            float ey1dt = std::exp(-y1 * dt);

            x = j0 * ey0dt + j1 * ey1dt + c;
            v = -y0 * j0 * ey0dt - y1 * j1 * ey1dt;
        }
    }

    static void _critical_spring_damper_exact(
        float& x, 
        float& v, 
        float x_goal, 
        float v_goal, 
        float halflife, 
        float dt
        ) {
        float g = x_goal;
        float q = v_goal;
        float d = halflife_to_damping(halflife);
        float c = g + (d * q) / ((d * d) / 4.0f);
        float y = d / 2.0f;
        float j0 = x - c;
        float j1 = v + j0 * y;
        float eydt = fast_negexp(y * dt);
        x = eydt * (j0 + j1 * dt) + c;
        v = eydt * (v - j1 * y * dt);
    }

    static inline PackedFloat32Array critical_spring_damper_exact(float x, float v, float x_goal,float v_goal,  float halflife, float dt) 
    {
        _critical_spring_damper_exact(x,v,x_goal,v_goal,halflife,dt);
        PackedFloat32Array result;
        result.append(x);
        result.append(v);
        return result;
    }

    static void _simple_spring_damper_exact(float& x, float& v, float x_goal, float halflife, float dt) {
        float y = halflife_to_damping(halflife) / 2.0f;
        float j0 = x - x_goal;
        float j1 = v + j0 * y;
        float eydt = fast_negexp(y * dt);
        x = eydt * (j0 + j1 * dt) + x_goal;
        v = eydt * (v - j1 * y * dt);
    }
    static inline PackedFloat32Array simple_spring_damper_exact(float x, float v, float x_goal, float halflife, float dt){
        _simple_spring_damper_exact(x,v,x_goal,halflife,dt);
        PackedFloat32Array result;
        result.append(x);
        result.append(v);
        return result;
    }

    static inline void _decay_spring_damper_exact(float& x,float& v, float halflife, float dt) {
        float y = halflife_to_damping(halflife) / 2.0f;
        float j1 = v + x * y;
        float eydt = fast_negexp(y * dt);
        x = eydt * (x + j1 * dt);
        v = eydt * (v - j1 * y * dt);
    }
    static inline void _decay_spring_damper_exact(Vector3& x,Vector3& v, float halflife, float dt) {
        float y = halflife_to_damping(halflife) / 2.0f;
        Vector3 j1 = v + x * y;
        float eydt = fast_negexp(y * dt);
        x = eydt * (x + j1 * dt);
        v = eydt * (v - j1 * y * dt);
    }    
    static inline void _decay_spring_damper_exact(
        Quaternion& x, 
        Vector3& v, 
        const float halflife, 
        const float dt)
    {
        float y = halflife_to_damping(halflife) / 2.0f; 
        
        Vector3 j0 = quat_to_scaled_angle_axis(x);
        Vector3 j1 = v + j0*y;
        
        float eydt = fast_negexp(y*dt);

        x = quat_from_scaled_angle_axis(eydt*(j0 + j1*dt));
        v = eydt*(v - j1*y*dt);
    }
    static inline PackedFloat32Array decay_spring_damper_exact(float x, float v, float halflife,float dt){
        PackedFloat32Array result;
        _decay_spring_damper_exact(x,v,halflife,dt);
        result.append(x);
        result.append(v);
        return result;
    }


    //	Reach the x_goal at timed t_goal in the future
    //	Apprehension parameter controls how far into the future we try to track the linear interpolation
    static void _timed_spring_damper_exact(
        float& x, float& v,
        float& xi,
        const float x_goal, const float t_goal,
        const float halflife, const float& dt,
        float apprehension = 2.0f
    )
    {
        const float min_time = t_goal > dt ? t_goal : dt;

        const float v_goal = (x_goal - xi) / min_time;

        const float t_goal_future = dt + apprehension * halflife;
        const float x_goal_future = t_goal_future < t_goal ? xi + v_goal * t_goal_future : x_goal;

        _simple_spring_damper_exact(x, v, x_goal_future, halflife, dt);
        xi += v_goal * dt;
    }
    static inline PackedFloat32Array timed_spring_damper_exact(float x, float v,
        float xi,
        const float x_goal, const float t_goal,
        const float halflife, const float dt,
        const float apprehension = 2.0f
    ){
        PackedFloat32Array result;
        _timed_spring_damper_exact(x,v,xi,x_goal,t_goal,halflife,dt,apprehension);
        result.append(x);
        result.append(v);
        result.append(xi);
        return result;
    }

    static Dictionary character_update(
        Vector3 pos,
        Vector3 vel,
        Vector3 acc,
        Quaternion quaternion,
        Vector3 angular_velocity,
        Vector3 v_goal,
        Quaternion q_goal,
        float halflife_vel,
        float halflife_rot,
        float dt
    ) {
        Dictionary answer;
        {
            float y = halflife_to_damping(halflife_vel) / 2.0f;
            Vector3 j0 = vel - v_goal;
            Vector3 j1 = acc + j0 * y;
            float eydt = fast_negexp(y * dt);

            answer["world_position"] = eydt * ((-j1 / (y * y)) + ((-j0 - j1 * dt) / y)) +
                                    (j1 / (y * y)) + j0 / y + v_goal * dt + pos;
            answer["velocity"] = eydt * (j0 + j1 * dt) + v_goal;
            answer["acceleration"] = eydt * (acc - j1 * y * dt);
        }
        {
            float y = halflife_to_damping(halflife_rot) / 2.0f;
            Vector3 j0 = (quaternion * q_goal.inverse()).get_euler();
            Vector3 j1 = angular_velocity + j0 * y;
            float eydt = fast_negexp(y * dt);
            answer["quaternion"] = (Quaternion(eydt * (j0 + j1 * dt)) * q_goal).normalized();
            answer["angular_velocity"] = eydt * (angular_velocity - j1 * y * dt);
            answer["delta"] = dt;
        }
        return answer;
    }

    static Dictionary character_predict(
            Vector3 x, Vector3 v, Vector3 a, 
            Quaternion q, Vector3 angular_v, 
            Vector3 v_goal, Quaternion q_goal, 
            float halflife_v, float halflife_q,
            const PackedFloat32Array dts) {
        Dictionary answer;
        for (int i = 0; i < dts.size(); i++) {
            float dt = dts[i];
            answer[i] = character_update(x, v, a, q, angular_v, v_goal, q_goal, halflife_v, halflife_q, dt);
        }
        return answer;
    }

    static inline void inertialize_transition(
        Vector3& off_x, 
        Vector3& off_v, 
        const Vector3 src_x,
        const Vector3 src_v,
        const Vector3 dst_x,
        const Vector3 dst_v)
    {
        off_x = (src_x + off_x) - dst_x;
        off_v = (src_v + off_v) - dst_v;
    }



    static inline void inertialize_update(
        Vector3& out_x, 
        Vector3& out_v,
        Vector3& off_x, 
        Vector3& off_v,
        const Vector3 in_x, 
        const Vector3 in_v,
        const float halflife,
        const float dt)
    {
        CritDampSpring::_decay_spring_damper_exact(off_x, off_v, halflife, dt);
        out_x = in_x + off_x;
        out_v = in_v + off_v;
    }

    static inline void inertialize_transition(
        Quaternion &off_x,
        Vector3 &off_v,
        const Quaternion src_x,
        const Vector3 src_v,
        const Quaternion dst_x,
        const Vector3 dst_v)
    {
        off_x = CritDampSpring::quat_abs((off_x * src_x) * dst_x.inverse()).normalized();
        off_v = (off_v + src_v) - dst_v;
    }
    static inline void inertialize_update(
        Quaternion &out_x,
        Vector3 &out_v,
        Quaternion &off_x,
        Vector3 &off_v,
        const Quaternion in_x,
        const Vector3 in_v,
        const float halflife,
        const float dt)
    {
        CritDampSpring::_decay_spring_damper_exact(off_x, off_v, halflife, dt);
        out_x = (off_x * in_x).normalized();
        out_v = off_v + off_x.xform(in_v);
    }

    static inline Vector3 calculate_offset_vec3(const Vector3 src_x,const Vector3 dst_x,const Vector3 off_x = Vector3())
    {
        return (src_x + off_x) - dst_x;
    }
    static inline Quaternion calculate_offset_quat(const Quaternion src_q,const Quaternion dst_q,const Quaternion off_q = Quaternion())
    {
        return CritDampSpring::quat_abs((off_q * src_q) * dst_q.inverse());
    }



    static inline Dictionary binded_inertia_transition(const Vector3 off_x,
                                                       const Vector3 off_v,
                                                       const Vector3 src_x,
                                                       const Vector3 src_v,
                                                       const Vector3 dst_x,
                                                       const Vector3 dst_v,
                                                       const Quaternion off_q,
                                                       const Vector3 off_a,
                                                       const Quaternion src_q,
                                                       const Vector3 src_a,
                                                       const Quaternion dst_q,
                                                       const Vector3 dst_a)
    {
        Dictionary result;
        result["position_offset"] = (src_x + off_x) - dst_x;
        result["velocity_offset"] = (src_v + off_v) - dst_v;
        result["rotation_offset"] = CritDampSpring::quat_abs((off_q * src_q) * dst_q.inverse());
        result["angular_offset"] = (off_a + src_a) - dst_a;
        return result;
    }


    
protected:
    static void _bind_methods() {
        

        ClassDB::bind_static_method("CritDampSpring", D_METHOD("damper_exponential", "variable", "goal", "damping", "dt"), &CritDampSpring::damper_exponential);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("damp_adjustment_exact_quat", "g", "halflife", "dt", "eps"), &CritDampSpring::damp_adjustment_exact_quat, DEFVAL(1e-8f));
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("damp_adjustment_exact", "g", "halflife", "dt", "eps"), &CritDampSpring::damp_adjustment_exact, DEFVAL(1e-8f));

        ClassDB::bind_static_method("CritDampSpring", D_METHOD("halflife_to_duration", "halflife", "initial_value", "eps"), &CritDampSpring::halflife_to_duration, DEFVAL(1.0f), DEFVAL(1e-5f));
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("halflife_to_damping", "halflife", "eps"), &CritDampSpring::halflife_to_damping, DEFVAL(1e-5f));
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("damping_to_halflife", "damping", "eps"), &CritDampSpring::damping_to_halflife, DEFVAL(1e-5f));
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("frequency_to_stiffness", "frequency"), &CritDampSpring::frequency_to_stiffness);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("stiffness_to_frequency", "stiffness"), &CritDampSpring::stiffness_to_frequency);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("critical_halflife", "frequency"), &CritDampSpring::critical_halflife);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("critical_frequency", "halflife"), &CritDampSpring::critical_frequency);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("damping_ratio_to_stiffness", "ratio", "damping"), &CritDampSpring::damping_ratio_to_stiffness);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("damping_ratio_to_damping", "ratio", "stiffness"), &CritDampSpring::damping_ratio_to_damping);

        ClassDB::bind_static_method("CritDampSpring", D_METHOD("maximum_spring_velocity_to_halflife", "x", "x_goal", "v_max"), &CritDampSpring::maximum_spring_velocity_to_halflife);

        ClassDB::bind_static_method("CritDampSpring", D_METHOD("timed_spring_damper_exact", "x", "v", "xi", "x_goal", "t_goal", "halflife", "dt", "apprehension"), &CritDampSpring::timed_spring_damper_exact, DEFVAL(2.0f));
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("decay_spring_damper_exact", "pos", "vel", "halflife", "dt"), &CritDampSpring::decay_spring_damper_exact);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("simple_spring_damper_exact", "x", "v", "x_goal", "halflife", "dt"), &CritDampSpring::simple_spring_damper_exact);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("critical_spring_damper_exact", "x", "v", "x_goal", "v_goal", "halflife", "dt"), &CritDampSpring::critical_spring_damper_exact);
        // ClassDB::bind_static_method("CritDampSpring", D_METHOD("spring_damper_exact", "x", "v", "x_goal", "v_goal", "damping_ratio", "halflife", "dt", "eps"), &CritDampSpring::spring_damper_exact, DEFVAL(1e-5f));

        ClassDB::bind_static_method("CritDampSpring", D_METHOD("character_update", "pos", "vel", "acc", "quaternion", "angular_velocity", "v_goal", "q_goal", "halflife_vel", "halflife_rot", "dt"), &CritDampSpring::character_update);
        ClassDB::bind_static_method("CritDampSpring", D_METHOD("character_predict", "x", "v", "a", "q", "angular_v", "v_goal", "q_goal", "halflife_v", "halflife_q", "dts"),&CritDampSpring::character_predict);
    }

};




