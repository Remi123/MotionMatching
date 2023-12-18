@tool
class_name TrackBar extends Panel

const TRACK_BAR = preload("res://addons/MotionMatching/controls/Tags/TracksBar/track_bar.tscn")

@onready var popup_menu: PopupMenu = $PopupMenu
@export var animation_length : float = 1.0

var current_library : MMAnimationLibrary = null
var current_animation :Animation = null

signal added_track(id:int)
signal delete_track(id:int)
signal added_event(tag:TagInfo)

var last_right_click_pos := Vector2.ZERO

const CATEGORY_EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/Category_Event_Bar.tscn")
const EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/event_bar.tscn")
const JUNK_EVENT_BAR = preload("res://addons/MotionMatching/controls/Tags/EventBar/junk_event_bar.tscn")

func _can_drop_data(at_position: Vector2, data: Variant) -> bool:
	return data is MoveableUI or (data is ResizeableUI and data.ref.tag.track_id == get_index())

func _drop_data(at_position: Vector2, data: Variant) -> void:
	var mouse = get_local_mouse_position()
	if data is MoveableUI:
		var ev := (data as MoveableUI).ref
		ev.reparent(self,false)
		ev.tag.timestamp = animation_length * at_position.x / size.x
		ev.tag.track_id = get_index()
		#ev.position.x = at_position.x
	elif data is ResizeableUI:
		var old_pos = data.ref.tag.timestamp
		var old_size = data.ref.tag.duration
		if data.drag_left:
			# A    B  C
			var end_time = data.ref.tag.timestamp + data.ref.tag.duration
			data.ref.tag.timestamp = animation_length * at_position.x / size.x
			data.ref.tag.duration = (end_time - data.ref.tag.timestamp)
		elif data.drag_right:
			var end_time :float= animation_length * at_position.x / size.x
			data.ref.tag.duration = (end_time - data.ref.tag.timestamp)
			data.ref.tag.timestamp = data.ref.tag.timestamp
		pass

	queue_redraw()
	pass

func _gui_input(event: InputEvent) -> void:
	var mouse := event as InputEventMouseButton
	if mouse != null and mouse.button_index == MOUSE_BUTTON_RIGHT:
		last_right_click_pos = get_local_mouse_position()
		popup_menu.popup(Rect2i(get_global_mouse_position(),popup_menu.size))


# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	popup_menu.id_pressed.connect(_on_popup_select)

enum {ADDCATEGORY=00, ADDTIMING = 01, ADDJUNK = 02,
	ADDTRACK = 10,
	DELETETRACK = 20}

func _on_popup_select(id:int):
	var index := get_index()
	match id:
		ADDTRACK:
			added_track.emit(index)
		DELETETRACK:
			delete_track.emit(index)
			if index != 0:
				queue_free()
		ADDCATEGORY:
			var new_tag = TagCategory.new()
			new_tag.timestamp = (last_right_click_pos.x / size.x) * animation_length
			new_tag.track_id = get_index()
			populate_tag(new_tag)
		ADDJUNK:
			var new_tag = TagJunk.new()
			new_tag.timestamp = (last_right_click_pos.x / size.x) * animation_length
			new_tag.track_id = get_index()
			populate_tag(new_tag)
	queue_redraw()

func populate_tag(tag:TagInfo,emit:bool = true):
	var EVB :EventButton
	if tag is TagJunk:
		EVB = JUNK_EVENT_BAR.instantiate()
	elif tag is TagCategory:
		EVB = CATEGORY_EVENT_BAR.instantiate()
	else:
		prints("Not implemented",tag.resource_name)
	EVB.tag = tag
	EVB.animation = current_animation
	add_child(EVB)

	if emit:
		added_event.emit(tag)
	queue_redraw()


func _clear_no_delete():
	for c in get_children():
		if c is EventButton:
			c.queue_free()
	queue_redraw()

func _draw() -> void:
	# the size should be set now.
	prints("Track Transform",get_index(),position,size,get_child_count())
	for evbutton in get_children():
		#var evbutton := child as EventButton
		if evbutton == null or not evbutton is EventButton:
			continue;

		evbutton.animation = current_animation
		evbutton.position.x = size.x * evbutton.tag.timestamp / current_animation.length
		evbutton.size.x = size.x * evbutton.tag.duration / current_animation.length
		evbutton.queue_redraw()
	pass
