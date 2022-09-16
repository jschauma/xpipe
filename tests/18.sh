#! /bin/sh

. ./setup

NAME="escaping * matches *"
WANTED="$(grep -c ' \* ' "${PATTERNFILE}")"

begin

<"${PATTERNFILE}" ${XPIPE} -I -c -p ' \* ' notfound 2>/dev/null
GOT=$?
if [ x"${GOT}" != x"${WANTED}" ]; then
	echo "Expected exit value ${WANTED}, but got '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
