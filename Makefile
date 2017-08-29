# compiler to use
CC = clang

# flags to pass compiler
CFLAGS = -ggdb3 -O0 -Qunused-arguments -std=c11 -Wall -Werror

# name for executable
EXE_TRACE = trace
EXE_SIM = sim

# space-separated list of header files
HDRS = sim.h

# space-separated list of libraries, if any,
# each of which should be prefixed with -l
LIBS = -lm

# -lm needed for math.h
# -I/path could be used to add to include path

# space-separated list of source files
SRCS = trace.c 

SRCS_2 = sim.c static.c twolevel.c perceptron.c tage.c loop.c

# automatically generated list of object files from make's implicit rules
OBJS = $(SRCS:.c=.o)	
OBJS_2 = $(SRCS_2:.c=.o)
OBJS_3 = $(SRCS_3:.c=.S)

TEST = $(sort $(wildcard *.test))
EXE_TEST = $(patsubst %.test, %, $(TEST))
PROGS = $(patsubst %.test, %.out, $(TEST))

# dependencies 
$(OBJS) : $(HDRS) Makefile
$(OBJS_2) : $(HDRS) Makefile
$(OBJS_3) : $(HDRS) Makefile

# default target
all : $(OBJS) $(OBJS_2) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $(EXE_TRACE) $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $(EXE_SIM) $(OBJS_2) $(LIBS)
	@touch results

# housekeeping
clean:
	@rm -f core $(EXE_TRACE) $(EXE_SIM) $(EXE_TEST) *.o *.d *.s *.out results

# simulate performance on all .test files
test: all $(PROGS)
	@cat results

%.out : %.test
	@mv $< $*.c
	gcc -S $*.c
	@mv $*.c $<
	@echo "===================="
	./$(EXE_TRACE) $*.s > $*_dirty.s
	gcc -o $* $*_dirty.s
	./$* > $*.out
	./sim $*.out > $*_result.txt
	@echo "\n------------ $* results -----------\n" >> results
	@cat $*_result.txt >> results

# -----------------------------------
# bunch of extra make targets for me
out: test.s

trace: $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $(EXE_TRACE) $(OBJS) $(LIBS)

sim: $(OBJS_2) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS_2) $(LIBS)


test.s : test.c Makefile $(EXE_TRACE)
	gcc -S test.c
	./$(EXE_TRACE) test.s
	@echo "========== test result =========="
	./$(EXE_TRACE) test.s > test_out.s
	gcc -o test test_out.s
	./test > test_result.txt
	./sim test_result.txt

hello.s : hello.c Makefile $(EXE_TRACE)
	gcc -S hello.c
	./$(EXE_TRACE) hello.s > hello_out.s
	gcc -o hello hello_out.s
	@echo "========== hello =========="
	./hello
