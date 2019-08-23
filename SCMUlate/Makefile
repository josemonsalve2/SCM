CC=gcc-9
EXTRA_CFLAGS= -DVERBOSE_MODE=1
EXTRA_LDFLAGS=
CFLAGS = -fopenmp -Wall -Wextra
LDFLAGS = 
INCLUDE=-I./include

PROGRAM = proof_of_concept
SOURCES = $(addsuffix .c, ${PROGRAM})
OBJS = $(addsuffix .o, ${PROGRAM})

all: $(PROGRAM)

%.o:%.c
	${CC} ${CFLAGS} ${EXTRA_CFLAGS} ${INCLUDE} -c $< -o $@

${PROGRAM}:${OBJS}
	${CC} ${LDFLAGS} ${EXTRA_LDFLAGS} ${CFLAGS} $< -o $@

clean: 
	rm -rf ${OBJS} ${PROGRAM}