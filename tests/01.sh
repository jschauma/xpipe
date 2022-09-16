#! /bin/sh

. ./setup

NAME="split by bytes"

begin

<"${BYTEFILE}" ${XPIPE} -J % -b 100 /bin/sh -c "cat >${TDIR}/%.out" 2>"${STDERR}"

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

for f in ${GOT}; do
	s=$(${STAT_S} ${f})
	if [ x"${s}" != x"100" ]; then
		echo "File ${f} is ${s} bytes." >"${STDERR}"
	fi
done

end

exit 0
