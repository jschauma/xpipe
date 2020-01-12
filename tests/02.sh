#! /bin/sh

. ./setup

NAME="split by lines"

begin

<"${LINEFILE}" ${XPIPE} -J % -n 20 /bin/sh -c "cat >${TDIR}/%.out"

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
	s=$(echo $(wc -l <"${f}"))
	if [ x"${s}" != x"20" ]; then
		echo "File ${f} is ${s} lines." >"${STDERR}"
		fail
	fi
done

end

exit 0
