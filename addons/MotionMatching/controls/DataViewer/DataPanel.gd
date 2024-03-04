@tool
class_name DataPanel extends HBoxContainer

@onready var tree: Tree = %Tree
@onready var lign_selector: SpinBox = %LignSelector
@onready var choose_animation: MenuButton = %ChooseAnimation
@onready var lookup_similar_button: Button = %LookupSimilarButton

var lib : MMAnimationLibrary
var current_animation : Animation
var current_animation_name : StringName

signal request_pose(animation_name:StringName,timestamp:float)


func _on_mm_editor_on_new_library_change(_lib: MMAnimationLibrary) -> void:
	lib = _lib
	reset()

func reset():
	tree.clear()
	lign_selector.min_value = 0
	lign_selector.value = 0
	choose_animation.text = "Select Animation"
	var popup := choose_animation.get_popup()
	popup.clear()
	for a in lib.get_animation_list():
		popup.add_item(a)
	if !popup.id_pressed.is_connected(on_select_animation_popup):
		popup.id_pressed.connect(on_select_animation_popup)
	tree.clear()
	tree.queue_redraw()
	choose_animation.queue_redraw()

	#show_lign(0)
	pass # Replace with function body.

func on_select_animation_popup(ID:int):
	if not lib.get_animation_list().is_empty() :
		choose_animation.text = choose_animation.get_popup().get_item_text(ID)
		tree.clear()
		if lib.db_anim_index.size() != 0:
			var i :int= 0
			for index in lib.db_anim_index:
				if index == ID:
					lign_selector.value = i # will call show_lign by events
					return
				i += 1


func show_lign(lign:int)->void:
	tree.clear()
	tree.columns = 1
	#
	if lib.MotionData.size() <= 0:
		lign_selector.value = 0
		return
	elif lib.motion_features.is_empty():
		lign_selector.value = 0
		return

	if  lign*lib.nb_dimensions > lib.MotionData.size():
		lign = (lib.MotionData.size() - lib.nb_dimensions)/ lib.nb_dimensions
	var dim := lib.nb_dimensions
	var data := lib.MotionData.slice(lign*lib.nb_dimensions,lign*lib.nb_dimensions+lib.nb_dimensions)

	for f in lib.motion_features as Array[MotionFeature]:
		if f.get_dimension() > tree.columns:
			tree.columns = f.get_dimension()

	var root := tree.create_item()
	var data_folder := tree.create_item(root,0)
	data_folder.set_text(0,"Features Data")
	data_folder.set_custom_bg_color(0,Color.WHITE,true)
	var category_folder := tree.create_item(root,1)
	category_folder.set_text(0,"Category")
	category_folder.set_custom_bg_color(0,Color.WHITE,true)
	var category_item :TreeItem= tree.create_item(category_folder)
	category_item.set_text(0, str(lib.db_anim_category[lign]))

	var info_folder := tree.create_item(root,2)
	info_folder.set_text(0,"TimeStamp")
	info_folder.set_custom_bg_color(0,Color.WHITE,true)
	var timestamp_item :TreeItem= tree.create_item(info_folder)
	timestamp_item.set_text(0, str(lib.db_anim_timestamp[lign]))

	tree.columns += 1
	tree.set_column_title(0,"Info")
	tree.button_clicked.connect(func(item: TreeItem, column: int, id: int, mouse_button_index: int):
		if item.get_metadata(0) is MotionFeature:
			EditorInterface.inspect_object(item.get_metadata(0))
		)

	var offset = 0
	var f_index := 0
	for f in lib.motion_features as Array[MotionFeature]:
		var feature_row :TreeItem= tree.create_item(data_folder,f_index)


		var dim_row := tree.create_item(feature_row,0)
		var hint_row := tree.create_item(feature_row,1)
		var data_row := tree.create_item(feature_row,2)
		var weights_row := tree.create_item(feature_row,3)

		feature_row.set_text(0,f.resource_name if not f.resource_name.is_empty() else f.get_class())
		feature_row.set_metadata(0,f)
		#feature_row.set_text_alignment(0,HORIZONTAL_ALIGNMENT_LEFT)
		var texture := EditorInterface.get_base_control().get_theme_icon("Zoom","EditorIcons")

		feature_row.add_button(0,texture,-1,false,f.resource_name if not f.resource_name.is_empty() else f.get_class())


		data_row.set_text(0, "Data")
		data_row.set_text_alignment(0,HORIZONTAL_ALIGNMENT_RIGHT)
		hint_row.set_text(0, "Hints")
		hint_row.set_text_alignment(0,HORIZONTAL_ALIGNMENT_RIGHT)
		dim_row.set_text(0, "Dimension")
		dim_row.set_text_alignment(0,HORIZONTAL_ALIGNMENT_RIGHT)
		weights_row.set_text(0, "Weights(static)")
		weights_row.set_text_alignment(0,HORIZONTAL_ALIGNMENT_RIGHT)
		var hints :PackedStringArray= f.get_hints()
		for i in range(f.get_dimension()):
			var scale :float= lib.feature_scale[i]
			var avg :float= lib.feature_offset[i]
			data_row.set_text(i+1, "%0.3f" % ((data[offset+i] * scale) + avg ) )
			data_row.set_text_alignment(i+1,HORIZONTAL_ALIGNMENT_CENTER)

			dim_row.set_text(i+1,str(offset + i))
			dim_row.set_text_alignment(i+1,HORIZONTAL_ALIGNMENT_CENTER)

			hint_row.set_text(i+1,hints[i])
			hint_row.set_text_alignment(i+1,HORIZONTAL_ALIGNMENT_CENTER)

			weights_row.set_cell_mode(i+1,TreeItem.CELL_MODE_RANGE)
			weights_row.set_range_config(i+1,0.001,1000,0.1,true)
			weights_row.set_editable(i+1,true)
			weights_row.set_selectable(i+1,true)
			weights_row.set_range(i+1,lib.weights[offset+i])
		offset += f.get_dimension()
		f_index += 1



	var pose_index :int = lign as int
	if pose_index > lib.MotionData.size()/lib.nb_dimensions:
		return

	var animlib :MMAnimationLibrary= lib as MMAnimationLibrary
	var anim_name := animlib.get_animation_list()[lib.db_anim_index[pose_index]]
	var anim := animlib.get_animation(anim_name)
	var anim_timestep := lib.db_anim_timestamp[pose_index]

	request_pose.emit(anim_name,anim_timestep)

	tree.queue_redraw()

	pass


func _on_tree_item_edited() -> void:
	# Navigate the tree to get in index offset of the weights
	var item := tree.get_edited()
	if item.get_text(0) == "Weights(static)":
		var feature_nb := item.get_parent().get_index()
		var offset := 0
		for f in range(feature_nb):
			offset+= lib.motion_features[f].get_dimension()
		offset +=  tree.get_edited_column() - 1
		lib.weights[offset] = item.get_range(tree.get_edited_column())


	pass # Replace with function body.
