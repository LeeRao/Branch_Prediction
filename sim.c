#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim.h" /* angle includes for system headers, double quotes
					means you look in current directory */
/* Runs simulation based on traces provided in trace.c
 */

//this is a special string to indicate print instruction made by my program
char* my_signature = "LEE: "; // used to find if I printed the line or test program did

/* TODO:
	Improve Makefile
		- test dependencies could work better
		- maybe be able to make <test>.result
	Could add signature to branches in trace.c

    Add magic number instead of using my name
    Signature also currently throws Assembly warning
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
	
	printf("\n---------Static Prediction---------\n");
	static_predictor(input_file);

	printf("\n---------Two-Level Prediction---------\n");
	printf("Local prediction only:\n");
	two_level_predictor(input_file, 2, 8, 0);

	printf("Global prediction only:\n");
	two_level_predictor(input_file, 2, 8, 1);

    // NOTE: It is entirely possible for the Tournament Predictor to perform worse than global or local individually
    // The metaprediction mechanism is a table of saturation counters with 3 bits per entry
    // The advantage of the tournament predictor lies in increased consistency of branch prediction.
	printf("Tournament prediction:\n");
	two_level_predictor(input_file, 2, 8, 2);

	printf("\n---------Perceptron Prediction---------\n");
	printf("Local perceptrons only:\n");
	perceptron_predictor(input_file, 8, 18, 0);

	printf("Global perceptrons only:\n");
	perceptron_predictor(input_file, 8, 18, 1);

	printf("Tournament prediction:\n");
	perceptron_predictor(input_file, 8, 18, 2);

	printf("\n---------L-TAGE Prediction---------\n");
	tage_predictor(input_file, 4, 10, 0);
    // Note: Even if no loop prediction actually occurs, the accuracy of the tage predictor 
    // may change slightly with this next call. There is inherent randomness in the update tage() step
	printf("Loop Prediction on:\n");
	tage_predictor(input_file, 4, 10, 1);
//*/
	// printf("\n---------LOOP Prediction---------\n");
	// loop_predictor(input_file);

	return 0;
}
