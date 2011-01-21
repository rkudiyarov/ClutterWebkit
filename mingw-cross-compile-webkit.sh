#!/bin/bash

# This script will download and setup a cross compilation environment
# for targetting Win32 from Linux. It can also be used to build on
# Windows under the MSYS/MinGW environment. It will use the GTK
# binaries from Tor Lillqvist.

TOR_URL="http://ftp.gnome.org/pub/gnome/binaries/win32";

TOR_BINARIES=( \
    gdk-pixbuf/2.22/gdk-pixbuf{-dev,}_2.22.0-1_win32.zip \
    glib/2.26/glib{-dev,}_2.26.0-2_win32.zip \
    gtk+/2.22/gtk+{-dev,}_2.22.0-2_win32.zip \
    pango/1.28/pango{-dev,}_1.28.3-1_win32.zip \
    atk/1.32/atk{-dev,}_1.32.0-1_win32.zip \
    libsoup/2.26/libsoup{-dev,}_2.26.3-1_win32.zip \
    );

TOR_DEP_URL="http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies";

TOR_DEPS=( \
    cairo{-dev,}_1.10.0-2_win32.zip \
    gettext-runtime{-dev,}_0.18.1.1-2_win32.zip \
    fontconfig{-dev,}_2.8.0-2_win32.zip \
    freetype{-dev,}_2.4.2-1_win32.zip \
    expat{-dev,}_2.0.1-1_win32.zip \
    libpng{-dev,}_1.4.3-1_win32.zip \
    zlib{-dev,}_1.2.5-2_win32.zip \
    enchant{-dev,}_1.5.0-2_win32.zip \
    jpeg{-dev,}_8-1_win32.zip \
    sqlite3{-dev,}_3.6.0_win32.zip \
    gnutls{-dev,}_2.4.2-1_win32.zip \
    libxml2{-dev,}_2.7.7-1_win32.zip \
    libiconv-1.9.1.bin.woe32.zip
    );

GL_HEADER_URLS=( \
    http://cgit.freedesktop.org/mesa/mesa/plain/include/GL/gl.h \
    http://cgit.freedesktop.org/mesa/mesa/plain/include/GL/mesa_wgl.h \
    http://www.opengl.org/registry/api/glext.h );

GL_HEADERS=( gl.h mesa_wgl.h glext.h );

CLUTTER_SOURCES="http://source.clutter-project.org/sources/clutter/1.4/clutter-1.4.2.tar.bz2"
#GPG_ERROR_SOURCES="ftp://ftp.gnupg.org/gcrypt/libgpg-error/libgpg-error-1.8.tar.bz2"
#GCRYPT_SOURCES="ftp://ftp.gnupg.org/gcrypt/libgcrypt/libgcrypt-1.4.6.tar.bz2"
#GNUTLS_SOURCES="http://ftp.gnu.org/gnu/gnutls/gnutls-2.10.3.tar.bz2"
SOUP_SOURCES="http://ftp.gnome.org/pub/gnome/sources/libsoup/2.32/libsoup-2.32.1.tar.bz2"
ICU_LIBS="http://download.icu-project.org/files/icu4c/4.6/icu4c-4_6-Win32-msvc10.zip"

function download_file ()
{
    local url="$1"; shift;
    local filename="$1"; shift;
    
    if [ -f "$DOWNLOAD_DIR/$filename" ]; then
    echo "File ${filename} already exists in $DOWNLOAD_DIR. Skipping download.";
    return 0;
    fi;

    case "$DOWNLOAD_PROG" in
	curl)
	    curl -C - -o "$DOWNLOAD_DIR/$filename" "$url";
	    ;;
	*)
	    $DOWNLOAD_PROG -O "$DOWNLOAD_DIR/$filename" -c "$url";
	    ;;
    esac;

    if [ $? -ne 0 ]; then
	echo "Downloading ${url} failed.";
	exit 1;
    fi;
}

function guess_dir ()
{
    local var="$1"; shift;
    local suffix="$1"; shift;
    local msg="$1"; shift;
    local prompt="$1"; shift;
    local dir="${!var}";

    if [ -z "$dir" ]; then
	echo "Please enter ${msg}.";
	dir="$PWD/$suffix";
	read -r -p "$prompt [$dir] ";
	if [ -n "$REPLY" ]; then
	    dir="$REPLY";
	fi;
    fi;

    eval $var="\"$dir\"";

    if [ ! -d "$dir" ]; then
	if ! mkdir -p "$dir"; then
	    echo "Error making directory $dir";
	    exit 1;
	fi;
    fi;
}

function y_or_n ()
{
    local prompt="$1"; shift;

    while true; do
	read -p "${prompt} [y/n] " -n 1;
	echo;
	case "$REPLY" in
	    y) return 0 ;;
	    n) return 1 ;;
	    *) echo "Please press y or n" ;;
	esac;
    done;
}

function do_unzip ()
{
    do_unzip_d "$ROOT_DIR" "$@";
}

function do_unzip_d ()
{
    local exdir="$1"; shift;
    local zipfile="$1"; shift;

    unzip -o -q -d "$exdir" "$zipfile" "$@";

    if [ "$?" -ne 0 ]; then
	echo "Failed to extract $zipfile";
	exit 1;
    fi;
}

function add_env ()
{
    echo "export $1=\"$2\"" >> $env_file;
}

function find_compiler ()
{
    local gccbin fullpath;

    if [ -z "$MINGW_TOOL_PREFIX" ]; then
	for gccbin in i{3,4,5,6}86-mingw32{,msvc}-gcc; do
	    fullpath="`which $gccbin 2>/dev/null`";
	    if [ "$?" -eq 0 ]; then
		MINGW_TOOL_PREFIX="${fullpath%%gcc}";
		break;
	    fi;
	done;
	if [ -z "$MINGW_TOOL_PREFIX" ]; then
	    echo;
	    echo "No suitable cross compiler was found.";
	    echo;
	    echo "If you already have a compiler installed,";
	    echo "please set the MINGW_TOOL_PREFIX variable";
	    echo "to point to its location without the";
	    echo "gcc suffix (eg: \"/usr/bin/i386-mingw32-\").";
	    echo;
	    echo "If you are using Ubuntu, you can install a";
	    echo "compiler by typing:";
	    echo;
	    echo " sudo apt-get install mingw32";
	    echo;
	    echo "Otherwise you can try following the instructions here:";
	    echo;
	    echo " http://www.libsdl.org/extras/win32/cross/README.txt";

	    exit 1;
	fi;
    fi;

    CC="${MINGW_TOOL_PREFIX}gcc";
    add_env ADDR2LINE "${MINGW_TOOL_PREFIX}addr2line"
    add_env AS "${MINGW_TOOL_PREFIX}as"
    add_env CC "${CC}"
    add_env CPP "${MINGW_TOOL_PREFIX}cpp"
    add_env CPPFILT "${MINGW_TOOL_PREFIX}c++filt"
    add_env CXX "${MINGW_TOOL_PREFIX}g++"
    add_env DLLTOOL "${MINGW_TOOL_PREFIX}dlltool"
    add_env DLLWRAP "${MINGW_TOOL_PREFIX}dllwrap"
    add_env GCOV "${MINGW_TOOL_PREFIX}gcov"
    add_env LD "${MINGW_TOOL_PREFIX}ld"
    add_env NM "${MINGW_TOOL_PREFIX}nm"
    add_env OBJCOPY "${MINGW_TOOL_PREFIX}objcopy"
    add_env OBJDUMP "${MINGW_TOOL_PREFIX}objdump"
    add_env READELF "${MINGW_TOOL_PREFIX}readelf"
    add_env SIZE "${MINGW_TOOL_PREFIX}size"
    add_env STRINGS "${MINGW_TOOL_PREFIX}strings"
    add_env WINDRES "${MINGW_TOOL_PREFIX}windres"
    add_env AR "${MINGW_TOOL_PREFIX}ar"
    add_env RANLIB "${MINGW_TOOL_PREFIX}ranlib"
    add_env STRIP "${MINGW_TOOL_PREFIX}strip"

    TARGET="${MINGW_TOOL_PREFIX##*/}";
    TARGET="${TARGET%%-}";

    echo "Using compiler $CC and target $TARGET";
}

# If a download directory hasn't been specified then try to guess one
# but ask for confirmation first
guess_dir DOWNLOAD_DIR "downloads" \
    "the directory to download to" "Download directory";

# Try to guess a download program if none has been specified
if [ -z "$DOWNLOAD_PROG" ]; then
    # If no download program has been specified then check if wget or
    # curl exists
    #wget first, because my curl can't download libsdl...
    for x in wget curl; do
	if [ "`type -t $x`" != "" ]; then
	    DOWNLOAD_PROG="$x";
	    break;
	fi;
    done;

    if [ -z "$DOWNLOAD_PROG" ]; then
	echo "No DOWNLOAD_PROG was set and neither wget nor curl is ";
	echo "available.";
	exit 1;
    fi;
fi;

# If a download directory hasn't been specified then try to guess one
# but ask for confirmation first
guess_dir ROOT_DIR "clutter-cross" \
    "the root prefix for the build environment" "Root dir";
SLASH_SCRIPT='s/\//\\\//g';
quoted_root_dir=`echo "$ROOT_DIR" | sed "$SLASH_SCRIPT" `;

if y_or_n "Do you want to clean $ROOT_DIR?"; then
    rm -rf "$ROOT_DIR/"* "$ROOT_DIR/".??*;
    if [ "$?" -ne 0 ]; then
        echo "Can't clean $ROOT_DIR";
        exit 1;
    fi;
fi;

##
# Download files
##

if y_or_n "Do you want to download dependencies?"; then
    for bin in "${TOR_BINARIES[@]}"; do
        bn="${bin##*/}";
        download_file "$TOR_URL/$bin" "$bn"
    done;

    for dep in "${TOR_DEPS[@]}"; do
        download_file "$TOR_DEP_URL/$dep" "$dep";
    done;

    for dep in "${OTHER_DEPS[@]}"; do
        bn="${dep##*/}";
        download_file "$dep" "$bn";
    done;

    for dep in "${GL_HEADER_URLS[@]}"; do
        bn="${dep##*/}";
        download_file "$dep" "$bn";
    done;
fi;

##
# Extract files
##

if y_or_n "Do you want to extract dependencies?"; then
    for bin in "${TOR_BINARIES[@]}"; do
        echo "Extracting $bin...";
        bn="${bin##*/}";
        do_unzip "$DOWNLOAD_DIR/$bn";
    done;

    for dep in "${TOR_DEPS[@]}"; do
        echo "Extracting $dep...";
        do_unzip "$DOWNLOAD_DIR/$dep";
    done;

    echo "Fixing pkgconfig files...";
    for x in "$ROOT_DIR/lib/pkgconfig/"*.pc; do
        sed "s/^prefix=.*\$/prefix=${quoted_root_dir}/" \
      < "$x" > "$x.tmp";
        mv "$x.tmp" "$x";
    done;

    # The Pango FT pc file hardcodes the include path for freetype, so it
    # needs to be fixed separately
    sed -e 's/^Cflags:.*$/Cflags: -I${includedir}\/pango-1.0 -I${includedir}\/freetype2/' \
        -e 's/^\(Libs:.*\)$/\1 -lfreetype -lfontconfig/' \
        < "$ROOT_DIR/lib/pkgconfig/pangoft2.pc" \
        > "$ROOT_DIR/lib/pkgconfig/pangoft2.pc.tmp";
    mv "$ROOT_DIR/lib/pkgconfig/pangoft2.pc"{.tmp,};

    chmod a+x "$ROOT_DIR/bin/libgcrypt-config";

    echo "Copying GL headers...";
    if ! ( test -d "$ROOT_DIR/include/GL" || \
        mkdir "$ROOT_DIR/include/GL" ); then
        echo "Failed to create GL header directory";
        exit 1;
    fi;
    for header in "${GL_HEADERS[@]}"; do
        if ! cp "$DOWNLOAD_DIR/$header" "$ROOT_DIR/include/GL/"; then
            echo "Failed to copy $header";
            exit 1;
        fi;
    done;
fi;

##
# Build
##

env_file="$ROOT_DIR/share/env.sh";
echo "Writing build environment script to $env_file";
echo "#!/bin/bash" > "$env_file";

find_compiler;

add_env PKG_CONFIG_PATH "$ROOT_DIR/lib/pkgconfig:\$PKG_CONFIG_PATH";

add_env LDFLAGS "-L$ROOT_DIR/lib -mno-cygwin \$LDFLAGS"
add_env CPPFLAGS "-I$ROOT_DIR/include \$CPPFLAGS"
add_env CFLAGS "-I$ROOT_DIR/include -mno-cygwin -mms-bitfields -march=i686 \${CFLAGS:-"-g"}"
add_env CXXFLAGS "-I$ROOT_DIR/include -mno-cygwin -mms-bitfields -march=i686 \${CFLAGS:-"-g"}"

cat >> "$env_file" <<EOF
ROOT_DIR="$ROOT_DIR";
TARGET="$TARGET";

function do_json_glib_autogen()
{
  ./autogen.sh --prefix="\$ROOT_DIR" --host="\$TARGET" --target="\$TARGET" \\
    --disable-introspection;
}

function do_clutter_autogen()
{
  ./configure --prefix="\$ROOT_DIR" --host="\$TARGET" --target="\$TARGET" \\
    --with-flavour=win32 --enable-conformance=no --disable-introspection;
}

function do_soup_autogen()
{
  ./configure --prefix="\$ROOT_DIR" --host="\$TARGET" --target="\$TARGET" \\
    --without-gnome --enable-introspection=no --disable-glibtest --with-libgcrypt-prefix="\$ROOT_DIR"
}

function build_webkit()
{
  ./WebKitTools/Scripts/build-webkit --prefix="\$ROOT_DIR" --host="\$TARGET" --target="\$TARGET" \\
    --clutter --minimal --debug
}

# If any arguments are given then execute it as a program with the
# environment we set up

if test "\$#" -ge 1; then
    exec "\$@";
fi;

EOF

chmod a+x "$env_file";

source "$env_file";
if y_or_n "Do you want to build Clutter?"; then
    guess_dir JSON_GLIB_BUILD_DIR "json-glib" \
    "the build directory for json-glib" "Build dir";
    
    if y_or_n "git clone json-glib?"; then
        rm -rf "$JSON_GLIB_BUILD_DIR/"* "$JSON_GLIB_BUILD_DIR/".*
        git clone git://git.gnome.org/json-glib $JSON_GLIB_BUILD_DIR;
        if [ "$?" -ne 0 ]; then
        echo "git failed";
        exit 1;
        fi;
    fi;
    
    if y_or_n "Build json-glib?"; then
        ( cd "$JSON_GLIB_BUILD_DIR" && do_json_glib_autogen );
        if [ "$?" -ne 0 ]; then
    	echo "autogen failed";
    	exit 1;
        fi;
        ( cd "$JSON_GLIB_BUILD_DIR" && make all );
        if [ "$?" -ne 0 ]; then
    	echo "make failed";
    	exit 1;
        fi;
    fi
    
    if y_or_n "Install json-glib?"; then
        ( cd "$JSON_GLIB_BUILD_DIR" && make install );
        if [ "$?" -ne 0 ]; then
    	echo "make install failed";
    	exit 1;
        fi;
    fi

#    guess_dir CLUTTER_BUILD_DIR "clutter" \
#	"the build directory for clutter" "Build dir";
	
	if y_or_n "Download clutter?"; then
	    bn="${CLUTTER_SOURCES##*/}";
        download_file "$CLUTTER_SOURCES" "$bn";
        ( cd "$PWD" && tar jxf "$DOWNLOAD_DIR/$bn" );
    fi;
    
    if y_or_n "Build clutter?"; then
        CLUTTER_BUILD_DIR="$PWD/clutter-1.4.2"
        
    #    git clone "$CLUTTER_GIT/clutter" $CLUTTER_BUILD_DIR;
    #    if [ "$?" -ne 0 ]; then
    #    echo "git failed";
    #    exit 1;
    #    fi;
        ( cd "$CLUTTER_BUILD_DIR" && do_clutter_autogen );
        if [ "$?" -ne 0 ]; then
    	echo "autogen failed";
    	exit 1;
        fi;
        ( cd "$CLUTTER_BUILD_DIR" && make all install );
        if [ "$?" -ne 0 ]; then
    	echo "make failed";
    	exit 1;
        fi;
    fi;
fi;

if y_or_n "Download and unpack libsoup?"; then
    for f in "$SOUP_SOURCES"; do
        bn="${f##*/}";
        download_file "$f" "$bn";
        ( cd "$PWD" && tar jxf "$DOWNLOAD_DIR/$bn" );
    done;
fi;

if y_or_n "Do you want to build libsoup?"; then
    #     GPG_ERROR_BUILD_DIR="$PWD/libgpg-error-1.8"
    #     ( cd "$GPG_ERROR_BUILD_DIR" && do_gpg_error_autogen );
    #     if [ "$?" -ne 0 ]; then
    # echo "autogen failed";
    # exit 1;
    #     fi;
    #     ( cd "$GPG_ERROR_BUILD_DIR" && make all install );
    #     if [ "$?" -ne 0 ]; then
    # echo "make failed";
    # exit 1;
    #     fi;
    #     
    #     GCRYPT_BUILD_DIR="$PWD/libgcrypt-1.4.6"
    #     ( cd "$GCRYPT_BUILD_DIR" && do_gcrypt_autogen );
    #     if [ "$?" -ne 0 ]; then
    # echo "autogen failed";
    # exit 1;
    #     fi;
    #     ( cd "$GCRYPT_BUILD_DIR" && make all install );
    #     if [ "$?" -ne 0 ]; then
    # echo "make failed";
    # exit 1;
    #     fi;
    #     
    #     GNUTLS_BUILD_DIR="$PWD/gnutls-2.10.3"
    #     ( cd "$GNUTLS_BUILD_DIR" && do_gnutls_autogen );
    #     if [ "$?" -ne 0 ]; then
    # echo "autogen failed";
    # exit 1;
    #     fi;
    #     ( cd "$GNUTLS_BUILD_DIR" && make all install );
    #     if [ "$?" -ne 0 ]; then
    # echo "make failed";
    # exit 1;
    #     fi;
    
    SOUP_BUILD_DIR="$PWD/libsoup-2.32.1"
    ( cd "$SOUP_BUILD_DIR" && do_soup_autogen );
    if [ "$?" -ne 0 ]; then
	echo "autogen failed";
	exit 1;
    fi;
    ( cd "$SOUP_BUILD_DIR" && make all install );
    if [ "$?" -ne 0 ]; then
	echo "make failed";
	exit 1;
    fi;
fi;

if y_or_n "Do you want to download and install ICU?"; then
    bn="${ICU_LIBS##*/}";
    download_file "$ICU_LIBS" "$bn";
    ( cd "$PWD" && do_unzip_d "$PWD" "$DOWNLOAD_DIR/$bn" );
    
    if ! cp -R "$PWD"/icu/bin/* "$ROOT_DIR"/bin/; then
        echo "Failed to copy ICU binaries";
        exit 1;
    fi;
    if ! cp -R "$PWD"/icu/include/* "$ROOT_DIR"/include/; then
        echo "Failed to copy ICU headers";
        exit 1;
    fi;
    
    git clone git://github.com/crystalnix/pexports-0.44.git pexports-0.44;
    if [ "$?" -ne 0 ]; then
    echo "git failed";
    exit 1;
    fi;
    
    CFLAGS_TEMP="$CFLAGS";
    CPPFLAGS_TEMP="$CPPFLAGS";
    unset CFLAGS;
    unset CPPFLAGS;
    (cd pexports-0.44 && make all && cd "$PWD");
    if [ "$?" -ne 0 ]; then
    echo "pexports make failed";
    exit 1;
    fi;
    export CFLAGS="$CFLAGS_TEMP"
    export CPPFLAGS="$CPPFLAGS_TEMP"
    
    for icu_dll in "$PWD"/icu/bin/*.dll; do
        dll_basename=`basename $icu_dll`;
        pexports-0.44/pexports.exe $icu_dll > ${icu_dll%.dll}.def;
        if [ "$?" -ne 0 ]; then
        echo "pexports failed";
        exit 1;
        fi;
        $DLLTOOL --def ${icu_dll%.dll}.def --dllname $icu_dll --output-lib "$ROOT_DIR/lib/$dll_basename.a";
        if [ "$?" -ne 0 ]; then
        echo "$DLLTOOL failed";
        exit 1;
        fi;
    done;    
fi;


if y_or_n "Do you want to build ClutterWebkit?"; then
    guess_dir WEBKIT_BUILD_DIR "ClutterWebkit" \
        "the directory where WebKit sources are located" "WebKit directory";
    ( cd "$WEBKIT_BUILD_DIR" && build_webkit );
fi;