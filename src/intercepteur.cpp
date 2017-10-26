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

const long ADDR_FN = 400546;

using namespace std;

/**
 * @brief Fonction convertissant une chaîne de caractère C représentant un entier en un PID
 * @param str une chaîne de caractère C représentant un entier
 * @pre str doit représenter un entier
 * @todo Mettre en place une exception pour avertir l'utilisateur dans les rares cas où on peut détecter une mauvaise entrée
 */
pid_t conversionCharStrToPid(char * str) {
	int tmp = atoi(str);
	if (tmp == 0) {
		cerr << "ERROR : Incorrect pid \a" << endl;
		// TODO : Une vraie exception des familles
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
		cout << argv[0] << " <pid>" << endl;
		cout << "Où <pid> est l'identifiant du processus sur lequel il faut tenter de s'attacher." << endl;
		return -1;
	}

	pid_t pidCible = conversionCharStrToPid(argv[1]);

	// Tentative d'attache
	if (ptrace(PTRACE_ATTACH, pidCible, 0, 0) != 0) {
		cerr << "ERROR : Undefined error on PTRACE" << endl;
		return -1;
	}
	
	stringstream ss;
	ss << "/proc/" << pidCible << "/mem";
	
	siginfo_t childInfo;
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus se stoppe bien
	
	char path[30];
	ss.get(path, 30);
	
	//cout << path << endl;
	
	int memoire = open(path, O_WRONLY);
	
	//fseek(memoire, ADDR_FN, SEEK_SET); //Se place au niveau de la fonction a modifier
	signed char trap = {0xCC};
	ssize_t save = pwrite(memoire, &trap, sizeof(trap), ADDR_FN);
	
	cout << save << endl;
	
	if (save < 0){
		cout << "Erreur lors de l'ecriture" << endl;
		cout << strerror(errno) << endl; 
	}
	
	/*fseek(memoire, ADDR_FN, SEEK_SET);
	fgets(memoire);*/
	
	//ofstream memoire(ss.str() , ios::out);
	/*memoire.seekp(ADDR_FN, ios::beg);
	memoire.seekp(ADDR_FN, ios::beg);
	printf(memoire);
	while (1){
		memoire << "bob" << endl;
		memoire << "alice" << endl ;
		memoire << "bob" << endl ;
	}*/
	// Detachement du processus et relance le processus
	if (ptrace(PTRACE_DETACH, pidCible, 0, 0) != 0) {
		cerr << "ERROR : Undefined error on PTRACE" << endl;
		return -1;
	}
	

	cout << "bob" << endl;

	return 0;
}

