#!/usr/bin/env python
# coding: utf-8

import os, sys
base_dir = os.path.abspath(os.path.dirname(__file__))

sys.path.append(os.path.join(base_dir, 'third_party', 'gyp', 'pylib'))
import gyp
import gyp.generator.msvs

orig_fix_path = gyp.generator.msvs._FixPath

def _FixPath(path):
  if path == 'CORE' or path == 'EXPERIMENTAL' or path == 'off':
    # Don't touch js2c parameters
    return path
  return orig_fix_path(path)
gyp.generator.msvs._FixPath = _FixPath

if __name__ == '__main__':
  gyp.main(sys.argv[1:])
