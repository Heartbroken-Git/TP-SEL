/**
 * @file interceptable.c
 * @author Corentin CHÉDOTAL
 * @copyright TBD
 * @brief Fichier contenant un programme "bateau" destiné à être intercepté
 */

#include <stdio.h>
#include <unistd.h>

/**
 * @brief Fonction affichant un entier donné en paramère à l'écran
 * @details Fonction dont le code devrait être à terme modififé par l'intercepteur
 * @param disp un entier devant être affiché sur terminal
 */
void interceptable(int disp) {
	printf("%d \a \n", disp);
}

/**
 * @brief Main affichant un compte des secondes passées depuis le lancement du programme
 * @details Fait appel à la fonction interceptable() toutes les secondes pour faciliter sa modification
 */
int main() {
	int i = 0;
	while (1) {
		interceptable(i);
		i++;
		sleep(1);
	}
	return(0);
}
