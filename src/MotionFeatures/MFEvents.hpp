#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/method_bind.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>


#include <godot_cpp/classes/time.hpp>

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_player.hpp>

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/skeleton_profile.hpp>


#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>


#include <algorithm>
#include <limits>
#include <ranges>


#include <MotionFeatures/MotionFeatures.hpp>

namespace views = std::ranges::views;

struct MMAnimationLibrary;

// Macro setup. Mostly there to simplify writing all those
#define GETSET(type, variable, ...)            \
	type variable{ __VA_ARGS__ };              \
	type get_##variable() { return variable; } \
	void set_##variable(type value) { variable = value; }
#define STR(x) #x
#define STRING_PREFIX(prefix, s) STR(prefix##s)
#define BINDER_PROPERTY_PARAMS(type, variant_type, variable, ...)                                  \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(set_, variable), "value"), &type::set_##variable); \
	ClassDB::bind_method(D_METHOD(STRING_PREFIX(get_, variable)), &type::get_##variable);          \
	ADD_PROPERTY(PropertyInfo(variant_type, #variable, __VA_ARGS__), STRING_PREFIX(set_, variable), STRING_PREFIX(get_, variable));

using namespace godot;

int sec_to_frame(float seconds, int fps = -1) {
	if (fps <= 0) {
		fps = Engine::get_singleton()->get_physics_ticks_per_second();
	}
	return (int)ceil(seconds * Engine::get_singleton()->get_physics_ticks_per_second());
}

struct MFEvents : public MotionFeature {
	GDCLASS(MFEvents, MotionFeature)
	Ref<MMAnimationLibrary> mmlib = nullptr;
	std::vector<Ref<TagMFEvent>> animation_events{};

public:
	enum EventType {
		Timing,
		EmbedValue,

	};

	void _notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_POSTINITIALIZE:
				normalization_type = RawValue;
		}
	}

	GETSET(EventType, event_type);

	GETSET(bool, embed_as_frames);
	GETSET(bool, use_only_start, false);
	GETSET(real_t, max_signed_time, real_t(2.0));
	GETSET(godot::PackedStringArray, events_names);

	static constexpr float delta = 0.016f;

	virtual int get_dimension() override { return events_names.size(); }

	virtual PackedFloat32Array get_weights() override { return Array::make(1.0f); }

	virtual bool setup_bake_init(Ref<MMAnimationLibrary> animlib) override {
		// returning false will abort the process.
		// feel free to print more details
		mmlib = animlib;

		return true;
	}
	virtual PackedStringArray get_hints() const { return events_names; }

	virtual bool setup_bake_animation(Ref<Animation> animation) override {
		u::prints("Events", animation->get_name());
		auto tags = mmlib->tags;
		animation_events.clear();
		for (auto i = 0; i < mmlib->tags.size(); ++i) {
			if (TagMFEvent *event = Object::cast_to<TagMFEvent>(tags[i]); event != nullptr && event->animation_name == animation->get_name()) {
				animation_events.push_back(event);
			}
		}

		return true;
	}

	// the current logic is this : Take the first event
	virtual PackedFloat32Array bake_animation_pose(Ref<Animation> animation, float time) override {
		PackedFloat32Array result = {};
		// std::vector<Ref<TagMFEvent>> current_events{};
		const float time_offset = 1.0f / Engine::get_singleton()->get_physics_ticks_per_second();

		// [  5 4 3 2 1 0 -1 -2 -3 3 2 1 0 0 0 0 -1 -2 2 1 0 -1 ]
		// 0s are event. positive values are pre-event, negative are post-event

		for (auto i = 0; i < events_names.size(); ++i) {
			const auto event_name = events_names[i];
			auto is_event_name = [event_name](Ref<TagMFEvent> event) { return event->event_name == event_name; };
			std::vector<Ref<TagMFEvent>> current_events{};
			std::ranges::copy(animation_events | std::ranges::views::filter(is_event_name), std::back_inserter(current_events));

			if (current_events.size() == 0) {
				result.append(max_signed_time);
				continue;
			}

			// Check if event is inside the duration
			for (auto event : current_events) {
				if (event->timestamp <= time && time < (event->timestamp + event->duration)) {
					result.append(0.0f);
					continue;
				}
			}

			// Get the nearest event, otherwise send max value.
			auto nearest = *std::min_element(current_events.begin(), current_events.end(),
					[time](Ref<TagMFEvent> a, Ref<TagMFEvent> b) {
						const float A = time < a->timestamp ? abs(time - a->timestamp) : abs(time - (a->timestamp + a->duration));
						const float B = time < b->timestamp ? abs(time - b->timestamp) : abs(time - (b->timestamp + b->duration));
						return A < B;
					});
			//              0.5     1.0                 1.0                 0.5 :   0.5                                    1.0
			const float T = time < nearest->timestamp ? nearest->timestamp - time : nearest->timestamp + nearest->duration - time;
			result.append(u::clampf(T, -max_signed_time, max_signed_time));
		}
		return result;
	}

	virtual void debug_pose_gizmo(Ref<EditorNode3DGizmo> gizmo, const PackedFloat32Array data, godot::Transform3D tr = godot::Transform3D{}) { return; }

	static void _bind_methods() {
		BIND_ENUM_CONSTANT(Timing);
		BIND_ENUM_CONSTANT(EmbedValue);

		ClassDB::bind_method(D_METHOD("set_events_names", "value"), &MFEvents::set_events_names);
		ClassDB::bind_method(D_METHOD("get_events_names"), &MFEvents::get_events_names);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::PACKED_STRING_ARRAY, "events_names"), "set_events_names", "get_events_names");

		ClassDB::bind_method(D_METHOD("set_max_signed_time", "value"), &MFEvents::set_max_signed_time);
		ClassDB::bind_method(D_METHOD("get_max_signed_time"), &MFEvents::get_max_signed_time);
		godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::FLOAT, "max_signed_time"), "set_max_signed_time", "get_max_signed_time");

		// ClassDB::bind_method( D_METHOD("set_embed_as_frames" ,"value"), &MFEvents::set_embed_as_frames);
		// ClassDB::bind_method( D_METHOD("get_embed_as_frames" ), &MFEvents::get_embed_as_frames);
		// godot::ClassDB::add_property(get_class_static(), PropertyInfo(Variant::BOOL,"embed_as_frames"), "set_embed_as_frames", "get_embed_as_frames");

		ClassDB::bind_method(D_METHOD("get_dimension"), &MFEvents::get_dimension);

		ClassDB::bind_method(D_METHOD("get_weights"), &MFEvents::get_weights);

		ClassDB::bind_method(D_METHOD("get_hints"), &MFEvents::get_hints);

		ClassDB::bind_method(D_METHOD("setup_bake_init", "mm_animation_library"), &MFEvents::setup_bake_init);
		ClassDB::bind_method(D_METHOD("setup_bake_animation", "animation"), &MFEvents::setup_bake_animation);

		ClassDB::bind_method(D_METHOD("bake_animation_pose", "animation", "time"), &MFEvents::bake_animation_pose);

		ClassDB::bind_method(D_METHOD("debug_pose_gizmo", "gizmo", "data", "root_transform"), &MFEvents::debug_pose_gizmo);
	}
};

VARIANT_ENUM_CAST(MFEvents::EventType);