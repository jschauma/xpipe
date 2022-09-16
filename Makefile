NAME= xpipe
OBJS= src/xpipe.o

CFLAGS= -g -Wall -Werror -Wextra
LIBS= -lm

PREFIX?=/usr/local

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

${NAME}: ${OBJS}
	${CC} -o ${NAME} ${OBJS} ${LIBS}

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
