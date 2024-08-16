@tool
class_name StatsPanel extends PanelContainer

#TODO Save the data in the mmlib. 

@onready var mmlib : MMAnimationLibrary
@onready var data := Array()

@onready var dimension_number: SpinBox = %DimensionNumber
@onready var simple_stats: Tree = %SimpleStats
@onready var histogram: HBoxContainer = %Histogram
@onready var hints: Label = %Hints

@onready var histogram_item: VBoxContainer = %HistogramItem

func _on_lib_changed(_mmlib : MMAnimationLibrary):
	mmlib = _mmlib
	prints("lib setup")
	
func _on_dimension_number_value_changed(value: float) -> void:
	if mmlib == null:
		prints("oups")
		return
	dimension_number.max_value = mmlib.nb_dimensions
	data = mmlib.get_stats()
	var f_hints := PackedStringArray()
	for f in mmlib.motion_features:
		f_hints.append_array(f.get_hints())
	hints.text = f_hints[int(value)]
		
		
	
	queue_redraw()
	pass

func _draw() -> void:
	if data.size() <= 0:
		return
	dimension_number.queue_redraw()
	var dict :Dictionary= data[int(dimension_number.value)]
	simple_stats.clear()
	var keys := dict.keys().filter(func(k:String) : return not k.begins_with('density'))
	
	simple_stats.columns = keys.size()
	
	var root := simple_stats.create_item()
	var item := simple_stats.create_item(root)
	for i in range(keys.size()):
		simple_stats.set_column_title(i,keys[i])
		simple_stats.set_column_title_alignment(i,HORIZONTAL_ALIGNMENT_FILL)
		item.set_text(i,str(dict[keys[i]]))
		item.set_text_alignment(i,HORIZONTAL_ALIGNMENT_CENTER)
		
	prints(dict)
		
	var hist_bounds :PackedFloat32Array= dict['density_hist_bounds']
	var hist_values :PackedFloat32Array= dict['density_hist_values']
	
	for c in histogram.get_children():
		histogram.remove_child(c)
		c.queue_free()
	for i in range(hist_bounds.size()):
		var new_hist_item := histogram_item.duplicate()
		var label :Label= new_hist_item.get_child(1)
		var progress :TextureProgressBar= new_hist_item.get_child(0)
		new_hist_item.visible = true
		label.text = "%0.4f" % hist_bounds[i]
		label.clip_text = false
		
		progress.value = hist_values[i]
		if i == 0:
			label.text = '<'+label.text
		elif i == hist_bounds.size() - 1:
			label.text = '>'+label.text
		histogram.add_child(new_hist_item)
		
		pass
		
	
		
