#!/usr/bin/env python3
#
# Copyright (C) 2014 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Builds a VoltAir.apk, currently in "Release" mode."""
from __future__ import print_function
import argparse
import os
import shutil
import subprocess
import sys


class RunProcessError(Exception):
  """Raised when a process returns an error code.

  Attributes:
    process_str: process run, as a string.
    return_code: Non-zero status code returned by a process.
  """

  def __init__(self, process_str, return_code):
    """Constructor for RunProcessError."""

    Exception.__init__(self)
    self.process_str = process_str
    self.return_code = return_code

  def __str__(self):
    fmtstr = "Failed subprocess (returned: %d, CWD: %s): %s"
    return fmtstr % (self.return_code, os.getcwd(), self.process_str)


def RunSubprocess(argv, rundir=None):
  """Runs a process, printing important info, such as the command.

  Args:
    argv: list of arguments to process. Zeroeth arg is process to run.
    rundir: (optional) Directory in which to run the process.

  Raises:
      RunProcessError: If the subprocess returns fails
  """

  pushed = False
  try:
    if rundir is not None:
      Pushd(rundir)
      pushed = True
    joined_argv = " ".join(argv)
    print("Running subprocess: %s" % joined_argv, file=sys.stderr)

    try:
      process = subprocess.Popen(argv)
    except OSError as exc:
      # Provide an actionable message instead of a bare traceback.
      # This is the failure seen in issue #76 ("[Errno 2] No such file or
      # directory" for "/ndk-build"): it means the NDK path argument was
      # empty or the NDK was never extracted.
      print("ERROR: failed to execute '%s': %s" % (argv[0], exc),
            file=sys.stderr)
      print("HINT: if this is '<empty>/ndk-build' or "
            "'.../android-ndk-r9/ndk-build', the NDK R9 setup likely failed. "
            "Check that travisScripts/qt-5.8-android extracted "
            "android-ndk-r9 (it extracts to 'android-ndk-r9', not "
            "'android-ndk-r9-linux-x86') and that ANDROID_NDK9_ROOT was "
            "propagated between workflow steps (see issue #76).",
            file=sys.stderr)
      raise
    process.wait()
    print("Subprocess succeeded.", file=sys.stderr)
  finally:
    if pushed:
      Popd()
    #if process.returncode:
    #  raise RunProcessError(joined_argv, process.returncode)


def SetEnvironment(sdk_root, ndk_root, ndk_platform, ndk_rootR9):
  """Set up the environment variables we need to run the various commands.

  Args:
    sdk_root: Android SDK location
    ndk_root: Android NDK location
    ndk_platform: Android platform (e.g. android-18)
  """
  os.environ["ANDROID_NDK_PLATFORM"] = ndk_platform
  os.environ["ANDROID_HOME"] = sdk_root
  os.environ["ANDROID_SDK_ROOT"] = sdk_root
  os.environ["ANDROID_NDK_ROOT"] = ndk_root
  os.environ["ANDROID_NDK_ROOT_R9"] = ndk_rootR9

_dirstack = []


def Pushd(directory):
  """Pushes current directory onto a stack and chdirs to `directory`.

  Args:
    directory: directory to chdir to.
  """
  _dirstack.append(os.getcwd())
  os.chdir(directory)


def Popd():
  """Pops the directory stack and chdirs back."""
  os.chdir(_dirstack.pop())


def RunLiquidFunNdkBuild(cwd, ndk_root_r9):
  """Builds the liquidfun module.

  Args:
    cwd: working directory in which to run build.
    ndk_root: Android NDK location
  """

  if not ndk_root_r9:
    print("ERROR: --ndk-rootR9 is empty. ANDROID_NDK9_ROOT was not "
          "propagated from the Qt setup step (see issue #76).",
          file=sys.stderr)
    sys.exit(2)
  ndk_build = ndk_root_r9 + "/ndk-build"
  if not os.path.isfile(ndk_build):
    print("ERROR: ndk-build not found at '%s'. NDK R9 setup likely failed "
          "(see issue #76). Expected travisScripts/qt-5.8-android to extract "
          "android-ndk-r9-linux-x86.tar.bz2 (which contains top-level dir "
          "'android-ndk-r9')." % ndk_build, file=sys.stderr)
    sys.exit(2)
  if not os.path.isdir(cwd):
    print("ERROR: liquidfun Box2D directory not found at '%s'. Did the "
          "liquidfun submodule get checked out (actions/checkout with "
          "submodules: recursive)?" % cwd, file=sys.stderr)
    sys.exit(2)
  RunSubprocess([ndk_build, "V=1"], cwd)


def RunQmake(qt_root, voltair_root):
  """Run the 'qmake' command.

  Args:
    qt_root: Qt framework location
    voltair_root: VoltAir project location.
  """
  qmake = [
      qt_root + "/android_armv7/bin/qmake",
      voltair_root + "/VoltAir/VoltAir.pro",
      "-r",
      "-spec",
      "android-g++"
  ]
  RunSubprocess(qmake)


def RunMake(num_processes):
  """Run the 'make' command.

  Args:
    num_processes: How many processes to use to run 'make' command
  """
  RunSubprocess(["make", "-j", num_processes])


def RunMakeInstall():
  """Run the 'make install' command."""
  RunSubprocess(["make",
                 "install", "INSTALL_ROOT=%s/android-build" % os.getcwd()])


def RunAndroidDeployQt(qt_root, voltair_root, build_dir, ant, ndk_platform,
                       jdk):
  """Run the 'androiddeployqt' command.

  Args:
    qt_root: Qt framework location
    voltair_root: VoltAir project location.
    build_dir: Directory in which app was built. Need to run command here.
    ant: And location
    ndk_platform: Android platform (e.g. android-18)
    jdk: Java Development Kit location
  """
  androiddeployqt = [
      qt_root + "/android_armv7/bin/androiddeployqt",
      "--input",
      os.getcwd() + "/android-libVoltAir.so-deployment-settings.json",
      "--output",
      os.getcwd() + "/android-build",
      "--deployment",
      "bundled",
      "--ant",
      ant,
      "--debug",
      "--android-platform",
      ndk_platform,
      "--jdk",
      jdk
  ]
  RunSubprocess(androiddeployqt, build_dir)


def RenameApk(dst_apk):
  """Run the 'mv' command which moves the APK to a more reasonable name.

  Args:
    dst_apk: name of destination apk
  """

  src_apk = os.getcwd() + "/android-build/bin/QtApp-debug-unaligned.apk"
  print("Renaming %s to %s" % (src_apk, dst_apk), file=sys.stderr)
  os.rename(src_apk, dst_apk)


def RemoveBuildDir(build_dir):
  """Remove the entire build directory.

  Args:
    build_dir: Directory in which app was built. Need to run command here.
  """

  shutil.rmtree(build_dir)


def main():
  parser = argparse.ArgumentParser(description="Build VoltAir.apk")
  parser.add_argument("--build-dir", required=True,
                      help="directory in which to build")
  parser.add_argument("--voltair-root", default=".",
                      help="Location of VoltAir source code")
  parser.add_argument("--liquidfun-root", required=True,
                      help="directory in which liquidfun is located")
  parser.add_argument("--qt-root", required=True, help="Dir for Qt (>= Qt 5.3)")
  parser.add_argument("--jdk", required=True,
                      help="Java Development Kit location")
  parser.add_argument("--sdk-root", required=True, help="Android SDK location")
  parser.add_argument("--ndk-root", required=True, help="Android NDK location")
  parser.add_argument("--ndk-rootR9", required=True, help="Android NDK Release 9 location")
  parser.add_argument("--ndk-platform", default="android-18",
                      help="Android platform (must be >= android-18)")
  parser.add_argument("--ant", required=True, help="ant binary location")
  parser.add_argument("--num-processes", default="1",
                      help="Number of processes to use for 'make'")
  parser.add_argument("--output-apk", "-o", required=True,
                      help="Destination of produced APK")

  class ParsedArgs(object):
    """Class used to collect arguments from argparse."""
    pass

  # These calls will leave the parsed_args object with one attr per option. It
  # also will exit if there were issues parsing the arguments.
  parsed_args = ParsedArgs()
  parser.parse_args(namespace=parsed_args)

  # Make sure the root paths are absolute.
  orig_cwd = os.getcwd()
  voltair_root = os.path.join(orig_cwd, parsed_args.voltair_root)
  liquidfun_root = os.path.join(orig_cwd, parsed_args.liquidfun_root)
  qt_root = os.path.join(orig_cwd, parsed_args.qt_root)

  # Qt's deployment script requires you build into subdir of voltair_root.
  build_dir = os.path.join(voltair_root, parsed_args.build_dir)

  #if os.path.exists(build_dir):
  #  err = "Error: Target directory %s exists. Exiting" % build_dir
  #  print(err, file=sys.stderr)
  #  return -1

  try:

    # Move to liquidfun directory and build it.
    print("liquidfun_root " + liquidfun_root)
    RunLiquidFunNdkBuild(liquidfun_root + "/Box2D", parsed_args.ndk_rootR9)

    # Create and move to voltair build target dir and start building.
    if not os.path.exists(build_dir):
      os.mkdir(build_dir)
    Pushd(build_dir)

    try:
      # What follows are the required steps for building the an APK in Qt.  First
      # we need to set up the environment as certain of the steps read it.
      SetEnvironment(parsed_args.sdk_root, parsed_args.ndk_root,
                     parsed_args.ndk_platform, parsed_args.ndk_rootR9)

      # 'qmake' is Qt's project builder. It will construct a Makefile that is then
      # run.
      RunQmake(qt_root, voltair_root)

      # Run the makefile, which results in the building of the libVoltAir.so file.
      RunMake(parsed_args.num_processes)

      # This places the libVoltAir.so file in the appropriate directory.
      RunMakeInstall()

      # Runs 'androiddeployqt' which constructs the APK from the libVoltAir.so and
      # the other files specified in the Qt project (e.g. java files,
      # AndroidManifest.xml, and so on).
      RunAndroidDeployQt(qt_root, voltair_root, build_dir, parsed_args.ant,
                         parsed_args.ndk_platform, parsed_args.jdk)

      # The 'androiddeployqt' executable leaves the APK in
      # QtApp-release.apk. Move it to a more descriptive location (i.e.
      # VoltAir.apk).
      # RenameApk(parsed_args.output_apk)
    finally:
      Popd()

  except RunProcessError as error:
    print(str(error), file=sys.stderr)
    return error.return_code
  finally:
    #Popd()

    # Remove the build dir in order to leave qt tree as we found it.
    #RemoveBuildDir(build_dir)
    print("")

  return 0


if __name__ == "__main__":
  sys.exit(main())
