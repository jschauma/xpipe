#! /bin/sh

. ./setup

NAME="split by literal \\t"

begin

WANTED="$(grep -c '\\tfoo' "${PATTERNFILE}")"
GOT="$(echo $(${XPIPE} -I -p '\\tfoo' sed -n -e '$p' <"${PATTERNFILE}"| wc -l))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
