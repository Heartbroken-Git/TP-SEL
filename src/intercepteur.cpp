/**
 * @file intercepteur.cpp
 * @author Corentin CHÉDOTAL
 * @copyright LPRAB 1.0
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

//const long ADDR_FN = 0x400546;

using namespace std;

/**
 * @brief Fonction convertissant une chaîne de caractère C représentant un entier en un PID
 * @param str une chaîne de caractère C représentant un entier
 * @return l'entier représenté dans str dans le type pid_t
 * @pre str doit représenter un entier
 * @warning Comportement indéfini si str est une chaîne ne contenant pas un entier. Une tentative de vérification d'entrée est en place mais elle ne doit en aucun cas constituer une preuve de bon fonctionnement si aucune erreur n'est retournée.
 * @todo Mettre en place une exception pour avertir l'utilisateur dans les rares cas où on peut détecter une mauvaise entrée
 * @deprecated La collecte du PID se fait désormais par le biais de recupNoProcessus(). La fonction est encore conservée car utilisée par conversionCharStrToSize() pour définir la taille de l'espace à écrire dans l'interceptable. Cependant elle sera supprimée sous peu et conversionCharStrToSize() devra avoir été mise à jour.
 */
pid_t conversionCharStrToPid(char * str) {
	int tmp = atoi(str);
	if (tmp == 0) {
		cerr << "ERROR : Incorrect input char str \a" << endl;
		return -1;
	} 
	return (pid_t) tmp;
}

/**
 * @brief Fonction convertissant une chaîne de caractère C représentant un entier en un type de size_t
 * @details Pour éviter des doublons de code utilise la fonction conversionCharStrToPid() avec un pid_t comme représentation intermédiaire
 * @param str une chaîne de caractère C représentant un entier
 * @return l'entier représenté par str dans le type C size_t
 * @pre str doit représenter un entier
 * @warning Comportement indéfini si str est une chaîne ne contenant pas un entier. Une tentative de vérification d'entrée est en place mais elle ne doit en aucun cas constituer une preuve de bon fonctionnement si aucune erreur n'est retournée.
 * @sa conversionCharStrToPid()
 */
size_t conversionCharStrToSize(char * str) {
	pid_t tmp = conversionCharStrToPid(str);
	return (size_t) tmp;
}

/**
 * @brief Fonction récupérant l'adresse d'une fonction donnée dans un code exécutable fourni en entrée
 * @details Utilise les commandes UNIX ps, grep et cut pour récupérer l'adresse et la préparer pour le programme en C++, le tout dans un pipeline
 * @param nomFichier une chaîne de caractère C donnant le nom du fichier exécutable à explorer
 * @param nomFonction nom de la fonction à retrouver dans le code exécutable
 * @return l'adresse de la fonction au format long
 * @pre Le fichier et la fonction donnés en entrée doivent exister
 * @note En l'état actuel la fonction n'a accès qu'aux exécutables présent dans le dossier "bin"
 */
long recupAdresseFonction(char * nomFichier, char * nomFonction) {
	FILE *in;
	string cmd;
	char buff[17];
	
	
	cmd = "nm bin/";
	string var(nomFichier);
	cmd += var;
	cmd += " | grep ";
	string var2(nomFonction);
	cmd += var2;
	cmd += " | cut -b-16";
	
	
	if(!(in = popen(cmd.c_str(), "r"))) {
		cerr << "ERROR : Undefined error upon opening pipe" << endl;
		return(-1);
	}
	
	if (fgets(buff, sizeof(buff), in) == NULL) {
		cerr << "ERROR : Empty buffer" << endl;
		return(-2);
	}
	
	char hexbuff[19];
	hexbuff[0] = '0';
	hexbuff[1] = 'x';
	for (int i = 0; i < 17; i++) {
		hexbuff[i+2] = buff[i];
	}
	
	return strtol(hexbuff, (char**)0, 0);
}

/**
 * @brief Fonction récupérant le PID fourni par l'OS d'un processus dont le nom est donné en entrée
 * @details Fait appel à la commande UNIX pidof utilisée dans un pipeline. Retourne le PID donné par le système d'exploitation au processus si celui-ci tourne effectivement sur la machine.
 * @param nomFichier chaîne de caractère C du nom du processus
 * @return le pid_t du processus donné en entrée
 * @pre Le processus dont le nom est donné en entrée doit effectivement exister et être en cours d'exécution
 */
pid_t recupNoProcessus(char * nomFichier) {
	FILE *in;
	string cmd;
	char buff[10];
	
	cmd = "pidof ";
	string var(nomFichier);
	cmd += var;
	
	if(!(in = popen(cmd.c_str(), "r"))) {
		cerr << "ERROR : Undefined error upon opening pipe" << endl;
		return(-1);
	}
	
	if (fgets(buff, sizeof(buff), in) == NULL) {
		cerr << "ERROR : Empty buffer" << endl;
		return(-2);
	}
	
	return (pid_t) atoi(buff);
}
	
/**
 * @brief Main exécutant le programme d'interception d'un processus dont le PID en donné en entier
 * @details Utilise PTRACE pour s'attacher et modifie le code d'une fonction donnée en dur (actuellement)
 */
int main(int argc, char * argv[]) {
	
	// Controle de l'entrée
	if (argc != 4) {
		cout << argv[0] << " <nomFichier> <nomFonction> <size>" << endl;
		cout << "\t <nomFichier> est le nom du binaire à intercepter. \033[1m Doit être en cours d'exécution.\033[0m" << endl;
		cout << "\t <nomFonction> est le nom de la fonction du binaire à détourner pour placer le code." << endl;
		cout << "\t <size> est la taille en octets d'espace mémoire à allouer pour le nouveau code." << endl;
		return -1;
	}

	// Conversion et affectation des entrées
	pid_t pidCible = recupNoProcessus(argv[1]);
	const long ADDR_FN = recupAdresseFonction(argv[1],argv[2]);
	size_t allocSize = conversionCharStrToSize(argv[3]);
	
	cerr << "DEBUG - allocSize : " << allocSize << endl;
	cerr << "DEBUG - addr : " << ADDR_FN << endl;
	cerr << "DEBUG - noProcess : " << pidCible << endl;

	// Tentative d'attache
	if (ptrace(PTRACE_ATTACH, pidCible, 0, 0) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_ATTACH" << endl;
		return -1;
	}
	
	char path[30];
	sprintf(path, "/proc/%u/mem", pidCible);
	
	
	
	siginfo_t childInfo;
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus s'arrête bien
	
	//Challenge 2
	
	struct user_regs_struct emplRegs;
	
	if (ptrace(PTRACE_GETREGS, pidCible, 0, &emplRegs) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_GETREGS" << endl;
		return -1;
	}
	
	struct user_regs_struct emplRegsCopy = emplRegs;
	
	
	FILE *memoire = fopen(path, "w");
	fseek(memoire, ADDR_FN, SEEK_SET); //Se place au niveau de la fonction a modifier
	char trap[5] = {(char)0xcc,(char)0xff,(char)0xd0,(char)0xcc};
	size_t save = fwrite(&trap, 5*sizeof(char), 1, memoire);
	
	if (save < 0){
		cerr << "Erreur lors de l'ecriture" << endl;
		cerr << strerror(errno) << endl;
	}
	
	
	if (fclose(memoire) != 0){
		cerr << "Erreur lors de la fermeture du fichier" << endl;
		cerr << strerror(errno) << endl;
	}
	
	
	//Permet de se placer au niveau du premier trap
	if (ptrace(PTRACE_CONT, pidCible, NULL, NULL) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_CONT (n°1)" << endl;
		return -1;
	}
	
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus s'arrête bien
	
	emplRegs.rax = (long int) 0x88ea0; //Emplacement de posix_memalign dans la libc (a calculer)
	emplRegs.rsp -= 8;
	
	if (ptrace(PTRACE_POKETEXT, pidCible, emplRegs.rdi, emplRegs.rsp) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_POKETEXT" << endl;
		return -1;
	} //Parametre 1 de posix_memalign() modifié par ptrace(PTRACE_POKETEXT)
	emplRegs.rsi = (long int) 42; //Parametre 2 de posix_memalign()
	emplRegs.rdx = (long int) allocSize; //Parametre 3 de posix_memalign()
	
	
	if (ptrace(PTRACE_SETREGS, pidCible, 0, &emplRegs) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_SETREGS" << endl;
		return -1;
	}
	
	//Permet d'avoir executer la fonction posix_memalign avec les bons paramètres
	if (ptrace(PTRACE_CONT, pidCible, NULL, NULL) != 0) {
		cerr << "ERROR : Undefined error on PTRACE_CONT (n°2)" << endl;
		return -1;
	}
	
	waitid(P_PID, pidCible, &childInfo, WSTOPPED); // Attente que le processus s'arrête bien
	
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

