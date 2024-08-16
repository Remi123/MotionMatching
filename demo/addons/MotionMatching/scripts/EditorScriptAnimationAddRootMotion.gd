@tool
# CAUTION : Make sure that the hips_track_name and root_track_name are correct
# Modify the animation to take the position and rotation of the hips to the root bone.
# This is intended for animation that put all movement in the hips bone.
# Don't forget to add a root bone to your skeleton in order to see the effect.
class_name EditorScriptAnimationAddRootMotion extends EditorScript

@export var hips_track_name := "%GeneralSkeleton:Hips"
@export var root_track_name := "%GeneralSkeleton:Root"

var fileDialog : EditorFileDialog = null

# Let the user choose which animations will be modified
func _run() -> void:
	fileDialog = EditorFileDialog.new()
	fileDialog.file_mode = EditorFileDialog.FILE_MODE_OPEN_FILES
	fileDialog.access = EditorFileDialog.ACCESS_RESOURCES
	fileDialog.files_selected.connect(on_file_selected)
	var viewport = EditorInterface.get_editor_main_screen()
	viewport.add_child(fileDialog)
	fileDialog.set_meta("_created_by", self) # needed so the script is not directly freed after the run function. Would disconnect all signals otherwise
	fileDialog.popup_centered() # Giving the dialog a predefined size


func on_file_selected(files : PackedStringArray) :
	var i = 0
	prints("Files selected")

	for file in files:
		var anim : Animation = ResourceLoader.load(file)
		prints(i,file,anim.get_path())
		if anim == null :
			prints(file,"isn't loaded correctly")
			return
			
		add_root_motion(anim)
		i += 1
	if (fileDialog != null):
		fileDialog.queue_free() # Dialog has to be freed in order for the script to be called again.
	
	pass	

func get_twist(q : Quaternion, p_axis : Vector3) -> Quaternion:
	var rotationAxis := Vector3(q.x, q.y, q.z);
	var dotProd :float= p_axis.dot(rotationAxis);
	var projection :Vector3= p_axis * dotProd;
	var twist = Quaternion(projection.x, projection.y, projection.z, q.w).normalized();
	if (dotProd < 0.0):
		twist = -twist
	return twist;
func get_swing(q : Quaternion, axis : Vector3) -> Quaternion:
	return q * get_twist(q,axis).inverse()

func add_root_motion(anim:Animation):
		
		assert(anim != null)
		
		var proot := anim.find_track(root_track_name,Animation.TYPE_POSITION_3D)
		var rroot := anim.find_track(root_track_name,Animation.TYPE_ROTATION_3D)
		
		if proot == -1:
			proot = anim.add_track(Animation.TYPE_POSITION_3D)
			anim.track_set_path(proot,root_track_name)
			anim.track_move_to(proot,0)
			proot = 0
		else:
			push_warning("Root Track Detected, aborting for animation",anim.get_path())
			return
				
		if rroot == -1:
			rroot = anim.add_track(Animation.TYPE_ROTATION_3D)
			anim.track_set_path(rroot,root_track_name)
			anim.track_move_to(rroot,0)
		else:
			push_warning("Root Track Detected, aborting for animation",anim.get_path())
			return
		
		proot = anim.find_track(root_track_name,Animation.TYPE_POSITION_3D)
		assert(proot != -1)
		rroot = anim.find_track(root_track_name,Animation.TYPE_ROTATION_3D)
		assert(rroot != -1)
		var phips := anim.find_track(hips_track_name,Animation.TYPE_POSITION_3D)
		assert(phips != -1)
		var rhips := anim.find_track(hips_track_name,Animation.TYPE_ROTATION_3D)
		assert(rhips != -1)
		
		if phips != -1 :
			for key in range(anim.track_get_key_count(phips)):
				var time := anim.track_get_key_time(phips,key)
				var position :Vector3= anim.track_get_key_value(phips,key)
				anim.position_track_insert_key(proot,time,position * Vector3(1,0,1))
				anim.track_set_key_value(phips,key,position * Vector3(0,1,0))
				
		if rhips != -1:
			for key in range(anim.track_get_key_count(rhips)):
				var time := anim.track_get_key_time(rhips,key)
				var rotation :Quaternion= anim.track_get_key_value(rhips,key)
				var twist := get_twist(rotation,Vector3.UP)
				var swing := get_swing(rotation,Vector3.UP)
				anim.rotation_track_insert_key(rroot,time,twist)
				anim.track_set_key_value(rhips,key,twist.inverse() * rotation)
				
		ResourceSaver.save(anim,anim.get_path(),ResourceSaver.FLAG_NONE)
