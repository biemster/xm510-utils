GCC = /home/lars/temp/xm510_toolchain/arm-unknown-linux-uclibcgnueabi/bin/arm-unknown-linux-uclibcgnueabi-gcc
CFLAGS = -std=gnu99 -s -Os -ffunction-sections -Wl,--gc-sections

all: helloworld ftp

helloworld: helloworld.c
	$(GCC) $^ -o $@ $(CFLAGS)

ftp: ftp.c
	$(GCC) $^ -o $@ $(CFLAGS)

clean:
	rm helloworld ftp