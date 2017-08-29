#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define linesize 512 /* should be very safe line size for one line of assembly */

/* Injects Assembly instructions to print out branching information
 * Branch info format:
 * Memory Address
 * Taken/Not Taken
 * Loop or not Loop
 */

char* my_signature = "LEE: "; // same signature as sim.c

/* hashmap to keep track of previous branches, chain and bucket style
could do linked list, but prefer O(1) access*/
#define JUMP_MAP_SIZE 1024 // 2 ^ 10

/* second hash map to keep track of local ".L" labels generated
by GCC. Need for loop predictor */
#define LABEL_MAP_SIZE 512

// using same node struct for both hash maps
typedef struct t{
	long label;
	long addr;
    struct t *next;
}node;

node jump_map[JUMP_MAP_SIZE];
node label_map[LABEL_MAP_SIZE];

/* which_map == 0 is jump_map
which_map == 1 is label_map */
int hash(long num, char which_map){
	if(which_map){
		return num & (LABEL_MAP_SIZE - 1);
	}    
    return num & (JUMP_MAP_SIZE - 1);
}

// returns addr
long get(long label, char which_map) {
    int index = hash(label, which_map);
    node *current = which_map ? &label_map[index] : &jump_map[index];
    
    while(current != NULL){
    	if(label == current->label){
            return current->addr;
        }
        if(current->next == NULL ){
            return -1;
        } else{
            current = current->next;            
        }
    }
    return -1;
}

void set(long addr, long label, char which_map) {
    int index = hash(label, which_map); //100 is the length of the hashtable
    // try to find if first bucket value exists
    node *current = which_map ? &label_map[index] : &jump_map[index];
    if(current->next == NULL){
        current->addr = addr;
        current->label = label;
        current->next = NULL;
        return;
    }

    /* should never run into this segment of code,
    don't need to update a label's addr */
    node *trl = NULL;
    while(current != NULL){
        if(label == current->label){
            current->addr = addr;
            return;
        } else{
            trl = current;
            current = current->next;            
        }
    }

    // didn't find node in hash table
    current = (node*) malloc(sizeof(node));
    current->addr = addr;
    current->label = label;
    trl->next = current;
    current->next = NULL;
}

void inject_jump_map(){
	for(int i = 0; i < JUMP_MAP_SIZE; i++){
		node *current = &jump_map[i];

		while(current != NULL){
			if(current->label == 0){
				break;
			}
			printf("ORIGINAL_LABEL_%lu:\n", current->label);
  			printf("\t.string \"%lu\"\n", current->addr);
  			current = current->next;
		}
	}
}

char cond_jmp_list[200] = "je, jz, jne, jnz, js, jns, jg, jge, jl, jle, ja, jae, jnb, jb, jnae, jbe, jna,";

// grab the first 4 letters of the instruction, return NULL if not an inst
char * get_instruction(char *line){
	if(line[0] != '\t' || line[1] == '.'){
		return NULL;
	}
	char *inst = (char* ) malloc(5);
	int i = 1;
	while( (i < 5) && (line[i] >= 'A') && (line[i] <= 'z')){
		inst[i - 1] = line[i];
		i++;
	} 
	inst[i] = 0;
	return inst;
}

/* NOTE: calling puts, could be complications if they don't include stdio.h */
/* NOTE: Could check if program's labels match my labels to make my code more robust */
// repeat the instruction, but jump to place that prints jump is taken. make sure to return to inst after the next one
// right under the repeat, call the special inst occurs when jump not taken. ret will take you to where you want.
void inject_jmp_test(char* inst, long label, long addr, char is_loop){
    // char buffer [64]; // arbitrary buffer size
    // sprintf (buffer, "%lu" , addr);
	printf("\tpush\t%%rdi\n");
	printf("\tpushf\n"); // save flags register bc call changes flags

	printf("\tmovl $MY_SIGNATURE, %%edi\n");
	printf("\tcall puts\n");	
	printf("\tmovl $ORIGINAL_LABEL_%lu, %%edi\n", label);
	printf("\tcall puts\n");	
	printf("\tpopf\n"); // restore flags
	printf("\tpushf\n"); // save flags again
	//*/
	printf("\tpush $RETURN_FROM_TAKEN_%lu\n", label); // skip instructions potentially

	printf("\t%s TAKEN\n", inst);
	printf("\tcall NOT_TAKEN\n");
	printf("\tpop %%rdi\n"); // remove from stack after returning from NOT_TAKEN
	printf("RETURN_FROM_TAKEN_%lu:\n", label);

	if(is_loop){
		printf("\tmovl $IS_LOOP, %%edi\n");
		printf("\tcall puts\n");
	} else{
		printf("\tmovl $NOT_LOOP, %%edi\n");
		printf("\tcall puts\n");
	}

	printf("\tpopf\n");

	printf("\tpop\t%%rdi\n"); // restore %rdi value

}

void exec_not_taken(){
	printf("\t.section\t.text\n");
	printf("NOT_TAKEN:\n");
	// printf("\tmovl $PRINT_NOT_TAKEN, %%edi\n");
	// printf("\tcall puts\n");
	printf("\tmovl $PRINT_NOT_TAKEN, %%edi\n");
	printf("\tcall puts\n");
	printf("\tret\n");
}

void exec_taken(){
	printf("\t.section\t.text\n");
	printf("TAKEN:\n");
	printf("\tmovl $PRINT_TAKEN, %%edi\n");
	printf("\tcall puts\n");
	/*return to a different place */
	printf("\tret\n");
}

/* insert all of the print values I want */
// 	printf("\t.string \"%s%lu%s%s\"", "Lee's output: ", buffer, ":", string_value);
void inject_my_data(){
	// data exec_not_taken and exec_taken
	printf("\t.section\t.rodata\n");
	printf("MY_SIGNATURE:\n");
 	printf("\t.byte 10\n");
 	printf("\t.string \"%s\"\n", my_signature);

	printf("PRINT_NOT_TAKEN:\n");
 	printf("\t.string \"%s\"\n", "not taken");
 	printf("PRINT_TAKEN:\n");
 	printf("\t.string \"%s\"\n", "taken");

 	printf("IS_LOOP:\n");
 	printf("\t.string \"%s\"\n", "loop");
 	printf("NOT_LOOP:\n");
 	printf("\t.string \"%s\"\n", "not loop");

	inject_jump_map();
}

// returns label value
int get_local_label(char * label, char end){
	if(label[0] == '.' && label[1] == 'L' 
		&& (label[2] >= '0' && label[2] <= '9')){
		int len = 3;
		while(label[len] != end){
			len++;
		}
		label[len] = '\0';
		label = label + 2 * sizeof(char); // remove ".L"
		return atoi(label);
	}
	return -1;
}

char * strdup(char *id){
    char* dup = (char *) malloc(sizeof(char) * (strlen(id) + 1));
    for(int i = 0; i <= strlen(id); i++){
        dup[i] = id[i];
    }
    return dup;
}

void add_traces(char * input_file){
	FILE *file = fopen(input_file, "r");
	char* remove_ext = (char *) malloc(strlen(input_file) - 2);
	strncpy(remove_ext, input_file, strlen(input_file) - 2);
	// FILE *output = fopen(strcat(remove_ext, "_trace.s"), "w+");
	char line[linesize];
	long label = 1;
	long address = 0;
	while(fgets(line, sizeof(line), file)){
		char *inst = get_instruction(line);
		char *line_cpy = strdup(line);
		// label_id is num that comes after ".L"
		// write instruction info into new file if conditional jmp
		if(inst != NULL){
			if(strstr(cond_jmp_list, inst) != NULL){
				char is_loop = 0;
				set(address, label, 0);

				char *lbl_str = strstr(line_cpy, "\t.L");
				if(lbl_str != NULL){
					int lbl_val = get_local_label(lbl_str + sizeof(char), '\n');

					/* must have negative displacement if we've encountered it
					already, so is a loop*/
					// printf("lbl_val: %d, result: %ld\n", lbl_val, get(lbl_val, 1));
					if(get(lbl_val, 1) >= 0){
						is_loop = 1;
					}
				}
				inject_jmp_test(inst, label, address, is_loop);
				label++;
			}
		} else{
			int label_id = get_local_label(line_cpy, ':');
			if(label_id >= 0){
				set(address, label_id, 1);
			}
		}
		printf("%s", line);
		/* Each line takes up 4 bytes for simplicity. Certainly not realistic, but works
		for purposes of this simulation. */
		address += 4; 
		free(line_cpy);
	}
	exec_taken();
	exec_not_taken();
	inject_my_data();

	fclose(file);
}

int main(int argc, char *argv[]){
	char *input_file = argv[1];
	add_traces(input_file);
	return 0;
}
