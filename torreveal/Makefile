CC = clang
CFLAGS = -lcurl -lpthread

TARGET = torreveal
OBJS = torreveal.o common.o 

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} ${CFLAGS} -o ${TARGET} ${OBJS}

torreveal.o: torreveal.c common.h 
	${CC} -c torreveal.c -o $@
common.o: common.c common.h 
	${CC} -c common.c -o $@

clean:
	rm -f ${TARGET} ${OBJS}
