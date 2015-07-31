CC := arm-none-linux-gnueabi-gcc
LINUX_DIR = ~/workspace/exdroid/lichee/linux-3.3/include
CFLAGS := -I $(LINUX_DIR)
TARGET := app_basic 

.PHONY: all clean

all: $(TARGET) 

app_basic:app_basic.c 
	$(CC) $(CFLAGS) -static  $^  -o  $@

clean:
	rm -rf $(TARGET)  
