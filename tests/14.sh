#! /bin/sh

. ./setup

NAME="gzip example"

begin

WANTED="$(echo $(wc -l <"${CERTFILE}"))"

<"${CERTFILE}" ${XPIPE} -n 10 -J % /bin/sh -c "gzip >${TDIR}/%.gz"

GOT="$(echo $(gzip -d -c ${TDIR}/*.gz | wc -l))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
