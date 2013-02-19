TARGET	= client 
OBJS 	= client.o
CFLAGS	= -Wall -O2
LIBS	=

.PHONY: clean

${TARGET}: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${LIBS}

clean:
	${RM} *.o core* ${TARGET}
