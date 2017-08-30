#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sim.h" 

// Loop Termination Prediction paper: https://pdfs.semanticscholar.org/bd56/db0b8529c3d2ee3c0487e15d8132dcb68f8e.pdf 

typedef struct d{
	int tag; // 14 bits
	int nbiter; // number of iterations until termination counter, 14 bits
	int citer; // current iteration counter, 14 bits
	int conf; // confidence counter, 2 bits
	int age; // age counter, 8 bits
} loop_entry;

// following l-tage submission: 256 entries, 4-way associative
#define LOOP_ENTRIES 256
#define LOOP_WAYS 4
#define LOOP_TAG_SIZE 14
#define CITER_BITS 14
#define LOOP_SAT_SIZE (int)pow(2, 7)
loop_entry loop_pred[LOOP_ENTRIES][LOOP_WAYS];
int loop_sat_ctr; // 7 bit saturation counter, determines if want to loop predict


void free_loop_entry(int index, int way){
		loop_pred[index][way].nbiter = 0;
		loop_pred[index][way].citer = 0;
		loop_pred[index][way].conf = 0;
		loop_pred[index][way].age = 0;
		// no reason to set tag to 0
}	

void clear_loop_pred(){
	for(int i = 0; i < LOOP_ENTRIES; i++){
		for(int j = 0; j < LOOP_WAYS; j++){
			free_loop_entry(i, j);
		}
	}
}

int get_tag(int addr){
	int index_bits = (int) (log(LOOP_ENTRIES)/log(2));
	int tag_bits = (int) (pow(2, LOOP_TAG_SIZE) - 1);
	int tag = (addr >> index_bits) & tag_bits; // grab 14 bits after 8 bits of index 
	return tag;
}

int get_way(int index, int tag){  
	for(int way = 0; way < LOOP_WAYS; way++){
		if(loop_pred[index][way].tag == tag){
			return way;
		}
	}
	return -1;
}

int find_replacement(int index){
	for(int way = 0; way < LOOP_WAYS; way++){
		if(loop_pred[index][way].age == 0){
			return way;
		} else{
			loop_pred[index][way].age--;
		}
	}
	return -1;
}

// -1 is no prediction
int loop_prediction(int addr, int result, int tage_pred){
	int hit = 1;
	// look for loop entry 
	int index = addr & (LOOP_ENTRIES -1);
	int tag = get_tag(addr);
	int way = get_way(index, tag);

	// note: no valid bit, so if tag == 0 will match with uninitialized entry
	if(way == -1){
		hit = 0; // match not found
	}

	// make loop prediction
	int pred = -1;
	int valid = 0;
	if(hit){
		// prediction if hit and valid and loop sat ctr is confident
		if(loop_pred[index][way].conf == 3){
			valid = 1;
			if(loop_sat_ctr >= 0){
				pred = loop_pred[index][way].citer + 1 == loop_pred[index][way].nbiter ? 0 : 1;
			}
		}
	}

	// update loop predictor
	if(hit){
		if(valid){
			if(pred != result){
				free_loop_entry(index, way);	
			} else if(pred != tage_pred){
				loop_pred[index][way].age++; // increase age bc useful
			}

			// update global loop sat counter
			if(pred != tage_pred){
				loop_sat_ctr = pred == result ? MIN(loop_sat_ctr + 1, (LOOP_SAT_SIZE >> 1) - 1) : 
										MAX(loop_sat_ctr - 1, -1 * (LOOP_SAT_SIZE >> 1));
			}
		}
		loop_pred[index][way].citer++;

		if(loop_pred[index][way].citer > loop_pred[index][way].nbiter){
			if(loop_pred[index][way].nbiter != 0){
				free_loop_entry(index, way);
			} else if(loop_pred[index][way].citer >= pow(2, CITER_BITS)){
				// since we max at 14 bits for citer, just free the entry if flow over
				free_loop_entry(index, way);
			} else{
				loop_pred[index][way].conf = 0;
			}
		}

		// update loop entries if loop termination occurs
		if(result == 0){
			// either increase confidence, initialize nbiter, or free the entry because its wrong
			if(loop_pred[index][way].citer == loop_pred[index][way].nbiter){
				loop_pred[index][way].conf = MIN(loop_pred[index][way].conf + 1, 3);
			 
				// don't predict with low loop counts
				if(loop_pred[index][way].nbiter < 3){
					free_loop_entry(index, way);
				}	
			} else {
				if(loop_pred[index][way].nbiter == 0){
					loop_pred[index][way].conf = 0;
					loop_pred[index][way].nbiter = loop_pred[index][way].citer;
				} else{
					// free entry if loop count has changed
					free_loop_entry(index, way);
				}
			}
			loop_pred[index][way].citer = 0;
		}
	} else if(result){
		int way = find_replacement(index);
		// init loop entry
		if(way >= 0){
			loop_pred[index][way].tag = tag;
			loop_pred[index][way].nbiter = 0;
			loop_pred[index][way].citer = 0;
			loop_pred[index][way].conf = 0;
			loop_pred[index][way].age = 255;
		}
	}
	// return prediction made before update, -1 is no prediction
	return pred;
}

// just a test method to see how loop_predictor functions on own
void loop_predictor(char *input_file){
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

			if(loop){
				int pred = loop_prediction(addr, taken, 0);
				if(pred == taken){
					correct++;
					// printf("Guessed right\n");
				} else if (pred == -1){
					// printf("Didn't guess\n");
				} else{
					// printf("Guessed right\n");
				}
			} else{
				// printf("Not a loop\n");
			}
			total++;
			free(address);
		}
	}
	fclose(file);
	printf("\tPercentage correct: %f%%\n\tCorrect: %f\n\tTotal Branches: %f\n", 
		correct/total * 100, correct, total);
}
