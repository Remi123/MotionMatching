class_name simpleMMCharacterBody extends CharacterBody3D

@export var current_linear_halflife := 0.1
@export var current_angular_halflife := 0.1
@export var max_speed := 5.0


@onready var camera_rig: SpringArm3D = %CameraRig
@onready var animation_tree: AnimationTree = %AnimationTree

@onready var cam: Camera3D = %Camera3D
@onready var body :simpleMMCharacterBody= self

@onready var kform := Kform.new()
@onready var linear_acceleration := Vector3()

func _ready() -> void:
	animation_tree.active = true




func _physics_process(delta: float) -> void:
	var input_2d := Input.get_vector("ui_left","ui_right","ui_up","ui_down")
	var input_3d := Vector3(input_2d.x,0.0,input_2d.y)
	
	var dir := cam.global_basis.get_rotation_quaternion() 

	var desired_rotation := MMUtil.get_twist(dir,body.up_direction)
	var desired_velocity := desired_rotation * input_3d * max_speed
	
	if input_3d == Vector3.ZERO and !Input.is_action_pressed("ui_accept") :
		desired_rotation = body.quaternion
	elif Input.is_action_pressed("ui_accept"):
		desired_rotation *= Quaternion.from_euler(Vector3(0,deg_to_rad(180),0))
	else:
		desired_rotation = Quaternion(Vector3.MODEL_FRONT,desired_velocity.normalized()).normalized()
		
	var char_update := Spring.character_update(\
		global_position,velocity,linear_acceleration, \
		quaternion,kform.angular_velocity,\
		desired_velocity ,desired_rotation,\
		current_linear_halflife,current_angular_halflife,\
		delta)
		
	var next = char_update
	
	kform.angular_velocity = next.angular_velocity
	linear_acceleration = next.linear_acceleration
	velocity = next.linear_velocity

	quaternion = MMUtil.get_twist(next.angular_rotation,up_direction)
	
	velocity += -up_direction * 0.01 * delta

	body.move_and_slide()
	body.apply_floor_snap()
	
	camera_rig.global_position = global_position + Vector3(0,1.8,0)

	
	
