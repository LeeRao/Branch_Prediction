#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim.h" /* angle includes for system headers, double quotes
					means you look in current directory */

char* my_signature = "LEE: "; // used to find if I printed the line or test program did

/* TODO:
	Implement Loop prediction
		- is currently making no predictions
	Improve Makefile
		- make test dependencies need to work better
		- should clean it up
		- maybe be able to make <test>.result
	Could add signature to branches in trace.c
*/

char * strdup(char *id){
    char* dup = (char *) malloc(sizeof(char) * (strlen(id) + 1));
    for(int i = 0; i <= strlen(id); i++){
        dup[i] = id[i];
    }
    return dup;
}

int main(int argc, char *argv[]){
	char *input_file = argv[1];
	
	printf("---------Static Prediction---------\n");
	static_predictor(input_file);

	printf("---------Two-Level Prediction---------\n");
	printf("Local prediction only:\n");
	two_level_predictor(input_file, 2, 8, 0);

	printf("Global prediction only:\n");
	two_level_predictor(input_file, 2, 8, 1);

	printf("Tournament prediction:\n");
	two_level_predictor(input_file, 2, 8, 2);

	printf("---------Perceptron Prediction---------\n");
	printf("Local perceptrons only:\n");
	perceptron_predictor(input_file, 8, 18, 0);

	printf("Global perceptrons only:\n");
	perceptron_predictor(input_file, 8, 18, 1);

	printf("Tournament prediction:\n");
	perceptron_predictor(input_file, 8, 18, 2);

	printf("---------L-TAGE Prediction---------\n");
	// tage_predictor(input_file, 4, 10, 0);
	// printf("Loop Prediction on:\n");
	tage_predictor(input_file, 4, 10, 1);

	// printf("---------LOOP Prediction---------\n");
	// loop_predictor(input_file);

	return 0;
}