#! /bin/sh

. ./setup

NAME="split by foo\\nbar"

begin

WANTED="$(egrep -c '(^bar|ends with foo)' "${PATTERNFILE}")"
GOT="$(${XPIPE} -I -p 'foo\nbar' cat <"${PATTERNFILE}" | egrep -c '(^bar|ends with foo)')"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
