#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim.h"

int dot_product(int size, int a[], int b[]){
	int sum = 0;
	for(int i = 0; i < size; i++){
		sum += a[i] * b[i];
	}
	return sum;
}

/* Idea for algorithm is from this paper:
https://www.cs.utexas.edu/~lin/papers/hpca01.pdf
*/
void train(int hist[], int hist_length, int perceptron[], int taken){
	double threshold = 1.93 * hist_length + 14;
	int y = dot_product(hist_length, hist, perceptron);
	if(abs(y) <= threshold || ((y >= 0) != taken) ){
		for(int i = 0; i <= hist_length; i++){
			if(taken == hist[i]){
				perceptron[i] += 1;
			} else{
				perceptron[i] -= 1;
			}
		}
	}
}

/* return 1 if correct, 0 otherwise */
int update_perceptron(int hist[], int hist_length, int perceptron[],
	int size, char * addr, int taken){
	// formula taken from: https://www.cs.utexas.edu/~lin/papers/hpca01.pdf

	// warming up
	if(size < hist_length){
		// for consistency, most recent is in least signif index
		hist[hist_length - size] = taken; // lowest index is 1, bias lives in 0

		// can train now
		if(size == hist_length){
			train(hist, hist_length, perceptron, taken);
		}
		// if global hist not filled, assume not taken
		return taken == -1;
	}
	int prediction = dot_product(hist_length + 1, hist, perceptron) <= 0 ? -1 : 1;
	
	// should train first, then update global shift reg
	train(hist, hist_length, perceptron, taken);

	// update global history
	for(int i = hist_length; i > 1; i--){
		hist[i] = hist[i - 1]; 
	}
	hist[1] = taken;

	return taken == prediction;
}

void perceptron_predictor(char *input_file, int local_hist_length, int global_hist_length, int option){
	int global_hist[global_hist_length + 1]; // index 0 is the bias bit.
	int local_hist[ADDR_TABLE_SIZE][local_hist_length + 1];
	int local_size[ADDR_TABLE_SIZE];
	int global_perceptrons[ADDR_TABLE_SIZE][global_hist_length + 1];
	int local_perceptrons[ADDR_TABLE_SIZE][local_hist_length + 1];
	// perceptron is array of int values
	int global_size = 0;

	// meta-predictor, chooses between local and global
	int meta_sat_counter[ADDR_TABLE_SIZE];	

	/* initialize perceptron tables */
	for(int i = 0; i < ADDR_TABLE_SIZE; i++){
		local_size[i] = 0;
		local_hist[i][0] = 1; // bias bit
		meta_sat_counter[i] = 1; // favors local initially
		/* going to set all weights as 0 initially*/
		for(int j = 0; j < global_hist_length + 1; j++){
			global_perceptrons[i][j] = 0; 
			local_perceptrons[i][j] = 0;
		}
	}
	global_hist[0] = 1; // bias bit

	loop_sat_ctr = 0;

	FILE *file = fopen(input_file, "r");
	char line[LINE_SIZE];
	double correct = 0;
	double total = 0; 

	while(fgets(line, sizeof(line), file)){
		char * temp = line;
		temp[strlen(my_signature)] = '\0';
		if(strcmp(temp, my_signature) == 0){
			fgets(line, sizeof(line), file); // addr
			char *addr = strdup(line);
			int index = atoi(addr) & (ADDR_TABLE_SIZE - 1);
			fgets(line, sizeof(line), file); // taken or not
			
			int taken = (strcmp(line, "taken\n") == 0) ? 1 : -1;
			int global_res = update_perceptron(global_hist, global_hist_length, global_perceptrons[index],
				global_size, addr, taken);
			int local_res = update_perceptron(local_hist[index], local_hist_length, local_perceptrons[index],
				local_size[index], addr, taken);

			// NOTE: prediction_result defined in twolevel.c
			correct += prediction_result(local_res, global_res, local_size[index], local_hist_length,
			global_size, global_hist_length, meta_sat_counter, index, option);
			total++;
			global_size = MIN(global_size + 1, global_hist_length); // don't want it to overflow
			local_size[index] = MIN(local_size[index] + 1, local_hist_length);
			free(addr);

			/*
			printf("index %d\n", index);
			printf("Most recent: ");
			for(int i = 0; i <= global_hist_length; i++){
				printf("%d, ", global_hist[i]);
			}
			printf("\nWeight:      ");
			for(int i = 0; i < global_hist_length + 1; i++){
				printf("%d, ", global_perceptrons[index][i]);
			}//*/

			/*
			printf("index %d\n", index);
			printf("Most recent: ");
			for(int i = 0; i <= local_hist_length; i++){
				printf("%d, ", local_hist[index][i]);
			}
			printf("\nWeight:      ");
			for(int i = 0; i < local_hist_length + 1; i++){
				printf("%d, ", local_perceptrons[index][i]);
			}//*/
		}
	}
	fclose(file);
	printf("\tPercentage correct: %f%%\n\tCorrect: %f\n\tTotal Branches: %f\n", 
		correct/total * 100, correct, total);
}