#! /bin/sh

. ./setup

NAME="split by simple string at the end of line"

begin

WANTED="$(grep -c 'line 11$' "${PATTERNFILE}")"
GOT="$(echo $(${XPIPE} -I -p 'line 11$' sed -n -e '$p' <"${PATTERNFILE}" | wc -l))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
