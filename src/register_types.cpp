#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/godot.hpp>

#include "Spring.hpp"
#include "MotionFeatures/MotionFeatures.hpp"
#include "MotionFeatures/MFRootVelocity.hpp"
#include "MotionFeatures/MFBonesInfo.hpp"
#include "MotionFeatures/MFTrajectory.hpp"
#include "MotionFeatures/MFEvents.hpp"


#include <MMAnimationLibrary.hpp>
#include <MMAnimationPlayer.hpp>
#include <PostProcessAnimation/PPInertialization3D.hpp>
#include <PostProcessAnimation/PPIKLookAt3D.hpp>
#include <PostProcessAnimation/PPIKTwoBone3D.hpp>
#include "CircularBuffer.hpp"

#include <AnimTags/AnimTag.hpp>

namespace boost
{
#ifdef BOOST_NO_EXCEPTIONS
void throw_exception( std::exception const & e ){
	godot::UtilityFunctions::prints("MotionMatching catched exception : ",e.what());
    //throw 11; // This handle exceptions when dealing with no exception.
	// TODO 
};
#endif
}// namespace boost


using namespace godot;

void gdextension_MM_initialize(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		
		ClassDB::register_class<MMAnimationPlayer>();
		ClassDB::register_class<MMAnimationLibrary>();

		{	// Motion Features Resources
			ClassDB::register_class<MotionFeature>(true); // Abstract class
			
			ClassDB::register_class<MFRootVelocity>();
			ClassDB::register_class<MFBonesInfo>();
			ClassDB::register_class<MFTrajectory>();
			ClassDB::register_class<MFEvents>();
		}

		{	// Animation Tags
			ClassDB::register_class<TagInfo>(true); // Abstract

			ClassDB::register_class<TagMotionMatching>(true); // Abstract
			ClassDB::register_class<TagJunk>();
			ClassDB::register_class<TagCategory>();
			ClassDB::register_class<TagMFEvent>(); 
			
			ClassDB::register_class<TagAnimation>(true); // Abstract
			ClassDB::register_class<TagRootWarp>();
		}


		{	// PostProcessing Nodes
			ClassDB::register_class<PPInertialization3D>();
			ClassDB::register_class<PPIKLookAt3D>();
			ClassDB::register_class<PPIKTwoBone3D>();
		}

		
		{ 	// Various helper
			ClassDB::register_class<CircularBuffer>();
			ClassDB::register_class<Spring>();
		}
	}
}

void gdextension_MM_terminate(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
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
