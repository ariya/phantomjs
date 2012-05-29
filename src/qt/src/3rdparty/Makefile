PNGMAK=scripts/makefile.linux

all:
	cd zlib; ./configure; make
	cd libpng; make -f $(PNGMAK)

clean:
	-cd zlib; make clean
	-cd libpng; make -f $(PNGMAK) clean
