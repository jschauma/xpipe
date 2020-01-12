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
