CC = gcc
BIN_NAME = pipeline 
SRC_NAME = main.c i_type.c r_type.c j_type.c helper.c pipe_stages.c branch_pre.c

${BIN_NAME}: ${SRC_NAME}
	${CC} -g -o ${BIN_NAME} ${SRC_NAME} -lm

clean:
	rm -f *.o
	rm -f *.rpt
	rm -f *.txt
	rm -f pipeline
