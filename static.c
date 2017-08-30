#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim.h"

/* Static Branch Predictor: This predictor always predicts not taken on a branch.
 */

void static_predictor(char *input_file){
	FILE *file = fopen(input_file, "r");
	char line[LINE_SIZE];
	double correct = 0;
	double total = 0; 

	while(fgets(line, sizeof(line), file)){
		char * temp = line;
		temp[strlen(my_signature)] = '\0';
		if(strcmp(temp, my_signature) == 0){
			// this is info I printed
			fgets(line, sizeof(line), file);
			fgets(line, sizeof(line), file);
			if(strcmp(line, "not taken\n") == 0){
				correct++;
			}
			total++;
		}
	}
	fclose(file);
	printf("\tPercentage correct: %f%%\n\tCorrect: %f\n\tTotal Branches: %f\n", 
		correct/total * 100, correct, total);
}
