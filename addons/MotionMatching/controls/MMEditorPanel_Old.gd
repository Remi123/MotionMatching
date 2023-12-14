@tool
class_name MMEditorPanel extends Control

@onready var tags: TagEditor = $TabContainer/Tags


var _current : MMAnimationLibrary = null :
	get:
		return _current
	set(value):
		_current=value
		update_info()

var plugin_ref : EditorPlugin

@onready var rd : RichTextLabel = $TabContainer/Data/ScrollContainer/PoseData

# @onready var gizmo := preload("res://addons/MotionMatching/MMEditorGizmoPlugin.gd") # setup by the plugin

@onready var choose_anim :MenuButton= $TabContainer/Data/HBoxContainer/MarginContainer5/ChooseAnimation

@onready var infotext : RichTextLabel = $TabContainer/Info/PanelContainer/InfoText

func update_info()->void:
	$TabContainer/Info/MarginContainer4/HBoxContainer/PathText.text = _current.resource_path

	if _current != null:

		var nb_dim = _current.nb_dimensions

		if nb_dim == 0:
			nb_dim = 1

		choose_anim.get_popup().clear()
		for a in _current.get_animation_list():
			choose_anim.get_popup().add_item(a)
		choose_anim.get_popup().id_pressed.connect(_on_choose_animation_pressed)


		infotext.clear()
		var info := [	["Nb Features", _current.motion_features.size()],
						["Nb Dimension",nb_dim],
						["Nb Poses",_current.MotionData.size()/nb_dim],
					]
		infotext.push_table(info.size())
		for t in info:
			infotext.push_cell()
			infotext.set_cell_padding(Rect2i(5,0,5,5))
			infotext.set_cell_border_color(Color.LIGHT_GRAY)
			infotext.append_text(t[0]+":"+str(t[1]))
			infotext.pop()
		infotext.pop()
		prints("Gos")

		infotext.append_text("\n\nDimensional informations:\n")

		infotext.push_table(nb_dim+1)


		info = [
			["Dimensions",range(nb_dim)],
			["Means",_current.means],
			["Variances",_current.variances],
			["Weights",_current.weights],
		]

		for i in info:
			infotext.push_cell()
			infotext.set_cell_padding(Rect2i(5,0,5,5))
			infotext.set_cell_border_color(Color.LIGHT_GRAY)
			infotext.append_text(i[0])
			infotext.pop()
			prints(i[0],i[1].size())
			for subinfo in i[1]:
				infotext.push_cell()
				infotext.set_cell_row_background_color(Color(0.212, 0.239, 0.29)/0.9,Color(0.212, 0.239, 0.29)*0.8)
				infotext.set_cell_border_color(Color.LIGHT_GRAY)
				if subinfo is float:
					infotext.append_text("%*.*f" % [0,3, subinfo ])
				elif subinfo is int:
					infotext.append_text(str(subinfo))
				infotext.pop()

		infotext.pop()



func bake_data_current()->void:
	if _current == null:
		return
	_current.bake_data()
	update_info()

func update_shown_pose_data(value: float) -> void:
	assert(_current.nb_dimensions > 0)

	var nb_dim := _current.nb_dimensions

	var pose_index :int = value as int

	if pose_index > _current.MotionData.size()/nb_dim:
		return

	var pose := _current.MotionData.slice(pose_index*nb_dim,pose_index*nb_dim+nb_dim)
	#for i in range(pose.size()):
		#pose[i] = pose[i] * _current.variances[i] + _current.means[i]

	rd.clear()
	var animlib :AnimationLibrary= _current as AnimationLibrary
	var anim_name := animlib.get_animation_list()[_current.db_anim_index[pose_index]]
	var anim := animlib.get_animation(anim_name)
	var anim_timestep := _current.db_anim_timestamp[pose_index]
	var anim_cat := _current.db_anim_category[pose_index]

# 	if anim_name == "Idle":
# 		var v := anim.find_track("MotionPlayer:loco_category",Animation.TYPE_VALUE)
# 		prints("Values interpolated",anim.value_track_interpolate(v,anim_timestep))

# 	_current.set_skeleton_to_pose(anim,anim_timestep)
# 	var skel := _current.get("skeleton_node_path") as Skeleton3D
# 	var tr := skel.get_bone_global_pose(skel.find_bone("Root"))
# 	tr = Transform3D()

	rd.push_table(4)
	for t in [anim_name,"%*.*f" % [0,3, anim_timestep ], str(anim.length),str(anim_cat)]:
		rd.push_cell()
		rd.set_cell_padding(Rect2i(5,0,5,5))
		rd.set_cell_border_color(Color.LIGHT_GRAY)
		rd.append_text(t)
		rd.pop()
	rd.pop()
	rd.append_text("\n")

	rd.push_table(pose.size() + 1,INLINE_ALIGNMENT_CENTER)

# 	var debug_lines := PackedVector3Array()
# 	gizmo.instance.clear()

# #	gizmo.create_material("main",Color.BLUE)
	var global_offset := pose_index*nb_dim
	var offset := 0
	for r in _current.motion_features:
		var f :MotionFeature= r as MotionFeature
		rd.push_cell()
		rd.set_cell_row_background_color(Color(0.212, 0.239, 0.29),Color(0.212, 0.239, 0.29)/0.7)
		rd.set_cell_border_color(Color.LIGHT_GRAY)
		rd.add_text("Feature " + r.resource_name +' dim ' +str(r.get_dimension())+ ' ')
		rd.pop()
		for x in range(0,nb_dim):
			rd.push_cell()
			rd.set_cell_row_background_color(Color(0.212, 0.239, 0.29),Color(0.212, 0.239, 0.29)/0.7)

			if x < r.get_dimension():
				rd.set_cell_border_color(Color.LIGHT_SLATE_GRAY)
				var showed_value := pose[offset+x]# (pose[offset+x] * _current.variances[offset+x]) + _current.means[offset+x]
				rd.add_text("%*.*f" % [0,3, showed_value ])
			else:
				rd.add_text("")
			rd.pop()

		var showed_values :=  pose.slice(offset,offset+r.get_dimension())


#		r.debug_pose_gizmo(gizmo.instance,showed_values,tr )

		offset += r.get_dimension()

	rd.pop()


@onready var s := $TabContainer/Calculation/MarginContainer/HFlowContainer/SpinBox
@onready var e := $TabContainer/Calculation/MarginContainer/HFlowContainer/SpinBox2
@onready var dmax := $TabContainer/Calculation/MarginContainer/HFlowContainer/SpinBox3
@onready var a := $TabContainer/Calculation/MarginContainer/HFlowContainer/Answer2
func _max_der_calculate(x):
	Spring
	a.text = str(Spring.maximum_spring_velocity_to_halflife(s.value,e.value,dmax.value))


func on_recalculate_weights()->void:
	_current.recalculate_weights()
	prints(_current.weights)
# 	var animlib := _current.animation_library as AnimationLibrary
	update_info()




func on_look_for_similar_pressed() -> void:
	var pose_index = $TabContainer/Data/HBoxContainer/MarginContainer2/HBoxContainer/SpinBox.value


	var nb_dim = 0
	for r in _current.motion_features:
		var f :MotionFeature= r as MotionFeature
		nb_dim += r.get_dimension()
	prints(pose_index, nb_dim)
	var query := _current.MotionData.slice(pose_index*nb_dim,pose_index*nb_dim+nb_dim)
	var result := _current.check_query_results(query,10)

	for r in result:
		prints(r)

# 	pass # Replace with function body.




func _on_choose_animation_pressed(ID) -> void:
	if not _current.get_animation_list().is_empty() :
		var lib :AnimationLibrary = _current
		choose_anim.text = choose_anim.get_popup().get_item_text(ID)

		if _current.db_anim_index.size() != 0:
			var i :int= 0
			for index in _current.db_anim_index:
				if index == ID:
					$TabContainer/Data/HBoxContainer/MarginContainer2/HBoxContainer/SpinBox.value = i
					update_shown_pose_data(i)
					break
				i += 1

# 	pass # Replace with function body.


func _on_add_category() -> void:
	for animname in _current.get_animation_list():
		var anim := _current.get_animation(animname)
		var category_track := anim.find_track(_current.category_track_names[0],Animation.TYPE_VALUE)
		if category_track == -1:
			category_track = anim.add_track(Animation.TYPE_VALUE)
			anim.track_set_path(category_track,_current.category_track_names[0])

		anim.track_insert_key(category_track,0.0,0)
		var max_time := min(anim.length*0.98,anim.length-0.3)
		anim.track_insert_key(category_track,max_time,4294967295)

		anim.value_track_set_update_mode(category_track,Animation.UPDATE_DISCRETE)
		anim.track_set_interpolation_type(category_track,Animation.INTERPOLATION_NEAREST)

# 	pass # Replace with function body.
var fileDialog : EditorFileDialog
func add_animation():
	fileDialog = EditorFileDialog.new()
	fileDialog.file_mode = EditorFileDialog.FILE_MODE_OPEN_FILES
	fileDialog.mode = EditorFileDialog.MODE_MAXIMIZED
	fileDialog.access = EditorFileDialog.ACCESS_RESOURCES
	fileDialog.file_selected.connect(on_file_selected)
	fileDialog.files_selected.connect(on_files_selected)
	fileDialog.dir_selected.connect(on_file_selected)
	var viewport = plugin_ref.get_editor_interface().get_editor_main_screen()
	viewport.add_child(fileDialog)
	fileDialog.set_meta("_created_by", self) # needed so the script is not directly freed after the run function. Would disconnect all signals otherwise
	fileDialog.popup(Rect2(0,0, 700, 500)) # Giving the dialog a predefined size
	print("end")
	pass

func on_files_selected(filesname : PackedStringArray):
	for path in filesname:
		var anim_name = path.get_file().split(".")[0]
		var file := load(path)
		if file is Animation:
			prints(anim_name,file)
			if _current.add_animation(anim_name,file) != OK:
				prints("Didn't add",anim_name)
	if (fileDialog != null):
		fileDialog.queue_free() # Dialog has to be freed in order for the script t


	pass

func on_file_selected(filename : String) :
	print(filename)
	if (fileDialog != null):
		fileDialog.queue_free() # Dialog has to be freed in order for the script t

# Note: passing a value for the type parameter causes a crash
static func get_child_of_type(node: Node, child_type, recursive := false):
	for i in range(node.get_child_count()):
		var child = node.get_child(i)
		if is_instance_of(child, child_type):
			return child
		if recursive:
			var child_node = get_child_of_type(child, child_type, recursive)
			if child_node:
				return child_node


func set_skeleton_to_pose(index:float):
	# Find skeleton
	var skeleton :Skeleton3D= get_child_of_type(EditorInterface.get_edited_scene_root(),Skeleton3D,true)
	assert(skeleton != null,"Skeleton not found")
	var nb_dim := _current.nb_dimensions

	var pose_index :int = index as int

	if pose_index > _current.MotionData.size()/nb_dim:
		return

	var pose := _current.MotionData.slice(pose_index*nb_dim,pose_index*nb_dim+nb_dim)
	for i in range(pose.size()):
		pose[i] = pose[i] * _current.variances[i] + _current.means[i]

	var animlib :AnimationLibrary= _current as AnimationLibrary
	var anim_name := animlib.get_animation_list()[_current.db_anim_index[pose_index]]
	var anim := animlib.get_animation(anim_name)
	var anim_timestep := _current.db_anim_timestamp[pose_index]

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


