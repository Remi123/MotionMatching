#!/usr/bin/env python
from glob import glob
from pathlib import Path
import fnmatch
import os

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)

# TODO: Do not copy environment after godot-cpp/test is updated <https://github.com/godotengine/godot-cpp/blob/master/test/SConstruct>.
env = SConscript("godot-cpp/SConstruct")
opts.Update(env)


# Add source files.
sources = Glob("src/*.cpp","src/*.hpp")
sources += Glob("thirdparty/*.cpp","thirdparty/*.hpp")


# Find gdextension path even if the directory or extension is renamed (e.g. project/addons/example/example.gdextension).
# (extension_path,) = glob("project/addons/*/*.gdextension")

# Find the addon path (e.g. project/addons/example).
addon_path = "addons/MotionMatching/"

# Find the project name from the gdextension file (e.g. example).
project_name = "MotionMatching"

# TODO: Cache is disabled currently.
# scons_cache_path = os.environ.get("SCONS_CACHE")
# if scons_cache_path != None:
#     CacheDir(scons_cache_path)
#     print("Scons cache enabled... (path: '" + scons_cache_path + "')")

# Create the library target (e.g. libexample.linux.debug.x86_64.so).
debug_or_release = ""
if env["target"] == "release" or env["target"] == "template_release":
    debug_or_release = "template_release"
else:
    debug_or_release = "template_debug"


if env["platform"] == "macos":
    library = env.SharedLibrary(
        addon_path + "bin/lib{0}.{1}.{2}.framework/{0}.{1}.{2}".format(
            project_name,
            env["platform"],
            debug_or_release,
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        addon_path + "bin/lib{}.{}.{}.{}{}".format(
            project_name,
            env["platform"],
            debug_or_release,
            env["arch"],
            env["SHLIBSUFFIX"],
        ),
        source=sources,
    )

Default(library)
