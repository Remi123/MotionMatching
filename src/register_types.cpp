#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
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

void gdextension_MM_initialize(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {

		GDREGISTER_ABSTRACT_CLASS(MMUtil);

		GDREGISTER_CLASS(MMAnimationPlayer);
		GDREGISTER_CLASS(MMAnimationLibrary);
		GDREGISTER_INTERNAL_CLASS(QueryOptions);

		{ // Motion Features Resources
			GDREGISTER_VIRTUAL_CLASS(MotionFeature);

			GDREGISTER_CLASS(MFRootVelocity);
			GDREGISTER_CLASS(MFBonesInfo);
			GDREGISTER_CLASS(MFTrajectory);
			GDREGISTER_INTERNAL_CLASS(MFTrajectoryOptions);
			GDREGISTER_CLASS(MFEvents);
			// GDREGISTER_CLASS(MFDistance);
		}

		{ // Animation Tags
			GDREGISTER_VIRTUAL_CLASS(TagInfo);
			GDREGISTER_VIRTUAL_CLASS(TagMotionMatching);
			GDREGISTER_CLASS(TagJunk);
			GDREGISTER_CLASS(TagCategory);
			GDREGISTER_CLASS(TagMFEvent);
			GDREGISTER_CLASS(TagMFDistance);

			ClassDB::register_class<TagAnimation>(true); // Abstract
			ClassDB::register_class<TagRootWarp>();

			GDREGISTER_INTERNAL_CLASS(RangeIndex);
			GDREGISTER_CLASS(SetRangeIndex);
		}

		{ // PostProcessing Nodes
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
}

void gdextension_MM_terminate(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
	}
}

extern "C" {

// Initialization.

GDExtensionBool GDE_EXPORT gdextension_motion_matching_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(gdextension_MM_initialize);
	init_obj.register_terminator(gdextension_MM_terminate);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
