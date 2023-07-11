@tool

class_name MMEditorGizmoPlugin extends EditorNode3DGizmoPlugin

class MMGizmo extends EditorNode3DGizmo:
	var gizmo_size = 3.0
	var lines := PackedVector3Array()

	func _redraw():
		clear()
		var node3d = get_node_3d()

var instance : MMGizmo = null

func _init():
	create_material("white", Color.WHITE)
	create_material("blue", Color.BLUE)
	create_material("red", Color.RED)
	create_material("green", Color.GREEN)
	create_material("orange", Color.ORANGE_RED)

	prints("MMEditorGizmoPlugin")

func _create_gizmo(node):
	if node is Skeleton3D:
		if instance == null:
			instance = MMGizmo.new()
		return instance
	else:
		return null

func _get_gizmo_name() -> String:
	return "MMGizmo"

func set_lines(lines:PackedVector3Array):
	instance.lines = lines
	instance._redraw()


func _has_gizmo(node):
	return node.name is Skeleton3D


func _redraw(gizmo : EditorNode3DGizmo):
	pass

