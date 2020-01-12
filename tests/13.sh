#! /bin/sh

. ./setup

NAME="openssl example"

begin

WANTED="3"
GOT="$(${XPIPE} -p '^-----END CERTIFICATE-----$' openssl x509 -noout -subject <"${CERTFILE}" | grep -c "^subject=")"

if [ x"${WANTED}" != x"${GOT}" ]; then
	echo "Unexpected output '${GOT}'." >> "${STDERR}"
	fail
fi

end

exit 0
