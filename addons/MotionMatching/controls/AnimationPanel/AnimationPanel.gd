@tool
class_name AnimationPanel extends Control

var al :AnimationLibrary
@onready var tree: Tree = %Tree
@onready var add_anim_button: Button = %AddAnimButton

enum Column {Id,Name,Duration,NbEvents}

func _ready() -> void:
	add_anim_button.pressed.connect(add_animation)
	pass


func _on_change_al(_al:MMAnimationLibrary):
	tree.clear()
	tree.column_titles_visible = true
	al = _al
	var root := tree.create_item()
	tree.columns = Column.size()
	tree.set_column_expand(Column.Id,true)
	tree.set_column_expand(Column.Name,true)
	tree.set_column_expand_ratio(Column.Name,3)
	tree.set_column_expand(Column.Duration,true)
	tree.set_column_expand(Column.NbEvents,true)

	for i in range(Column.size()):
		tree.set_column_title(i,Column.keys()[i])
		tree.set_column_title_alignment(i,HORIZONTAL_ALIGNMENT_LEFT)
		tree.set_column_clip_content(i,false)

	var index := 0
	for animname in al.get_animation_list():
		var anim := al.get_animation(animname)
		var animitem := tree.create_item(root)
		animitem.set_text(Column.Id,"%0*d" % [2, index])
		animitem.set_text(Column.Name,animname)
		animitem.set_text(Column.Duration,"%.03f" % anim.length)
		animitem.set_text(Column.NbEvents,str(14))

		animitem.set_text_overrun_behavior(Column.NbEvents,TextServer.OVERRUN_NO_TRIMMING)
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




