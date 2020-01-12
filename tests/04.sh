#! /bin/sh

. ./setup

NAME="split by simple pattern"

begin

WANTED="an empty"
GOT="$(head -1 "${PATTERNFILE}" | ${XPIPE} -I -p empty cat)"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
