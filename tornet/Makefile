CC = clang
CFLAGS = -lpthread

TARGET = tornet
OBJS = tornet.o common.o 

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o ${TARGET} ${OBJS}

tornet.o: tornet.c common.h 
	${CC} ${CFLAGS} -c tornet.c -o $@
common.o: common.c common.h 
	${CC} ${CFLAGS} -c common.c -o $@

clean:
	rm -f ${TARGET} ${OBJS}
