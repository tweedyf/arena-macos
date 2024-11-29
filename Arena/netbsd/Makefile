#	Make WWW product
# include ../../All/$(WWW_MACH)/Makefile.include
#include Makefile.include
#  Make WWW under unix for netbsd
#

# For W3 distribution, machine type for subdirectories
WWW_MACH = netbsd

# The ASIS repository's name for the machine we are on
ASIS_MACH = netbsd/amiga

# Options for cc 
#CC = cc
#CFLAGS = -DDEBUG
#LFLAGS = -Bstatic

# Options for gcc
CC = gcc
CFLAGS = -O -g -DNO_STRERROR
#CFLAGS = -Wall -g -DNO_GETWD -DHAS_GETCWD 
# CFLAGS = -Wall -g
#LFLAGS = -static

# In both cases: Ask for static linking so that binary will be transportable

# Directory for installed binary:
BINDIR = /usr/local/bin
#BINDIR = ../../LineMode/sun4

#_________________ OK if normal W3 distribution
# Where is the WWW source root?
WWW = ../..

#  Where should temporary (object) files go?
# WTMP = /usr/tmp
WTMP = $(WWW)

# Where is the W3 object library?
#LIBDIR = $(WWW)/Library/Implementation/$(WWW_MACH)

USELIBDIR = ../../Library/$(WWW_MACH)
XLIBDIR = /usr/X11R5/lib
OTHERLIB = -lm

#include ../Implementation/CommonMakefile
#	Makefile for WorldWideWeb    Arena browser (www)
#
#	(c) CERN 1990, 1991 -- see Copyright.html for conditions
#
# This file should be invariant between systems.
#	DEPENDENCIES NOT COMPLETE @@
#
#	make		Compile and link the software (private version)
#	make install	Copy it into the system (implies make)
#	make update	Copy installed version into installed version
#	make uninstall	Unlink installed version from the system
#	make clean	Remove intermediate files
#	make cleanall	Remove intremediate files and products
#	
# Macros required to be defined already for make:
#
# CC		The C compiler
# CFLAGS	Flags for $(CC)
# LFLAGS	Flags for ld
# WWW		Root of WWW source tree
#
# Macros needed for make install:
#
# BINDIR	Directory for installed binary
#______________________________________________________________________


#  If this env var is set to something else Some makes will use that instead
SHELL = /bin/sh

# If this is actually run in a subdirectory.
#
# WWW = ../../..

# If it is run in this directory:
#
# WWW = ../..	For [cernlib] build in this directory

ARENA =  $(WWW)/Arena/Implementation/
LMD = $(WWW)/Arena/Defaults
CMN = $(WWW)/Library/Implementation/
DEFAULTS = $(WWW)/Arena/Defaults

CFLAGS2 = $(CFLAGS) -I$(CMN) -I$(ARENA)

CERNLIBBIN = $(WWW)/bin

#	WTMP is either WWW or /tmp for example. It allows the 
# machine-dependent files to be put in a separate tree so that
#	- The source tree can be read-only
#	- The source tree can be shared across platforms
#
USELIBDIR = $(WTMP)/Library/$(WWW_MACH)
# XLIBDIR = /usr/lib/X11
JPEGDIR = ../../Image/jpeg
PNGDIR = ../../Image/libpng
ZLIBDIR = ../../Image/zlib
OBJ = $(WTMP)/Arena/$(WWW_MACH)
BIN = $(WWW)/Arena/$(WWW_MACH)

OE = $(OBJ)/.created

WWWLIB = -L$(USELIBDIR) -lwww
XLIB = -L$(XLIBDIR) -lX11
JPEGLIB = $(JPEGDIR)/$(WWW_MACH)/libjpeg.a
PNGLIB = $(PNGDIR)/$(WWW_MACH)/libpng.a
ZLIB = $(ZLIBDIR)/$(WWW_MACH)/libz.a

OBJS = $(OBJ)/www.o $(OBJ)/display.o \
  $(OBJ)/scrollbar.o $(OBJ)/toolbar.o $(OBJ)/entities.o $(OBJ)/forms.o \
  $(OBJ)/status.o $(OBJ)/html.o $(OBJ)/parsehtml.o \
  $(OBJ)/image.o $(OBJ)/gif.o $(OBJ)/bridge.o \
  $(OBJ)/mailcap.o $(OBJ)/XGetHClrs.o $(OBJ)/util.o \
  $(OBJ)/math.o $(OBJ)/editor.o $(OBJ)/icon.o $(OBJ)/history.o \
  $(OBJ)/jpeg.o $(OBJ)/dither.o $(OBJ)/style.o $(OBJ)/x11.o $(OBJ)/png.o \
  $(OBJ)/menu.o $(OBJ)/tools.o $(OBJ)/neweditor.o $(OBJ)/popup.o \
  $(OBJ)/parsecommand.o

# Final main product
PROD = arena 

SOURCES = $(ARENA)www.c $(ARENA)display.c \
  $(ARENA)scrollbar.c $(ARENA)toolbar.c $(ARENA)entities.c forms.c \
  $(ARENA)status.c $(ARENA)html.c $(ARENA)parsehtml.c \
  $(ARENA)image.c $(ARENA)gif.c bridge.c \
  $(ARENA)mailcap.c $(ARENA)XGetHClrs.c util.c \
  $(ARENA)math.c $(ARENA)editor.c $(ARENA)icon.c $(ARENA)history.c \
  $(ARENA)jpeg.c $(ARENA)dither.c $(ARENA)style.c $(ARENA)x11.c \
  $(ARENA)png.o $(ARENA)menu.o $(ARENA)tools.o $(ARENA)neweditor.c \
  $(ARENA)popup.c $(ARENA)parsecommand.c



INCLUDES = $(ARENA)www.h $(ARENA)bridge.h $(ARENA)neweditor.h


SPECIFIC =  \
	$(ARENA)apollo_m68k/Makefile \
	$(ARENA)decstation/Makefile \
	$(ARENA)next/Makefile \
	$(ARENA)pc-nfs/makefile \
	$(ARENA)pc-nfs/README \
	$(ARENA)pc-nfs/topc \
	$(ARENA)rs6000/Makefile \
	$(ARENA)sgi/Makefile \
	$(ARENA)snake/Makefile \
	$(ARENA)sun3/Makefile \
	$(ARENA)sun4/Makefile \
	$(ARENA)sun4-sol2/Makefile \
	$(ARENA)unix/Makefile \
	$(ARENA)vax_ultrix/Makefile \
	$(ARENA)vax_ultrix_cc/Makefile \
	$(ARENA)vms/descrip.mms \
	$(ARENA)vms/setup.com

# Bug: This path, if relative, is taken relative to the directory
# in which this makefile is, not the pwd.  This screws up the 
# recursive invocation

all: $(BIN)/$(PROD) 

#$(BIN)/jpeg

#arena: $(OBJS) $(USELIBDIR)/libwww.a


$(BIN)/$(PROD) : $(OBJS) $(USELIBDIR)/libwww.a
	$(CC) -o $@ $(OBJS) $(LFLAGS) $(JPEGLIB) $(PNGLIB) $(ZLIB) $(XLIB) $(WWWLIB) $(OTHERLIB) 


$(USELIBDIR)/libwww.a :
	@echo You must get and make the WWWLibrary product first! $(USELIBDIR)
	(cd $(WWW)/Library/$(WWW_MACH) && $(MAKE))

lib :
	(cd $(WWW)/Library/$(WWW_MACH) && $(MAKE))

#	Make a copy of the binary and documentation on this machine
install :	$(PROD)
	cp $(PROD) $(BINDIR)/$(PROD)
	- mkdir /usr/local
	- mkdir /usr/local/lib
	- mkdir /usr/local/lib/WWW
	cp $(DEFAULTS)/*.html /usr/local/lib/WWW

#	Link system to the binary and documentation in this tree

link : Works/$(PROD)
	ln -s `pwd`/$(PROD) $(BINDIR)/$(PROD)
	- mkdir /usr/local
	- mkdir /usr/local/lib
	ln -s $(DEFAULTS) /usr/local/lib/WWW

uninstall :
	rm $(BINDIR)/$(PROD)
	rm -r /usr/local/lib/WWW

#	Clean up everything generatable except final products
#	Including obj directory UNLESS there is anything else in it
clean :
	rm $(OBJ)/.created $(OBJ)/*.o
	-rmdir $(OBJ)

#	Clean up everything generatable including final products

cleanall : clean
	rm $(PROD)

#	Binary release
#
#	Must be done on anonymous FTP server!
#
#  This makefile does not knw how to make $(PROD)_$(VL) so that trying
#  to make binay or asis on a weird machine doesn't try to build $(PROD)!

binary : $(PROD)_$(VL)
	-mkdir /mnt/www0/pub/www/bin/$(WWW_MACH)
	cp $(PROD)_$(VL) /mnt/www0/pub/www/bin/$(WWW_MACH)/$(PROD)_$(VL)

#	ASIS installation
#	-----------------
#	Can only happen on an allocated ASIS development machine and
#	with uid of the controlling user for the project.
#
asis : /asis/share/usr.local/lib/WWW/default.html \
	/asis/specific/$(ASIS_MACH)/usr.local/bin/$(PROD)
	touch asis
	echo ASIS distribution is now up-to-date.

/asis/specific/$(ASIS_MACH)/usr.local/bin/$(PROD) : $(PROD)_$(VL)
	cp $(PROD)_$(VL) /asis/specific/$(ASIS_MACH)/usr.local/bin/$(PROD)_$(VL)
	rm -f /asis/specific/$(ASIS_MACH)/usr.local/bin/$(PROD)
	ln -s $(PROD)_$(VL) \
		/asis/specific/$(ASIS_MACH)/usr.local/bin/$(PROD)

/asis/share/usr.local/lib/WWW/default.html : $(DEFAULTS)/cern.html
	- mkdir /asis/share/usr.local/lib/WWW
	cp $(DEFAULTS)/*.html /asis/share/usr.local/lib/WWW
	cp $(DEFAULTS)/cern.html /asis/share/usr.local/lib/WWW/default.html
	cp $(DEFAULTS)/default.html /asis/share/usr.local/lib/WWW/top.html

#	CERNLIB installation
#	--------------------
#	Assuming the cernlib-style directory structure
#	Warning: Not suitable for machine-shared file structure.
#
cernlib :	$(PROD)
	- mkdir $(CERNLIBBIN)
	cp $(PROD) $(CERNLIBBIN)


#_________________________________________________________________________	
# Compiling the Objects (Dependenies incomplete)

#	Check directory exists
$(OE) :
	-mkdir $(WTMP)
	-mkdir $(WTMP)/Arena
	-mkdir $(WTMP)/Arena/$(WWW_MACH)
	touch $(OE)

#


$(OBJ)/www.o : $(OE) $(ARENA)www.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)www.c
$(OBJ)/file.o : $(OE) $(ARENA)file.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)file.c
$(OBJ)/display.o : $(OE) $(ARENA)display.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)display.c
$(OBJ)/scrollbar.o : $(OE) $(ARENA)scrollbar.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)scrollbar.c
$(OBJ)/toolbar.o : $(OE) $(ARENA)toolbar.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)toolbar.c
$(OBJ)/entities.o : $(OE) $(ARENA)entities.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)entities.c
$(OBJ)/forms.o : $(OE) $(ARENA)forms.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)forms.c
$(OBJ)/status.o : $(OE) $(ARENA)status.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)status.c
$(OBJ)/html.o : $(OE) $(ARENA)html.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)html.c
$(OBJ)/parsehtml.o : $(OE) $(ARENA)parsehtml.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)parsehtml.c
$(OBJ)/http.o : $(OE) $(ARENA)http.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)http.c
$(OBJ)/cache.o : $(OE) $(ARENA)cache.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)cache.c
$(OBJ)/image.o : $(OE) $(ARENA)image.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)image.c
$(OBJ)/gif.o : $(OE) $(ARENA)gif.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)gif.c
$(OBJ)/bridge.o : $(OE) $(ARENA)bridge.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)bridge.c
$(OBJ)/strptime.o : $(OE) $(ARENA)strptime.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)strptime.c
$(OBJ)/mailcap.o : $(OE) $(ARENA)mailcap.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)mailcap.c
$(OBJ)/XGetHClrs.o : $(OE) $(ARENA)XGetHClrs.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)XGetHClrs.c
$(OBJ)/util.o : $(OE) $(ARENA)util.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)util.c
$(OBJ)/math.o : $(OE) $(ARENA)math.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)math.c
$(OBJ)/editor.o : $(OE) $(ARENA)editor.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)editor.c
$(OBJ)/icon.o : $(OE) $(ARENA)icon.c 
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)icon.c 
$(OBJ)/history.o : $(OE) $(ARENA)history.c 
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)history.c 
$(OBJ)/dither.o : $(OE) $(ARENA)dither.c 
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)dither.c 
$(OBJ)/jpeg.o : $(OE) $(ARENA)jpeg.c 
	$(CC) -c -o $@ $(CFLAGS2) -I$(JPEGDIR)/Implementation $(ARENA)jpeg.c 
$(OBJ)/style.o : $(OE) $(ARENA)style.c 
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)style.c 
$(OBJ)/neweditor.o : $(OE) $(ARENA)neweditor.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)neweditor.c
$(OBJ)/popup.o : $(OE) $(ARENA)popup.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)popup.c
$(OBJ)/parsecommand.o : $(OE) $(ARENA)parsecommand.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)parsecommand.c

# $(OBJ)/skyblue.o : $(OE) $(ARENA)skyblue.c $(ARENA)skyblue.h
# 	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)skyblue.c 
# $(OBJ)/constraints.o : $(OE) $(ARENA)constraints.c $(ARENA)skyblue.h
# 	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)constraints.c 

$(OBJ)/x11.o : $(OE) $(ARENA)x11.c
	$(CC) -c -o $@ $(CFLAGS2) $(ARENA)x11.c 
$(OBJ)/png.o : $(OE) $(ARENA)png.c
	$(CC) -c -o $@ $(CFLAGS2) -I$(PNGDIR)/Implementation -I$(ZLIBDIR)/Implementation $(ARENA)png.c 
$(OBJ)/menu.o : $(OE) $(ARENA)menu.c
	$(CC) -c -o $@ $(CFLAGS2) -I$(PNGDIR)/Implementation -I$(ZLIBDIR)/Implementation $(ARENA)menu.c 
$(OBJ)/tools.o : $(OE) $(ARENA)tools.c
	$(CC) -c -o $@ $(CFLAGS2) -I$(PNGDIR)/Implementation -I$(ZLIBDIR)/Implementation $(ARENA)tools.c 


#		W3 Source + documentation release
#		---------------------------------

distribute :  /mnt/www0/pub/www/README.txt /mnt/www0/pub/www/Copyright.txt
	(cd $(WWW)/.. && WWW=WWW ABS=`pwd`/ $(MAKE) $(MFLAGS) \
		-f WWW/Arena/Implementation/CommonMakefile \
		/mnt/www0/pub/www/src/WWWArena_$(VL).tar.Z)
	(cd ../Implementation; cvs tag \
	    `sed -e 's/V. = /v/' Version.make | sed -e 's?\.?/?'` )
	echo Distribution of Line Mode version $(VC) up to date.

# THIS MUST BE BUILT FROM THE DIRECTORY ABOVE WWW ! USE distribute above!
/mnt/www0/pub/www/src/WWWArena_$(VL).tar.Z : $(SOURCES) $(LMD)/line-mode-guide.txt
	tar cf /mnt/www0/pub/www/src/WWWArena_$(VL).tar \
	    $(SOURCES)  $(LMD)/line-mode-guide.txt \
	    $(WWW)/Arena/*/Makefile \
	    $(WWW)/Arena/vms/descrip.mms $(WWW)/Arena/vms/build_multinet.com \
            $(WWW)/Arena/vms/setup.com 	    
	compress /mnt/www0/pub/www/src/WWWArena_$(VL).tar


#		Hypertext supplied in text format
#		---------------------------------

$(WWW)/README.txt : $(WWW)/../README.html
	www -n -p66 http://info.cern.ch/hypertext/README.html \
		> $(WWW)/README.txt
/mnt/www0/pub/www/README.txt : $(WWW)/README.txt
	cp $(WWW)/README.txt /mnt/www0/pub/www/README.txt

$(WWW)/Copyright.txt : $(WWW)/../Copyright.html
	www -n -p66 http://info.cern.ch/hypertext/Copyright.html \
		> $(WWW)/Copyright.txt
/mnt/www0/pub/www/Copyright.txt : $(WWW)/Copyright.txt
	cp $? $@

#	Line Mode Manual
#
QUICKGUIDE = \
	$(LMD)/QuickGuide.html \
	$(LMD)/Commands.html \
	    $(LMD)/SourceExamples.html \
	$(LMD)/CommandLine.html \
	$(LMD)/Shortcuts.html \
	    $(LMD)/VMS_Shortcuts.html \
	$(LMD)/Installation.html \
	    $(LMD)/Installation_VMS.html \
	    $(LMD)/Installation_VMS_NewTCP.html \
	    $(LMD)/Distribution.html \
	    $(LMD)/Porting.html \
	    $(LMD)/MachineTypes.html \
	    $(LMD)/InstallNotSU.html \
	    $(LMD)/Path.html \
	    $(LMD)/LoginScript.html \
	$(LMD)/Customisation.html \
	    $(LMD)/EnvVariables.html \
	    $(LMD)/GlobalV.html \
	    $(LMD)/NewsServer.html \
	$(LMD)/Bugs.html \
	$(LMD)/Features.html

doc : /mnt/www0/pub/www/doc/line-mode-guide.txt
	echo line mode browser guide exists.

/mnt/www0/pub/www/doc/line-mode-guide.txt : $(LMD)/line-mode-guide.txt
	cp $? $@

$(LMD)/line-mode-guide.txt : $(QUICKGUIDE)
	-rm $@
	for i in $(QUICKGUIDE) ; do www -n -na -p66 $$i >> $@; done



