@tool
extends HFlowContainer
@onready var spin_box: SpinBox = $MarginContainer/HFlowContainer/SpinBox
@onready var spin_box_2: SpinBox = $MarginContainer/HFlowContainer/SpinBox2
@onready var spin_box_3: SpinBox = $MarginContainer/HFlowContainer/SpinBox3
@onready var answer_2: Label = $MarginContainer/HFlowContainer/Answer2


func on_discover_halflife(value:float):
	answer_2.text = str(Spring.maximum_spring_velocity_to_halflife(spin_box.value,spin_box_2.value,spin_box_3.value))
