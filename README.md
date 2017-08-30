# Branch_Prediction
Performance simulation of various branch predictors, including the two-level adaptive predictor, the perceptron predictor, and the L-TAGE predictor

The simulation works in two steps:
1) Code Injection: The C program is compiled to assembly. "trace.c" will parse the assembly file and inject assembly instructions that print branch information. This process will create a .trace file that is analyzed by the next step. 
2) Branch Predictor Simulation: The trace file will be run through various branch predictors, and predictor accuracy will be printed to terminal output as well as the "results" file. 

How to use:
1) Rename your C Program to a .test file
2) Run "make [program name].trace"
    2a) If you want to pipe a file into your program's stdin, add "STDIN=[file to pipe]" to the make command
    2b) If you want to specify command line arguments, add "ARGS=[command line arguments]" to the make command
3) Enjoy your branch predictor simulation!

Other make targets:
make test - run branch prediction simulation on all .test files
make clean - housekeeping, removes .o, .s, .trace, and results
make all - compiles trace.c and sim.c

