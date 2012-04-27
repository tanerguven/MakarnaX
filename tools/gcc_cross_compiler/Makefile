OSNAME=makarnax
BINUTILS_VER=2.22
GCC_VER=4.6.3
NEWLIB_VER=1.20.0
TARGET=i686-${OSNAME}
PREFIX=${CURDIR}/local

PATH:=${PATH}:${PREFIX}/bin

NCPU=12 # make -j

BUILD_DIR = build

all: clean-prefix binutils gcc newlib clean-buildfiles
	@echo

create-dirs:
	mkdir -p ${BUILD_DIR}
	mkdir -p ${PREFIX}

binutils: create-dirs
	cd ${BUILD_DIR}; rm -rf build-binutils binutils-${BINUTILS_VER}
	wget -c "http://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.bz2"
	tar xf binutils-${BINUTILS_VER}.tar.bz2 -C ${BUILD_DIR}
	patch -p1 -d ${BUILD_DIR}/binutils-${BINUTILS_VER} < files/binutils.patch

	mkdir -p ${BUILD_DIR}/build-binutils
	cd ${BUILD_DIR}/build-binutils; ../binutils-${BINUTILS_VER}/configure --target=${TARGET} --prefix=${PREFIX}
	cd ${BUILD_DIR}/build-binutils; make -j${NCPU}; make install

gcc: create-dirs
	cd ${BUILD_DIR}; rm -rf build-gcc gcc-${GCC_VER}
	wget -c "http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-core-${GCC_VER}.tar.gz"
	tar xf gcc-core-${GCC_VER}.tar.gz -C ${BUILD_DIR}
	patch -p1 -d ${BUILD_DIR}/gcc-${GCC_VER} < files/gcc.patch
	cp files/gcc_makarnax.h ${BUILD_DIR}/gcc-${GCC_VER}/gcc/config/${OSNAME}.h

	mkdir -p ${BUILD_DIR}/build-gcc
	cd ${BUILD_DIR}/build-gcc; \
		../gcc-${GCC_VER}/configure --target=${TARGET} --prefix=${PREFIX} \
		--enable-languages=c --disable-nls --with-newlib;
	cd ${BUILD_DIR}/build-gcc; \
		make all-gcc all-target-libgcc -j${NCPU}; make install-gcc install-target-libgcc

newlib: create-dirs
	cd ${BUILD_DIR}; rm -rf build-newlib newlib-${NEWLIB_VER}
	wget -c "ftp://sources.redhat.com/pub/newlib/newlib-${NEWLIB_VER}.tar.gz"

	tar xf newlib-${NEWLIB_VER}.tar.gz -C ${BUILD_DIR}
	patch -p1 -d ${BUILD_DIR}/newlib-${NEWLIB_VER} < files/newlib.patch

	mkdir -p ${BUILD_DIR}/newlib-${NEWLIB_VER}/newlib/libc/sys/${OSNAME}
	cp files/newlib/* ${BUILD_DIR}/newlib-${NEWLIB_VER}/newlib/libc/sys/${OSNAME}

	cd ${BUILD_DIR}/newlib-${NEWLIB_VER}/newlib/libc/sys; autoconf; \
		cd ${OSNAME}; autoreconf;

	mkdir ${BUILD_DIR}/build-newlib
	cd ${BUILD_DIR}/build-newlib; \
		../newlib-${NEWLIB_VER}/configure --target=${TARGET} --prefix=${PREFIX}
	cd ${BUILD_DIR}/build-newlib; \
		make -j${NCPU} && make install

clean-buildfiles:
	rm -rf ${BUILD_DIR}

clean-prefix:
	rm -rf ${PREFIX}

clean: clean-buildfiles clean-prefix
	rm -f *.tar.*