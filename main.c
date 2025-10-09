#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>

enum COLOR {
    RED = 0,
    GREEN = 1,
    BLUE = 2,
    YELLOW = 3,
    BLACK = 4,
};

void setColor(enum COLOR color)
{
    switch (color) {
        case RED:
            printf("\033[1;31m");
            break;
        case GREEN:
            printf("\033[1;32m");
            break;
        case BLUE:
            printf("\033[1;34m");
            break;
        case YELLOW:
            printf("\033[1;33m");
            break;
        case BLACK:
            printf("\033[0m"); // Reset couleur
            break;

    }
}

void afficher_fichier(const char* fichier, enum COLOR color) {
    setlocale(LC_ALL, "");
    FILE *f = fopen(fichier, "r");
    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", fichier);
        exit(1);
    }

    char ligne[256];

    setColor(color);

    /*while (fgets(ligne, sizeof(ligne), f)) {
        if (strcmp(ligne, "─") == 0 || strcmp(ligne, "┐") == 0
            || strcmp(ligne, "┌") == 0 || strcmp(ligne, "│") == 0) {
            setColor(GREEN);
            printf("%s", ligne);
            setColor(BLACK);
        }
        else {
            printf("%s", ligne);
        }
    }*/

    wint_t wc;

    while ((wc = fgetwc(f)) != WEOF)
    {
        if (wc == L'─' || wc == L'┐'
            || wc == L'┌' || wc == L'│'
            | wc == L'├' || wc == L'┤'
            | wc == L'└' || wc == L'┘') {
            setColor(GREEN);
            putwchar(wc);
            setColor(color);
            }
        else {
            putwchar(wc);
        }
    }

    setColor(BLACK);

    fclose(f);
}

int main(void) {
    printf("★ Bienvenue cher joueur ! ★\n");
    afficher_fichier("../logo.txt", BLUE);
    printf("\n★ Pret à commencer l'aventure ? Votre destin vous attend ! ★\n\n");

    printf("★ Regle du jeu ★\n");
    printf("Jeu permettant de simuler le stationnement de voitures dans un parking payant\n\n");

    printf("Pour commencer veuillez choisir votre mode de jeu :\n");
    printf("[1] Fluide : moins de vehicules seront generes et tout roule normalement\n");
    printf("[2] Charge : plus de vehicules generes pouvant entrainer des bouchons ou meme des accidents dans le parking\n");
    //printf("[3] Mano : prenez le controle de votre voiture\n");

    while (1) {
        int mode;
        printf("Veuillez choisir votre mode de jeu : ");

        if (scanf("%i", &mode) != 1) {
            printf("Veuillez choisir un mode de jeu valide.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            continue;
        }
        enum COLOR color;
        switch (mode) {
            case 1:
                printf("\n");
                color = RED;
                afficher_fichier("../parking.txt", color);
                exit(0);
            case 2:
                printf("\n");
                color = BLUE;
                afficher_fichier("../parking.txt", color);
                exit(0);
            default:
                exit(0);
        }
    }

    return 0;
}