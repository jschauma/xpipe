NAME= xpipe
OBJS= src/xpipe.o

PREFIX?=/usr/local

CFLAGS+= -Wall -Werror -Wextra -I${PREFIX}/include
LDFLAGS+= -L${PREFIX}/lib
LIBS= -lm

OS!=uname

.PHONY: test

all: ${NAME}

help:
	@echo "The following targets are available:"
	@echo "${NAME}  build the ${NAME} executable"
	@echo "clean    remove executable and intermediate files"
	@echo "install  install ${NAME} under ${PREFIX}"
	@echo "readme   generate the README after a manual page update"
	@echo "test     run the tests under tests/"

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

# Sorry, no ./configure, and portable make(1) is a PITA.
configure:
	@ 							\
		if [ "x${OS}" = x"Linux" ]; then		\
			if [ ! -d /usr/include/bsd ] && [ ! -d ${PREFIX}/include/bsd ]; then	\
				echo "Please install 'libbsd' and 'libbsd-devel'." >&2; \
				exit 1;				\
			fi;					\
		fi


${NAME}: configure ${OBJS}
	${CC} -o ${NAME} ${OBJS} ${LDFLAGS} ${LIBS} $$(echo ${OS} | sed -n -e 's/Linux/-lbsd/p')

clean:
	rm -fr ${NAME} ${OBJS}

test: ${NAME}
	@cd tests && for t in *.sh; do			\
		sh $${t};				\
	done

install: ${NAME}
	mkdir -p ${PREFIX}/bin ${PREFIX}/share/man/man1
	install -c -m 555 ${NAME} ${PREFIX}/bin/${NAME}
	install -c -m 444 doc/${NAME}.1 ${PREFIX}/share/man/man1/${NAME}.1

man: doc/${NAME}.1.txt

doc/${NAME}.1.txt: doc/${NAME}.1
	mandoc -T ascii -c -O width=80 $? | col -b >$@

readme: man
	sed -n -e '/^NAME/!p;//q' README.md >.readme
	sed -n -e '/^NAME/,$$p' -e '/emailing/q' doc/${NAME}.1.txt >>.readme
	echo '```' >>.readme
	mv .readme README.md
