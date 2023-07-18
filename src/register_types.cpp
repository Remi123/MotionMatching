#pragma once

#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/godot.hpp>

#include "CritSpringDamper.hpp"
#include <MotionMatcher.hpp>
#include "MotionFeatures/MotionFeatures.hpp"
#include "MotionFeatures/RootVelocityMotionFeature.hpp"
#include "MotionFeatures/BonePositionVelocityMotionFeature.hpp"
#include "MotionFeatures/TrajectoryMotionFeature.hpp"
#include "CircularBuffer.hpp"

//#include "AnimationNodeInertialization.hpp"

using namespace godot;

void gdextension_initialize(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		//ClassDB::register_class<AABBTree>();
		//ClassDB::register_class<KDTree>();

		//ClassDB::register_class<AnimationNodeInertialization>();

		ClassDB::register_class<CircularBuffer>();
		
		ClassDB::register_class<MotionFeature>(true); // Abstract class
		
		ClassDB::register_class<RootVelocityMotionFeature>();
		ClassDB::register_class<BonePositionVelocityMotionFeature>();
		ClassDB::register_class<TrajectoryMotionFeature>();


		ClassDB::register_class<MotionMatcher>();
		ClassDB::register_class<CritDampSpring>();
	}
}

void gdextension_terminate(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
	}
}

extern "C" {

// Initialization.

	GDExtensionBool GDE_EXPORT gdextension_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(gdextension_initialize);
		init_obj.register_terminator(gdextension_terminate);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}