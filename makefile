SHELL := cmd.exe

CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type -Iinclude 

LDFLAGS = -L. -Llib -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -lcomdlg32

FINAL = -mwindows

FINALTARGET = build\image-cropper.exe

TARGET = build\main.exe

all:
	$(CC) main.c foreign\tinyfiledialogs.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)

ready:
	$(CC) main.c foreign\tinyfiledialogs.c -o $(FINALTARGET) $(CFLAGS) $(LDFLAGS) $(FINAL)

clean:
	del $(TARGET)
	del $(FINALTARGET)
