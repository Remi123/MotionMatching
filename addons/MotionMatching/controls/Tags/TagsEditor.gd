@tool
class_name TagEditor extends Control

@onready var animation_selector: MenuButton = %AnimationSelector
@onready var zoom: HSlider = %Zoom
@onready var timeline: HSlider = %Timeline
@onready var track_list: VBoxContainer = %TrackList


var current_animlib : MMAnimationLibrary = ResourceLoader.load("res://Resources/AnimationLibrary/MM.tres")
var current_animation : Animation

const TRACK_BAR = preload("res://addons/MotionMatching/controls/Tags/TracksBar/track_bar.tscn")

const CATEGORY_EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/Category_Event_Bar.tscn")
const EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/event_bar.tscn")
const JUNK_EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/junk_event_bar.tscn")

@export var tags :Array[AnimTag] = [] :
	get:
		return [] if current_animlib == null else current_animlib.tags
	#set(value)

func _on_library_selected(lib:MMAnimationLibrary):
	current_animlib = lib
	prints(lib.tags.size)
	pass

func get_length() -> int:
	return $MarginContainer/ScrollContainer.size.x / zoom.value

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	prints("TagEditor Ready")
	zoom.value = 1.0
	zoom.value_changed.connect(_on_zoom_changed)

	var popup = animation_selector.get_popup()
	animation_selector.get_popup().clear()
	for a in current_animlib.get_animation_list():
		animation_selector.get_popup().add_item(a)
	animation_selector.get_popup().id_pressed.connect(_on_anim_selected)
	popup.id_pressed.connect(_on_anim_selected)

	animation_selector.text = current_animlib.get_animation_list()[0]
	animation_selector.get_popup().id_pressed.emit(0)



func _on_anim_selected(index:int):
	current_animation = current_animlib.get_animation(current_animlib.get_animation_list()[index])
	animation_selector.text = current_animlib.get_animation_list()[index]
	prints("Current",index)
	zoom.value = 1.0

	timeline.step = current_animation.step
	timeline.tick_count = 8
	timeline.max_value = current_animation.length
	prints(current_animation.length,current_animation.step,current_animation.length/current_animation.step)

	_on_zoom_changed(zoom.value)
	pass

func _on_zoom_changed(value:float):
	if current_animation == null:
		return;

	timeline.custom_minimum_size.x = get_length()
	$MarginContainer/ScrollContainer/VBoxContainer.queue_redraw()
	for child in track_list.get_children():
		var track := child as TrackBar
		if track == null:
			continue
		track.animation_length = current_animation.length
		track.current_animation = current_animation
		track.queue_redraw()
	timeline.queue_redraw()

func on_new_track(index:int):
	var new_track :TrackBar= TRACK_BAR.instantiate()
	if(index < track_list.get_child_count()):
		track_list.get_child(index).add_sibling(new_track)
	else:
		track_list.add_child(new_track)
	new_track.owner = track_list
	new_track.added_track.connect(on_new_track)
	new_track.delete_track.connect(on_delete_track)
	new_track.added_event.connect(on_new_tag)
	new_track.animation_length = current_animation.length

func on_delete_track(index:int):
	for tag in tags:
		if tag.track_id == index:
			tags.erase(tag)
	pass

func on_new_tag(tag:EventButton):
	tag.tag.duration = current_animation.step
	tags.append(tag.tag)
	return;
	#tags.append(tag)

	#var track :TrackBar= track_list.get_child(tag.track_id)
	#var bar :EventButton= EVENT_BAR.instantiate()
	#bar.tag = tag
	#track.add_child(bar)
	#bar.owner = track
	#track.queue_redraw()
	pass



