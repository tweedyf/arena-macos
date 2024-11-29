cc_defs = /inc=[-.zlib]
c_deb = 

..ifdef __DECC__
pref = /prefix=all
..endif



OBJS = png.obj, pngrcb.obj, pngrutil.obj, pngtrans.obj, pngwutil.obj,\
	pngread.obj, pngmem.obj, pngwrite.obj, pngrtran.obj, pngwtran.obj,\
   pngio.obj, pngerror.obj


CFLAGS= $(C_DEB) $(CC_DEFS) $(PREF)

all : pngtest.exe libpng.olb
      @ write sys$output " pngtest available"

libpng.olb : libpng.olb($(OBJS))
	@ write sys$output " Libpng available"


pngtest.exe : pngtest.obj libpng.olb
              link pngtest,libpng.olb/lib,[-.zlib]libz.olb/lib

test: pngtest.exe
   pngtest

clean :
	delete *.obj;*,libz.olb;*


# Other dependencies.
png.obj : png.h, pngconf.h
pngrcb.obj : png.h, pngconf.h
pngread.obj : png.h, pngconf.h
pngrtran.obj : png.h, pngconf.h
pngrutil.obj : png.h, pngconf.h
pngerror.obj : png.h, pngconf.h
pngmem.obj : png.h, pngconf.h
pngio.obj : png.h, pngconf.h
pngtest.obj : png.h, pngconf.h
pngtrans.obj : png.h, pngconf.h
pngwrite.obj : png.h, pngconf.h
pngwtran.obj : png.h, pngconf.h
pngwutil.obj : png.h, pngconf.h

* Martin P.J. Zinser		           Email: 
* KP II					          m.zinser@gsi.de
* Gesellschaft f. Schwerionenforschung GSI        vipmzs.physik.uni-mainz.de
* Postfach 11 05 52                               mzdmza.zdv.uni-mainz.de
* D-64220 Darmstadt 		           Voice: 0049+6151/3592887
