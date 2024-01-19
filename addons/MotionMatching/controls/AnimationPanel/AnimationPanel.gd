@tool
class_name AnimationPanel extends Control

var al :AnimationLibrary
@onready var tree: Tree = %Tree
@onready var add_anim_button: Button = %AddAnimButton

enum Column {Id,Name,Duration,Actions}
enum Action_ID {Rename,MirrorX,Delete = 99}

func _ready() -> void:
	add_anim_button.pressed.connect(add_animation)
	var add_icon :Texture2D= EditorInterface.get_editor_theme().get_icon("Load","EditorIcons")
	add_anim_button.icon = add_icon
	pass


func _on_mm_editor_on_library_change(lib: MMAnimationLibrary) -> void:
	al = lib
	queue_redraw()

func _draw() -> void:
	tree.clear()
	tree.column_titles_visible = true

	var root := tree.create_item()
	tree.columns = Column.size()
	tree.set_column_expand(Column.Id,true)
	tree.set_column_expand(Column.Name,true)
	tree.set_column_expand_ratio(Column.Name,10)
	tree.set_column_expand(Column.Duration,true)
	tree.set_column_expand(Column.Actions,true)

	for i in range(Column.size()):
		tree.set_column_title(i,str(Column.keys()[i]))
		tree.set_column_title_alignment(i,HORIZONTAL_ALIGNMENT_LEFT)
		tree.set_column_clip_content(i,false)
	tree.set_column_title_alignment(Column.Duration,HORIZONTAL_ALIGNMENT_RIGHT)
	tree.set_column_title_alignment(Column.Actions,HORIZONTAL_ALIGNMENT_RIGHT)

	var index := 0
	if al != null:
		for animname in al.get_animation_list():
			var anim := al.get_animation(animname)
			var animitem := tree.create_item(root)
			animitem.set_cell_mode(0,TreeItem.CELL_MODE_RANGE)
			animitem.set_range(Column.Id,index)
			animitem.set_text(Column.Name,animname)
			animitem.set_text(Column.Duration,"%0.3f" % anim.length)
			animitem.set_suffix(Column.Duration,"s")
			animitem.set_text_alignment(Column.Duration,HORIZONTAL_ALIGNMENT_RIGHT)

			var rename_icon := EditorInterface.get_editor_theme().get_icon("Edit","EditorIcons")
			var delete_icon := EditorInterface.get_editor_theme().get_icon("Remove","EditorIcons")
			var mirror_icon := EditorInterface.get_editor_theme().get_icon("MirrorX","EditorIcons")
			animitem.add_button(Column.Actions,rename_icon,Action_ID.Rename,false,"Rename")
			animitem.add_button(Column.Actions,mirror_icon,Action_ID.MirrorX,false,"Mirror Animation on X Axis")
			animitem.add_button(Column.Actions,delete_icon,Action_ID.Delete,false,"Delete Animation")
			animitem.set_text_alignment(Column.Actions,HORIZONTAL_ALIGNMENT_RIGHT)


			animitem.set_text_overrun_behavior(Column.Actions,TextServer.OVERRUN_NO_TRIMMING)
			index += 1

	tree.queue_redraw()

#region Add Animations

var fileDialog : EditorFileDialog
func add_animation():
	fileDialog = EditorFileDialog.new()
	fileDialog.file_mode = EditorFileDialog.FILE_MODE_OPEN_FILES
	fileDialog.mode = EditorFileDialog.MODE_MAXIMIZED
	fileDialog.access = EditorFileDialog.ACCESS_RESOURCES
	fileDialog.file_selected.connect(on_file_selected)
	fileDialog.files_selected.connect(on_files_selected)
	fileDialog.dir_selected.connect(on_file_selected)
	var viewport = EditorInterface.get_editor_main_screen()
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
			if al.add_animation(anim_name,file) != OK:
				prints("Didn't add",anim_name)
	if (fileDialog != null):
		fileDialog.queue_free() # Dialog has to be freed in order for the script t


	pass

func on_file_selected(filename : String) :
	if (fileDialog != null):
		fileDialog.queue_free() # Dialog has to be freed in order for the script t
#endregion









func _on_mirror_anim_button_pressed(anim_name:String) -> void:
	# mirror on X
	var anim := al.get_animation(anim_name)
	var mirror := Animation.new()

	if anim == null || mirror == null:
		push_warning("Animation not selected or not found")
		return

	#clean mirror
	mirror.clear()
	mirror.length = anim.length
	for track in range(anim.get_track_count()):
		anim.copy_track(track,mirror)
		if mirror.track_get_type(track) == Animation.TYPE_POSITION_3D:
			for key in range(mirror.track_get_key_count(track)):
				var value : Vector3 = mirror.track_get_key_value(track,key)
				value.x *= -1
				mirror.track_set_key_value(track,key,value)
		if mirror.track_get_type(track) == Animation.TYPE_ROTATION_3D:
			for key in range(mirror.track_get_key_count(track)):
				var value : Quaternion = mirror.track_get_key_value(track,key)
				value.y *= -1
				value.z *= -1
				mirror.track_set_key_value(track,key,value)
		var path := mirror.track_get_path(track)
		if path.get_concatenated_subnames().begins_with("Left"):
			var new_path := path.get_concatenated_names() + ":Right" + path.get_concatenated_subnames().trim_prefix("Left")
			mirror.track_set_path(track,new_path)
		elif path.get_concatenated_subnames().begins_with("Right"):
			var new_path := path.get_concatenated_names() + ":Left" + path.get_concatenated_subnames().trim_prefix("Right")
			mirror.track_set_path(track,new_path)

	# Save the animation
	al.add_animation(anim_name + "_Mx",mirror)
	queue_redraw()
	pass # Replace with function body.

@onready var confirmation_dialog: ConfirmationDialog = $ConfirmationDialog

func _on_remove_anim_button_pressed() -> void:
	if tree.get_selected() == null:
		return
	var anim_name :StringName= tree.get_selected().get_text(1)
	confirmation_dialog.popup_centered()
	confirmation_dialog.dialog_text = "Delete the animation named " + anim_name + "?"

	pass # Replace with function body.


func _on_confirm_delete_anim(anim_name:StringName) -> void:
	al.remove_animation(anim_name)
	queue_redraw()


func _on_tree_button_clicked(item: TreeItem, column: int, id: int, mouse_button_index: int) -> void:
	print(column,id)
	if column == Column.Actions:
		match id:
			Action_ID.Delete:
				var anim_name = item.get_text(1)
				confirmation_dialog.popup_centered()
				confirmation_dialog.dialog_text = "Delete the animation named " + anim_name + "?"
				if confirmation_dialog.confirmed.is_connected(_on_confirm_delete_anim):
					confirmation_dialog.confirmed.disconnect(_on_confirm_delete_anim)
				confirmation_dialog.confirmed.connect(_on_confirm_delete_anim.bind(anim_name),Object.CONNECT_ONE_SHOT)
			Action_ID.MirrorX:
				var anim_name = item.get_text(1)
				_on_mirror_anim_button_pressed(anim_name)
			Action_ID.Rename:
				old_name = item.get_text(Column.Name)
				item.set_editable(Column.Name,true)
				tree.set_selected(item,Column.Name)
				tree.edit_selected()
				pass

	pass # Replace with function body.

var old_name := ""
func _on_tree_item_edited() -> void:
	var item := tree.get_edited()
	var column := tree.get_edited_column()
	if column == Column.Name:
		var new_name := item.get_text(Column.Name)
		al.rename_animation(old_name,new_name)
		item.set_editable(Column.Name,false)
	pass # Replace with function body.
