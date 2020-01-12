#! /bin/sh

. ./setup

NAME="gzip example"

begin

WANTED="$(echo $(wc -l </var/log/messages))"

<"/var/log/messages" ${XPIPE} -n 100 -J % /bin/sh -c "gzip >${TDIR}/%.gz"

GOT="$(echo $(gzcat ${TDIR}/*.gz | wc -l))"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
