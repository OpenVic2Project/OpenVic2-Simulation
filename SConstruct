#!/usr/bin/env python

import os
import platform
import sys

import SCons

BINDIR = "bin"

env = SConscript("scripts/SConstruct")

env.PrependENVPath("PATH", os.getenv("PATH"))

opts = env.SetupOptions()

opts.Add(BoolVariable("run_ovsim_tests", "Build and run the openvic simulation unit tests", env.is_standalone))
opts.Add(BoolVariable(key="build_ovsim_library", help="Build the openvic simulation library.", default=env.get("build_ovsim_library", not env.is_standalone)))
opts.Add(BoolVariable("build_ovsim_headless", "Build the openvic simulation headless executable", env.is_standalone))

env.FinalizeOptions()

env.exposed_includes = []

SConscript("deps/SCsub", "env")

env.openvic_simulation = {}

# For future reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Tweak this if you want to use different folders, or more folders, to store your source code in.
source_path = "src/openvic-simulation"
include_path = "src"
env.Append(CPPPATH=[[env.Dir(p) for p in [source_path, include_path]]])
sources = env.GlobRecursive("*.cpp", [source_path])
env.simulation_sources = sources

suffix = ".{}.{}".format(env["platform"], env["target"])
if env.dev_build:
    suffix += ".dev"
if env["precision"] == "double":
    suffix += ".double"
suffix += "." + env["arch"]

# Expose it when included from another project
env["suffix"] = suffix

library = None
env["OBJSUFFIX"] = suffix + env["OBJSUFFIX"]
library_name = "libopenvic-simulation{}{}".format(suffix, env["LIBSUFFIX"])

default_args = []

if env["run_ovsim_tests"]:
    env["build_ovsim_library"] = True

if env["build_ovsim_library"]:
    library = env.StaticLibrary(target=env.File(os.path.join(BINDIR, library_name)), source=sources)
    default_args += [library]

    env.Append(LIBPATH=[env.Dir(BINDIR)])
    env.Prepend(LIBS=[library_name])

    env.openvic_simulation["LIBPATH"] = env["LIBPATH"]
    env.openvic_simulation["LIBS"] = env["LIBS"]
    env.openvic_simulation["INCPATH"] = [env.Dir(include_path)] + env.exposed_includes

headless_program = None
env["PROGSUFFIX"] = suffix + env["PROGSUFFIX"]

if env["build_ovsim_headless"]:
    headless_name = "openvic-simulation"
    headless_env = env.Clone()
    headless_path = ["src/headless"]
    headless_env.Append(CPPDEFINES=["OPENVIC_SIM_HEADLESS"])
    headless_env.Append(CPPPATH=[headless_env.Dir(headless_path)])
    headless_env.headless_sources = env.GlobRecursive("*.cpp", headless_path)
    if not env["build_ovsim_library"]:
        headless_env.headless_sources += sources
    headless_program = headless_env.Program(
        target=os.path.join(BINDIR, headless_name),
        source=headless_env.headless_sources,
        PROGSUFFIX=".headless" + env["PROGSUFFIX"]
    )
    default_args += [headless_program]

if env["run_ovsim_tests"]:
    SConscript("tests/SCsub", "env")

# Add compiledb if the option is set
if env.get("compiledb", False):
    default_args += ["compiledb"]

Default(*default_args)

if "env" in locals():
    # FIXME: This method mixes both cosmetic progress stuff and cache handling...
    env.show_progress(env)

Return("env")
