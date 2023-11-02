#!/usr/bin/env python
from glob import glob
from pathlib import Path
import fnmatch
import os


try:
    env = Environment()
    print(env.ARGUMENTS)
except:
    # Default tools with no platform defaults to gnu toolchain.
    # We apply platform specific toolchains via our custom tools.
    env = Environment(tools=["default"], PLATFORM="")

# TODO: Do not copy environment after godot-cpp/test is updated <https://github.com/godotengine/godot-cpp/blob/master/test/SConstruct>.
env = SConscript("godot-cpp/SConstruct",'env')


# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)
opts.Add("Boost_INCLUDE_DIR", "boost library include path", "")
opts.Add("Boost_LIBRARY_DIRS", "boost library library path", "")
opts.Update(env)
boost_path = Dir(env['Boost_INCLUDE_DIR'])

# Add Included files.
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
scons_cache_path = os.environ.get("SCONS_CACHE")
if scons_cache_path != None:
    CacheDir(scons_cache_path)
    print("Scons cache enabled... (path: '" + scons_cache_path + "')")


if env["platform"] == "macos":
    library = env.SharedLibrary(
        addon_path + "bin/lib{0}.{1}.{2}.framework/{0}.{1}.{2}".format(
            project_name,
            env["platform"],
            env["target"],
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        addon_path + "bin/lib{}.{}.{}.{}{}".format(
            project_name,
            env["platform"],
            env["target"],
            env["arch"],
            env["SHLIBSUFFIX"],
        ),
        source=sources,
    )

Default(library)
