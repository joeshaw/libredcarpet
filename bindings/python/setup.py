import os, string
from distutils.core import setup, Extension

pkgs = "glib-2.0 gobject-2.0 gmodule-2.0 libxml-2.0"
 
pc = os.popen("pkg-config --cflags-only-I %s" % pkgs, "r")
pkg_includes = map(lambda x:x[2:], string.split(pc.readline()))
pc.close()
 
pc = os.popen("pkg-config --libs-only-l %s" % pkgs, "r")
pkg_libs = map(lambda x:x[2:], string.split(pc.readline()))
pc.close()
 
pc = os.popen("pkg-config --libs-only-L %s" % pkgs, "r")
pkg_libdirs = map(lambda x:x[2:], string.split(pc.readline()))
pc.close()


other_includes = ["../../src"]
other_libs     = ["redcarpet"]
other_libdirs  = ["../../src"]
 

py_tsar_sources = [ "pyutil.c",
                    "packman.c",
                    "redcarpet.c",
                    ]

 
module_redcarpet = Extension("xxx_redcarpet",
                             sources=py_tsar_sources,
                             extra_compile_args=["-Wall", "-g"],
                             include_dirs=pkg_includes+other_includes,
                             libraries=pkg_libs+other_libs,
                             library_dirs=pkg_libdirs+other_libdirs)

setup (name="redcarpet",
       version="0.1",
       description="Red Carpet",
       ext_modules=[module_redcarpet])
