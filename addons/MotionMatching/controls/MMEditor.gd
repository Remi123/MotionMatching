@tool
class_name MMEditor extends PanelContainer

@onready var anim_panel: AnimationPanel = %Animations
@onready var stat_panel: Control = %Stats
@onready var data_panel: DataPanel = %Data
@onready var calc_panel: Control = %Calculation
@onready var tags_panel: TagEditor = %Tags

@onready var path_label: Label = %PathLabel
@onready var bake_button: Button = %BakeButton


signal on_library_change(lib:MMAnimationLibrary)
signal on_bake
signal on_weights

var library : MMAnimationLibrary:
	get:
		return library
	set(value):
		library = value
		path_label.text = library.resource_path
		on_library_change.emit(library)

func _ready() -> void:
	var reload_icon := EditorInterface.get_editor_theme().get_icon("Reload","EditorIcons")
	bake_button.icon = reload_icon

static func get_child_of_type(node: Node, child_type, recursive := false):
	for i in range(node.get_child_count()):
		var child = node.get_child(i)
		if is_instance_of(child, child_type):
			return child
		if recursive:
			var child_node = get_child_of_type(child, child_type, recursive)
			if child_node:
				return child_node


func set_skeleton_to_pose(anim_name:StringName,timestamp:float):
	# Find skeleton
	var skeleton :Skeleton3D= get_child_of_type(EditorInterface.get_edited_scene_root(),Skeleton3D,true)
	if(skeleton == null):
		return

	var anim := library.get_animation(anim_name)
	var anim_timestep := timestamp

	for i in range(anim.get_track_count()):
		var bone_id = skeleton.find_bone(anim.track_get_path(i).get_subname(0))
		if bone_id == -1:
			continue
		elif anim.track_get_type(i) == Animation.TYPE_POSITION_3D:
			var position := anim.position_track_interpolate(i,anim_timestep)
			skeleton.set_bone_pose_position(bone_id,position * skeleton.get_motion_scale())
		elif anim.track_get_type(i) == Animation.TYPE_ROTATION_3D:
			var rotation := anim.rotation_track_interpolate(i,anim_timestep)
			skeleton.set_bone_pose_rotation(bone_id,rotation)
		elif anim.track_get_type(i) == Animation.TYPE_SCALE_3D:
			var scale := anim.scale_track_interpolate(i,anim_timestep)
			skeleton.set_bone_pose_scale(bone_id,scale)






func _on_bake_button_pressed() -> void:
	library.bake_data()
	on_bake.emit()
	ResourceSaver.save(library,library.resource_path)
	pass # Replace with function body.


func _on_weight_button_pressed() -> void:
	library.recalculate_weights()
	on_weights.emit()
	ResourceSaver.save(library,library.resource_path)
	pass # Replace with function body.
