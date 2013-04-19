#!/usr/bin/env python
# coding: utf-8

import sys, os, codecs, re, json
import xml.dom.minidom as minidom
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

def convertXMLFile(outHandle, file):
    fileHandle = codecs.open(file, 'rb', encoding='utf-8')
    doc = minidom.parse(file)
    fileHandle.close()

    data = []
    for node in doc.documentElement.childNodes:
      if node.nodeType != node.ELEMENT_NODE:
        continue
      result = {'type': node.tagName}
      for name, value in node.attributes.items():
        result[name] = value
      data.append(result)
    fileNameString = toCString(os.path.basename(file))
    print >>outHandle, fileNameString + ','
    print >>outHandle, toCString('require.scopes[%s] = %s;' % (fileNameString, json.dumps(data))) + ','
    fileHandle.close()

def convertJsFile(outHandle, file):
  converted = doRewrite([os.path.abspath(file)], ['module=true', 'source_repo=https://hg.adblockplus.org/adblockplus/'])
  print >>outHandle, toCString(os.path.basename(file)) + ','
  print >>outHandle, toCString(converted) + ','

def convert(verbatimBefore, convertFiles, verbatimAfter, outFile):
  outHandle = open(outFile, 'wb')
  print >>outHandle, 'const char* jsSources[] = {'

  printFilesVerbatim(outHandle, verbatimBefore)

  for file in convertFiles:
    if file.endswith('.xml'):
      convertXMLFile(outHandle, file)
    else:
      convertJsFile(outHandle, file)

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
