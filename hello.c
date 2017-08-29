#include <stdio.h>
#include <stdlib.h>

int main(){
	int x = 10;
	while(x > 0){
		printf("Hello world\n");
		x--;
		if(x > 5){
			printf("Hello I'm over 5\n");
		} else{
			printf("Hello I'm less than 5");
		}
	}

	return 0;
}