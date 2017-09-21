#include <stdio>
#include <sys/ptrace.h>

using namespace std;

int main(int argc, char * argv[]) {
	if (argc != 2) {
		cout << "intercepteur.cpp <pid>" << endl;
		cout << "Où <pid> est l'identifiant du processus sur lequel il faut tenter de s'attacher." << endl;
		return(-1);
	}

	// Tentative d'attache
	// TODO
}
