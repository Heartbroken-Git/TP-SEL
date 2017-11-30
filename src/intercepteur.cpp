/**
 * @file intercepteur.cpp
 * @author Corentin CHÉDOTAL
 * @copyright TBD
 * @brief Fichier contenant le programme destiné à intercepter un processus et éventuellement à modifier en temps réel du code
 */

#include <iostream>
#include <cstdlib> // pour atoi()
#include <cstdio>
#include <unistd.h>
#include <exception>
#include <sys/ptrace.h>
#include <sys/types.h> // pour pid_t et signinfo_t
#include <sys/wait.h> // pour waitid()
#include <sys/stat.h>
#include <sys/user.h>
#include <errno.h>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sstream>

const long ADDR_FN = 0x400546;

using namespace std;

/**
 * @brief Fonction convertissant une chaîne de caractère C représentant un entier en un PID
 * @param str une chaîne de caractère C représentant un entier
 * @pre str doit représenter un entier
 * @warning Comportement indéfini si str est une chaîne ne contenant pas un entier. Une tentative de vérification d'entrée est en place mais elle ne doit en aucun cas constituer une preuve de bon fonctionnement si aucune erreur n'est retournée.
 * @todo Mettre en place une exception pour avertir l'utilisateur dans les rares cas où on peut détecter une mauvaise entrée
 */
pid_t conversionCharStrToPid(char * str) {
	int tmp = atoi(str);
	if (tmp == 0) {
		cerr << "ERROR : Incorrect input char str \a" << endl;
		// TODO : Une vraie exception des familles
	} 
	return (pid_t) tmp;
}

/**
 * @brief Fonction convertissant une chaîne de caractère C représentant un entier en un type de size_t
 * @details Pour éviter des doublons de code utilise la fonction conversionCharStrToPid avec un pid_t comme représentation intermédiaire
 * @param str une chaîne de caractère C représentant un entier
 * @pre str doit représenter un entier
 * @warning Comportement indéfini si str est une chaîne ne contenant pas un entier. Une tentative de vérification d'entrée est en place mais elle ne doit en aucun cas constituer une preuve de bon fonctionnement si aucune erreur n'est retournée.
 * @sa conversionCharStrToPid()
 */
size_t conversionCharStrToSize(char * str) {
	pid_t tmp = conversionCharStrToPid(str);
	return (size_t) tmp;
}	
	
/**
 * @brief Main exécutant le programme d'interception d'un processus dont le PID en donné en entier
 * @details Utilise PTRACE pour s'attacher et modifie le code d'une fonction donnée en dur (actuellement)
 */
int main(int argc, char * argv[]) {
	
	// Controle de l'entrée
	if (argc != 3) {
		cout << argv[0] << " <pid> <size>" << endl;
		cout << "\t <pid> est l'identifiant du processus sur lequel il faut tenter de s'attacher." << endl;
		cout << "\t <size> est la taille en octets d'espace mémoire à allouer pour le nouveau code." << endl;
		return -1;
	}

	pid_t pidCible = conversionCharStrToPid(argv[1]);
	size_t allocSize = conversionCharStrToSize(argv[2]);
	cout << "DEBUG - allocSize : " << allocSize << endl;

	// Tentative d'attache
	if (ptrace(PTRACE_ATTACH, pidCible, 0, 0) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_ATTACH" << endl;
		return -1;
	}
	
	system("");
	
	char path[30];
	sprintf(path, "/proc/%s/mem", argv[1]);
	
	siginfo_t childInfo;
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus se stoppe bien
	
	//Challenge 2
	
	struct user_regs_struct emplRegs;
	
	if (ptrace(PTRACE_GETREGS, pidCible, 0, &emplRegs) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_GETREGS" << endl;
		return -1;
	}
	
	struct user_regs_struct emplRegsCopy = emplRegs;
	/*l'adresse de posix_memalign se recupère en faisant dans l'ordre :
	-pidof interceptable
	-cat /proc/pidInterceptable/maps (recherche de la libc)
	-nm (emplacement libc) /usr/lib64/libc-2.24.so | grep posix_memalign (pour connaitre le code de posix_memalign)
	*/
	
	FILE *memoire = fopen(path, "w");
	fseek(memoire, ADDR_FN, SEEK_SET); //Se place au niveau de la fonction a modifier
	char trap[5] = {(char)0xcc,(char)0xff,(char)0xd0,(char)0xcc};
	size_t save = fwrite(&trap, 4*sizeof(char), 1, memoire);
	
	if (save < 0){
		cout << "Erreur lors de l'ecriture" << endl;
		cout << strerror(errno) << endl; 
	}
	
	
	/*
	int *posMemoire;
	size_t alignment = 42;
	
	int res = posix_memalign((void**) &posMemoire, alignment, allocSize);
	if (res != 0){
		cout << "Erreur lors de l'allocation de memoire (posix_memalign)" << endl;
	}
	*/
	
	
	emplRegs.rsp -= 8;
	
	if (ptrace(PTRACE_POKETEXT, pidCible, emplRegs.rdi, emplRegs.rsp) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_POKETEXT" << endl;
		return -1;
	}
	
	emplRegs.rax = 0x88ea0; //Emplacement de posix_memalign dans la libc (a calculer)
	//emplRegs.rdi = ; //Parametre 1 de posix_memalign()
	emplRegs.rsi = 42; //Parametre 2 de posix_memalign()
	emplRegs.rdx = allocSize; //Parametre 3 de posix_memalign()
	
	
	if (ptrace(PTRACE_SETREGS, pidCible, 0, &emplRegs) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_SETREGS" << endl;
		return -1;
	}
	
	
	
	emplRegs = emplRegsCopy;
	
	if (ptrace(PTRACE_SETREGS, pidCible, 0, &emplRegs) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_SETREGS" << endl;
		return -1;
	}
	
	// Detachement du processus et relance le processus
	if (ptrace(PTRACE_DETACH, pidCible, 0, 0) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_DETACH" << endl;
		return -1;
	}
	

	return 0;
}

