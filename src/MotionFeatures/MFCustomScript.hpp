#pragma once

#include <MotionFeatures/MotionFeatures.hpp>

#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>

// The goal of this class is to be overwrite by GDScript
struct MFCustomScript : MotionFeature {
	GDCLASS(MFCustomScript, MotionFeature)
public :
	GDVIRTUAL0R(int,get_dimension);
	GDVIRTUAL0R(PackedFloat32Array,get_weights);
	GDVIRTUAL0R(PackedStringArray,get_hints);

    GDVIRTUAL1R(bool,setup_bake_init,Ref<MMAnimationLibrary>);

	virtual bool setup_bake_init(Ref<MMAnimationLibrary> mmal) override {
		bool ret;
		if (GDVIRTUAL_CALL(setup_bake_init, mmal,ret)) {
			return ret;
		}
		return false;
	}

	virtual int get_dimension() override {
		int ret;
		if (GDVIRTUAL_CALL(get_dimension,ret)) {
			return ret;
		}
		return false;
	}
protected:
	static void _bind_methods() {
		
		BIND_VIRTUAL_METHOD(MFCustomScript, get_dimension);
		// GDVIRTUAL_BIND(get_dimension);
		GDVIRTUAL_BIND(get_weights) 
		GDVIRTUAL_BIND(get_hints) 
		GDVIRTUAL_BIND(setup_bake_init,"animlib") 
	}

};