#!/usr/bin/env python
from glob import glob
from pathlib import Path
import fnmatch
import os

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)
# Define options
opts.Add("Boost_INCLUDE_DIR", "boost library include path", "")
opts.Add("Boost_LIBRARY_DIRS", "boost library library path", "")


# TODO: Do not copy environment after godot-cpp/test is updated <https://github.com/godotengine/godot-cpp/blob/master/test/SConstruct>.
env = SConscript("godot-cpp/SConstruct")
opts.Update(env)

# Add Included files files.

boost_path = Dir(env['Boost_INCLUDE_DIR'])
env.Append(CPPPATH=["src/","thirdparty/",boost_path])

sources = []
for root,dirnames,filenames in os.walk("./src/"):
    for filename in fnmatch.filter(filenames,"*.cpp"):
        print(os.path.join(root, filename))
        sources.append(Glob(os.path.join(root, filename)))
for root,dirnames,filenames in os.walk("./thirdparty/"):
    for filename in fnmatch.filter(filenames,"*.cpp"):
        sources.append(Glob(os.path.join(root, filename)))




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

# Exception handling, or lack-there-off
if env["platform"] == "windows":
    env.Append(CXXFLAGS=" /EHsc ") # for some reason it became not default
else :
    env.Append(CXXFLAGS=" -fexceptions ")

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
