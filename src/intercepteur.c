#include <stdlib.h> // pour atoi()
#include <stdio.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h> // pour pid_t et signinfo_t
#include <sys/wait.h> // pour waitid()
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


const long ADDR_FN = 0x400546;


/**
 * @brief Fonction convertissant une chaîne de caractère C représentant un entier en un PID
 * @param str une chaîne de caractère C représentant un entier
 * @pre str doit représenter un entier
 */
pid_t conversionCharStrToPid(char * str) {
	int tmp = atoi(str);
	if (tmp == 0) {
		perror("ERROR : Incorrect pid \n");
	} 
	return (pid_t) tmp;
}

/**
 * @brief Main exécutant le programme d'interception d'un processus dont le PID en donné en entier
 * @details Utilise PTRACE pour s'attacher et modifie le code d'une fonction donnée en dur (actuellement)
 */
int main(int argc, char * argv[]) {
	
	// Controle de l'entrée
	if (argc != 2) {
		printf("%s <pid>\n", argv[0]);
		printf("Où <pid> est l'identifiant du processus sur lequel il faut tenter de s'attacher.\n");
		return -1;
	}

	pid_t pidCible = conversionCharStrToPid(argv[1]);

	// Tentative d'attache
	if (ptrace(PTRACE_ATTACH, pidCible, 0, 0) != 0) {
		perror("ERROR : Undefined error on PTRACE\n");
		return -1;
	}
	
	
	char path[30];
	sprintf(path, "/proc/%s/mem", argv[1]);
	
	siginfo_t childInfo;
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus se stoppe bien
	
	
	//int memoire = open(path, O_WRONLY);
	FILE *memoire = fopen(path, "w");
	
	fseek(memoire, ADDR_FN, SEEK_SET); //Se place au niveau de la fonction a modifier
	
	char trap = 0xCC;
	
	
	
	size_t save = fwrite(&trap, sizeof(char), 1, memoire);
	//ssize_t save = pwrite(memoire, &trap, sizeof(trap), ADDR_FN);
	
	
	if (save < 0){
		perror("Erreur lors de l'ecriture\n");
	}
	
	if (ptrace(PTRACE_DETACH, pidCible, 0, 0) != 0) {
		perror("ERROR : Undefined error on PTRACE\n");
		return -1;
	}
	
	printf("bob\n");

	return 0;
}


