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
other_libs     = ["redcarpet", "bz2"]
other_libdirs  = ["../../src"]
 

py_redcarpet_sources = [ "pyutil.c",
                         "distro.c",
                         "verification.c",
                         "package-importance.c",
                         "package-spec.c",
                         "package-dep.c",
                         "package-match.c",
                         "package-update.c",
                         "package.c",
                         "package-file.c",
                         "packman.c",
                         "channel.c",
                         "world.c",
                         "world-store.c",
                         "resolver-info.c",
                         "resolver-context.c",
                         "resolver-queue.c",
                         "resolver.c",
                         "redcarpet.c",
                         ]

 
module_redcarpet = Extension("redcarpet",
                             sources=py_redcarpet_sources,
                             extra_compile_args=["-Wall", "-g"],
                             include_dirs=pkg_includes+other_includes,
                             libraries=pkg_libs+other_libs,
                             library_dirs=pkg_libdirs+other_libdirs)

setup (name="xxx_redcarpet",
       version="0.1",
       description="Red Carpet",
       ext_modules=[module_redcarpet])
