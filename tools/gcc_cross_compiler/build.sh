#!/bin/bash

######################
# configuration
######################
OSNAME=makarnax
BINUTILS_VER=2.22
GCC_VER=4.6.3
NEWLIB_VER=1.20.0
TARGET=i686-${OSNAME}

NCPU=8 # make -j


#######################
#
#######################

export TARGET
if [ -z $PREFIX ]
then
	mkdir -p local
	export PREFIX=`pwd`/local
fi

echo "OSNAME="$OSNAME
echo "TARGET="$TARGET
echo "PREFIX="$PREFIX
echo "NCPU="$NCPU

export PATH=$PREFIX/bin:$PATH
echo "PATH="$PATH


###################################
# programlari indir
###################################
wget -c http://ftp.gnu.org/gnu/binutils/binutils-${BINUTILS_VER}.tar.bz2
wget -c http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VER}/gcc-core-${GCC_VER}.tar.gz
wget -c ftp://sources.redhat.com/pub/newlib/newlib-${NEWLIB_VER}.tar.gz


rm -rf build && mkdir build && cd build || exit


#################################
# binutils
################################

echo "BINUTILS-"$BINUTILS_VER
tar xf ../binutils-${BINUTILS_VER}.tar.bz2
patch -p1 -d binutils-${BINUTILS_VER} < ../files/binutils.patch || exit

mkdir -p build-binutils && cd build-binutils || exit
../binutils-${BINUTILS_VER}/configure --target=$TARGET --prefix=$PREFIX || exit
make -j12 && make install || exit
cd ..


#################################
# gcc
################################

echo "GCC"
tar xf ../gcc-core-${GCC_VER}.tar.gz
patch -p1 -d gcc-${GCC_VER} < ../files/gcc.patch || exit
cp ../files/gcc_makarnax.h gcc-${GCC_VER}/gcc/config/${OSNAME}.h || exit

mkdir -p build-gcc && cd build-gcc || exit
../gcc-${GCC_VER}/configure --target=$TARGET --prefix=$PREFIX \
	--enable-languages=c --disable-nls --with-newlib || exit
make all-gcc all-target-libgcc -j12 && make install-gcc install-target-libgcc || exit
cd ..


#################################
# newlib
################################

echo "NEWLIB"
tar xf ../newlib-${NEWLIB_VER}.tar.gz
patch -p1 -d newlib-${NEWLIB_VER} < ../files/newlib.patch || exit

mkdir -p newlib-${NEWLIB_VER}/newlib/libc/sys/${OSNAME}
cp ../files/newlib/* newlib-${NEWLIB_VER}/newlib/libc/sys/${OSNAME} || exit

cd newlib-${NEWLIB_VER}/newlib/libc/sys && autoconf && \
	cd ${OSNAME} && autoreconf && cd ../../../../.. || exit

mkdir build-newlib && cd build-newlib || exit
../newlib-${NEWLIB_VER}/configure --target=$TARGET --prefix=$PREFIX || exit

make -j12 && make install || exit
cd ..


##################################
#
##################################

mv -f $PREFIX/$TARGET/lib/* $PREFIX/lib || exit
mv -f $PREFIX/$TARGET/include/* $PREFIX/include || exit
rm -rf $PREFIX/$TARGET || exit
ln -s $PREFIX $PREFIX/$TARGET || exit
