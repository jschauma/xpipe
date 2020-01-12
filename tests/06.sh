#! /bin/sh

. ./setup

NAME="split by beginning of line"

begin

WANTED="$(grep -c '^a line' "${PATTERNFILE}")"
GOT="$(echo $(${XPIPE} -I -p '^a line' awk 'END { print }' <"${PATTERNFILE}" | wc -l))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
