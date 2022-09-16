#! /bin/sh

. ./setup

NAME=".* matches anything until the end of the line"

begin

<"${PATTERNFILE}" ${XPIPE} -I -J % -c -p 'ending.*' /bin/sh -c "cat >${TDIR}/%.out" 2>"${STDERR}"

WANTED="1.out
2.out
3.out
4.out
5.out"

cd ${TDIR}

GOT="$(ls *.out)"

if [ x"${WANTED}" != x"${GOT}" ]; then
	fail
fi

if ! tail -1 ${TDIR}/1.out | grep -q ending ; then
	echo "Last line in split file does not match wildcard." >> "${STDERR}"
	fail
fi

end

exit 0
