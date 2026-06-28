CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type -Iinclude 

ifeq ($(OS), Windows_NT)
	TARGET = build/main.exe
	LDFLAGS = -L. -Llib -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -lcomdlg32
	FINALTARGET = build/image-cropper.exe
	FINAL = -mwindows
	RM = del

else
	TARGET = build/main
	LDFLAGS = -L. -lraylib -lm -lpthread -ldl -lrt -lX11
	FINALTARGET = build/image-cropper
	FINAL =
	RM = rm -f

endif


all:
	$(CC) main.c foreign/tinyfiledialogs.c -o $(TARGET) $(CFLAGS) $(LDFLAGS)

sb:
	$(CC) sbuilder.c -o build/sbuilder.exe $(CFLAGS)

ready:
	$(CC) main.c foreign/tinyfiledialogs.c -o $(FINALTARGET) $(CFLAGS) $(LDFLAGS) $(FINAL)

clean:
	$(RM) $(TARGET)
	$(RM) $(FINALTARGET)
