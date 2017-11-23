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
		cerr << "ERROR : Undefined error on PTRACE" << endl;
		return -1;
	}
	
	//stringstream ss;
	//ss << "/proc/" << pidCible << "/mem";
	char path[30];
	sprintf(path, "/proc/%s/mem", argv[1]);
	
	siginfo_t childInfo;
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus se stoppe bien
	
	//cout << path << endl;
	
	FILE *memoire = fopen(path, "w");
	fseek(memoire, ADDR_FN, SEEK_SET); //Se place au niveau de la fonction a modifier
	char trap = 0xCC;
	size_t save = fwrite(&trap, sizeof(char), 1, memoire);
	
	cout << save << trap << endl;
	
	if (save < 0){
		cout << "Erreur lors de l'ecriture" << endl;
		cout << strerror(errno) << endl; 
	}
	
	// Detachement du processus et relance le processus
	if (ptrace(PTRACE_DETACH, pidCible, 0, 0) != 0) {
		cerr << "ERROR : Undefined error on PTRACE" << endl;
		return -1;
	}
	

	cout << "bob" << endl;

	return 0;
}

