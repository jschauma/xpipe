#! /bin/sh

. ./setup

NAME="'-c' behavior"
WANTED="$(grep -c '^a line' "${PATTERNFILE}")"

begin

<"${PATTERNFILE}" ${XPIPE} -I -c -p '^a line' notfound 2>/dev/null
GOT=$?
if [ x"${GOT}" != x"${WANTED}" ]; then
	echo "Expected exit value ${WANTED}, but got '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
