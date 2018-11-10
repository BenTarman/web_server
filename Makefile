CXX = gcc
LD = gcc
CXXFLAGS = -g -std=c99
LDFLAGS = -g

TARGET = web_server
OBJ_FILES = ${TARGET}.o
INC_FILES = ${TARGET}.h


${TARGET}: ${OBJ_FILES}
	${LD} ${LDFLAGS} ${OBJ_FILES} -o $@ ${LIBRARYS}

%.o : %.c ${INC_FILES}
	${CXX} -c ${CXXFLAGS} -o $@ $<

#
# Please remember not to submit objects or binarys.
#
clean:
	rm -f core ${TARGET} ${OBJ_FILES}

#
# This might work to create the submission tarball in the formal I asked for.
#
submit:
	rm -f core project3 ${OBJ_FILES}
	mkdir `whoami`
	cp Makefile README.txt *.h *.c `whoami`
	tar cf `whoami`.tar `whoami`
