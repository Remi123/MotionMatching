@tool
class_name MMBottonScene extends Control


signal animation_selected(mmlib:MMAnimationLibrary,anim :String)
signal pose_selected(mmlib:MMAnimationLibrary,anim :String, time:float)

@onready var prev_button: Button = %PrevButton
@onready var next_button: Button = %NextButton
@onready var anim_label: Label = %AnimLabel

@onready var pose_slider: HSlider = %PoseSlider
@onready var time: SpinBox = %TimeEdit
@onready var time_timer: Timer = %TimeEditTimer

@onready var animations: AnimationPanel = %Animations
@onready var data: DataPanel = %Data
@onready var tags: TagsPanel = %Tags
@onready var stats: StatsPanel = %Stats

var current_lib : MMAnimationLibrary = null
var current_animation : String
var current_time : float = 0.0

func _ready() -> void:
	pose_slider.value_changed.connect(time.set_value)
	time.value_changed.connect(pose_slider.set_value_no_signal)
	
	time_timer.timeout.connect(func():
		pose_selected.emit(current_lib,current_animation,current_time))
	time.value_changed.connect(func(t):
		if current_time != t:
			current_time = t
			if time_timer.is_stopped():
				time_timer.start()
			)

	_connect_tab_animation()
	_connect_tab_data()
	_connect_tab_stats()
	_connect_tab_tags()
	
	

func _connect_tab_animation():
	animations.on_animation_selected.connect(_on_animation_selected)
	pass
func _connect_tab_tags():
	animation_selected.connect(tags._on_anim_changed)
	pass
func _connect_tab_data():
	pose_selected.connect(data.show_data)
	pass
func _connect_tab_stats():
	pass

func on_lib_selected(lib:MMAnimationLibrary):
	if current_lib:
		current_lib.changed.disconnect(_reset_slider)
	current_lib	= lib
	current_animation = current_lib.get_animation_list()[0]
	current_time = 0.0
	current_lib.changed.connect(_reset_slider)
	
	animations._on_mm_editor_on_library_change(lib)
	stats._on_lib_changed(lib)
	
	prev_button.disabled = false;
	next_button.disabled = false;
	
	_reset_slider()
	
func _on_animation_selected(lib:MMAnimationLibrary,n :String):
		current_lib = lib 
		current_animation = n
		current_time = 0.0
		time.value = current_time
		_reset_slider()
		pose_selected.emit(current_lib,current_animation,current_time)
		animation_selected.emit(current_lib,current_animation)
		anim_label.text = n
		
	
func _reset_slider():
	if not current_animation in current_lib.get_animation_list():
		current_animation =  current_lib.get_animation_list()[0]
		current_time = 0.0
		pose_selected.emit(current_lib,current_animation,current_time)
	
	var anim := current_lib.get_animation(current_animation)
	var multiple :float= current_lib.time_interval
	var nearest :float= anim.length - fposmod(anim.length ,multiple)
	pose_slider.step = current_lib.time_interval
	pose_slider.tick_count = nearest/multiple + 1
	pose_slider.min_value = 0.0
	pose_slider.max_value = nearest
	pose_slider.queue_redraw()
	time.min_value = 0.0
	time.max_value = anim.length
	time.step = pose_slider.step
	time.suffix = '/' + "%0.3f" % anim.length + "s"
	time.queue_redraw()
	pass
	
	
	


func _on_bake_button_pressed() -> void:
	current_lib.bake_data()
	ResourceSaver.save(current_lib)
	pass # Replace with function body.


func _on_weights_button_pressed() -> void:
	current_lib.recalculate_weights()
	ResourceSaver.save(current_lib)
	pass # Replace with function body.


func _on_prev_button_pressed() -> void:
	var curr_index := current_lib.get_animation_list().find(current_animation)
	var prev_index := clampi(curr_index-1,0,current_lib.get_animation_list().size()) 
	var prev_animation_name := current_lib.get_animation_list()[prev_index]
	if current_animation != prev_animation_name:
		_on_animation_selected(current_lib,prev_animation_name)
	pass # Replace with function body.


func _on_next_button_pressed() -> void:
	var curr_index := current_lib.get_animation_list().find(current_animation)
	var next_index := clampi(curr_index+1,0,current_lib.get_animation_list().size()) 
	var next_animation_name := current_lib.get_animation_list()[next_index]
	if current_animation != next_animation_name:
		_on_animation_selected(current_lib,next_animation_name)
	pass # Replace with function body.
