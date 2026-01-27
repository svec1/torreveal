CC = clang
CFLAGS = -lcurl -lpthread

TARGET = tornet
OBJS = tornet.o common.o 

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJS}

tornet.o: tornet.c common.h 
	${CC} -c tornet.c -o $@
common.o: common.c common.h 
	${CC} -c common.c -o $@

clean:
	rm -f ${TARGET} ${OBJS}
