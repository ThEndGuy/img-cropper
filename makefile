SHELL := cmd.exe

CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type -Iinclude 

LDFLAGS = -L. -Llib -lraylib -lopengl32 -lgdi32 

WINDOWS = -lwinmm -lole32 -lcomdlg32

FINAL = -mwindows

FINALTARGET = build\image-cropper.exe

TARGET = build\main.exe

all:
	$(CC) main.c foreign\tinyfiledialogs.c -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(WINDOWS)

sb:
	$(CC) sbuilder.c -o build\sbuilder.exe $(CFLAGS) $(WINDOWS)

ready:
	$(CC) main.c foreign\tinyfiledialogs.c -o $(FINALTARGET) $(CFLAGS) $(LDFLAGS) $(WINDOWS) $(FINAL)

clean:
	del $(TARGET)
	del $(FINALTARGET)
