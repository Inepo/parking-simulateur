//
// Created by Ines KADJI on 30/12/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <wchar.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>
#include <time.h>

#ifdef __APPLE__
    #include <ncurses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

#include "../library/configuration.h"

char getToucheViaDirection(char direction) {
    if (direction == 'N') return 'z';
    if (direction == 'S') return 's';
    if (direction == 'O') return 'q';

    return 'd';
}

char getDirectionOppose(char direction) {
    if (direction == 'N') return 'S';
    if (direction == 'S') return 'N';
    if (direction == 'O') return 'E';

    return 'O';
}

// affichage d'un fichier texte sur la console, notamment le logo
void afficherFichier(const char* fichier) {
    FILE *f = fopen(fichier, "r");
    if (!f) {
        fprintf(stderr, "Erreur: impossible d'ouvrir %s\n", fichier);
        exit(1);
    }

    printf("\033[1;31m");

    // Lire en mode binaire caractère par caractère
    int c;
    while ((c = fgetc(f)) != EOF) {
        putchar(c);
    }

    printf("\033[0m\n");
    fclose(f);
}

// Verifie si le caractère est un mur
int isMur(wchar_t wc)
{
    return (wc == L'─' || wc == L'┐'
         || wc == L'┌' || wc == L'│'
         || wc == L'├' || wc == L'┤'
         || wc == L'└' || wc == L'┘'
         || wc == L'║' || wc == L'═'
         || wc == L'╗' || wc == L'╔'
         || wc == L'╚' || wc == L'╝'
         || wc == L'█');
}


/* *
 * Fonction de détection de touche non-bloquante
 * -> recuperer dans l'enonce
 * */
char key_pressed() {
    struct termios oldterm, newterm;
    int oldfd;
    int c, result = 0;

    tcgetattr(STDIN_FILENO, &oldterm);
    newterm = oldterm;
    newterm.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newterm);

    oldfd = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldfd | O_NONBLOCK);

    c = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
    fcntl(STDIN_FILENO, F_SETFL, oldfd);

    if (c != EOF) {
        ungetc(c, stdin);
        result = getchar();
    }
    return result;
}


void colorerMur(VEHICULE *vehiculeCourant, wchar_t plan[HAUTEUR][LARGEUR]) {
    if (!isDansPlaceParking(vehiculeCourant, plan))vehiculeCourant->statut = 'E';
    else {
        COORDONEE coord;
        for (int i = 0; i < 5; ++i) {
            coord = vehiculeCourant->coord_mur_place[i];
            if (plan[coord.y][coord.x] == L'█') {
                attron(COLOR_PAIR(3));
                mvprintw(coord.y, coord.x, "%lc", plan[coord.y][coord.x]);
                attroff(COLOR_PAIR(3));
            }
        }
    }
}