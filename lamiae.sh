#!/usr/bin/env bash
# LAMIAE_DIR should refer to the directory in which sandbox is placed, the default should be good enough.
#LAMIAE_DIR=~/lamiae
#LAMIAE_DIR=/usr/local/lamiae
LAMIAE_DIR=$(dirname $(readlink -f $0))

LAMIAE_OPTIONS=""
LAMIAE_HOME="${HOME}/.lamiae"
LAMIAE_SUFFIX=".bin"
LAMIAE_EXEC=""
LAMIAE_PLATFORM=""
LAMIAE_MAKEFILE="Makefile"
forced=1

case $(uname -s) in
Linux)
    LAMIAE_PLATFORM="unix"
    ;;
FreeBSD)
    LAMIAE_PLATFORM="bsd"
    ;;
SunOS)
	LAMIAE_PLATFORM="sun"
	;;
*)
    LAMIAE_PLATFORM="unk"
    ;;
esac


case $(uname -m) in
i486|i586|i686)
	MACHINE_BIT=32
	;;
x86_64|amd64)
	MACHINE_BIT=64
	;;
*)
	MACHINE_BIT=64
	echo "Your platform ($(uname -m)) is unknown, assuming 64bit."
	echo "please file a bug report at https://github.com/Hirato/lamiae/issues"
	echo "\nPress Enter to continue"
	read -r
	;;
esac

while [ $# -ne 0 ]
do
	case $1 in
		"-h"|"-?"|"-help"|"--help")
			echo ""
			echo "Lamiae Launching Script"
			echo "Example: ./lamiae.sh --debug -t1"
			echo ""
			echo "   Script Arguments"
			echo "  -h|-?|-help|--help	show this help message"
			echo "  --force-32		forces use of 32bit executables on architectures other than i486, i586 and i686"
			echo "  --force-64		forces use of 64bit executables on architectures other than x86_64, and amd64"
			echo "			NOTE: your architecture ($(uname -m)) can be queried via \"uname -m\""
			echo "  --force-unix		forces use of Linux binaries when outside Linux"
			echo "  --force-bsd		forces use of the BSD binaries when outside BSD"
			echo "  --force-sun		forces use of SunOS binaries when outside SunOS"
			echo "			NOTE: your platform ($(uname -s)) can be queried via \"uname -s\""
			echo ""
			echo "  --debug		starts the debug build(s) inside GDB"
			echo "			note that all arguments passed to this script will be"
			echo "			passed to lamiae when 'run' is invokved in gdb."
			echo ""
			echo "   Engine Options"
			echo "  -u<string>		use <string> as the home directory (default: ${LAMIAE_HOME})"
			echo "  -k<string>		mounts <string> as a package directory"
			echo "  -f<num>		sets fullscreen to <num>"
			echo "  -d<num>		runs a dedicated server (0), or a listen server (1)"
			echo "  -w<num>		sets window width, height is set to width * 3 / 4 unless also provided"
			echo "  -h<num>		sets window height, width is set to height * 4 / 3 unless also provided"
			echo "  -v<num>		sets vsync to <num> -- -1 turns vsync on but allows tearing."
			echo "  -l<string>		loads map <string> after initialisation"
			echo "  -x<string>		executes script <string> after initialisation"
			echo ""
			echo "Script by Kevin \"Hirato Kirata\" Meyer"
			echo "(c) 2012-2015 - zlib/libpng licensed"
			echo ""

			exit 1
		;;
	esac

	tag=$(expr substr "$1" 1 2)
	argument=$(expr substr "$1" 3 1022)

	case $tag in
		"-u")
			LAMIAE_HOME="$argument"
		;;
		"--")
			case $argument in
			"force-32")
				MACHINE_BIT=32
				forced=0
			;;
			"force-64")
				MACHINE_BIT=64
				forced=0
			;;
			"force-unix")
				LAMIAE_PLATFORM="unix"
				forced=0
			;;
			"force-bsd")
				LAMIAE_PLATFORM="bsd"
				forced=0
			;;
			"force-sunos")
				LAMIAE_PLATFORM="sun"
				forced=0
			;;
			"debug")
				LAMIAE_SUFFIX=".dbg"
				LAMIAE_EXEC="gdb --args"
				LAMIAE_MAKEFILE="Makefile.debug"
			;;
			esac
		;;
		*)
			LAMIAE_OPTIONS+=" \"$tag$argument\""
		;;
	esac

	shift
done

function build {
	echo "${LAMIAE_DIR}/bin/${LAMIAE_PLATFORM}/lamiae${MACHINE_BIT}${LAMIAE_SUFFIX} does not exist"
	echo "Lamiae will attempt to compile one by executing the following command."
	echo "	make -C src -f ${LAMIAE_MAKEFILE} install"
	echo ""
	echo "Please make sure the SDL2, SDL2_image, SDL2_mixer, and zlib *Development* libraries are available."
	echo "Press Enter to proceed or Ctrl-C to abort."

	read -r
	make -C src -f ${LAMIAE_MAKEFILE} install
	if [ $? -ne 0 ]
	then
		echo "compilation failed"
		failed
	fi

	run 0
}

function failed {
	echo ""
	echo "\"${LAMIAE_DIR}/bin/${LAMIAE_PLATFORM}/lamiae${MACHINE_BIT}${LAMIAE_SUFFIX}\" does not exist and the program is unable to launch as a result."
	echo "This is typically due to there not being an available build for your system."
	echo ""
	echo "If you believe this is in error, try some combination of the --force flags or if not,"
	echo "make sure that you have the sdl, sdl-image, sdl-mixer, and zlib DEVELOPMENT libraries installed."
	echo "Then execute \"make -C ${LAMIAE_DIR}/src install\" before trying this script again."
	echo ""
	exit 1
}

function run {
	cd ${LAMIAE_DIR}
	if [ -a "bin/${LAMIAE_PLATFORM}/lamiae${MACHINE_BIT}${LAMIAE_SUFFIX}" ]
	then
		eval ${LAMIAE_EXEC} "./bin/${LAMIAE_PLATFORM}/lamiae${MACHINE_BIT}${LAMIAE_SUFFIX}" "-u${LAMIAE_HOME}" ${LAMIAE_OPTIONS}
	else
		if [ $1 -ne 1 ]
		then
			failed
		else
			build
		fi
	fi
}

run $forced
