@tool
extends Control

var _current: MotionPlayer = null
var _animplayer: AnimationPlayer = null

@onready var rd: RichTextLabel = $TabContainer/Data/ScrollContainer/PoseData

@onready var gizmo: MMEditorGizmoPlugin  # setup by the plugin

@onready
var choose_anim: MenuButton = $TabContainer/Data/HBoxContainer/MarginContainer5/ChooseAnimation


func _ready() -> void:
	print("MMEditorPanel Ready")
	update_info()


@onready var infotext: RichTextLabel = $TabContainer/Info/PanelContainer/InfoText


func update_info() -> void:
	if _current:
		var nb_dim = 0
		for r in _current.motion_features:
			prints(r.resource_name)
			var f: MotionFeature = r as MotionFeature
			nb_dim += r.get_dimension()

		infotext.clear()
		var info := [
			["Nb Features", _current.motion_features.size()],
			["Nb Dimension", nb_dim],
			["Nb Poses", _current.MotionData.size() / nb_dim],
		]
		infotext.push_table(info.size())
		for t in info:
			infotext.push_cell()
			infotext.set_cell_padding(Rect2i(5, 0, 5, 5))
			infotext.set_cell_border_color(Color.LIGHT_GRAY)
			infotext.append_text(t[0] + ":" + str(t[1]))
			infotext.pop()
		infotext.pop()

		infotext.append_text("\n\nDimensional informations:\n")

		infotext.push_table(1 + nb_dim)

		info = [
			["Dimensions", range(nb_dim)],
			["Means", _current.means],
			["Variances", _current.variances],
			["Weights", _current.weights],
		]
		for i in info:
			infotext.push_cell()
			infotext.set_cell_border_color(Color.LIGHT_GRAY)
			infotext.append_text(i[0])
			infotext.pop()
			for subinfo in i[1]:
				infotext.push_cell()
				infotext.set_cell_row_background_color(
					Color(0.212, 0.239, 0.29) / 0.9, Color(0.212, 0.239, 0.29) * 0.8
				)
				infotext.set_cell_border_color(Color.LIGHT_GRAY)
				if subinfo is float:
					infotext.append_text("%*.*f" % [0, 3, subinfo])
				elif subinfo is int:
					infotext.append_text(str(subinfo))
				infotext.pop()

		infotext.pop()

		for i in range(_current.densities.size()):
			prints("Dimension", i)
			for data in _current.densities[i]:
				prints(data[0], data[1])

		if _current.animation_library != null:
			var lib: AnimationLibrary = _current.animation_library
			choose_anim.get_popup().clear()
			for anim_name in lib.get_animation_list():
				choose_anim.get_popup().add_item(anim_name)
			choose_anim.get_popup().connect("id_pressed", _on_choose_animation_pressed)


func bake_data_current() -> void:
	if _current == null:
		return
	_current.baking_data()
	update_info()


func update_shown_pose_data(pose_index: int) -> void:
	var nb_dim = 0
	for r in _current.motion_features:
		var f: MotionFeature = r as MotionFeature
		nb_dim += r.get_dimension()

	if pose_index > _current.MotionData.size() / nb_dim:
		return

	var pose := _current.MotionData.slice(pose_index * nb_dim, pose_index * nb_dim + nb_dim)
	for i in range(pose.size()):
		pose[i] = pose[i] * _current.variances[i] + _current.means[i]

	rd.clear()
	var animlib: AnimationLibrary = _current.animation_library as AnimationLibrary
	var anim_name := animlib.get_animation_list()[_current.db_anim_index[pose_index]]
	var anim := animlib.get_animation(anim_name)
	var anim_timestep := _current.db_anim_timestamp[pose_index]
	var anim_cat := _current.db_anim_category[pose_index]

	if anim_name == "Idle":
		var v := anim.find_track("MotionPlayer:loco_category", Animation.TYPE_VALUE)
		prints("Values interpolated", anim.value_track_interpolate(v, anim_timestep))

	_current.set_skeleton_to_pose(anim, anim_timestep)
	var skel := _current.get_node(_current.get("skeleton_node_path")) as Skeleton3D
	var tr := skel.get_bone_global_pose(skel.find_bone("Root"))
	tr = Transform3D()

	rd.push_table(4)
	for t in [anim_name, "%*.*f" % [0, 3, anim_timestep], str(anim.length), str(anim_cat)]:
		rd.push_cell()
		rd.set_cell_padding(Rect2i(5, 0, 5, 5))
		rd.set_cell_border_color(Color.LIGHT_GRAY)
		rd.append_text(t)
		rd.pop()
	rd.pop()
	rd.append_text("\n")

	rd.push_table(pose.size() + 1, INLINE_ALIGNMENT_CENTER)

	var debug_lines := PackedVector3Array()
	gizmo.instance.clear()

	gizmo.create_material("main", Color.BLUE)

	var offset := 0
	for r in _current.motion_features:
		var f: MotionFeature = r as MotionFeature
		rd.push_cell()
		rd.set_cell_row_background_color(Color(0.212, 0.239, 0.29), Color(0.212, 0.239, 0.29) / 0.7)
		rd.set_cell_border_color(Color.LIGHT_GRAY)
		rd.add_text(r.resource_name + " " + str(r.get_dimension()))
		rd.pop()
		for x in range(0, nb_dim):
			rd.push_cell()
			rd.set_cell_row_background_color(
				Color(0.212, 0.239, 0.29), Color(0.212, 0.239, 0.29) / 0.7
			)

			if x < r.get_dimension():
				rd.set_cell_border_color(Color.LIGHT_SLATE_GRAY)
				var showed_value := pose[offset + x]  #* _current.max_values[offset + x]
				rd.add_text("%*.*f" % [0, 3, showed_value])
			else:
				rd.add_text("")
			rd.pop()

		var showed_values := pose.slice(offset, offset + r.get_dimension())

		r.debug_pose_gizmo(gizmo.instance, showed_values, tr)

		offset += r.get_dimension()

	rd.pop()


@onready var s := $TabContainer/Test/MarginContainer/HFlowContainer/SpinBox
@onready var e := $TabContainer/Test/MarginContainer/HFlowContainer/SpinBox2
@onready var dmax := $TabContainer/Test/MarginContainer/HFlowContainer/SpinBox3
@onready var a := $TabContainer/Test/MarginContainer/HFlowContainer/Answer2


func _max_der_calculate(x):
	a.text = str(CritDampSpring.maximum_spring_velocity_to_halflife(s.value, e.value, dmax.value))


func on_recalculate_weights() -> void:
	_current.recalculate_weights()
	var animlib := _current.animation_library as AnimationLibrary
	update_info()
	add_category_track_to_anims()


func add_category_track_to_anims():
	for animname in _current.animation_library.get_animation_list():
		var anim := _current.animation_library.get_animation(animname)
		var category_track := anim.find_track(
			_current.category_track_names[0], Animation.TYPE_VALUE
		)
		if category_track == -1:
			category_track = anim.add_track(Animation.TYPE_VALUE)
			anim.track_set_path(category_track, _current.category_track_names[0])

		anim.value_track_set_update_mode(category_track, Animation.UPDATE_DISCRETE)
		anim.track_set_interpolation_type(category_track, Animation.INTERPOLATION_NEAREST)


func on_look_for_similar_pressed() -> void:
	var pose_index = $TabContainer/Data/HBoxContainer/MarginContainer2/HBoxContainer/SpinBox.value

	var nb_dim = 0
	for r in _current.motion_features:
		var f: MotionFeature = r as MotionFeature
		nb_dim += r.get_dimension()
	prints(pose_index, nb_dim)
	var query := _current.MotionData.slice(pose_index * nb_dim, pose_index * nb_dim + nb_dim)
	var result := _current.check_query_results(query, 10)

	for r in result:
		prints(r)


func _on_choose_animation_pressed(ID) -> void:
	if _current.animation_library != null:
		var lib: AnimationLibrary = _current.animation_library
		choose_anim.text = choose_anim.get_popup().get_item_text(ID)

		if _current.anim_index_duration_category.size() != 0:
			var i: int = 0
			for index in _current.anim_index_duration_category:
				if index[0] == ID:
					$TabContainer/Data/HBoxContainer/MarginContainer2/HBoxContainer/SpinBox.value = i
					update_shown_pose_data(i)
					break
				i += 1

	pass  # Replace with function body.
