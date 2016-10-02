SUBDIR= modules sbin

.include <bsd.subdir.mk>

MAKEOBJDIRPREFIX=/tmp

TOOLSDIR=	${.CURDIR}/tools
TESTSDIR=	${.CURDIR}/tests
RUNDIR=		${.CURDIR}/run
JAILDIR=	${RUNDIR}/jails
DISTSITE=	ftp://ftp.freebsd.org/pub/FreeBSD/releases
DISTDIR=	${RUNDIR}/distfiles
DISTS=	 	10.3-RELEASE
ARCHS=		amd64
FILES=		base.txz src.txz kernel.txz

test: jails
	echo "testing.."
cleanjails:
	rm -rf ${JAILDIR}

jails: distfiles
	JAILDIR="${JAILDIR}" DISTS="${DISTS}" ARCHS="${ARCHS}" FILES="${FILES}" DISTDIR="${DISTDIR}" ${TOOLSDIR}/makejails

distfiles: 
	DISTS="${DISTS}" ARCHS="${ARCHS}" FILES="${FILES}" DISTSITE="${DISTSITE}" DISTDIR="${DISTDIR}" ${TOOLSDIR}/fetchdists

