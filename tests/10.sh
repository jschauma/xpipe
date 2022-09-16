#! /bin/sh

. ./setup

NAME="split by tab"

begin

WANTED="$(echo $(grep -o '	' "${PATTERNFILE}" | wc -l))"
GOT="$(${XPIPE} -I -p '	' awk 'END { print }' <"${PATTERNFILE}"| grep -c '	')"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
