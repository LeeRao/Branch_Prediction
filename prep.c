#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define linesize 512 /* should be very safe line size for one line of assembly */

/* this is a special string to indicate print instruction made by
my program */
char* my_signature = "\nLEE: ";/* results in assembly warning
currently, could print new line separately to fix*/

// hashmap to keep track of previous branches, chain and bucket style
// could do linked list, but prefer O(1) access
#define HASH_MAP_SIZE 4096 // 2 ^ 12

typedef struct t{
	unsigned long label;
	unsigned long addr;
    struct t *next;
}node;

node jump_map[HASH_MAP_SIZE];

int hash(long addr){    
    return addr & 0xfff;
}

// returns label
unsigned long get(unsigned long addr) {
    int index = hash(addr);
    node *current = &jump_map[index];
    while(current != NULL){
    	if(addr == current->addr){
        	// printf("Matched addresses\n");
            return current->label;
        }
        if(current->next == NULL ){
            return 0;
        } else{
            current = current->next;            
        }
    }
    return 0; // if not found in hashmap, return 0, label will never be outside of overflow
}

void set(unsigned long addr, unsigned long label) {
    int index = hash(addr); //100 is the length of the hashtable

    // try to find if first bucket value exists
    node *current = &jump_map[index];
    if(current->next == NULL){
        current->addr = addr;
        current->label = label;
        current->next = NULL;
        return;
    }

    /* try to find node
    should really never run into this section of code
    bc I always check with get before set*/
    node *trl = NULL;
    while(current != NULL){
        if(addr == current->addr){
            current->label = label;
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
	for(int i = 0; i < HASH_MAP_SIZE; i++){
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
void inject_jmp_test(char* inst, unsigned long label, unsigned long addr){
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
	
	printf("\tpush $RETURN_FROM_TAKEN_%lu\n", label); // skip instructions potentially

	printf("\t%s TAKEN\n", inst);
	printf("\tcall NOT_TAKEN\n");
	printf("\tpop %%rdi\n"); // remove from stack after returning from NOT_TAKEN
	printf("RETURN_FROM_TAKEN_%lu:\n", label);
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
 	printf("\t.string \"%s\"\n", my_signature);

	printf("PRINT_NOT_TAKEN:\n");
 	printf("\t.string \"%s\"\n", "not taken");
 	printf("PRINT_TAKEN:\n");
 	printf("\t.string \"%s\"\n", "taken");

	inject_jump_map();
}

void add_traces(char * input_file){
	FILE *file = fopen(input_file, "r");
	char* remove_ext = (char *) malloc(strlen(input_file) - 2);
	strncpy(remove_ext, input_file, strlen(input_file) - 2);
	// FILE *output = fopen(strcat(remove_ext, "_trace.s"), "w+");
	char line[linesize];
	unsigned long label = 1;
	unsigned long address = 0;
	while(fgets(line, sizeof(line), file)){
		char *inst = get_instruction(line);
		// fputs(line, output);
		// write instruction info into new file
		if(inst != NULL){
			if(strstr(cond_jmp_list, inst) != NULL){
				inject_jmp_test(inst, label, address);
				set(address, label);
				label++;
			}
		}
		printf("%s", line);

		/* Each line takes up 4 bytes for simplicity. Certainly not realistic, but works
		for purposes of this simulation. */
		address += 4; 
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