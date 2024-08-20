#pragma once

#include "register_types.h"
#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "Math/Spring.hpp"
#include "MotionFeatures/MFBonesInfo.hpp"
#include "MotionFeatures/MFDistance.hpp"
#include "MotionFeatures/MFEvents.hpp"
#include "MotionFeatures/MFRootVelocity.hpp"
#include "MotionFeatures/MFTrajectory.hpp"
#include "MotionFeatures/MotionFeatures.hpp"

#include "Util/CircularBuffer.hpp"
#include "Util/Util.hpp"
#include <MMAnimationLibrary.hpp>
#include <MMAnimationPlayer.hpp>
#include <PostProcessAnimation/MMIKLookAt3D.hpp>
#include <PostProcessAnimation/MMIKTwoBone3D.hpp>
#include <PostProcessAnimation/MMInertialization3D.hpp>

#include <AnimTags/AnimTag.hpp>
#include <AnimTags/IndexSet.hpp>

namespace boost {
#ifdef BOOST_NO_EXCEPTIONS
void throw_exception(std::exception const &e) {
	godot::UtilityFunctions::prints("MotionMatching catched exception : ", e.what());
	//throw 11; // This handle exceptions when dealing with no exception.
};
#endif
} // namespace boost

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_ABSTRACT_CLASS(MMUtil);

	GDREGISTER_CLASS(MMAnimationPlayer);
	GDREGISTER_CLASS(MMQueryOptions);
	GDREGISTER_CLASS(MMAnimationLibrary);


	{ // Motion Features Resources
		GDREGISTER_VIRTUAL_CLASS(MotionFeature);

		GDREGISTER_CLASS(MFRootVelocity);
		GDREGISTER_CLASS(MFBonesInfo);
		GDREGISTER_CLASS(MFTrajectory);
		GDREGISTER_CLASS(MFTrajectoryOptions);
		// GDREGISTER_CLASS(MFEvents);
		// GDREGISTER_CLASS(MFDistance);
	}

	{ // Animation Tags
		GDREGISTER_VIRTUAL_CLASS(TagInfo);
		GDREGISTER_VIRTUAL_CLASS(TagMotionMatching);
		GDREGISTER_CLASS(TagJunk);
		GDREGISTER_CLASS(TagCategory);
		// GDREGISTER_CLASS(TagMFEvent);
		// GDREGISTER_CLASS(TagMFDistance);

		GDREGISTER_INTERNAL_CLASS(RangeIndex);
		GDREGISTER_CLASS(SetRangeIndex);
	}

	{ // SkeletonModifier3D Nodes
		GDREGISTER_CLASS(MMInertialization3D);
		GDREGISTER_CLASS(MMIKLookAt3D);
		GDREGISTER_CLASS(MMIKTwoBone3D);
	}

	{ // Various helper
		GDREGISTER_CLASS(CircularBuffer);
		GDREGISTER_CLASS(Spring);
		GDREGISTER_CLASS(Kform);
	}
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
// Initialization
GDExtensionBool GDE_EXPORT mm_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_gdextension_types);
	init_obj.register_terminator(uninitialize_gdextension_types);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}