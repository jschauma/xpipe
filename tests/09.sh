#! /bin/sh

. ./setup

NAME="split by literal '$' at the end of line"

begin

WANTED="$(grep -c '\$$' "${PATTERNFILE}")"
GOT="$(echo $(${XPIPE} -I -p '\$$' awk 'END { print }' <"${PATTERNFILE}" | wc -l))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
