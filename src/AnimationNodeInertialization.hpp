#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>



#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

#include "godot_cpp/core/math.hpp"

#include <godot_cpp/classes/animation_root_node.hpp>

#include <CritSpringDamper.hpp>


struct AnimationNodeInertialization : godot::AnimationRootNode
{
    GDCLASS(AnimationNodeInertialization,AnimationRootNode);
    using u = godot::UtilityFunctions;

private:
    double delta_time = 0.0;

public:
    virtual double _process(double time, bool seek, bool is_external_seeking, bool test_only) const
    {
        return 0.0;
    }
	

    protected:
    static void _bind_methods()
    {

    }
};