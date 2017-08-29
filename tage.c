#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim.h" /* angle includes for system headers, double quotes
					means you look in current directory */
#define COMPONENT_SIZE 1024
#define BIMODAL_SIZE 4096
#define TAG_LENGTH 8 
#define CSR2_LEN 7
#define ALPHA 2
#define USEFUL_RESET 256*1024

typedef struct c{
	struct component_entry{
		int pred; // 3 bit saturation counter
		int tag; // 8 bit tag
		int u; // 2 bit useful ctr
	} entry[COMPONENT_SIZE]; // could also have used 2d array
} component;
int use_alt_on_na; // 4 bit sat counter deciding if want to use newly allocated entries
int num_branches;
int cleared_bit;

int compute_index(int hist[], int addr, int hist_cap){
	int len = (int) (log(COMPONENT_SIZE) / log(2)); // should be 10
	int temp[len];
	for(int i = 0; i < len; i++){
		temp[i] = hist[i];
	}

	// XOR-folding of history
	for(int i = 10; i < hist_cap; i++){
		int j = 0;
		while(j < 10){
			temp[j] = temp[j] ^ hist[i];
			i++;
			j++;
		}
		i--;
	}

	// convert temp into an int
	int convert = 0;
	for(int i = len - 1; i >=0; i--){
		convert = (convert << 1) + temp[i];
	}
	// printf("");
	// printf("Index: %d\n", (addr ^ convert) & 0x3ff);
	// TODO: don't hard code the 0x3ff
	// printf("Computed: %d\t%d\t%d\n", (addr ^ convert) & 0x3ff, addr, convert);
	return (addr ^ convert) & 0x3ff; // grab last 10 bits
}

// we use tag length of 8 bits
int compute_tag(int hist[], int addr, int hist_cap){
	// first circular shift register XOR's using 8 bit increments
	int csr1 = 0;
	for(int i = TAG_LENGTH - 1; i >= 0; i--){
		csr1 = (csr1 << 1) + hist[i];
	}

	for(int i = TAG_LENGTH; i < hist_cap; i += TAG_LENGTH){
		int convert = 0;
		
		// convert each 8 bit piece to int
		for(int j = MIN( i + TAG_LENGTH - 1, hist_cap); j >= i; j--){
			convert = (convert << 1) + hist[i];
		}
		csr1 = csr1 ^ convert;
	}

	// second CSR uses 7 bit increments
	int csr2 = 0;
	for(int i = CSR2_LEN - 1; i >= 0; i--){
		csr2 = (csr2 << 1) + hist[i];
	}
	for(int i = CSR2_LEN; i < hist_cap; i += CSR2_LEN){
		int convert = 0;
		
		// convert each 8 bit piece to int
		for(int j = MIN( i + CSR2_LEN - 1, hist_cap); j >= i; j--){
			convert = (convert << 1) + hist[i];
		}
		csr2 = csr2 ^ convert;
	}
	return (csr1 ^ addr ^ (csr2 << 1)) & 0xff;
}

/* return 1 if correct, 0 otherwise */
int update_tage(int hist[], component T[], int num_components, int base_predictor[], int L0_Length, 
	int size, int global_max, int addr, int taken, int loop, int loop_on){
	// warming up
	int base_index = addr & 0xfff;
	int base_res = base_predictor[base_index] > 1;
	int provider = -1;
	int alt = -1;
	int prov_pred = base_res;
	int alt_pred = base_res;
	int p_index = base_index; // provider index
	int alt_index = base_index;

	for(int i = num_components - 1; i >= 0; i--){
		int hist_cap = (int) (pow(ALPHA, i) * L0_Length + 0.5);
		// first check if it's warmed up
		if(size >= hist_cap){
			int tag = compute_tag(hist, addr, hist_cap);
			int index = compute_index(hist, addr, hist_cap);

			// int tag = compute_tag(hist, addr, hist_cap);
			// printf("Index: %d\n", index);
			// printf("comp_index_res: %d\n", comp_index_res);
			// printf(""); 
			if(T[i].entry[index].tag == tag){
				if(i > provider){
					provider = i;
					prov_pred = T[i].entry[index].pred >= 0;
					p_index = index;
				} else if(i > alt){
					alt = i;
					// printf("Alt: %d\n", alt);
					alt_pred = T[i].entry[index].pred >= 0;
					alt_index = index;
					break; // exit loop once alt and provider found
				}
			}
		}
	}
	// printf("provider: %d\n", provider);
	// printf("alt: %d\n", alt);
	// printf("Alt2: %d\n", alt);

	// now decide how to allocate entries based on branch result

	// UPDATE PREDICTION COUNTERS
	// first check if base predictor was used as provider
	if(p_index == -1){
		// TODO : write this logic as a function
		if(taken){
			base_predictor[base_index] = MIN(base_predictor[base_index] + 1, 3);
		} else{
			base_predictor[base_index] = MAX(base_predictor[base_index] - 1, 0);
		}
	}
	else{
		if(taken){
			T[provider].entry[p_index].pred = MIN(T[provider].entry[p_index].pred + 1, 3);
		} else{
			T[provider].entry[p_index].pred = MAX(T[provider].entry[p_index].pred - 1, -4);
		}
		// update alt pred as well if provider component useful ctr is null
		if(T[provider].entry[p_index].u == 0){
			if(alt == -1){
				if(taken){
					base_predictor[base_index] = MIN(base_predictor[base_index] + 1, 3);
				} else{
					base_predictor[base_index] = MAX(base_predictor[base_index] - 1, 0);
				}
			} else{
				if(taken){
					T[alt].entry[alt_index].pred = MIN(T[alt].entry[alt_index].pred + 1, 3);
				} else{
					T[alt].entry[alt_index].pred = MAX(T[alt].entry[alt_index].pred - 1, -4);
				}
			}
		}
	}

	// UPDATE USEFUL COUNTERS
	if(prov_pred != alt_pred){
		/* "graceful resetting" as detailed in this paper, could include bit flip later
		https://www.jilp.org/vol8/v8paper1.pdf
		note: the paper was rather unclear about which occurs first, the reset or the updating of
		useful counters. I am assuming the reset occurs first */
		if(num_branches == USEFUL_RESET){
			num_branches = 0;
			for(int i = 0; i < COMPONENT_SIZE; i++){
				for(int j = 0; j < num_components; j++){
					if(cleared_bit == 1){
						T[j].entry[i].u = T[j].entry[i].u & 0x1;
					}else{
						T[j].entry[i].u = T[j].entry[i].u & 0x2;
					}
				}
			}
			cleared_bit = 1 - cleared_bit;
		}

		if(prov_pred == taken){
			T[provider].entry[p_index].u = MIN(T[provider].entry[p_index].u + 1, 3);
		}else{
			T[provider].entry[p_index].u = MAX(T[provider].entry[p_index].u - 1, 0);
		}
	}

	// ALLOCATION PROCESS
	/* I assume we cannot allocate an entry that is not warm yet. Seznec's paper was unclear about this point */
	// find the index of largest warm predictor component
	double s = (double) size;
	double l = (double) L0_Length;
	int max_warm = (int) (log(s / l) / log(ALPHA));
	if(prov_pred != taken && provider < max_warm){
		/* randomly select a higher entry for allocation among next three entries, with bias on first entry
		Probabilities are 1/2 1/4 1/4, or 2/3 1/3, or 1*/
		int avail = MIN(num_components - max_warm, 3);
		int choice = rand() % (avail + 1);
		choice = (choice == 0 || choice == 1) ? provider + 1 : provider + choice;
		
		// initialize the entry
		int hist_cap = (int) (pow(ALPHA, choice) * L0_Length + 0.5);
		int tag = compute_tag(hist, addr, hist_cap);
		int index = compute_index(hist, addr, hist_cap);

		T[choice].entry[index].tag = tag;
		T[choice].entry[index].pred = taken - 1; // initialize to weakly correct 1-> 0, 0 -> -1
		T[choice].entry[index].u = 0;
	}

	/* update global history, most recent branch res goes in index 0
	Makes sense to allocate first, THEN update the global history*/
	for(int i = global_max -1; i >= 1; i--){
		hist[i] = hist[i - 1]; 
	}
	hist[0] = taken;

	// return original result of prediction, based on use_alt_on_na
	int final_pred;
	if( (prov_pred != -1 || prov_pred != 0) || use_alt_on_na < 0){
		final_pred = prov_pred;
	} else{
		final_pred = alt_pred;
	}

	// LOOP PREDICTION
	if(loop && loop_on){
		/*int loop_guess = *///loop_prediction(addr, taken, final_pred);
		// final_pred = loop_guess >= 0 ? loop_guess : final_pred;
		/*printf("%d\n", loop_guess);
		if (loop_guess >= 0){
			printf("Used loop pred\n");
		}//*/
	}

	return final_pred == taken;
}

/* l-tage predictor design idea taken from this paper:
https://www.jilp.org/vol9/v9paper6.pdf
I try to follow design outlined in paper as faithfully as possible
NOTE: num_components is one less than true number of components (which includes base predictor)
*/

/*int base_predictor[BIMODAL_SIZE]; // table of 2 bit saturation counters
component predictors[4];//*/ // num_components doesn't include base predictor
// int global_pattern[80];
void tage_predictor(char *input_file, int num_components, int L0_Length, int loop_on){
	int base_predictor[BIMODAL_SIZE]; // table of 2 bit saturation counters 
	component predictors[num_components]; // num_components doesn't include base predictor //*/
	int global_max = (int) (pow(ALPHA, num_components - 1) * L0_Length + 0.5); // going to use alpha of 2
	int global_pattern[global_max]; 

	int global_size = 0;
	use_alt_on_na = 0;
	num_branches = 0;
	cleared_bit = 1; // decides which column to clear when resetting

	/* initialize components */
	for(int i = 0; i < BIMODAL_SIZE; i++){
		base_predictor[i] = 1;
	}
	for(int i = 0; i < COMPONENT_SIZE; i++){
		for(int j = 0; j < num_components; j++){
			predictors[j].entry[i].pred = 0; // weakly taken
			predictors[j].entry[i].tag = 0;
			predictors[j].entry[i].u = 0;
		}
	}
	for(int i = 0; i < global_max; i++){
		global_pattern[i] = 0;
	}

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
			char *address = strdup(line);
			int addr = atoi(address);
			fgets(line, sizeof(line), file); // taken or not
			int taken = (strcmp(line, "taken\n") == 0) ? 1 : 0;

			fgets(line, sizeof(line), file); // loop or not
			int loop = (strcmp(line, "loop\n") == 0) ? 1 : 0;

			correct += update_tage(global_pattern, predictors, num_components, base_predictor, L0_Length, global_size, 
				global_max, addr, taken, loop, loop_on);
			total++;
			num_branches++;
			global_size = MIN(global_size + 1, global_max); // don't want it to go over max
			free(address);
		}
	}
	fclose(file);
	printf("\tPercentage correct: %f%%\n\tCorrect: %f\n\tTotal Branches: %f\n", 
		correct/total * 100, correct, total);
}