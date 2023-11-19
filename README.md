# MotionMatching for Godot 4

This library is a generic implementation of Motion Matching inside the Godot Engine 4.2.

We provide a few Nodes and Resources that are useful for playing animations, either with or without Motion Matching. The goal is to be as modular as possible.

Here is what you can expect :

- MMAnimationLibrary and MotionFeatures ( Resources )
  Don't use. Not ready for usage. Actively changing interface and features. WIP. 

- MMAnimationPlayer (Node). 
  This animation player use inertialization as the method of transition. Set the halflife ( default 0.1 ) to control how much time the transition occurs, however it is not a set-in-stone timed transition.
  However, any other method other than `request_animation(animation_name:String, timestamp:float = 0.0)` and `request_pose(animation_name:String, timestamp:float)` is not supported. Cannot play animation backward and can't enqueue animation (queuing might be supported in the future).
  You can retrieve the inertialized root bone linear velocity using `get_inertialized_root_motion_velocity()` and angular velocity using `get_inertialized_root_motion_angular(delta:float)`. Other bones info can be retrieved using `get_[local,model,raw]_bone_info(bone_name:String)`.

  Just a note : `Skeleton3D` use `global_pose` to refer to the position relative to the root bone. It's more accurate to call it the `model` pose. It also dependant on the presence of a root bone. To avoid confusion, we refer to `model` as relative to the root bone, and `raw` as what is in the animation file.

  Usage : Change the type of your AnimationPlayer node to MMAnimationPlayer. Use `request_animation` to play your animation and enjoy sweet transitions.

- Post Processing Animations (Node3D)
  Having an animation player with custom transitions is great, however the elephant in the room is the AnimationTree. Contrary to AnimationPlayer, AnimationTree receive a bunch of instructions to blend animation together in any order, so there is no clear starting point to do proper inertialization. Also, some inverse kinematic are missing. So here are the nodes. All post processing nodes requires you to define which AnimationMixer-inherited node you refer, and which Skeleton you want bones to be modified. Those nodes doesn't work well with MMAnimationPlayer. All those nodes are made to be process after the AnimationMixer-inherited node, so you can either add those nodes as child of AnimationMixer, or set the process priority to an higher value than the mixer. Works also in Editor.

  - PPIKLookAt3D ( Node3D )
  A simple look at ik. The position of the node is where the bone will orient its rotation. There is no joints limit, so it's on you to manage that. Recommended priority = 1

  - PPIKTwoBone3D ( Node3D )
  A simple two bone ik logic. Doesn't handle bones inbetween the two defined bones. The position of the node define where two the bone will reach, and the rotation of the node will adjust the orientation. Recommended process priority = Mixer.priority + 1

  - PPInertialization ( Node3D)
  This is the node that tries to inertialize the AnimationTree. It's technically more of a filter than a true inertialization, but it does the job for simple transition. The bone tries to reach the positions and rotations calculated by the AnimationMixer Please keep the halflife low, something like 0.05. In the AnimationTree, you can set the transition time ( for example in AnimationNodeStateMachineTransition ) to 0 and it will work fine. If you set it to higher, you will technically inertialize the default blending of AnimationTree, which might be interesting in some cases. Fun fact : If you set this node to be processed after my PPIK Nodes, it will inertialize the transition of the bones, faking an animation instead of blending between current position of the animation and desired target. Recommended process priority = Mixer.priority + 2. 

- Spring ( GDScript )
  This is a small package of functions related to spring as defined by https://theorangeduck.com/page/spring-roll-call. Not all functions are there, and some are missing a quaternion equivalent. The other problem is that I need to use Dictionary as return values in most cases, but this will be solve when Godot add Structure object.

- Circularbuffer ( GDScript )
  A wrapper around `Boost.Circular_buffer<Variant>`. 

## Status

The project is currently in a working state but still under development. Please note that everything is subject to changes.

## Documentation

Documentation is currently missing. We are actively working on it.

## Demo

A demo is not available at this moment. We have a working demo with our assets and animations, but due to licensing restrictions, we cannot share them. We are working on creating a tutorial.

## About the Implementation

The implementation is based around the ideas described in [Simon Clavet's video](https://www.youtube.com/watch?v=jcpIrw38E-s).

The goal of this library is to provide a generic set of utilities and ease the management of the data required for Motion Matching. It is extracted from my projects in the hope to help others and also provide a small framework for others to contribute.

## Learning Resources for Motion Matching

To learn more about Motion Matching, you can refer to the following resources:

1. [Introduction video](https://www.gdcvault.com/play/1023280/Motion-Matching-and-The-Road)
2. [O3DE implementation blog](https://github.com/o3de/o3de/tree/development/Gems/MotionMatching)
3. [Daniel Holden aka OrangeDuck's blog](https://theorangeduck.com/)
   - [Spring algorithm and controller](https://theorangeduck.com/page/spring-roll-call)
   - [Motion Matching implementation](https://theorangeduck.com/page/code-vs-data-driven-displacement)
   - [Fitting Motion with Animation](https://theorangeduck.com/page/fitting-code-driven-displacement)
4. [UFC 3 Geoff Harrower Talk about events](https://www.gdcvault.com/play/1025228/Real-Player-Motion-Tech-in)
