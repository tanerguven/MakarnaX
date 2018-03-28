QEMU_VER=1.7.0
BUILD_DIR = build
PREFIX=${CURDIR}/local
NCPU=4

all: qemu
	@

qemu-%.tar.gz:
	git clone https://github.com/geofft/qemu.git --depth 1 -b 6.828-$*
	mv qemu qemu-$*
	tar czf qemu-$*.tar.gz qemu-$*
	rm -rf qemu-$*

create-dirs:
	mkdir -p ${BUILD_DIR}

qemu: qemu-${QEMU_VER}.tar.gz create-dirs
	cd ${BUILD_DIR}; rm -rf build-qemu qemu-${QEMU_VER}
	cd ${BUILD_DIR}; tar xf ../qemu-${QEMU_VER}.tar.gz

	mkdir -p ${BUILD_DIR}/build-qemu
	cd ${BUILD_DIR}/build-qemu; \
	../qemu-${QEMU_VER}/configure --enable-sdl --disable-kvm --prefix=${PREFIX} --target-list="i386-softmmu x86_64-softmmu"
	cd ${BUILD_DIR}/build-qemu; make -j${NCPU}; make install
	rm -rf build/build-qemu build/qemu-${QEMU_VER}

install:
	ln -s ${PREFIX}/bin/qemu /usr/local/bin/qemu-os6828

uninstall:
	rm -f /usr/local/bin/qemu-os6828

clean:
	rm -rf ${BUILD_DIR} ${PREFIX} *.tar*
