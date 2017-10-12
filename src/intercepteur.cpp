#include <iostream>
#include <cstdlib> // pour atoi()
#include <exception>
#include <sys/ptrace.h>
#include <sys/types.h> // pour pid_t et signinfo_t
#include <sys/wait.h> // pour waitid()
#include <fstream>
#include <string>
#include <sstream>

#define ADDR_FN 0x54d

using namespace std;


// TODO : Documentation
pid_t conversionCharStrToPid(char * str) {
	int tmp = atoi(str);
	if (tmp == 0) {
		cerr << "ERROR : Incorrect pid \a" << endl;
		// TODO : Une vraie exception des familles
	} 
	return (pid_t) tmp;
}

// TODO : Documentation
// WARNING : Undefined behavior si ce n'est pas un entier en entrée
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
	siginfo_t childInfo;
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus se stoppe bien
	
	stringstream ss;
	ss << "/proc/" << pidCible << "/mem";
	
	ofstream memoire(ss.str() , ios::out);
	memoire.seekp(ADDR_FN, ios::beg);
	
	while (1){
		memoire << "bob" << endl;
		memoire << "alice" << endl ;
		memoire << "bob" << endl ;
	}
	// Detachement du processus et relance le processus
	if (ptrace(PTRACE_DETACH, pidCible, 0, 0) != 0) {
		cerr << "ERROR : Undefined error on PTRACE" << endl;
		return -1;
	}
	

	cout << "bob" << endl;

	return 0;
}

