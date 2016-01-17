CC = g++
CFLAGS = -Wall
PROG = sample2D

SRCS = Sample_GL3_2D.cpp
LIBS = glad.c  -lSOIL -lGL -lGLU -lglfw -lX11 -lXrandr -ldl 

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
