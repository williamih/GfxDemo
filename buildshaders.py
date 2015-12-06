#!/usr/bin/env python

import subprocess
import tempfile
import os
import os.path

TOOL_METAL = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/usr/bin/metal"
TOOL_METAL_AR = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/usr/bin/metal-ar"
TOOL_METALLIB = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/usr/bin/metallib"

SYSROOT = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk"

def run_metal(input_path, output_path, diag_path):
	args = [TOOL_METAL]
	args.append("-emit-llvm")
	args.append("-c")
	args.append("-isysroot")
	args.append(SYSROOT)
	args.append("-ffast-math")
	args.append("-serialize-diagnostics")
	args.append(diag_path)
	args.append("-o")
	args.append(output_path)
	args.append("-mmacosx-version-min=10.9")
	args.append("-std=osx-metal1.1")
	args.append(input_path)
	subprocess.check_call(args)

def run_metal_ar(input_path, output_path):
	args = [TOOL_METAL_AR, "r", output_path, input_path]
	subprocess.check_call(args)

def run_metallib(input_path, output_path):
	args = [TOOL_METALLIB, "-o", output_path, input_path]
	subprocess.check_call(args)

def compile_shader(path, output_path):
	tempdir = tempfile.mkdtemp()

	air_file_path = os.path.join(tempdir, "out.air")
	diag_file_path = os.path.join(tempdir, "diag.dia")
	metal_ar_path = os.path.join(tempdir, "out.metal-ar")

	run_metal(path, air_file_path, diag_file_path)

	os.remove(diag_file_path)

	run_metal_ar(air_file_path, metal_ar_path)
	os.remove(air_file_path)

	run_metallib(metal_ar_path, output_path)
	os.remove(metal_ar_path)

	os.rmdir(tempdir)

def main():
	compile_shader("Shaders/ModelVS.metal", "Assets/Shaders/ModelVS.metallib")
	compile_shader("Shaders/ModelPS.metal", "Assets/Shaders/ModelPS.metallib")

if __name__ == "__main__":
	main()
