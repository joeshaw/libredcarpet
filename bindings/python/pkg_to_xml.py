#!/usr/bin/python2.2

import sys
import os.path
import redcarpet
import xml.dom.minidom

packman = redcarpet.Packman()

failed = 0
xml_str = ""
for file in sys.argv[1:]:
    if not os.path.isfile(file):
        sys.stderr.write("File '%s' not a regular file.\n" % file)
        failed = 1
        break

    pkg = packman.query_file(file)
    if not pkg:
        sys.stderr.write("File '%s' does not appear to be a valid package.\n" % file)
        failed = 1
        break

    xml_str += pkg.to_xml()

if failed:
    sys.stderr.write("XML generation cancelled.\n")
else:
    if xml_str:
        xml_str = "<packages>" + xml_str + "</packages>"
    else:
        xml_str = "<packages/>"
    dom = xml.dom.minidom.parseString(xml_str)
    print dom.toxml()
