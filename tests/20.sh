#! /bin/sh

. ./setup

NAME=".* matches anything until the next character"

begin

<"${PATTERNFILE}" ${XPIPE} -I -J % -c -p 'e.*g' /bin/sh -c "cat >${TDIR}/%.out" 2>"${STDERR}"

WANTED="12.out"

cd ${TDIR}

GOT="$(ls -rt *.out | tail -1)"

if [ x"${WANTED}" != x"${GOT}" ]; then
	fail
fi

if ! tail -1 ${TDIR}/1.out | grep -q 'e.*g'; then
	echo "Last line in split file does not match wildcard." >> "${STDERR}"
	fail
fi

end

exit 0
