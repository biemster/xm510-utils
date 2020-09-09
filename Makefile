GCC = /home/lars/temp/xm510_toolchain/arm-unknown-linux-musleabi/bin/arm-unknown-linux-musleabi-gcc
CFLAGS = -static -std=gnu99 -s -Os -ffunction-sections -Wl,--gc-sections

all: helloworld ftp

helloworld: helloworld.c
	$(GCC) $^ -o $@ $(CFLAGS)

ftp: ftp.c
	$(GCC) $^ -o $@ $(CFLAGS)

clean:
	rm helloworld ftp