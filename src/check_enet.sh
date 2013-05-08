#!/bin/sh
# ENet cflags detection for unix by Daniel 'q66' Kolesa <quaker66@gmail.com>
# I hereby put this file into public domain, use as you wish

CC=$1
CFLAGS=$2
while [ "$3" != "" ]; do CFLAGS=$CFLAGS$3; shift; done

cat << EOF > check_func.c
void TEST_FUN();
int main() { TEST_FUN(); return 0; }
EOF
cat << EOF > check_member.c
#include "check_member.h"
static void pass() {}
int main() { struct TEST_STRUCT test; pass(test.TEST_FIELD); return 0; }
EOF
cat << EOF > check_type.c
#include "check_type.h"
int main() { TEST_TYPE test; return 0; }
EOF

CHECK_FUNC() {
    $CC $CFLAGS check_func.c -DTEST_FUN=$1 -o check_func 2>/dev/null
    if [ $? -eq 0 ]; then echo -n " $2"; rm check_func; fi
}
CHECK_MEMBER() {
    echo -e "$1" > check_member.h
    $CC $CFLAGS check_member.c -DTEST_STRUCT=$2 -DTEST_FIELD=$3 \
        -o check_member 2>/dev/null
    if [ $? -eq 0 ]; then echo -n " $4"; rm check_member; fi
    rm check_member.h
}
CHECK_TYPE() {
    echo -e "$1" > check_type.h
    $CC $CFLAGS check_type.c -DTEST_TYPE=$2 -o check_type 2>/dev/null
    if [ $? -eq 0 ]; then echo -n " $3"; rm check_type; fi
    rm check_type.h
}

CHECK_FUNC gethostbyaddr_r -DHAS_GETHOSTBYADDR_R
CHECK_FUNC gethostbyname_r -DHAS_GETHOSTBYNAME_R
CHECK_FUNC poll -DHAS_POLL
CHECK_FUNC fcntl -DHAS_FCNTL
CHECK_FUNC inet_pton -DHAS_INET_PTON
CHECK_FUNC inet_ntop -DHAS_INET_NTOP

CHECK_MEMBER "#include <sys/socket.h>" msghdr msg_flags -DHAS_MSGHDR_FLAGS
CHECK_TYPE "#include <sys/types.h>\n#include <sys/socket.h>" socklen_t \
    -DHAS_SOCKLEN_T

echo ''
rm check_func.c
rm check_member.c
rm check_type.c
