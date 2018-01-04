CC=g++

CFLAGS=-g
CFLAGS+=-std=c++1y

######## log ########
log_cpp := ${wildcard log/*.cpp}
log_o := ${log_cpp:.cpp=.o}

log/%.o: log/%.cpp log/%.h
	${CC} ${CFLAGS} -c $< -o $@
	
log: ${log_o}
	
######## xml ########
xml_cpp := ${wildcard xml/*.cpp}
xml_o := ${xml_cpp:.cpp=.o}

xml/%.o: xml/%.cpp xml/%.h ${log_o}
	${CC} ${CFLAGS} -I./log -c $< -o $@
	
# test
xml/test/test: xml/test/test.cpp ${xml_o} ${log_o}
	${CC} ${CFLAGS} -I./log -I./xml $^ -o $@
	
xml: ${xml_o} xml/test/test

######## net ########
net_cpp := ${wildcard net/*.cpp}
net_o := ${net_cpp:.cpp=.o}

net/%.o: net/%.cpp net/%.h ${log_o}
	${CC} ${CFLAGS} -I./log -c $< -o $@
	
# test
net/test/test_server: net/test/test_server.cpp ${net_o} ${log_o}
	${CC} ${CFLAGS} -I./log -I./net $^ -o $@

net/test/test_client: net/test/test_client.cpp ${net_o} ${log_o}
	${CC} ${CFLAGS} -I./log -I./net $^ -o $@
	
net: ${net_o} net/test/test_server net/test/test_client

######## util ########
util_cpp := ${wildcard util/*.cpp}
util_o := ${util_cpp:.cpp=.o}

util/%.o: util/%.cpp util/%.h
	${CC} ${CFLAGS} -c $< -o $@
	
# test
util/test/weekday_test: util/test/weekday_test.cpp ${util_o}
	${CC} ${CFLAGS} -I./util  $^ -o $@

util: ${util_o} util/test/weekday_test
	
######## all ########
all: log xml net
	@echo "do make all"
	
######## clean ########
.PHONY: clean help
clean:
	@echo "do clean"
	rm -f ${log_o}
	rm -f ${xml_o}
	rm -f ${net_o}
	
help:
	@echo "make clean"
	@echo "make all"
