#! /bin/sh

. ./setup

NAME="split by ^ at the end of line"

begin

WANTED="$(grep -c '\^$' "${PATTERNFILE}")"
GOT="$(echo $(${XPIPE} -I -p '\^$' sed -n -e '$p' <"${PATTERNFILE}" | wc -l))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
