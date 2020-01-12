#! /bin/sh

. ./setup

NAME="exit values"
WANTED="127"

begin

note "Testing rval 127 on ENOENT..."
echo | ${XPIPE} -n 100 notfound >/dev/null 2>&1
GOT=$?
if [ x"${GOT}" != x"${WANTED}" ]; then
	echo "Expected exit value ${WANTED}, but got '${GOT}'." >> "${STDERR}"
	fail
fi

note "Testing rval 126 on exec eror..."
WANTED="126"
echo | ${XPIPE} -n 100 /etc/passwd >/dev/null 2>&1
GOT=$?
if [ x"${GOT}" != x"${WANTED}" ]; then
	echo "Expected exit value ${WANTED}, but got '${GOT}'." >> "${STDERR}"
	fail
fi

note "Testing rval 125 on signal..."
WANTED="125"

ln $(which sleep) "${TDIR}/sleep"
( echo | ${XPIPE} -n 100 "${TDIR}/sleep" 300 >/dev/null 2>&1 ) &

bgpid=$!

sleep 1
pkill -f  "^${TDIR}/sleep 300"

wait ${bgpid}

GOT=$?
if [ x"${GOT}" != x"${WANTED}" ]; then
	echo "Expected exit value ${WANTED}, but got '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
