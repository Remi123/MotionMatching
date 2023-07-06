def can_build(env, platform):
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "MotionPlayer",
        "MotionFeature",
        "CritDampSpring",
        "PredictionMotionFeature",
        "RootVelocityMotionFeature",
        "BonePositionVelocityMotionFeature",
    ]


def get_doc_path():
    return "doc_classes"
