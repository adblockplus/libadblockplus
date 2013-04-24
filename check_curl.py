#!/usr/bin/env python
# coding: utf-8

import os, sys, subprocess, tempfile;
baseDir = os.path.abspath(os.path.dirname(__file__))

def check_curl():
  buildDir = os.path.join(baseDir, 'build')
  if not os.path.exists(buildDir):
    os.makedirs(buildDir);
  (fd, name) = tempfile.mkstemp(dir=buildDir, suffix='.h')
  try:
    handle = os.fdopen(fd, 'wb');
    handle.write('#include <curl/curl.h>')
    handle.close();

    # This command won't work for Windows or Android build environments but we
    # don't want to use curl there anyway
    command = ['/usr/bin/env', 'gcc', '-E', name];
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    process.communicate()

    if process.returncode:
      # call failed, curl not available
      sys.stdout.write('0')
    else:
      # call succeeded, curl can be used
      sys.stdout.write('1')
  finally:
    os.remove(name)

if __name__ == '__main__':
  check_curl()
