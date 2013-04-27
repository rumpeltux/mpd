#!/bin/sh
# You'll need to define the path to your toolchain
NDK=~/adt-bundle-linux-x86_64/android-ndk-r8d
ADB=~/adt-bundle-linux-x86_64/sdk/platform-tools/adb

set -e
MPD=$(realpath $(dirname $0))

# Set up the standalone toolchain (you can skip it if you already have it)
[ -d /tmp/ndk-$USER ] || $NDK/build/tools/make-standalone-toolchain.sh --system=linux-x86_64
cd /tmp/ndk-$USER
TC=/tmp/ndk-$USER/arm-linux-androideabi-4.6
[ -d $TC ] || (
    echo "Extracting toolchain"
    tar xjf arm-linux-androideabi-4.6.tar.bz2
)

export PATH=$TC/bin:$PATH
export PKG_CONFIG_PATH=$TC/sysroot/usr/lib/pkgconfig/
LIBDIR=$TC/sysroot/usr/lib

CONFIG_SUB="cp -av /usr/share/misc/config.sub ."
CONFIGURE="./configure --host=arm-linux-androideabi --prefix=/usr --with-sysroot=$TC/sysroot"
MAKE="make -j4"
INSTALL="make install DESTDIR=$TC/sysroot"
NDK_BUILD="$NDK/ndk-build -j4"


echo "Downloading dependency sources"
for url in \
    ftp://ftp.mars.org/pub/mpeg/libmad-0.15.1b.tar.gz \
    ftp://ftp.mars.org/pub/mpeg/libid3tag-0.15.1b.tar.gz \
    http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.3.tar.gz \
    http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz \
    http://downloads.xiph.org/releases/flac/flac-1.2.1.tar.gz \
    http://downloads.xiph.org/releases/opus/opus-1.0.2.tar.gz \
    http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.9.2.tar.gz
do
    NAME=`basename $url`
    [ -f $NAME ] || wget -nv $url
    tar xzf $NAME
done

echo "Compiling dependency libraries"

[ -f $LIBDIR/libid3tag.so ] || (
    cd libid3tag-0.15.1b/
    $CONFIG_SUB
    $CONFIGURE
    $MAKE
    $INSTALL
)

[ -f $LIBDIR/libmad.so ] || (
    cd libmad-0.15.1b/
    $CONFIG_SUB
    $CONFIGURE CFLAGS=-mandroid
    $MAKE
    $INSTALL
)

[ -f $LIBDIR/libogg.so ] || (
    cd libogg-1.3.0/
    $CONFIG_SUB
    $CONFIGURE
    $MAKE
    $INSTALL
)

[ -f $LIBDIR/libvorbis.so ] || (
    cd libvorbis-1.3.3/
    $CONFIG_SUB
    $CONFIGURE LIBS=-lm
    $MAKE
    $INSTALL
)

[ -f $LIBDIR/libopus.so ] || (
    cd opus-1.0.2/
    $CONFIG_SUB
    $CONFIGURE
    $MAKE
    $INSTALL
)

[ -f $LIBDIR/libiconv.so ] || (
    cd libiconv-1.9.2/
    cp -av /usr/share/misc/config.sub autoconf/
    cp -av /usr/share/misc/config.sub libcharset/autoconf/
    $CONFIGURE
    $MAKE
    $INSTALL
)

echo "Compiling glib"

[ -d glib ] || git clone https://github.com/ieei/glib.git
GLIBDIR=`pwd`/glib
[ -f $GLIBDIR/libs/armeabi/libglib-2.0.so ] || (
    cd glib
    git am --3way $MPD/glib-use-external-gnu-libiconv.patch
    $NDK_BUILD NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk \
        "LOCAL_EXPORT_C_INCLUDES=$TC/sysroot/usr/include" \
        "LOCAL_EXPORT_LDLIBS=-L$TC/sysroot/usr/lib/ -liconv"
)

echo "Compiling mpd"

[ -f $MPD/src/mpd ] || (
    cd $MPD
    $CONFIGURE CFLAGS=-mandroid --build=x86_64-pc-linux-gnu \
        "GLIB_CFLAGS=-I$GLIBDIR -I$GLIBDIR/glib -I$GLIBDIR/android" \
        "GLIB_LIBS=-L$GLIBDIR/libs/armeabi/ -lglib-2.0 -lgthread-2.0" \
        "OPUS_CFLAGS=-I$TC/sysroot/usr/include/opus"
    $MAKE
)

TARGET=/data/local/tmp
echo "Uploading binaries to android:$TARGET"
$ADB push $MPD/src/mpd $TARGET
$ADB push $GLIBDIR/libs/armeabi/libglib-2.0.so $TARGET
$ADB push $GLIBDIR/libs/armeabi/libgthread-2.0.so $TARGET

for sofile in libid3tag.so.0 libiconv.so.2 libmad.so.0 libogg.so.0 \
    libopus.so.0 libvorbisenc.so.2 libvorbisfile.so.3 libvorbis.so.0
do
    $ADB push $LIBDIR/$sofile $TARGET
done

echo "Compilation & Installation finished."
echo "Now run: adb shell"
echo "  cd $TARGET; LD_LIBRARY_PATH=. ./mpd --help"

