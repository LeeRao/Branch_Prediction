# compiler to use
CC = clang

# flags to pass compiler
CFLAGS = -ggdb3 -O0 -Qunused-arguments -std=c11 -Wall -Werror

# name for executable
EXE = trace
EXE_2 = sim

# space-separated list of header files
HDRS = 

# space-separated list of libraries, if any,
# each of which should be prefixed with -l
LIBS = -lm

# -lm needed for math.h
# -I/path could be used to add to include path

# space-separated list of source files
SRCS = trace.c

SRCS_2= sim.c static.c twolevel.c perceptron.c tage.c loop.c

# automatically generated list of object files
OBJS = $(SRCS:.c=.o)	
OBJS_2 = $(SRCS_2:.c=.o)
OBJS_3 = $(SRCS_3:.c=.S)

# dependencies 
$(OBJS): $(HDRS) Makefile
$(OBJS_2): $(HDRS) Makefile
$(OBJS_3): $(HDRS) Makefile

TEST=$(sort $(wildcard *.test))
TEST_EXE=$(patsubst %.test, %, $(TEST))
ASM=$(wildcard *.s)
PROGS = $(patsubst %.test,%.out,$(TEST))
PROGS_2 = $(patsubst %.s,%.out,$(ASM))

# default target
all: $(OBJS) $(OBJS_2) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $(EXE) $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $(EXE_2) $(OBJS_2) $(LIBS)
	@touch results

sim: $(OBJS_2) $(HDRS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS_2) $(LIBS)

# housekeeping
clean:
	@rm -f core $(EXE) $(EXE_2) $(TEST_EXE) *.o *.d *.s *.out results

test: $(EXE) $(PROGS) $(TEST) $(OBJS) $(OBJS_2) $(HDRS) Makefile
	@cat results

%.out : %.test
	@mv $< $*.c
	gcc -S $*.c
	@mv $*.c $<
	@echo "===================="
	./$(EXE) $*.s > $*_dirty.s
	gcc -o $* $*_dirty.s
	./$* > $*.out
	./sim $*.out > $*_result.txt
	@echo "\n------------ $* results -----------\n" >> results
	@cat $*_result.txt >> results

out: test.s

test.s : test.c Makefile $(EXE)
	gcc -S test.c
	./$(EXE) test.s
	@echo "========== test result =========="
	./$(EXE) test.s > test_out.s
	gcc -o test test_out.s
	./test > test_result.txt
	./sim test_result.txt

hello.s : hello.c Makefile $(EXE)
	gcc -S hello.c
	./$(EXE) hello.s > hello_out.s
	gcc -o hello hello_out.s
	@echo "========== hello =========="
	./hello

