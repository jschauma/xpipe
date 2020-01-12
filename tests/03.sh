#! /bin/sh

. ./setup

NAME="split by lines ignoring lefovers"

begin

<"${LINEFILE}" ${XPIPE} -I -J % -n 33 /bin/sh -c "cat >${TDIR}/%.out"

WANTED="1.out
2.out
3.out"

cd ${TDIR}

GOT="$(ls *.out)"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output files." >"${STDERR}"
	echo "Wanted:" >> "${STDERR}"
	echo "${WANTED}" >> "${STDERR}"
	echo "Got:" >> "${STDERR}"
	echo "${GOT}" >> "${STDERR}"
	fail
fi

for f in ${GOT}; do
	s=$(echo $(wc -l <"${f}"))
	if [ x"${s}" != x"33" ]; then
		echo "File ${f} is ${s} lines." >"${STDERR}"
		fail
	fi
done

end

exit 0
