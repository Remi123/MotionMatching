@tool
class_name TagEditor extends Control

@onready var animation_selector: MenuButton = %AnimationSelector
@onready var zoom: HSlider = %Zoom
@onready var timeline: HSlider = %Timeline
@onready var track_list: VBoxContainer = %TrackList


var current_animlib : MMAnimationLibrary = ResourceLoader.load("res://Resources/AnimationLibrary/MM.tres")
var current_animation : Animation
var current_animation_name : StringName

const TRACK_BAR = preload("res://addons/MotionMatching/controls/Tags/TracksBar/track_bar.tscn")

const CATEGORY_EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/Category_Event_Bar.tscn")
const EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/event_bar.tscn")
const JUNK_EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/junk_event_bar.tscn")

@export var tags :Array[TagInfo] = [] :
	get:
		return [] if current_animlib == null else current_animlib.tags
	#set(value)

func _on_library_selected(lib:MMAnimationLibrary):
	current_animlib = lib
	animation_selector.get_popup().clear(true)
	for a in current_animlib.get_animation_list():
		animation_selector.get_popup().add_item(a)

	timeline.value = 0;
	pass

func get_length() -> int:
	return $MarginContainer/ScrollContainer.size.x / zoom.value

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	zoom.value = 1.0
	zoom.value_changed.connect(_on_zoom_changed)

	var popup = animation_selector.get_popup()
	animation_selector.get_popup().clear()
	for a in current_animlib.get_animation_list():
		animation_selector.get_popup().add_item(a)
	animation_selector.get_popup().id_pressed.connect(_on_anim_selected)

var current_tags :Array[TagInfo]= []

func _on_anim_selected(index:int):
	current_animation_name = current_animlib.get_animation_list()[index]
	current_animation = current_animlib.get_animation(current_animation_name)
	animation_selector.text = current_animlib.get_animation_list()[index]

	zoom.value = 1.0

	current_tags = tags.filter(func(tag:TagInfo):
		return current_animation_name == tag.tag_name)

	for track in track_list.get_children() as Array[TrackBar]:
		track.queue_free()

	var max_track := 0
	if current_tags.size() > 0:
		max_track = current_tags.reduce( func(max:TagInfo, val:TagInfo):return val if val.track_id >= max.track_id  else max).track_id
	prints("FOUND",current_tags.size(),"CURRENT TAGS,MAXTRACK=",max_track)
	# add new empty tracks
	for t in range(max_track+1):
		var new_track :TrackBar= TRACK_BAR.instantiate()
		track_list.add_child(new_track)
		track_list.move_child(new_track,t)
		new_track.added_track.connect(add_track)
		new_track.delete_track.connect(on_delete_track)
		new_track.added_event.connect(on_new_tag)
		new_track.animation_length = current_animation.length
		new_track.current_animation = current_animation
		new_track.current_library = current_animlib
	prints("Created",track_list.get_child_count(),"tracks")
	for t in track_list.get_children():
		t.size.y = 30
		prints("Track#",t.get_index(),t.position,t.size)


	for events in current_tags as Array[TagInfo]:
		var track := track_list.get_child(events.track_id) as TrackBar
		# Add events panel there
		prints("Events",events.track_id,events.timestamp,track.get_index())
		track.populate_tag(events,false)


	timeline.step = current_animation.step
	timeline.tick_count = 8
	timeline.max_value = current_animation.length
	prints("AnimInfo",current_animation.length,current_animation.step,current_animation.length/current_animation.step)

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
		track.current_library = current_animlib
		track.animation_length = current_animation.length
		track.current_animation = current_animation
		track.queue_redraw()
	timeline.queue_redraw()
	track_list.queue_redraw()


func add_track(index:int):
	var new_track :TrackBar= TRACK_BAR.instantiate()
	if(index < track_list.get_child_count()):
		track_list.get_child(index).add_sibling(new_track)
	else:
		track_list.add_child(new_track)

	new_track.size.y = 30
	new_track.added_track.connect(add_track)
	new_track.delete_track.connect(on_delete_track)
	new_track.added_event.connect(on_new_tag)
	new_track.animation_length = current_animation.length
	new_track.current_animation = current_animation
	new_track.current_library = current_animlib
	queue_redraw()


# the track bar is responsible to delete itself from the tree.
func on_delete_track(index:int):
	for tag in tags:
		if tag.track_id == index:
			tags.erase(tag)
	pass

func on_new_tag(tag:TagInfo):
	tag.tag_name = current_animation_name
	tag.duration = current_animation.step
	tags.append(tag)
	for t in track_list.get_children() as Array[Control]:
		t.queue_redraw()
	return;




func _on_category_flag_edit_text_submitted(new_text: String) -> void:
	for tag in current_animlib.tags as Array[TagInfo]:
		if tag is TagCategory:
			var category_tag := tag as TagCategory
			category_tag.property_hint_string = new_text
	pass # Replace with function body.
