#!/bin/sh
# the next line restarts using tclsh \
    exec tclsh "$0" "$@"

################################################################################
#
# genlib.tcl
#
# generate the Lib directory with the needed components for slicer
# to build
#
# Steps:
# - pull code from anonymous cvs
# - configure (or cmake) with needed options
# - build for this platform
#
# Packages: cmake, tcl, itcl, iwidgets, blt, ITK, VTK, teem
# 
# Usage:
#   genlib [options] [target]
#
# run genlib from the Slicer3 directory next to where you want the packages to be built
# E.g. if you run /home/pieper/Slicer3/Scripts/genlib.tcl it will create
# /home/pieper/Slicer3-lib
#
# - sp - 2004-06-20
#


if {[info exists ::env(CVS)]} {
    set ::CVS "{$::env(CVS)}"
} else {
    set ::CVS cvs
}

# for subversion repositories (Sandbox)
if {[info exists ::env(SVN)]} {
    set ::SVN $::env(SVN)
} else {
    set ::SVN svn
}

if {[info exists ::env(GIT)]} {
    set ::GIT $::env(GIT)
} else {
    set ::GIT git 
}

foreach tool {::CVS ::SVN ::GIT} {
  if { [catch "exec [set $tool] --version"] } {
    error "Cannot execute $tool - please install it before building"
  }
}


################################################################################
#
# check to see if need to build a package
# returns 1 if need to build, 0 else
# if { [BuildThis  ""] == 1 } {
proc BuildThis { testFile packageName } {
    if {![file exists $testFile] || $::GENLIB(update) || [lsearch $::GENLIB(buildList) $packageName] != -1} {
        # puts "Building $packageName (testFile = $testFile, update = $::GENLIB(update), buildlist = $::GENLIB(buildList) )"
        return 1
    } else {
        # puts "Skipping $packageName"
        return 0
    }
}
#

################################################################################
#
# simple command line argument parsing
#

proc Usage { {msg ""} } {
    global SLICER
    
    set msg "$msg\nusage: genlib \[options\] \[target\]"
    set msg "$msg\n  \[target\] is the the Slicer3_LIB directory"
    set msg "$msg\n             and is determined automatically if not specified"
    set msg "$msg\n  \[options\] is one of the following:"
    set msg "$msg\n   --help : prints this message and exits"
    set msg "$msg\n   --clean : delete the target first"
    set msg "$msg\n   --update : do a cvs update even if there's an existing build"
    set msg "$msg\n   --release : compile with optimization flags"
    set msg "$msg\n   --relwithdebinfo : compile with optimization flags and debugging symbols"
    set msg "$msg\n   --nobuild : only download and/or update, but don't build"
    set msg "$msg\n   --test-type : type of ctest to run (for enabled packages)"
    set msg "$msg\n   optional space separated list of packages to build (lower case)"
    set msg "$msg\n   -32 : does a 32 bit build of Slicer and all the libs (Default: isainfo -b on Solaris, 32 bit on other OS'es)"
    set msg "$msg\n   -64 : does a 64 bit build of Slicer and all the libs"
    set msg "$msg\n   --suncc : builds Slicer with Sun's compilers (The default is gcc)"
    set msg "$msg\n   --gcc : builds Slicer with GNU compilers"
    puts stderr $msg
}

set GENLIB(clean) "false"
set GENLIB(update) "false"
set GENLIB(buildit) "true"
set GENLIB(test-type) ""
set ::GENLIB(buildList) ""
if {$tcl_platform(os) == "SunOS"} {
  set isainfo [exec isainfo -b]
  set ::GENLIB(bitness) "-$isainfo"
} else {
  set GENLIB(bitness) "32"
}
set GENLIB(compiler) "gcc"

set isRelease 0
set isRelWithDebInfo 0
set strippedargs ""
set argc [llength $argv]
for {set i 0} {$i < $argc} {incr i} {
    set a [lindex $argv $i]
    switch -glob -- $a {
        "--clean" -
        "-f" {
            set GENLIB(clean) "true"
        }
        "--update" -
        "-u" {
            set GENLIB(update) "true"
        }
        "--release" {
            set isRelease 1
            set ::VTK_BUILD_TYPE "Release"
        }
        "--relwithdebinfo" {
            set isRelWithDebInfo 1
            set ::VTK_BUILD_TYPE "ReleaseWithDebInfo"
        }
        "--nobuild" {
            set ::GENLIB(buildit) "false"
        }
        "-t" -
        "--test-type" {
            incr i
            if { $i == $argc } {
                Usage "Missing test-type argument"
            } else {
                set ::GENLIB(test-type) [lindex $argv $i]
            }
        }
        "-64" {
            set ::GENLIB(bitness) "64"
            set ::env(BITNESS) $::GENLIB(bitness)
        }
        "-32" {
            set ::GENLIB(bitness) "32"
            set ::env(BITNESS) $::GENLIB(bitness)
        }
        "--suncc" {
            set ::GENLIB(compiler) "suncc"
        }
        "--gcc" {
            set ::GENLIB(compiler) "gcc"
        }
        "--help" -
        "-h" {
            Usage
            exit 1
        }
        "-*" {
            Usage "unknown option $a\n"
            exit 1
        }
        default {
            lappend strippedargs $a 
        }
    }
}

set argv $strippedargs
set argc [llength $argv]

set ::Slicer3_LIB ""
if {$argc > 1 } {
    # the stripped args list now has the Slicer3_LIB first and then the list of packages to build
    set ::GENLIB(buildList) [lrange $strippedargs 1 end]
    set strippedargs [lindex $strippedargs 0]
# puts "Got the list of package to build: '$::GENLIB(buildList)' , stripped args = $strippedargs"
} 
set ::Slicer3_LIB $strippedargs


################################################################################
#
# Utilities:

proc runcmd {args} {
    global isWindows
    puts "running: $args"

    # print the results line by line to provide feedback during long builds
    # interleaves the results of stdout and stderr, except on Windows
    if { $isWindows } {
        # Windows does not provide native support for cat
        set fp [open "| $args" "r"]
    } else {
        set fp [open "| $args |& cat" "r"]
    }
    while { ![eof $fp] } {
        gets $fp line
        puts $line
    }
    set ret [catch "close $fp" res] 
    if { $ret } {
        puts stderr $res
        if { $isWindows } {
            # Does not work on Windows
        } else {
            error $ret
        }
    } 
}

# helper proc to edit a file in place (cross platform)
proc replaceStringInFile {fileName oldString newString} {
  set fp [open $fileName "r"]
  set contents [read $fp]
  close $fp
  regsub -all $oldString $contents $newString contents
  set fp [open $fileName "w"]
  puts $fp $contents
  close $fp
}

################################################################################
# First, set up the directory
# - determine the location
# - determine the build
# 

# hack to work around lack of normalize option in older tcl
# set Slicer3_HOME [file dirname [file dirname [file normalize [info script]]]]
set cwd [pwd]
cd [file dirname [info script]]
cd ..
set Slicer3_HOME [pwd]
cd $cwd

if { $::Slicer3_LIB == "" } {
  set ::Slicer3_LIB [file dirname $::Slicer3_HOME]/Slicer3-lib
  puts "Slicer3_LIB is $::Slicer3_LIB"
}

#######
#
# Note: the local vars file, slicer2/slicer_variables.tcl, overrides the default values in this script
# - use it to set your local environment and then your change won't 
#   be overwritten when this file is updated
#
set localvarsfile $Slicer3_HOME/slicer_variables.tcl
catch {set localvarsfile [file normalize $localvarsfile]}
if { [file exists $localvarsfile] } {
    puts "Sourcing $localvarsfile"
    source $localvarsfile
} else {
    puts "stderr: $localvarsfile not found - use this file to set up your build"
    exit 1
}

#initialize platform variables
switch $tcl_platform(os) {
    "SunOS" {
        set isSolaris 1
        set isWindows 0
        set isDarwin 0
        set isLinux 0
    }
    "GNU/kFreeBSD" {
        set isSolaris 0
        set isWindows 0
        set isDarwin 0
        set isLinux 1
    }
    "Linux" { 
        set isSolaris 0
        set isWindows 0
        set isDarwin 0
        set isLinux 1
    }
    "Darwin" { 
        set isSolaris 0
        set isWindows 0
        set isDarwin 1
        set isLinux 0
    }
    default { 
        set isSolaris 0
        set isWindows 1
        set isDarwin 0
        set isLinux 0
    }
}

set ::VTK_DEBUG_LEAKS "ON"
if ($isRelease) {
    set ::VTK_BUILD_TYPE "Release"
    set ::env(VTK_BUILD_TYPE) $::VTK_BUILD_TYPE
    if ($isWindows) {
        set ::VTK_BUILD_SUBDIR "Release"
    } else {
        set ::VTK_BUILD_SUBDIR ""
    }
    puts "Overriding slicer_variables.tcl; VTK_BUILD_TYPE is '$::env(VTK_BUILD_TYPE)', VTK_BUILD_SUBDIR is '$::VTK_BUILD_SUBDIR'"
    set ::VTK_DEBUG_LEAKS "OFF"
}

if ($isRelWithDebInfo) {
    set ::VTK_BUILD_TYPE "RelWithDebInfo"
    set ::env(VTK_BUILD_TYPE) $::VTK_BUILD_TYPE
    if ($isWindows) {
        set ::VTK_BUILD_SUBDIR "RelWithDebInfo"
    } else {
        set ::VTK_BUILD_SUBDIR ""
    }
    puts "Overriding slicer_variables.tcl; VTK_BUILD_TYPE is '$::env(VTK_BUILD_TYPE)', VTK_BUILD_SUBDIR is '$::VTK_BUILD_SUBDIR'"
    set ::VTK_DEBUG_LEAKS "OFF"
}

# tcl file delete is broken on Darwin, so use rm -rf instead
if { $::GENLIB(clean) } {
    puts "Deleting slicer lib files..."
    if { $isDarwin } {
        runcmd rm -rf $Slicer3_LIB
        if { [file exists $Slicer3_LIB/tcl/isPatched] } {
            runcmd rm $Slicer3_LIB/tcl/isPatched
        }

    } else {
        file delete -force $Slicer3_LIB
    }
}

if { ![file exists $Slicer3_LIB] } {
    file mkdir $Slicer3_LIB
}


################################################################################
# If is Darwin, don't use cvs compression to get around bug in cvs 1.12.13
#

if {$isDarwin} {
    set CVS_CO_FLAGS "-q"  
} else {
    set CVS_CO_FLAGS "-q -z3"    
}


################################################################################
# Get and build CMake
#

# set in slicer_vars
if { [BuildThis $::CMAKE "cmake"] == 1 } {
    file mkdir $::CMAKE_PATH
    cd $Slicer3_LIB


    if {$isWindows} {
      runcmd $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/Binaries/Windows/CMake-build CMake-build
    } else {
        if { [info exists ::CMAKE_GIT_REPO] } {
            if { ![file exists CMake] } {
              eval "runcmd $::GIT clone $::CMAKE_GIT_REPO CMake"
            }
            cd $::Slicer3_LIB/CMake
            eval "runcmd $::GIT checkout $::CMAKE_GIT_BRANCH"
            eval "runcmd $::GIT pull"
            eval "runcmd $::GIT checkout $::CMAKE_GIT_TAG"
            cd $::Slicer3_LIB
        } else {
            runcmd $::CVS -d :pserver:anonymous:cmake@www.cmake.org:/cvsroot/CMake login
            eval "runcmd $::CVS $CVS_CO_FLAGS -d :pserver:anonymous@www.cmake.org:/cvsroot/CMake checkout -r $::CMAKE_TAG CMake"
        }

        if {$::GENLIB(buildit)} {
          cd $::CMAKE_PATH
          if { $isDarwin } {
            # for mac, create a dummy cache to work around bad 
            # jni.h file - see http://cmake.org/Bug/view.php?id=11357
            set fp [open "initialCache.cmake" "w"]
            puts $fp "SET(JNI_H FALSE CACHE BOOL \"\" FORCE)"
            puts $fp "SET(Java_JAVA_EXECUTABLE FALSE CACHE BOOL \"\" FORCE)"
            puts $fp "SET(Java_JAVAC_EXECUTABLE FALSE CACHE BOOL \"\" FORCE)"
            close $fp
            runcmd $Slicer3_LIB/CMake/configure --init=initialCache.cmake
          } else {
            runcmd $Slicer3_LIB/CMake/configure
          }

          eval runcmd $::MAKE
       }
    }
}


################################################################################
# Get and build tcl, tk, itcl, widgets
#
#

# on windows, tcl won't build right, as can't configure, so save commands have to run
if { [BuildThis $::TCL_TEST_FILE "tcl"] == 1 } {

    if {$isWindows} {
      cd $::Slicer3_LIB
      if {$::GENLIB(bitness) == "64"} {
        runcmd $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/Binaries/Windows/$::TCL_VERSION-x64-build tcl-build
      } else {
        runcmd $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/Binaries/Windows/$::TCL_VERSION-build tcl-build
      }
    } else {
      file mkdir $Slicer3_LIB/tcl
      cd $Slicer3_LIB/tcl
      runcmd $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/$::TCL_VERSION/tcl tcl
    }
    

    if {$::GENLIB(buildit)} {
      if {$isWindows} {
          # can't do windows
      } else {
        cd $Slicer3_LIB/tcl/tcl/unix

        if {$tcl_platform(os) == "SunOS" && $tcl_platform(osVersion) == "5.10"} {
          replaceStringInFile tcl.m4 "SunOS-5.1\[\[1-9\]\]*|SunOS-5.\[\[2-9\]\]\[\[0-9\]\]" "SunOS-5.1\[\[0-9\]\]*|SunOS-5.\[\[2-9\]\]\[\[0-9\]\]"
          replaceStringInFile configure "SunOS-5.1\[1-9\]*|SunOS-5.\[2-9\]\[0-9\]" "SunOS-5.1\[0-9\]*|SunOS-5.\[2-9\]\[0-9\]"

        }
        if {$::GENLIB(bitness) == "64"} {
          runcmd ./configure --enable-64bit --prefix=$Slicer3_LIB/tcl-build
        } else {
          runcmd ./configure --prefix=$Slicer3_LIB/tcl-build
        }

        eval runcmd $::MAKE
        eval runcmd $::MAKE install
      }
    }
}

if { [BuildThis $::TK_TEST_FILE "tk"] == 1 } {

    if {$::GENLIB(buildit)} {
      if {$isWindows} {
         # ignore, already downloaded with tcl
      } else {
        cd $Slicer3_LIB/tcl

        runcmd $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/$::TCL_VERSION/tk tk
         
        cd $Slicer3_LIB/tcl/tk/unix

        if {$tcl_platform(os) == "SunOS" && $tcl_platform(osVersion) == "5.10"} {
          replaceStringInFile tcl.m4 "SunOS-5.1\[\[1-9\]\]*|SunOS-5.\[\[2-9\]\]\[\[0-9\]\]" "SunOS-5.1\[\[0-9\]\]*|SunOS-5.\[\[2-9\]\]\[\[0-9\]\]"
          replaceStringInFile configure "SunOS-5.1\[1-9\]*|SunOS-5.\[2-9\]\[0-9\]" "SunOS-5.1\[0-9\]*|SunOS-5.\[2-9\]\[0-9\]"

        }

        if { $isDarwin } {
          runcmd ./configure --with-tcl=$Slicer3_LIB/tcl-build/lib --prefix=$Slicer3_LIB/tcl-build --disable-corefoundation --x-libraries=/usr/X11R6/lib --x-includes=/usr/X11R6/include --with-x
        } else {
          if {$::GENLIB(bitness) =="64"} {
            runcmd ./configure --with-tcl=$Slicer3_LIB/tcl-build/lib --enable-64bit --prefix=$Slicer3_LIB/tcl-build
          } else {
            runcmd ./configure --with-tcl=$Slicer3_LIB/tcl-build/lib --prefix=$Slicer3_LIB/tcl-build
          }
        } 
        eval runcmd $::MAKE
        eval runcmd $::MAKE install
         
        file copy -force $Slicer3_LIB/tcl/tk/generic/default.h $Slicer3_LIB/tcl-build/include
        file copy -force $Slicer3_LIB/tcl/tk/unix/tkUnixDefault.h $Slicer3_LIB/tcl-build/include
      }
   }
}

if { [BuildThis $::ITCL_TEST_FILE "itcl"] == 1 } {

    if {$::GENLIB(buildit)} {
      if {$isWindows} {
         # ignore, already downloaded with tcl
      } else {

        cd $Slicer3_LIB/tcl

        runcmd $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/$::TCL_VERSION/incrTcl incrTcl

        cd $Slicer3_LIB/tcl/incrTcl

        exec chmod +x ../incrTcl/configure 

        set extraArgs ""
        if { $isDarwin } {
          exec cp ../incrTcl/itcl/configure ../incrTcl/itcl/configure.orig
          exec sed -e "s/\\*\\.c | \\*\\.o | \\*\\.obj) ;;/\\*\\.c | \\*\\.o | \\*\\.obj | \\*\\.dSYM | \\*\\.gnoc ) ;;/" ../incrTcl/itcl/configure.orig > ../incrTcl/itcl/configure 

          set extraArgs "--x-includes=/usr/X11R6/include --x-libraries=/usr/X11R6/lib --with-cflags=-fno-common"
      }
      if {$::GENLIB(bitness) == "64"} {
        set ::env(CC) "$::GENLIB(compiler) -m64"
        puts "genlib incrTcl 64 bit branch: $::env(CC)"
        eval runcmd ../incrTcl/configure --with-tcl=$Slicer3_LIB/tcl-build/lib --with-tk=$Slicer3_LIB/tcl-build/lib --prefix=$Slicer3_LIB/tcl-build $extraArgs
      } else {
        set ::env(CC) "$::GENLIB(compiler)"
        puts "genlib incrTcl 32 bit branch: $::env(CC)"
        eval runcmd ../incrTcl/configure --with-tcl=$Slicer3_LIB/tcl-build/lib --with-tk=$Slicer3_LIB/tcl-build/lib --prefix=$Slicer3_LIB/tcl-build $extraArgs
      }

      if { $isDarwin } {
        # need to run ranlib separately on lib for Darwin
        # file is created and ranlib is needed inside make all
        catch "eval runcmd $::MAKE all"
        if { [file exists ../incrTcl/itcl/libitclstub3.2.a] } {
          runcmd ranlib ../incrTcl/itcl/libitclstub3.2.a
        }
      }

      eval runcmd $::MAKE all
      eval runcmd $::SERIAL_MAKE install
    }
  }
}

################################################################################
# Get and build iwidgets
#

if { [BuildThis $::IWIDGETS_TEST_FILE "iwidgets"] == 1 } {

    if {$::GENLIB(buildit)} {
        if {$isWindows} {
            # is present in the windows binary download
        } else {

            cd $Slicer3_LIB/tcl
            runcmd  $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/$::TCL_VERSION/iwidgets iwidgets

            cd $Slicer3_LIB/tcl/iwidgets
            runcmd ../iwidgets/configure --with-tcl=$Slicer3_LIB/tcl-build/lib --with-tk=$Slicer3_LIB/tcl-build/lib --with-itcl=$Slicer3_LIB/tcl/incrTcl --prefix=$Slicer3_LIB/tcl-build
            # make all doesn't do anything...
            # iwidgets won't compile in parallel (with -j flag)
            eval runcmd $::SERIAL_MAKE all
            eval runcmd $::SERIAL_MAKE install
        }
    }
}

################################################################################
# Get and build blt
#

if { 0 && [BuildThis $::BLT_TEST_FILE "blt"] == 1 } {

  if {$::GENLIB(buildit)} {

    if { $isWindows } { 
      # is present in the windows binary download
    } else {
      cd $Slicer3_LIB/tcl
      runcmd  $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/$::TCL_VERSION/blt blt

      if { $isDarwin } {

        if { ![file exists $Slicer3_LIB/tcl/isPatchedBLT] } { 
          puts "Patching..." 
          runcmd curl -k -O https://share.spl.harvard.edu/share/birn/public/software/External/Patches/bltpatch 
          cd $Slicer3_LIB/tcl/blt 
          runcmd patch -p2 < ../bltpatch 

          # create a file to make sure BLT isn't patched twice 
          runcmd touch $Slicer3_LIB/tcl/isPatchedBLT 
          file delete $Slicer3_LIB/tcl/bltpatch 
        } else { 
          puts "BLT already patched." 
        }
        cd $Slicer3_LIB/tcl/blt
        runcmd ./configure --with-tcl=$Slicer3_LIB/tcl/tcl/unix --with-tk=$Slicer3_LIB/tcl-build --prefix=$Slicer3_LIB/tcl-build --enable-shared --x-includes=/usr/X11R6/include --x-libraries=/usr/X11R6/lib --with-cflags=-fno-common
        eval runcmd $::MAKE
        catch "eval runcmd $::MAKE install" ;# install fails at end, so catch so build doesn't fail

      } elseif { $isSolaris } {

        cd $Slicer3_LIB/tcl/blt

        # On Solaris 10 - due to bug http://bugs.opensolaris.org/bugdatabase/view_bug.do?bug_id=6223255 - we need to set some -L and -R paths.
        # I does not affect later Solaris releases.
        set EXTRAS10LIBS ""
        set MYSQLDIR ""             
        if {$::GENLIB(bitness) == "64"} {
          set ::env(CC) "$::GENLIB(compiler) -m64"
          set ::env(LDFLAGS) "-m64 -L/usr/sfw/lib/64 -R/usr/sfw/lib/64"
          puts "genlib blt 64 bit branch: $::env(CC)"
          if {$tcl_platform(osVersion) == "5.10"} {
            replaceStringInFile src/Makefile.in "@XFT_LIB_SPEC@" "@EXPAT_LIB_SPEC@ @XFT_LIB_SPEC@"
            set EXTRAS10LIBS "--with-freetype2libdir=/usr/sfw/lib/64 --with-expatlibdir=/usr/sfw/lib/64"
            set MYSQLDIR "--without-mysqlincdir --without-mysqllibdir"
            puts "ExtraS10Libs_64 are: $EXTRAS10LIBS"
          } else {
            set MYSQLDIR "--with-mysqlincdir=/usr/mysql/5.1/include --with-mysqllibdir=/usr/mysql/5.1/lib/64/mysql"
          } 
        } else {
          set ::env(CC) "$::GENLIB(compiler)"
          puts "genlib blt 32 bit branch: $::env(CC)"
          if {$tcl_platform(osVersion) == "5.10"} {
            replaceStringInFile src/Makefile.in "@XFT_LIB_SPEC@" "@EXPAT_LIB_SPEC@ @XFT_LIB_SPEC@"
            set EXTRAS10LIBS "--with-freetype2libdir=/usr/sfw/lib --with-expatlibdir=/usr/sfw/lib"
            set MYSQLDIR "--without-mysqlincdir --without-mysqllibdir"
            puts "ExtraS10Libs_32 are: $EXTRAS10LIBS"
          } else {
            set MYSQLDIR "--with-mysqlincdir=/usr/mysql/5.1/include --with-mysqllibdir=/usr/mysql/5.1/lib/mysql"
          }
        }

        eval runcmd ./configure --with-tcl=$Slicer3_LIB/tcl/tcl/unix --with-tk=$Slicer3_LIB/tcl-build --prefix=$Slicer3_LIB/tcl-build --enable-shared $EXTRAS10LIBS $MYSQLDIR
        eval runcmd $::SERIAL_MAKE
        eval runcmd $::SERIAL_MAKE install

      } else {
        cd $Slicer3_LIB/tcl/blt

        if {$::GENLIB(bitness) == "64"} {
          runcmd ./configure --with-tcl=$Slicer3_LIB/tcl/tcl/unix --with-tk=$Slicer3_LIB/tcl-build --prefix=$Slicer3_LIB/tcl-build --enable-64bit --with-cflags=-fPIC
        } else {
          runcmd ./configure --with-tcl=$Slicer3_LIB/tcl/tcl/unix --with-tk=$Slicer3_LIB/tcl-build --prefix=$Slicer3_LIB/tcl-build
        }
        eval runcmd $::SERIAL_MAKE
        eval runcmd $::SERIAL_MAKE install
      }
    }
  }
}

################################################################################
# Get and build python
#

if {  [BuildThis $::PYTHON_TEST_FILE "python"] && !$::USE_SYSTEM_PYTHON && [string tolower $::USE_PYTHON] == "on" } {
    if { $isWindows } {

      file mkdir $::Slicer3_LIB/python
      cd $::Slicer3_LIB

      runcmd $::SVN co $::PYTHON_TAG -r $::PYTHON_REVISION python-build
      cd $Slicer3_LIB/python-build

      # point the tkinter build file to the slicer tcl-build 
      # for python 2.5
      #replaceStringInFile "PCbuild/_tkinter.vcproj" "tcltk" "tcl-build"
      # for python 2.6 - replace properties file with paths to slicer's tcl-build for 8.4
      file copy -force $::Slicer3_HOME/Base/GUI/Python/slicer.pyproject.vsprops PCbuild/pyproject.vsprops

      if { $::GENERATOR != "Visual Studio 7 .NET 2003" } {
         if {[file tail $::MAKE] != "VCExpress.exe"} {
           runcmd $::MAKE PCbuild/pcbuild.sln /Upgrade
         } 
      }

      runcmd $::MAKE PCbuild/pcbuild.sln /out buildlog.txt /build $::PYTHON_CONFIG

      # fix distutils to ignore it's hardcoded python version info
      replaceStringInFile Lib/distutils/msvccompiler.py "raise DistutilsPlatformError," "print"

      
      # copy the lib so that numpy and slicer can find it easily
      # copy the socket shared library so python can find it
      # TODO: perhaps we need an installer step here

      set suffix ""
      if { $::PYTHON_BUILD_TYPE == "Debug" } {
        # python adds _d to debug versions, but we want to treat them all the same 
        # for slicer purposes
        set suffix "_d"
        file copy -force $::PYTHON_BUILD_DIR/python$suffix.exe $PYTHON_BUILD_DIR/python.exe
        file copy -force $::PYTHON_BUILD_DIR/python26$suffix.lib $PYTHON_BUILD_DIR/python26.lib
      }
      
      set ret [catch "file copy -force $::PYTHON_BUILD_DIR/python26$suffix.lib $::Slicer3_LIB/python-build/Lib/python26.lib "]
      if {$ret == 1} {
          puts "ERROR: couldn't copy $::PYTHON_BUILD_DIR/python26$suffix.lib to $::Slicer3_LIB/python-build/Lib/"
          exit 1
      }
      set ret [catch "file copy -force $::PYTHON_BUILD_DIR/_socket$suffix.pyd $::Slicer3_LIB/python-build/Lib/_socket.pyd"]
      if {$ret == 1} {
         puts "ERROR: failed to copy $::PYTHON_BUILD_DIR/_socket$suffix.pyd to $::Slicer3_LIB/python-build/Lib/_socket.pyd"
         exit 1
       }
      set ret [catch "file copy -force $::PYTHON_BUILD_DIR/_ctypes$suffix.pyd $::Slicer3_LIB/python-build/Lib/_ctypes.pyd"]
      if {$ret == 1} {
        puts "ERROR: failed to copy $::PYTHON_BUILD_DIR/_ctypes$suffix.pyd to $::Slicer3_LIB/python-build/Lib/_ctypes.pyd"
        exit 1
      }

    } else {

      file mkdir $::Slicer3_LIB/python
      file mkdir $::Slicer3_LIB/python-build
      cd $::Slicer3_LIB

      cd $Slicer3_LIB/python
      runcmd $::SVN co $::PYTHON_TAG -r $::PYTHON_REVISION
      cd $Slicer3_LIB/python/Python-2.6.6
      foreach flag {LD_LIBRARY_PATH LDFLAGS CPPFLAGS} {
        if { ![info exists ::env($flag)] } { set ::env($flag) "" }
      }
      set ::env(LDFLAGS) "$::env(LDFLAGS) -L$Slicer3_LIB/tcl-build/lib"
      set ::env(CPPFLAGS) "$::env(CPPFLAGS) -I$Slicer3_LIB/tcl-build/include"
      set ::env(LD_LIBRARY_PATH) $Slicer3_LIB/tcl-build/lib:$Slicer3_LIB/python-build/lib:$::env(LD_LIBRARY_PATH)
      if { $isSolaris } {
          if {$::GENLIB(bitness) == "64"} {
              set ::env(CC) "$::GENLIB(compiler) -m64"
              set ::env(LDFLAGS)  "$::env(LDFLAGS) -L/usr/sfw/lib/64"
          } elseif {$::GENLIB(bitness) == "32"} {
              set ::env(CC) "$::GENLIB(compiler)"
              set ::env(LDFLAGS)  "$::env(LDFLAGS) -L/usr/sfw/lib"
          }
      }
      runcmd ./configure --prefix=$Slicer3_LIB/python-build --with-tcl=$Slicer3_LIB/tcl-build --enable-shared
      eval runcmd $::MAKE
      eval runcmd $::SERIAL_MAKE install
        
      if { $isDarwin } {
            # Special Slicer hack to build and install the .dylib
            file mkdir $::Slicer3_LIB/python-build/lib/
            file delete -force $::Slicer3_LIB/python-build/lib/libpython2.6.dylib
            set fid [open environhack.c w]
            puts $fid "char **environ=0;"
            close $fid
            runcmd gcc -c -o environhack.o environhack.c
            runcmd libtool -o $::Slicer3_LIB/python-build/lib/libpython2.6.dylib -dynamic  \
                -all_load libpython2.6.a environhack.o -single_module \
                -install_name $::Slicer3_LIB/python-build/lib/libpython2.6.dylib \
                -compatibility_version 2.6 \
                -current_version 2.6 -lSystem -lSystemStubs

        }
    }
}

################################################################################
# Get and build netlib (blas and lapack)
#

if { [BuildThis $::NETLIB_TEST_FILE "netlib"] && !$::USE_SYSTEM_PYTHON && [string tolower $::USE_SCIPY] == "on" } {

    file mkdir $::Slicer3_LIB/netlib
    file mkdir $::Slicer3_LIB/netlib-build
    file mkdir $::Slicer3_LIB/netlib-build/BLAS-build
    cd $::Slicer3_LIB/netlib

    # do blas

    runcmd $::SVN co $::BLAS_TAG BLAS

    if { $isWindows } {
        # build c lapack
    } else {

        cd $::Slicer3_LIB/netlib-build/BLAS-build
        set files [glob $::Slicer3_LIB/netlib/BLAS/*.f]
        foreach f $files {
              if { $isLinux && $::tcl_platform(machine) == "x86_64" } {
                runcmd $::FORTRAN_COMPILER -O3 -fno-second-underscore -fPIC -m64 -c $f
              } else {
                runcmd $::FORTRAN_COMPILER -fno-second-underscore -O2 -c $f
              }
          set ofile [file root [file tail $f]].o        
          runcmd ar r libblas.a $ofile
        }
        runcmd ranlib libblas.a
    }

    # do lapack

    file mkdir $::Slicer3_LIB/netlib-build/lapack-build
    cd $::Slicer3_LIB/netlib
    runcmd $::SVN co $::LAPACK_TAG lapack

    if { $isWindows } {
        # windows binary already checked out
    } else {

        set utilDir $::Slicer3_LIB/../Slicer3/Base/GUI/Python/util
        cd $::Slicer3_LIB/netlib/lapack
        if { $isDarwin } {
          set platform DARWIN
        } elseif { $isLinux && $::tcl_platform(machine) == "x86_64" } {
          set platform LINUX64
        } else {
          set platform LINUX
        }
        # TODO: these have hardcoded gfortran (not controlled by ::FORTRAN_COMPILER)
        file copy -force $utilDir/lapack-make.inc.$platform make.inc
        runcmd make lapacklib
        file copy -force lapack_$platform.a $::Slicer3_LIB/netlib-build/lapack-build/liblapack.a
    }
}

################################################################################
# Get and build CLAPACK
#

if { [BuildThis $::CLAPACK_TEST_FILE "CLAPACK"] == 1 } {
    cd $Slicer3_LIB

    runcmd $::SVN co $::CLAPACK_TAG CLAPACK

    if {$::GENLIB(buildit)} {
      file mkdir $Slicer3_LIB/CLAPACK-build
      cd $Slicer3_LIB/CLAPACK-build

      set CMAKE_C_FLAGS_CLAPACK ""
      if { $isLinux && $::tcl_platform(machine) == "x86_64" } {
        set CMAKE_C_FLAGS_CLAPACK "-fPIC"
      }

      runcmd $::CMAKE \
        -G$GENERATOR \
        -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
        -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
        -DCMAKE_C_FLAGS:STRING=$CMAKE_C_FLAGS_CLAPACK \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        -DCMAKE_SKIP_RPATH:BOOL=ON \
        -DCMAKE_BUILD_TYPE:STRING=$::PYTHON_BUILD_TYPE \
        ../CLAPACK

      if {$isWindows} {
        if { $MSVC6 } {
            runcmd $::MAKE CLAPACK.dsw /MAKE "ALL_BUILD - $::PYTHON_BUILD_TYPE"
        } else {
            runcmd $::MAKE CLAPACK.SLN /out buildlog.txt /build  $::PYTHON_BUILD_TYPE
        }
      } else {
        eval runcmd $::MAKE 
      }
  }
}

################################################################################
# Get and build numpy and scipy
#

if {  [BuildThis $::NUMPY_TEST_FILE "python"] && !$::USE_SYSTEM_PYTHON && [string tolower $::USE_PYTHON] == "on" && [string tolower $::USE_NUMPY] == "on" } {

    set ::env(PYTHONHOME) $::Slicer3_LIB/python-build
    cd $::Slicer3_LIB/python

    # do numpy

    runcmd $::SVN co $::NUMPY_TAG numpy

    #
    # as of 3.6.4, follow the approach used in slicer4 superbuild
    #

    if { $isDarwin } {
        if { ![info exists ::env(DYLD_LIBRARY_PATH)] } { set ::env(DYLD_LIBRARY_PATH) "" }
        set ::env(DYLD_LIBRARY_PATH) $::Slicer3_LIB/python-build/lib:$::env(DYLD_LIBRARY_PATH)
    } else {
        if { !$isWindows } {
          if { ![info exists ::env(LD_LIBRARY_PATH)] } { set ::env(LD_LIBRARY_PATH) "" }
          set ::env(LD_LIBRARY_PATH) $::Slicer3_LIB/python-build/lib:$::env(LD_LIBRARY_PATH)
        }
    }

    set path_sep ":"
    if { $isWindows } {
      set path_sep ";"
    }

    # As explained in site.cfg.example, the library name without the prefix "lib" should be used.
    # Nevertheless, on windows, only "libf2c" leads to a successful configuration and 
    # installation of NUMPY
    set f2c_libname "f2c"
    if { $isWindows } {
      set f2c_libname "libf2c"
    }

    # setup the site.cfg file
    set fp [open "numpy/site.cfg" "w"]
    puts $fp ""
    puts $fp "\[blas\]"
    if { $isWindows } {
      puts $fp "library_dirs = $::Slicer3_LIB/CLAPACK-build/BLAS/SRC/$::PYTHON_BUILD_TYPE/$path_sep$::Slicer3_LIB/CLAPACK-build/F2CLIBS/libf2c/$::PYTHON_BUILD_TYPE/"
    } else {
      puts $fp "library_dirs = $::Slicer3_LIB/CLAPACK-build/BLAS/SRC/$path_sep$::Slicer3_LIB/CLAPACK-build/F2CLIBS/libf2c/"
    }
    puts $fp "libraries = blas,$f2c_libname"
    puts $fp ""
    puts $fp "\[lapack\]"
    if { $isWindows } {
      puts $fp "library_dirs = $::Slicer3_LIB/CLAPACK-build/SRC/$::PYTHON_BUILD_TYPE/"
    } else {
      puts $fp "library_dirs = $::Slicer3_LIB/CLAPACK-build/SRC/"
    }
    puts $fp "lapack_libs = lapack"
    puts $fp ""
    close $fp

    if { $isWindows } {
      # store the old path - on some systems (some cyginw installs)
      # having the vc and devenv paths prefixed to the path
      # prevents subsequent cmake builds from functioning
      set ::GENLIB(prePythonPATH) $::env(PATH)

      # prepare the environment for numpy build script
      # - the path setup depends on how cygwin was configured (either
      # with or without the /cygdrive portion of the path)
      if { [string match *cygdrive* $env(PATH)] } {
        # Steve's way - cygwin does not mount c:/ as /c
        regsub -all ":" [file dirname $::MAKE] "" devenvdir
        regsub -all ":" $::COMPILER_PATH "" vcbindir
        set devenvdir /cygdrive/$devenvdir
        set vcbindir /cygdrive/$vcbindir
        set ::env(PATH) $devenvdir:$vcbindir:$::env(PATH)
        regsub -all ":" $::PYTHON_BUILD_DIR "" pcbuildpath
        set ::env(PATH) /cygdrive/$pcbuildpath:$::env(PATH)
        regsub -all ":" $::MSSDK_PATH/Bin "" sdkpath
        set ::env(PATH) /cygdrive/$sdkpath:$::env(PATH)
      } else {
        # Jim's way - cygwin does mount c:/ as /c and doesn't use cygdrive
        set devenvdir [file dirname $::MAKE]
        set vcbindir $::COMPILER_PATH
        set ::env(PATH) $devenvdir\;$vcbindir\;$::MSSDK_PATH/bin\;$::env(PATH)
        set ::env(PATH) $::env(PATH)\;$::PYTHON_BUILD_DIR
      }
      set ::env(INCLUDE) [file dirname $::COMPILER_PATH]/include
      set ::env(INCLUDE) $::MSSDK_PATH/Include\;$::env(INCLUDE)
      set ::env(INCLUDE) [file normalize $::Slicer3_LIB/python-build/Include]\;$::env(INCLUDE)
      set ::env(LIB) $::MSSDK_PATH/Lib\;[file dirname $::COMPILER_PATH]/lib
      set ::env(LIBPATH) $devenvdir

      set ::PYTHON_EXE $::PYTHON_BUILD_DIR/python.exe
    } else {
      set ::PYTHON_EXE $::PYTHON_BUILD_DIR/bin/python
    }


    cd $::Slicer3_LIB/python/numpy
    runcmd $::PYTHON_EXE ./setup.py config

    set ::env(VS_UNICODE_OUTPUT) ""
    
    # Note that for slicer we forcably disable the fortran compiler
    # to workaround numpy build errors on linux machines where gfortran
    # is installed.
    if { $::PYTHON_BUILD_TYPE == "Debug" } {
      runcmd $::PYTHON_EXE ./setup.py build  --debug --fcompiler=/dev/null
    } else {
      runcmd $::PYTHON_EXE ./setup.py build --fcompiler=/dev/null
    }

    runcmd $::PYTHON_EXE ./setup.py install

    ## in earlier slicer versions we needed to add manifests to the pyd files.
    ## this is now handled by a patch to python's distutils 

    if { $isWindows } {
      # restore the old path 
      set ::env(PATH) $::GENLIB(prePythonPATH)
    }

}


################################################################################
# Get and build vtk
#

if { [BuildThis $::VTK_TEST_FILE "vtk"] == 1 } {
    cd $Slicer3_LIB

    if { [info exists ::VTK_GIT_REPO] } {
      if { ![file exists VTK] } {
          eval "runcmd $::GIT clone $::VTK_GIT_REPO VTK"
      }
      cd $Slicer3_LIB/VTK
      eval "runcmd $::GIT checkout $::VTK_GIT_BRANCH"
      eval "runcmd $::GIT pull"
      eval "runcmd $::GIT checkout $::VTK_GIT_TAG"
    } else {
      runcmd $::SVN co $::VTK_TAG VTK
    }

    # Andy's temporary hack to get around wrong permissions in VTK cvs repository
    # catch statement is to make file attributes work with RH 7.3
    if { !$isWindows } {
        catch "file attributes $Slicer3_LIB/VTK/VTKConfig.cmake.in -permissions a+rw"
    }
    if {$::GENLIB(buildit)} {

      file mkdir $Slicer3_LIB/VTK-build
      cd $Slicer3_LIB/VTK-build

      set USE_VTK_ANSI_STDLIB ""
      if { $isWindows } {
        if {$MSVC6} {
            set USE_VTK_ANSI_STDLIB "-DVTK_USE_ANSI_STDLIB:BOOL=ON"
        }
      }

      #
      # Note - 
      # -- the text needs to be duplicated to avoid quoting problems with paths that have spaces
      #
      if { $isLinux && $::tcl_platform(machine) == "x86_64" } {
        runcmd $::CMAKE \
            -G$GENERATOR \
            -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
            -DBUILD_SHARED_LIBS:BOOL=ON \
            -DCMAKE_SKIP_RPATH:BOOL=ON \
            -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DBUILD_TESTING:BOOL=OFF \
            -DVTK_USE_CARBON:BOOL=OFF \
            -DVTK_USE_X:BOOL=ON \
            -DVTK_WRAP_TCL:BOOL=ON \
            -DVTK_USE_HYBRID:BOOL=ON \
            -DVTK_USE_PATENTED:BOOL=ON \
            -DVTK_USE_PARALLEL:BOOL=ON \
            -DVTK_DEBUG_LEAKS:BOOL=$::VTK_DEBUG_LEAKS \
            -DTCL_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTK_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTCL_LIBRARY:FILEPATH=$::VTK_TCL_LIB \
            -DTK_LIBRARY:FILEPATH=$::VTK_TK_LIB \
            -DTCL_TCLSH:FILEPATH=$::VTK_TCLSH \
            $USE_VTK_ANSI_STDLIB \
            -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
            -DVTK_USE_64BIT_IDS:BOOL=ON \
            ../VTK
      } elseif { $isDarwin } {
        ## Need to set the library path so that vtkhash is found while building the parellel libraries
        set ::env(DYLD_LIBRARY_PATH) "$::Slicer3_LIB/VTK-build/bin"
        set OpenGLString "-framework OpenGL;/usr/X11R6/lib/libGL.dylib"
        runcmd $::CMAKE \
            -G$GENERATOR \
            -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
            -DBUILD_SHARED_LIBS:BOOL=ON \
            -DCMAKE_SKIP_RPATH:BOOL=OFF \
            -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DCMAKE_SHARED_LINKER_FLAGS:STRING="-Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib" \
            -DCMAKE_EXE_LINKER_FLAGS="-Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib" \
            -DBUILD_TESTING:BOOL=OFF \
            -DVTK_USE_CARBON:BOOL=OFF \
            -DVTK_USE_COCOA:BOOL=OFF \
            -DVTK_USE_X:BOOL=ON \
            -DVTK_WRAP_TCL:BOOL=ON \
            -DVTK_USE_HYBRID:BOOL=ON \
            -DVTK_USE_PATENTED:BOOL=ON \
            -DVTK_USE_PARALLEL:BOOL=ON \
            -DVTK_DEBUG_LEAKS:BOOL=$::VTK_DEBUG_LEAKS \
            -DOPENGL_INCLUDE_DIR:PATH=/usr/X11R6/include \
            -DTCL_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTK_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTCL_LIBRARY:FILEPATH=$::VTK_TCL_LIB \
            -DTK_LIBRARY:FILEPATH=$::VTK_TK_LIB \
            -DTCL_TCLSH:FILEPATH=$::VTK_TCLSH \
            -DOPENGL_gl_LIBRARY:STRING=$OpenGLString \
            $USE_VTK_ANSI_STDLIB \
            ../VTK
      } elseif { $isSolaris && $::GENLIB(bitness) == "64" } {
        runcmd $::CMAKE \
            -G$GENERATOR \
            -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
            -DBUILD_SHARED_LIBS:BOOL=ON \
            -DCMAKE_SKIP_RPATH:BOOL=ON \
            -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DBUILD_TESTING:BOOL=OFF \
            -DVTK_USE_CARBON:BOOL=OFF \
            -DVTK_USE_X:BOOL=ON \
            -DVTK_WRAP_TCL:BOOL=ON \
            -DVTK_USE_HYBRID:BOOL=ON \
            -DVTK_USE_PATENTED:BOOL=ON \
            -DVTK_USE_PARALLEL:BOOL=ON \
            -DVTK_DEBUG_LEAKS:BOOL=$::VTK_DEBUG_LEAKS \
            -DTCL_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTK_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTCL_LIBRARY:FILEPATH=$::VTK_TCL_LIB \
            -DTK_LIBRARY:FILEPATH=$::VTK_TK_LIB \
            -DTCL_TCLSH:FILEPATH=$::VTK_TCLSH \
            $USE_VTK_ANSI_STDLIB \
            -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
            -DVTK_USE_64BIT_IDS:BOOL=ON \
            ../VTK
      } elseif { $isWindows } {
        runcmd $::CMAKE \
            -G$GENERATOR \
            -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
            -DBUILD_SHARED_LIBS:BOOL=ON \
            -DCMAKE_SKIP_RPATH:BOOL=ON \
            -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DCMAKE_C_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_C_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DBUILD_TESTING:BOOL=OFF \
            -DVTK_USE_CARBON:BOOL=OFF \
            -DVTK_USE_X:BOOL=ON \
            -DVTK_WRAP_TCL:BOOL=ON \
            -DVTK_USE_HYBRID:BOOL=ON \
            -DVTK_USE_PATENTED:BOOL=ON \
            -DVTK_USE_PARALLEL:BOOL=ON \
            -DVTK_DEBUG_LEAKS:BOOL=$::VTK_DEBUG_LEAKS \
            -DTCL_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTK_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTCL_LIBRARY:FILEPATH=$::VTK_TCL_LIB \
            -DTK_LIBRARY:FILEPATH=$::VTK_TK_LIB \
            -DTCL_TCLSH:FILEPATH=$::VTK_TCLSH \
            $USE_VTK_ANSI_STDLIB \
            ../VTK
      } else {
        runcmd $::CMAKE \
            -G$GENERATOR \
            -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
            -DBUILD_SHARED_LIBS:BOOL=ON \
            -DCMAKE_SKIP_RPATH:BOOL=ON \
            -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DBUILD_TESTING:BOOL=OFF \
            -DVTK_USE_CARBON:BOOL=OFF \
            -DVTK_USE_X:BOOL=ON \
            -DVTK_WRAP_TCL:BOOL=ON \
            -DVTK_USE_HYBRID:BOOL=ON \
            -DVTK_USE_PATENTED:BOOL=ON \
            -DVTK_USE_PARALLEL:BOOL=ON \
            -DVTK_DEBUG_LEAKS:BOOL=$::VTK_DEBUG_LEAKS \
            -DTCL_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTK_INCLUDE_PATH:PATH=$TCL_INCLUDE_DIR \
            -DTCL_LIBRARY:FILEPATH=$::VTK_TCL_LIB \
            -DTK_LIBRARY:FILEPATH=$::VTK_TK_LIB \
            -DTCL_TCLSH:FILEPATH=$::VTK_TCLSH \
            $USE_VTK_ANSI_STDLIB \
            ../VTK
      }

      if { $isWindows } {
        if { $MSVC6 } {
            runcmd $::MAKE VTK.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
        } else {
            runcmd $::MAKE VTK.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
        }
      } else {
        eval runcmd $::MAKE 
    }
  }
}

################################################################################
# Get and build kwwidgets
#

if { [BuildThis $::KWWidgets_TEST_FILE "kwwidgets"] == 1 } {
    cd $Slicer3_LIB

    runcmd $::CVS -d :pserver:anoncvs:@www.kwwidgets.org:/cvsroot/KWWidgets login
    eval "runcmd $::CVS $CVS_CO_FLAGS -d :pserver:anoncvs@www.kwwidgets.org:/cvsroot/KWWidgets checkout -r $::KWWidgets_TAG KWWidgets"

    if {$::GENLIB(buildit)} {
      file mkdir $Slicer3_LIB/KWWidgets-build
      cd $Slicer3_LIB/KWWidgets-build



      runcmd $::CMAKE \
        -G$GENERATOR \
        -DVTK_DIR:PATH=$Slicer3_LIB/VTK-build \
        -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
        -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
        -DBUILD_SHARED_LIBS:BOOL=ON \
        -DCMAKE_SKIP_RPATH:BOOL=ON \
        -DBUILD_EXAMPLES:BOOL=OFF \
        -DKWWidgets_BUILD_EXAMPLES:BOOL=OFF \
        -DBUILD_TESTING:BOOL=OFF \
        -DKWWidgets_BUILD_TESTING:BOOL=OFF \
        -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
        ../KWWidgets

      if { $isDarwin } {
        runcmd $::CMAKE \
          -DCMAKE_SHARED_LINKER_FLAGS:STRING="-Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib" \
          -DCMAKE_EXE_LINKER_FLAGS="-Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib" \
          ../KWWidgets
      }

      if {$isWindows} {
        if { $MSVC6 } {
            runcmd $::MAKE KWWidgets.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
        } else {
            runcmd $::MAKE KWWidgets.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
        }
      } else {
        eval runcmd $::MAKE 
      }
  }
}

################################################################################
# Get and build itk
#

if { [BuildThis $::ITK_TEST_FILE "itk"] == 1 } {
    cd $Slicer3_LIB

    if { [info exists ::ITK_GIT_REPO] } {
      if { ![file exists Insight] } {
          eval "runcmd $::GIT clone -b $::ITK_GIT_BRANCH $::ITK_GIT_REPO Insight"
      }
      cd $Slicer3_LIB/Insight
      eval "runcmd $::GIT checkout $::ITK_GIT_BRANCH"
      eval "runcmd $::GIT pull"
      eval "runcmd $::GIT checkout $::ITK_GIT_TAG"
    } else {
      runcmd $::CVS -d :pserver:anoncvs:@www.vtk.org:/cvsroot/Insight login
      eval "runcmd $::CVS $CVS_CO_FLAGS -d :pserver:anoncvs@www.vtk.org:/cvsroot/Insight checkout -r $::ITK_TAG Insight"
    }

    if {$::GENLIB(buildit)} {
      file mkdir $Slicer3_LIB/Insight-build
      cd $Slicer3_LIB/Insight-build


      if {$isDarwin} {
        runcmd $::CMAKE \
          -G$GENERATOR \
          -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
          -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
          -DITK_USE_REVIEW:BOOL=ON \
          -DITK_USE_REVIEW_STATISTICS:BOOL=ON \
          -DITK_USE_OPTIMIZED_REGISTRATION_METHODS:BOOL=ON \
          -DITK_USE_PORTABLE_ROUND:BOOL=ON \
          -DITK_USE_CENTERED_PIXEL_COORDINATES_CONSISTENTLY:BOOL=ON \
          -DITK_USE_TRANSFORM_IO_FACTORIES:BOOL=ON \
          -DBUILD_SHARED_LIBS:BOOL=ON \
          -DCMAKE_SKIP_RPATH:BOOL=OFF \
          -DBUILD_EXAMPLES:BOOL=OFF \
          -DBUILD_TESTING:BOOL=OFF \
          -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
          -DITK_LEGACY_REMOVE:BOOL=ON \
        ../Insight
      } else {
        runcmd $::CMAKE \
          -G$GENERATOR \
          -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
          -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
          -DITK_USE_REVIEW:BOOL=ON \
          -DITK_USE_REVIEW_STATISTICS:BOOL=ON \
          -DITK_USE_OPTIMIZED_REGISTRATION_METHODS:BOOL=ON \
          -DITK_USE_PORTABLE_ROUND:BOOL=ON \
          -DITK_USE_CENTERED_PIXEL_COORDINATES_CONSISTENTLY:BOOL=ON \
          -DITK_USE_TRANSFORM_IO_FACTORIES:BOOL=ON \
          -DBUILD_SHARED_LIBS:BOOL=ON \
          -DCMAKE_SKIP_RPATH:BOOL=ON \
          -DBUILD_EXAMPLES:BOOL=OFF \
          -DBUILD_TESTING:BOOL=OFF \
          -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
          -DITK_LEGACY_REMOVE:BOOL=ON \
          ../Insight
      }

      if {$isWindows} {
        if { $MSVC6 } {
            runcmd $::MAKE ITK.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
        } else {
            runcmd $::MAKE ITK.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
        }
      } else {
        eval runcmd $::MAKE 
    }
    puts "Patching ITK..."

    set fp1 [open "$Slicer3_LIB/Insight-build/Utilities/nifti/niftilib/cmake_install.cmake" r]
    set fp2 [open "$Slicer3_LIB/Insight-build/Utilities/nifti/znzlib/cmake_install.cmake" r]
    set data1 [read $fp1]
    set data2 [read $fp2]

    close $fp1
    close $fp2

    regsub -all /usr/local/lib $data1 \${CMAKE_INSTALL_PREFIX}/lib data1
    regsub -all /usr/local/include $data1 \${CMAKE_INSTALL_PREFIX}/include data1
    regsub -all /usr/local/lib $data2 \${CMAKE_INSTALL_PREFIX}/lib data2
    regsub -all /usr/local/include $data2 \${CMAKE_INSTALL_PREFIX}/include data2

    set fw1 [open "$Slicer3_LIB/Insight-build/Utilities/nifti/niftilib/cmake_install.cmake" w]
    set fw2 [open "$Slicer3_LIB/Insight-build/Utilities/nifti/znzlib/cmake_install.cmake" w]

    puts -nonewline $fw1 $data1
    puts -nonewline $fw2 $data2
 
    close $fw1
    close $fw2
  }
}


################################################################################
# Get and build teem
# -- relies on VTK's png and zlib
#

if { [BuildThis $::Teem_TEST_FILE "teem"] == 1 } {
  cd $Slicer3_LIB

  runcmd $::SVN co $::Teem_TAG teem


  if {$::GENLIB(buildit)} {
    file mkdir $Slicer3_LIB/teem-build
    cd $Slicer3_LIB/teem-build

    if { $isDarwin } {
      set C_FLAGS -DCMAKE_C_FLAGS:STRING=-fno-common \
    } else {
      set C_FLAGS ""
    }
# !!! FIXME How to append -m64 the -fno-common if we want to build 64 bit on Mac?

    if {$::GENLIB(bitness) == "64"} {
      set C_FLAGS -DCMAKE_C_FLAGS:STRING=-m64 \
    } else {
      set C_FLAGS ""
    }

    switch $::tcl_platform(os) {
      "SunOS" -
      "GNU/kFreeBSD" {
          set ::env("LD_LIBRARY_PATH") "$::Slicer3_LIB/VTK-build/bin"
          set zlib "libvtkzlib.so"
          set png "libvtkpng.so"
      }
      "Linux" {
          set ::env("LD_LIBRARY_PATH") "$::Slicer3_LIB/VTK-build/bin"
          set zlib "libvtkzlib.so"
          set png "libvtkpng.so"
      }
      "Darwin" {
          ## Need to set the library path so that the tests pass
          set ::env(DYLD_LIBRARY_PATH) "$::Slicer3_LIB/VTK-build/bin"
          set zlib "libvtkzlib.dylib"
          set png "libvtkpng.dylib"
      }
      "Windows NT" {
          set zlib "vtkzlib.lib"
          set png "vtkpng.lib"
      }
    }

    runcmd $::CMAKE \
      -G$GENERATOR \
      -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
      -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF \
      -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
      -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
      $C_FLAGS \
      -DBUILD_SHARED_LIBS:BOOL=ON \
      -DBUILD_TESTING:BOOL=ON \
      -DTeem_PTHREAD:BOOL=OFF \
      -DTeem_BZIP2:BOOL=OFF \
      -DTeem_ZLIB:BOOL=ON \
      -DTeem_PNG:BOOL=ON \
      -DTeem_VTK_MANGLE:BOOL=ON \
      -DTeem_VTK_TOOLKITS_IPATH:FILEPATH=$::Slicer3_LIB/VTK-build \
      -DZLIB_INCLUDE_DIR:PATH=$::Slicer3_LIB/VTK/Utilities \
      -DTeem_VTK_ZLIB_MANGLE_IPATH:PATH=$::Slicer3_LIB/VTK/Utilities/vtkzlib \
      -DTeem_ZLIB_DLLCONF_IPATH:PATH=$::Slicer3_LIB/VTK-build/Utilities \
      -DZLIB_LIBRARY:FILEPATH=$::Slicer3_LIB/VTK-build/bin/$::VTK_BUILD_SUBDIR/$zlib \
      -DPNG_PNG_INCLUDE_DIR:PATH=$::Slicer3_LIB/VTK/Utilities/vtkpng \
      -DTeem_PNG_DLLCONF_IPATH:PATH=$::Slicer3_LIB/VTK-build/Utilities \
      -DPNG_LIBRARY:FILEPATH=$::Slicer3_LIB/VTK-build/bin/$::VTK_BUILD_SUBDIR/$png \
      -DTeem_USE_LIB_INSTALL_SUBDIR:BOOL=ON \
      ../teem

    if {$isWindows} {
      if { $MSVC6 } {
        runcmd $::MAKE teem.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
      } else {
        runcmd $::MAKE teem.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
      }
    } else {
      eval runcmd $::MAKE 
    }
  }
}



################################################################################
# Get and build OpenIGTLink 
#

if { [BuildThis $::OPENIGTLINK_TEST_FILE "openigtlink"] == 1 && [string tolower $::USE_OPENIGTLINK] == "on" } {
    cd $Slicer3_LIB

    runcmd $::SVN co $::OpenIGTLink_TAG OpenIGTLink

    if {$::GENLIB(buildit)} {
      file mkdir $Slicer3_LIB/OpenIGTLink-build
      cd $Slicer3_LIB/OpenIGTLink-build

      if {$isDarwin} {
        runcmd $::CMAKE \
            -G$GENERATOR \
            -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DBUILD_SHARED_LIBS:BOOL=ON \
            -DCMAKE_SKIP_RPATH:BOOL=OFF \
            -DOpenIGTLink_DIR:FILEPATH=$Slicer3_LIB/OpenIGTLink-build \
            -DOpenIGTLink_PROTOCOL_VERSION_2:BOOL=ON \
            -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
            ../OpenIGTLink
      } else {
        runcmd $::CMAKE \
            -G$GENERATOR \
            -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
            -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
            -DBUILD_SHARED_LIBS:BOOL=ON \
            -DCMAKE_SKIP_RPATH:BOOL=ON \
            -DOpenIGTLink_DIR:FILEPATH=$Slicer3_LIB/OpenIGTLink-build \
            -DOpenIGTLink_PROTOCOL_VERSION_2:BOOL=ON \
            -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
            ../OpenIGTLink
      }

      if { $isDarwin } {
        runcmd $::CMAKE \
          -DCMAKE_SHARED_LINKER_FLAGS:STRING="-Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib" \
          -DCMAKE_EXE_LINKER_FLAGS="-Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib" \
          ../OpenIGTLink
      }

      if {$isWindows} {
        if { $MSVC6 } {
            runcmd $::MAKE OpenIGTLink.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
        } else {
            runcmd $::MAKE OpenIGTLink.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
        }
      } else {
        # Running this cmake again will populate those CMake variables 
        eval runcmd $::CMAKE ../OpenIGTLink 

        eval runcmd $::MAKE 
      }
  }
}


################################################################################
# Get and build BatchMake
#
#

if { ![file exists $::BatchMake_TEST_FILE] || $::GENLIB(update) } {
    cd $::Slicer3_LIB

    runcmd $::CVS -d :pserver:anoncvs:@batchmake.org:/cvsroot/BatchMake co -r $::BatchMake_TAG BatchMake

    file mkdir $::Slicer3_LIB/BatchMake-build
    cd $::Slicer3_LIB/BatchMake-build

    runcmd $::CMAKE \
        -G$GENERATOR \
        -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF \
        -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
        -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        -DBUILD_TESTING:BOOL=OFF \
        -DUSE_FLTK:BOOL=OFF \
        -DDASHBOARD_SUPPORT:BOOL=OFF \
        -DGRID_SUPPORT:BOOL=ON \
        -DUSE_SPLASHSCREEN:BOOL=OFF \
        -DITK_DIR:FILEPATH=$ITK_BINARY_PATH \
        ../BatchMake

    if {$isWindows} {
        if { $MSVC6 } {
            runcmd $::MAKE BatchMake.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
        } else {
            runcmd $::MAKE BatchMake.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
        }
    } else {
        eval runcmd $::MAKE
    }
}


################################################################################
# Get and build OpenCV 
#
#
if { [BuildThis $::OpenCV_TEST_FILE "cv"] == 1 && [string tolower $::USE_OPENCV] == "on" } {

    cd $::Slicer3_LIB
    runcmd $::SVN co $::OpenCV_TAG OpenCV
    file mkdir $::Slicer3_LIB/OpenCV-build
    cd $::Slicer3_LIB/OpenCV-build

    runcmd $::CMAKE \
         -G$GENERATOR \
         -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
         -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
         -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
         -DBUILD_SHARED_LIBS:BOOL=ON \
         -DBUILD_TESTING:BOOL=OFF \
         -DBUILD_NEW_PYTHON_SUPPORT:BOOL=OFF \
         -DOpenCV_DIR:FILEPATH=$Slicer3_LIB/OpenCV-build \
         ../OpenCV
    
    if {$isWindows} {
        if { $MSVC6 } {
            runcmd $::MAKE OpenCV.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
        } else {
            runcmd $::MAKE OpenCV.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
        }
    } else {
        eval runcmd $::MAKE
    }
}


################################################################################
# Get and build SLICERLIBCURL (slicerlibcurl)
#
#

if { [BuildThis $::SLICERLIBCURL_TEST_FILE "libcurl"] == 1 } {
    cd $::Slicer3_LIB

    runcmd $::SVN co http://svn.slicer.org/Slicer3-lib-mirrors/trunk/cmcurl cmcurl
    if {$::GENLIB(buildit)} {

      file mkdir $::Slicer3_LIB/cmcurl-build
      cd $::Slicer3_LIB/cmcurl-build
if {$isSolaris} {
      runcmd $::CMAKE \
        -G$GENERATOR \
        -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF \
        -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
        -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
        -DBUILD_SHARED_LIBS:BOOL=ON \
        -DBUILD_TESTING:BOOL=OFF \
        ../cmcurl
     } else {
      runcmd $::CMAKE \
        -G$GENERATOR \
        -DCMAKE_BUILD_TYPE:STRING=$::VTK_BUILD_TYPE \
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF \
        -DCMAKE_CXX_COMPILER:STRING=$COMPILER_PATH/$COMPILER \
        -DCMAKE_CXX_COMPILER_FULLPATH:FILEPATH=$COMPILER_PATH/$COMPILER \
        -DBUILD_SHARED_LIBS:BOOL=OFF \
        -DBUILD_TESTING:BOOL=OFF \
        ../cmcurl
     }
      if {$isWindows} {
        if { $MSVC6 } {
            runcmd $::MAKE SLICERLIBCURL.dsw /MAKE "ALL_BUILD - $::VTK_BUILD_TYPE"
        } else {
            runcmd $::MAKE SLICERLIBCURL.SLN /out buildlog.txt /build  $::VTK_BUILD_TYPE
        }
      } else {
        eval runcmd $::MAKE
      }
  }
}


if {! $::GENLIB(buildit)} {
 exit 0
}

# Are all the test files present and accounted for?  If not, return error code

if { ![file exists $::CMAKE] } {
    puts "CMake test file $::CMAKE not found."
}
if { ![file exists $::Teem_TEST_FILE] } {
    puts "Teem test file $::Teem_TEST_FILE not found."
}
if { ![file exists $::OPENIGTLINK_TEST_FILE] && [string tolower $::USE_OPENIGTLINK] == "on" } {
    puts "OpenIGTLink test file $::OPENIGTLINK_TEST_FILE not found."
}
if { ![file exists $::SLICERLIBCURL_TEST_FILE] } {
    puts "SLICERLIBCURL test file $::SLICERLIBCURL_TEST_FILE not found."
}
if { ![file exists $::TCL_TEST_FILE] } {
    puts "Tcl test file $::TCL_TEST_FILE not found."
}
if { ![file exists $::TK_TEST_FILE] } {
    puts "Tk test file $::TK_TEST_FILE not found."
}
if { ![file exists $::ITCL_TEST_FILE] } {
    puts "incrTcl test file $::ITCL_TEST_FILE not found."
}
if { ![file exists $::IWIDGETS_TEST_FILE] } {
    puts "iwidgets test file $::IWIDGETS_TEST_FILE not found."
}
if { ![file exists $::BLT_TEST_FILE] } {
    puts "BLT test file $::BLT_TEST_FILE not found."
}
if { ![file exists $::VTK_TEST_FILE] } {
    puts "VTK test file $::VTK_TEST_FILE not found."
}
if { ![file exists $::ITK_TEST_FILE] } {
    puts "ITK test file $::ITK_TEST_FILE not found."
}

if { ![file exists $::CMAKE] || \
         ![file exists $::Teem_TEST_FILE] || \
         ![file exists $::SLICERLIBCURL_TEST_FILE] || \
         ![file exists $::TCL_TEST_FILE] || \
         ![file exists $::TK_TEST_FILE] || \
         ![file exists $::ITCL_TEST_FILE] || \
         ![file exists $::IWIDGETS_TEST_FILE] || \
         !(1 || [file exists $::BLT_TEST_FILE]) || \
         ![file exists $::VTK_TEST_FILE] || \
         ![file exists $::ITK_TEST_FILE] } {
    puts "Not all packages compiled; check errors and run genlib.tcl again."
    exit 1 
} else { 
    puts "All packages compiled."
    exit 0 
}
