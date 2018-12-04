#!/usr/bin/env python
# coding: utf-8
"""This script fixes the support of manually compiled static libraries for
android NDK build system.

Issue:
Let's say there are some manually compiled libraries specified in
link_settings.libraries or in ldflags of a gyp file. In this case ndk-build
passes them to a linker after system libraries, like c++_static, and
as the result linker cannot find functions from system libraries.

Desire:
Pass manually compiled libraries after regular dependencies.

How it works:
The recommended way to do it is to use LOCAL_STATIC_LIBRARIES. However, to
simply add libraries to the list is not enough because here ndk-build is trying
to be too smart and removes any libraries which are not defined in Makefiles.

So, the idea is to monkey patch gyp to inject definition of static libraries
and add them to LOCAL_STATIC_LIBRARIES. The former is to prevent them from
being removed.

Firstly some excerpts from gyp codebase:
source: gyp/pylib/gyp/generator/make.py

MakefileWriter(object):
    def Write(self, qualified_target, base_path, output_filename, spec, configs,
            part_of_all):
    // The main entry point: writes a .mk file for a single target.
        ...
        if android:
           self.WriteAndroidNdkModuleRule(...)

    def WriteAndroidNdkModuleRule(self, module_name, all_sources, link_deps):
    /* Write a set of LOCAL_XXX definitions for Android NDK.

    These variable definitions will be used by Android NDK but do nothing for
    non-Android applications.

    Arguments:
      module_name: Android NDK module name, which must be unique among all
          module names.
      all_sources: A list of source files (will be filtered by Compilable).
      link_deps: A list of link dependencies, which must be sorted in
          the order from dependencies to dependents.
    */
        ...
        self.WriteList(
                DepsToModules(link_deps,
                              generator_default_variables['STATIC_LIB_PREFIX'],
                              generator_default_variables['STATIC_LIB_SUFFIX']),
                'LOCAL_STATIC_LIBRARIES')
        ....

    def WriteList(self, value_list, variable=None, prefix='',
                quoter=QuoteIfNecessary):
    // Write a variable definition that is a list of values.

The only class which is writing Makefiles is MakefileWriter and the "entry
point" is MakefileWriter.Write. To write LOCAL_STATIC_LIBRARIES it uses
MakefileWriter.WriteList, so one could simply override
- Write method to store all required information and to define (CLEAR_VARS,
  LOCAL_MODULE, etc) libraries.
- WriteList to change the value of LOCAL_STATIC_LIBRARIES.
However merely to reduce any potential influence we override
MakefileWriter.WriteAndroidNdkModuleRule to define manually compiled libraries
and override WriteList only while we are in the WriteAndroidNdkModuleRule method.
The aim of the latter method is exactly to write Rules for ndk-build and it is
called at the end of Write. However, since WriteAndroidNdkModuleRule lacks
required information, we override method Write to store configurations as
_abp_configs attribute of instance of MakefileWriter.
"""

import os
import platform
import sys
import types

base_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(base_dir, 'third_party', 'gyp', 'pylib'))
import gyp
from gyp.generator.make import MakefileWriter, QuoteIfNecessary


orig_MakefileWriter_Write = MakefileWriter.Write
orig_MakefileWriter_WriteAndroidNdkModuleRule = MakefileWriter.WriteAndroidNdkModuleRule

def overridden_Write(self, qualified_target, base_path, output_filename, spec, configs, part_of_all):
    if hasattr(self, "_abp_configs"):
      sys.exit("MakefileWriter already has property _abp_configs")
    self._abp_configs = configs
    orig_MakefileWriter_Write(self, qualified_target, base_path, output_filename, spec, configs, part_of_all)
    delattr(self, "_abp_configs")

def overridden_WriteAndroidNdkModuleRule(self, module_name, all_sources, link_deps):
    for config in self._abp_configs:
        libs = self._abp_configs[config].get("user_libraries")
        if not libs:
            continue
        self.WriteLn("ifeq (${{BUILDTYPE}}, {})".format(config))
        for lib in libs:
            self.WriteLn("include $(CLEAR_VARS)")
            self.WriteLn("LOCAL_MODULE := {}".format(lib))
            self.WriteLn("LOCAL_SRC_FILES := {}".format(lib))
            self.WriteLn("include $(PREBUILT_STATIC_LIBRARY)")
            self.WriteLn("ABP_STATIC_LIBRARIES_${{BUILDTYPE}} += {}".format(lib))
        self.WriteLn("endif")
    orig_WriteList = self.WriteList 
    def overridden_WriteList(self, orig_value_list, variable=None, prefix='', quoter=QuoteIfNecessary):
        value_list = orig_value_list[:]
        if variable == "LOCAL_STATIC_LIBRARIES":
            value_list.append("${ABP_STATIC_LIBRARIES_${BUILDTYPE}}")
        orig_WriteList(value_list, variable, prefix, quoter)
    self.WriteList = types.MethodType(overridden_WriteList, self)
    orig_MakefileWriter_WriteAndroidNdkModuleRule(self, module_name, all_sources, link_deps)
    self.WriteList = orig_WriteList

MakefileWriter.Write = overridden_Write
MakefileWriter.WriteAndroidNdkModuleRule = overridden_WriteAndroidNdkModuleRule

# Issue 5393,6236
# replace $(LD_INPUTS) by "-Wl,--start-group $(LD_INPUTS) -Wl,--end-group" but
# only for cmd_link_host and only on linux.
if platform.system() == "Linux":
    gyp.generator.make.LINK_COMMANDS_ANDROID = \
        gyp.generator.make.LINK_COMMANDS_ANDROID[:663] + \
        "-Wl,--start-group $(LD_INPUTS) -Wl,--end-group" + \
        gyp.generator.make.LINK_COMMANDS_ANDROID[675:]

if __name__ == '__main__':
    gyp.main(sys.argv[1:])
