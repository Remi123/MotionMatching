#pragma once

#include <godot_cpp/variant/utility_functions.hpp>
#include "godot_cpp/core/math.hpp"
#include "godot_cpp/variant/vector3.hpp"
#include <CritSpringDamper.hpp>

using namespace godot;

struct kform
{
    Quaternion rot = Quaternion();
    Vector3 pos = Vector3();
    Vector3 scl= Vector3(1.0,1.0,1.0);
    Vector3 vel= Vector3();
    Vector3 ang= Vector3();
    Vector3 svl= Vector3();

    kform() = default;
    ~kform() = default;

    kform(Transform3D tr):
        pos{tr.origin},
        rot{tr.basis.get_rotation_quaternion()},
        scl{tr.basis.get_scale()},
        vel{},ang{},svl{}
    {}

    static Vector3 _log(Vector3 v){
        return Vector3(std::logf(v.x),std::logf(v.y),std::logf(v.z));
    }

    kform finite_difference(kform input_next, float dt) const
    {
        kform output = *this;
        output.vel = (input_next.pos - pos) / dt;

        output.ang = CritDampSpring::quat_to_scaled_angle_axis(CritDampSpring::quat_abs(
                            input_next.rot * rot.inverse())) /
                        dt;

        output.svl = _log(input_next.scl / scl) / dt;
        return output;
    }

    friend kform operator*(const kform v,const kform w){
        kform out;
        out.pos = v.rot.xform(w.pos * v.scl) + v.pos;        
        out.rot = v.rot * w.rot;        
        out.scl = w.scl * v.scl;        
        out.vel = v.rot.xform( w.vel * v.scl)+ v.vel + 
                v.ang.cross(v.rot.xform( w.pos * v.scl)) +
                v.rot.xform( w.pos * v.scl * v.svl);
        out.ang = v.rot.xform(w.ang) + v.ang;        
        out.svl = w.svl + v.svl;        
        return out;     
    }
    friend kform operator/(const kform v,const kform w)
    {
        kform out;
        out.pos = v.rot.xform_inv(w.pos - v.pos);
        out.rot = v.rot.inverse() * w.rot;
        out.scl = w.scl / v.scl;
        out.vel = v.rot.xform_inv(w.vel - v.vel - v.ang.cross(v.rot.xform(out.pos * v.scl))) -
                  v.rot.xform(out.pos * v.scl * v.svl);
        out.ang = v.rot.xform_inv(w.ang - v.ang);
        out.svl = w.svl - v.svl;
        return out;
    }
    kform inverse() const{
        return kform() / (*this);
    }
}; 