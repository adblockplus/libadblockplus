#!/usr/bin/env python
# coding: utf-8

import sys, os, codecs, re
baseDir = os.path.abspath(os.path.dirname(__file__))
sys.path.append(os.path.join(baseDir, 'adblockplus', 'buildtools', 'jshydra'))
from abp_rewrite import doRewrite

def toCString(string):
  string = string.replace('\\', '\\\\').replace('"', '\\"')
  string = string.replace('\r', '').replace('\n', '\\n')
  return '"%s"' % string.encode('utf-8')

def convert(convertFiles, verbatimFiles, outFile):
  outHandle = open(outFile, 'wb')
  print >>outHandle, 'const char* jsSources[] = {'

  for file in verbatimFiles:
    fileHandle = codecs.open(file, 'rb', encoding='utf-8')
    print >>outHandle, toCString(os.path.basename(file)) + ','
    print >>outHandle, toCString(fileHandle.read()) + ','
    fileHandle.close()

  convertFiles = map(lambda f: f if os.path.isabs(f) else os.path.join(baseDir, f), convertFiles)
  converted = doRewrite(convertFiles, ['module=true', 'source_repo=https://hg.adblockplus.org/adblockplus/'])
  print >>outHandle, toCString('adblockplus.js') + ','
  print >>outHandle, toCString(converted) + ','

  print >>outHandle, '0, 0'
  print >>outHandle, '};'

if __name__ == '__main__':
  args = sys.argv[1:]
  outFile = args.pop()

  verbatimFiles = []
  while len(args):
    file = args.pop()
    if file == '--':
      break
    else:
      verbatimFiles.insert(0, file)

  convertFiles = args
  convert(convertFiles, verbatimFiles, outFile)
