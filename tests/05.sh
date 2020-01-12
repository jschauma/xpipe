#! /bin/sh

. ./setup

NAME="split by ^\$"

begin

WANTED="2 2 2 2 2 2 3 3 3 4 1"
GOT="$(echo $(${XPIPE} -p '^$' wc -l <patternfile | cut -f1))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
