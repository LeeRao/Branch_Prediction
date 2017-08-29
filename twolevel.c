#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim.h"

typedef struct a{
	char valid;
	unsigned long addr;
}table_entry;
table_entry addr_table[ADDR_TABLE_SIZE];

typedef struct b{
	int hist_length;
	unsigned long pattern;
	// can't put sat_counter in here bc array cannot have variable length.
} local_entry;

/* handles meta-prediction as well */
int prediction_result(int local_res, int global_res, int local_size, int local_hist_length,
	int global_size, int global_hist_length, int meta_sat_counter[], int index, int option){
	if(option == 0){
		return local_res;
	} else if(option == 1){
		return global_res;
	} else{
		// if still warming up, then predicted wrong.
		if(local_size < local_hist_length
			&& global_size < global_hist_length){
			return 0;
		} else if(local_size < local_hist_length){
			// choose global if local still warming up
			return global_res;
		} else if(global_size < global_hist_length){
			// choose local if global still warming up
			return local_res;
		} else{
			// if both warm, choose based on saturation counter.
			int result = meta_sat_counter[index] < 2 ? local_res : global_res;
			// update meta-predictor, don't change counter if both right or both wrong
			if(global_res - local_res > 0){
				meta_sat_counter[index] = MIN(meta_sat_counter[index] + 1, 3);
			} else if(global_res - local_res < 0){
				meta_sat_counter[index] = MAX(meta_sat_counter[index] - 1, 0);
			}
			return result;
		}
	}
}

/* return 1 if correct, 0 if wrong*/
int add_local_hist(local_entry* local_history, unsigned long local_hist_length, int sat_counter[], 
	char *addr, int taken){
	int index = atoi(addr) & (ADDR_TABLE_SIZE - 1);

	if(local_history[index].hist_length < local_hist_length){
		local_history[index].hist_length++;
		// most recent branch is in least sig bit
		local_history[index].pattern = (local_history[index].pattern << 1) + taken;
		return 1 - taken; // if taken return 0 bc we predict not taken
	}

	// grab lowest x bits, x = local_hist_length
	unsigned long sat_index = local_history[index].pattern << (64 - local_hist_length) >> (64 - local_hist_length);
	int prediction = sat_counter[sat_index] > 1 ? 1 : 0;

	// update sat_counters
	if(taken){
		sat_counter[sat_index] = MIN(sat_counter[sat_index] + 1, 3);
	} else{
		sat_counter[sat_index] = MAX(sat_counter[sat_index]- 1, 0);
	}
	

	// update pattern
	local_history[index].pattern = (local_history[index].pattern << 1) + taken;
	return taken == prediction;
}

/* return 1 if correct, 0 if wrong*/
int add_global_hist(int global_history, int global_size, unsigned long global_hist_length, int global_sat_counter[], 
	char *addr, int taken){
	int index = (atoi(addr) ^ global_history) & (ADDR_TABLE_SIZE - 1);

	if(global_size < global_hist_length){
		global_history = (global_history << 1) + taken;
		return 1 - taken;
	}

	int prediction = global_sat_counter[index] > 1 ? 1 : 0;

	// update sat_counters
	if(taken){
		global_sat_counter[index] = MIN(global_sat_counter[index] + 1, 3);
	} else{
		global_sat_counter[index] = MAX(global_sat_counter[index]- 1, 0);
	}

	// update global history
	/*
	for(int i = global_hist_length - 1; i > 0; i--){
		global_history[i] = global_history[i - 1]; 
	}
	global_history[0] = taken;*/
	global_history = (global_history << 1) + taken;


	return taken == prediction;
}


/* option variable decides which predictor to use: 0-local, 1-global, 2-both
Local is Yeh and Patt's Local Two Level Adaptive Predictor
Global is the gshare prediction scheme (XOR branch address with global history) 
note: I do not handle aliasing in this simulation*/
void two_level_predictor(char *input_file, unsigned long local_hist_length, unsigned long global_hist_length, int option){
	local_entry local_history[ADDR_TABLE_SIZE];
	int sat_counter[ADDR_TABLE_SIZE][(int)(pow(2, local_hist_length))];
	int sat_size = pow(2, local_hist_length);

	/*	Two different options for global prediction.
	1) XOR Branch address with global address and index into table of 1K 2-bit sat_counters
	2) Keep 2^n sat_counters (n-length global history) for EACH branch address, on top of local history sat_counters

	1 seems to be more realistic, although 2 would be more accurate. Will implement 1.
	*/
	int global_history = 0;
	int global_sat_counter[ADDR_TABLE_SIZE];
	int global_size = 0;

	// meta-predictor, chooses between local and global
	int meta_sat_counter[ADDR_TABLE_SIZE];	

	loop_sat_ctr = 0;

	/* initialize local history, stack variable so have to*/
	for(int i = 0; i < ADDR_TABLE_SIZE; i++){
		local_history[i].hist_length = 0;
		local_history[i].pattern = 0;
		global_sat_counter[i] = 1;
		meta_sat_counter[i] = 1;
		for(int j = 0; j < sat_size; j++){
			sat_counter[i][j] = 1; // init all sat counters to 1
		}
	}

	// unsigned long global_pattern = 0;
	// unsigned int global_sat_counter[pow(2, global_hist_length)];

	FILE *file = fopen(input_file, "r");
	char line[LINE_SIZE];
	double correct = 0;
	double total = 0; 

	while(fgets(line, sizeof(line), file)){
		char * temp = line;
		temp[strlen(my_signature)] = '\0';
		if(strcmp(temp, my_signature) == 0){
			// this is info I printed
			fgets(line, sizeof(line), file); // addr
			char *addr = strdup(line);
			int index = atoi(addr) & (ADDR_TABLE_SIZE - 1);

			fgets(line, sizeof(line), file); // taken or not
			int taken = (strcmp(line, "taken\n") == 0) ? 1 : 0;
			int local_res = add_local_hist(local_history, local_hist_length, sat_counter[index], addr, taken);
			int global_res = add_global_hist(global_history, global_size, global_hist_length, 
				global_sat_counter, addr, taken);
			
			correct += prediction_result(local_res, global_res, local_history[index].hist_length, local_hist_length,
			global_size, global_hist_length, meta_sat_counter, index, option);
			total++;
			global_size = MIN(global_size + 1, global_hist_length);
			free(addr);
		}
	}
	fclose(file);
	printf("\tPercentage correct: %f%%\n\tCorrect: %f\n\tTotal Branches: %f\n", 
		correct/total * 100, correct, total);
	/*
	printf("pattern %lu\n", local_history[196].pattern);
		for(int i = 0; i < 3; i++){
			printf("%d: %d\n", i, sat_counter[196][i]);
		}*/
}