@tool
class_name MMEditor extends PanelContainer

@onready var anim_panel: AnimationPanel = %Animations
@onready var stat_panel: PanelContainer = %Stats
@onready var data_panel: VBoxContainer = %Data
@onready var calc_panel: HFlowContainer = %Calculation
@onready var tags_panel: TagEditor = %Tags

@onready var path_label: Label = %PathLabel


signal on_library_change(lib:MMAnimationLibrary)
signal on_bake
signal on_weights

var library : MMAnimationLibrary:
	get:
		return library
	set(value):
		library = value
		path_label.text = library.resource_path
		on_library_change.emit(library)


# Nothing interesting to do here
func _ready() -> void:
	on_library_change.connect(anim_panel._on_change_al)
	on_library_change.connect(tags_panel._on_library_selected)

	pass




