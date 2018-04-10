ARMV7 - Linux

export NDK_PATH=/home/user/SDKS/ANDROID/NDK/android-ndk-r13b
export ANDROID_DEV=$NDK_PATH/platforms/android-9/arch-arm/usr
export AR=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-ar
export AS=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-as
export CC=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc
export CFLAGS=--sysroot=$NDK_PATH/platforms/android-9/arch-arm/
export CPP=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-cpp
export CPPFLAGS=--sysroot=$NDK_PATH/platforms/android-9/arch-arm/
export CXX=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-g++
export CXXFLAGS="--sysroot=$NDK_PATH/platforms/android-9/arch-arm/ -I$NDK_PATH/sources/cxx-stl/gnu-libstdc++/4.9/include -I$NDK_PATH/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a/include"
export LD=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-ld
export NDK_PROJECT_PATH=$NDK_PATH
export RANLIB=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-ranlib
export PATH=$NDK_PATH/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/:$PATH


2. Dependencies

2.1 OpenSSL 
cd /path/to/libraries/take3
wget -t0 https://www.openssl.org/source/openssl-1.0.2l.tar.gz
tar xzvf openssl-1.0.2l.tar.gz -C .
cd openssl-1.0.2l
./Configure no-shared no-dso android-armv7
make

2.2 Berkeley DB
cd /path/to/libraries/take3
wget -t0 http://download.oracle.com/berkeley-db/db-6.0.20.tar.gz
tar xzvf db-6.0.20.tar.gz -C .
cd db-6.0.20/build_unix
../dist/configure --host=arm-linux-androideabi --enable-cxx --enable-shared --disable-replication
make

2.3 Boost
-Download http://sourceforge.net/projects/boost/files/boost/1.57.0/boost_1_57_0.7z/download
7z x ~/Downloads/boost_1_57_0.7z -o/path/to/libraries/take3/
cd /path/to/libraries/take3/boost_1_57_0
./bootstrap.sh

edit boost_1_57_0\project-config.jam file and append the text bellow:
import option ;
using gcc : arm : arm-linux-androideabi-g++ ;
option.set keep-going : false ; 

save the file and enter the command bellow:

./b2 --layout=versioned --build-type=complete --with-chrono --with-filesystem --with-program_options --with-system --with-thread toolset=gcc-arm variant=release link=static threading=multi threadapi=pthread target-os=android define=BOOST_MATH_DISABLE_FLOAT128 include=$NDK_PATH/sources/cxx-stl/gnu-libstdc++/4.9/include include=$NDK_PATH/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi/include include=$NDK_PATH/platforms/android-9/arch-arm/usr/include

Check folder /path/to/libraries/take3/boost_1_57_0/stage/lib to verify that files bellow exists:
libboost_atomic-gcc-mt-s-1_57.a
libboost_chrono-gcc-mt-s-1_57.a
libboost_filesystem-gcc-mt-s-1_57.a
libboost_program_options-gcc-mt-s-1_57.a
libboost_system-gcc-mt-s-1_57.a
libboost_thread_pthread-gcc-mt-s-1_57.a

2.4 LevelDB
cd git/gostcoin
cd src/leveldb
TARGET_OS=OS_ANDROID_CROSSCOMPILE make libleveldb.a libmemenv.a

2.5 android-ifaddr
https://github.com/GOSTSec/android-ifaddrs-from-android-source   

3. Build
3.1 Start Qt.pro

Open project beonpush-qt.pro in Qt-Creator  

Build and run the project from Qt-Creator  
