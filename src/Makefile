CC=g++

CFLAGS=-g
CFLAGS+="-std=c++1y"

#### log ####

log_cpp := ${wildcard log/*.cpp}
log_o := ${log_cpp:.cpp=.o}

log/%.o: log/%.cpp log/%.h
	${CC} ${CFLAGS} -c $< -o $@
	
log: ${log_o}
	
.PHONY: clean
clean:
	@echo "do clean"