.PATH: /usr/src/sbin/geom/misc

MAKEOBJDIRPREFIX=/tmp
BINDIR?=    /sbin

GEOM_CLASS_DIR?=    /lib/geom
GEOM_CLASS=         dedupe

SHLIBDIR?=      ${GEOM_CLASS_DIR}
SHLIB_NAME?=    geom_${GEOM_CLASS}.so

SRCS+=	    geom_${GEOM_CLASS}.c subr.c
LINKS=	    ${BINDIR}/geom ${BINDIR}/g${GEOM_CLASS}
CFLAGS+=    -I/usr/src/sbin/geom

.include <bsd.lib.mk>

