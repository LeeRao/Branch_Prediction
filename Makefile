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

SRCS_2 = sim.c prediction.c static.c twolevel.c perceptron.c tage.c loop.c

# automatically generated list of object files from make's implicit rules
OBJS = $(SRCS:.c=.o)	
OBJS_2 = $(SRCS_2:.c=.o)
OBJS_3 = $(SRCS_3:.c=.S)

TEST = $(sort $(wildcard *.test))
EXE_TEST = $(patsubst %.test, %, $(TEST))
PROGS = $(patsubst %.test, %.trace, $(TEST))

.PHONY : run all clean test hi 

# dependencies 
$(OBJS) : $(HDRS) Makefile
$(OBJS_2) : $(HDRS) Makefile
$(OBJS_3) : $(HDRS) Makefile

# default target
all : $(OBJS) $(OBJS_2) $(HDRS) Makefile
	make --no-print-directory trace
	make --no-print-directory sim
	@touch results

# housekeeping
clean:
	@rm -f core $(EXE_TRACE) $(EXE_SIM) $(EXE_TEST) *.o *.s *.trace results

# simulate performance on all .test files
test: all $(PROGS)

# $(STDIN): files that the user wants to be piped into program
# $(ARGS): Arguments to te program
%.trace : %.test all
	@mv $< $*.c
	gcc -S $*.c
	@mv $*.c $<
	@echo "===================="
	./$(EXE_TRACE) $*.s > $*_dirty.s
	gcc -o $* $*_dirty.s
# handle programs that take in arguments and stdin pipes
ifeq ($(strip $(STDIN)),)
	./$* $(ARGS) > $*.trace
else
	./$* $(ARGS) < $(STDIN) > $*.trace
endif
	@echo "\n------------ $* results -----------\n" | tee -a results
	./sim $*.trace | tee $*_result.txt
	@cat $*_result.txt >> results

# -----------------------------------
# bunch of extra make targets for my personal use
trace: $(OBJS) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $(EXE_TRACE) $(OBJS) $(LIBS)

sim: $(OBJS_2) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS_2) $(LIBS)

hi : hello.c Makefile $(EXE_TRACE) $(EXE_SIM)
	gcc -S hello.c
	./$(EXE_TRACE) hello.s > hello_dirty.s
	gcc -o hello hello_dirty.s
	@echo "========== hello =========="
	./hello > hello.trace
	./sim hello.trace
