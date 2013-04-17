#!/usr/bin/env python
# coding: utf-8

import sys, os, codecs, re
baseDir = os.path.abspath(os.path.dirname(__file__))
sys.path.append(os.path.join(baseDir, 'adblockplus', 'buildtools', 'jshydra'))
from abp_rewrite import doRewrite

def toCString(string):
  string = string.replace('\\', '\\\\').replace('"', '\\"')
  string = string.replace('\r', '').replace('\n', '\\n')
  # Work around MSVC line length limitation
  #(see http://msdn.microsoft.com/en-us/library/dddywwsc(v=vs.80).aspx)
  string = re.sub(r'((?:[^\\]|\\.){16000})(.)', r'\1"\n"\2', string, re.S)
  return '"%s"' % string.encode('utf-8')

def printFilesVerbatim(outHandle, files):
  for file in files:
    fileHandle = codecs.open(file, 'rb', encoding='utf-8')
    print >>outHandle, toCString(os.path.basename(file)) + ','
    print >>outHandle, toCString(fileHandle.read()) + ','
    fileHandle.close()

def convert(verbatimBefore, convertFiles, verbatimAfter, outFile):
  outHandle = open(outFile, 'wb')
  print >>outHandle, 'const char* jsSources[] = {'

  printFilesVerbatim(outHandle, verbatimBefore)

  convertFiles = map(lambda f: f if os.path.isabs(f) else os.path.join(baseDir, f), convertFiles)
  converted = doRewrite(convertFiles, ['module=true', 'source_repo=https://hg.adblockplus.org/adblockplus/'])
  print >>outHandle, toCString('adblockplus.js') + ','
  print >>outHandle, toCString(converted) + ','

  printFilesVerbatim(outHandle, verbatimAfter)

  print >>outHandle, '0, 0'
  print >>outHandle, '};'

if __name__ == '__main__':
  args = sys.argv[1:]
  outFile = args.pop()

  verbatimBefore = []
  verbatimAfter = []
  convertFiles = []
  for fileName in args:
    if fileName.startswith('before='):
      verbatimBefore.append(fileName.replace('before=', ''))
    elif fileName.startswith('after='):
      verbatimAfter.append(fileName.replace('after=', ''))
    else:
      convertFiles.append(fileName)

  convert(verbatimBefore, convertFiles, verbatimAfter, outFile)
