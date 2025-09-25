#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void afficher_fichier(const char* fichier) {
    FILE *f = fopen(fichier, "r");
    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", fichier);
        exit(1);
    }

    char ligne[256];

    while (fgets(ligne, sizeof(ligne), f)) {
        printf("%s", ligne);
    }

    fclose(f);
}

int main(void) {
    printf("★ Bienvenue cher joueur ! ★\n");
    afficher_fichier("../logo.txt");
    printf("\n★ Prêt à commencer l'aventure ? Votre destin vous attend ! ★\n\n");

    printf("★ Régle du jeu ★\n");
    printf("Jeu permettant de simuler le stationnement de voitures dans un parking payant\n\n");

    printf("Pour commencer veuillez choisir votre mode de jeu :\n");
    printf("[1] Fluide : moins de véhicules seront générés et tout roule normalement\n");
    printf("[2] Chargé : plus de véhicules générés pouvant entraîner des bouchons ou même des accidents dans le parking\n");
    //printf("[3] Mano : prenez le controle de votre voiture\n");

    while (true) {
        int mode;
        printf("Veuillez choisir votre mode de jeu : ");

        if (scanf("%i", &mode) != 1) {
            printf("Veuillez choisir un mode de jeu valide.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            continue;
        }

        switch (mode) {
            case 1:
                afficher_fichier("../parking.txt");
                exit(0);
            case 2:
                afficher_fichier("../parking.txt");
                exit(0);
            default:
                exit(0);
        }
    }

    return 0;
}