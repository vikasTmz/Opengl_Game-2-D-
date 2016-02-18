CC = g++
CFLAGS = -Wall
PROG = exe

SRCS = Sample_GL3_2D.cpp
# LIBS = glad.c  -lSOIL -lGL -lGLU -lglfw -lX11 -lXrandr -ldl 
LIBS = glad.c  -lglfw  -lSOIL -lGLU  -ldl -lfreetype -lftgl -I/usr/local/include -I/usr/local/include/freetype2 -L/usr/local/include -L/usr/local/lib -lGL


all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
