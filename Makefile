CC=armv7a-linux-androideabi19-clang
CFLAGS=-I. -O2
FAN_OBJ = fan_control.o

.PHONY: repo

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: fan_control

fan_control: $(FAN_OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

goggle_ipk: fan_control
	$(eval PKG_NAME := $(shell cat ./ipk/goggle/control/control | grep Package | cut -d" " -f2))
	$(eval ARCH := $(shell cat ./ipk/goggle/control/control | grep Architecture | cut -d" " -f2))
	$(eval VERSION :=$(shell cat ./ipk/goggle/control/control | grep Version | cut -d" " -f2))
	$(eval IPK_NAME := "${PKG_NAME}_${VERSION}_${ARCH}.ipk")
	mkdir -p ipk/goggle/build
	cp -r ipk/goggle/data ipk/goggle/build/
	mkdir -p ipk/goggle/build/data/opt/bin
	mkdir -p ipk/goggle/build/opt/fonts
	echo "2.0" > ipk/goggle/build/debian-binary
	cp -r ipk/goggle/control ipk/goggle/build/
	cp fan_control ipk/goggle/build/data/opt/bin
	chmod +x ipk/goggle/build/data/opt/bin/fan_control
	cd ipk/goggle/build/control && tar czvf ../control.tar.gz .
	cd ipk/goggle/build/data && tar czvf ../data.tar.gz .
	cd ipk/goggle/build && tar czvf "../../${IPK_NAME}" ./control.tar.gz ./data.tar.gz ./debian-binary

ipk: goggle_ipk 

repo: ipk
	mkdir -p repo
	cp ipk/*.ipk repo/
	../opkg-utils-0.5.0/opkg-make-index ./repo/ > repo/Packages
	http-server -p 8042 ./repo/
	
clean:
	rm -rf fan_control
	rm -rf *.o
	rm -rf repo
	rm -rf ipk/goggle/build
	rm -rf ipk/*.ipk
