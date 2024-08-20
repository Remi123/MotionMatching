#!/usr/bin/env python
import os


def normalize_path(val, env):
    return val if os.path.isabs(val) else os.path.join(env.Dir("#").abspath, val)


def validate_parent_dir(key, val, env):
    if not os.path.isdir(normalize_path(os.path.dirname(val), env)):
        raise UserError("'%s' is not a directory: %s" % (key, os.path.dirname(val)))


libname = "MotionMatching"
projectdir = "demo"

localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add("Boost_INCLUDE_DIR", "boost library include path", "")
opts.Add("Boost_LIBRARY_DIRS", "boost library library path", "")
opts.Add("precision","floating point precision","single")

opts.Add(
    BoolVariable(
        key="compiledb",
        help="Generate compilation DB (`compile_commands.json`) for external tools",
        default=localEnv.get("compiledb", False),
    )
)
opts.Add(
    PathVariable(
        key="compiledb_file",
        help="Path to a custom `compile_commands.json` file",
        default=localEnv.get("compiledb_file", "compile_commands.json"),
        validator=validate_parent_dir,
    )
)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()
env["compiledb"] = False

boost_path = Dir(env['Boost_INCLUDE_DIR'])


env.Tool("compilation_db")
compilation_db = env.CompilationDatabase(
    normalize_path(localEnv["compiledb_file"], localEnv)
)
env.Alias("compiledb", compilation_db)

# env.Append(gdextension_dir= ("gdextension_api_files/"))

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

# Require C++20
if env.get("is_msvc", False):
    wtmp = str(env["CXXFLAGS"])
    wtmp = wtmp.replace("/std:c++17","/std:c++20")
    env.Replace(CXXFLAGS = wtmp)
else:
    # env["CXXFLAGS"].Replace("-std=c++17","-std=c++20")
    wtmp = str(env["CXXFLAGS"])
    wtmp = wtmp.replace("-std=c++17","-std=c++20")
    env.Replace(CXXFLAGS = wtmp)

env.Append(CPPPATH=["src/",boost_path])
sources = Glob("src/*.cpp")

print(env["CXXFLAGS"])

if env["target"] in ["editor", "template_debug"]:
    doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
    sources.append(doc_data)

file = "{}{}{}".format(libname, env["suffix"], env["SHLIBSUFFIX"])

if env["platform"] == "macos" or env["platform"] == "ios":
    platlibname = "{}.{}.{}".format(libname, env["platform"], env["target"])
    file = "{}.framework/{}".format(env["platform"], platlibname, platlibname)

libraryfile = "bin/{}/{}".format(env["platform"], file)
library = env.SharedLibrary(
    libraryfile,
    source=sources,
)

copy = env.InstallAs("{}/addons/{}/bin/{}/lib{}".format(projectdir,libname, env["platform"], file), library)

default_args = [library, copy]
if localEnv.get("compiledb", False):
    default_args += [compilation_db]
Default(*default_args)
