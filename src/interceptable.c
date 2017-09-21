/*Programme idiot utilsé pour le premier exercice de TP de SEL*/
/*Destiné à être intercepté*/

#include <stdio.h>
#include <unistd.h>

void interceptable(int disp) {
	printf("%d \a \n", disp);
}

int main() {
	int i = 0;
	while (1) {
		interceptable(i);
		i++;
		sleep(1);
	}
	return(0);
}
