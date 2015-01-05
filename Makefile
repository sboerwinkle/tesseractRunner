CSTUFF = gcc -O2 -Wall $(DEBUG)
LINK = -lm -lSDL2 -lGLEW -lGL -lGLU -lrt

.PHONY: clean debug remake

run: test.o gfx.o graphicsProcessing.o networking.o
	$(CSTUFF) -o run gfx.o test.o graphicsProcessing.o networking.o $(LINK)

test.o: test.c gfx.h test.h graphicsProcessing.h networking.h
	$(CSTUFF) -c test.c

gfx.o: gfx.c
	$(CSTUFF) -c gfx.c

graphicsProcessing.o: graphicsProcessing.c test.h graphicsProcessing.h gfx.h
	$(CSTUFF) -c graphicsProcessing.c

networking.o: networking.c test.h
	$(CSTUFF) -c networking.c

clean:
	rm -f *.o run

remake:
	$(MAKE) clean
	$(MAKE)

debug:
	$(MAKE) DEBUG="-g -O0"
