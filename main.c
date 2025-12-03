#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <locale.h>
#include <wchar.h>

#define HAUTEUR 40
#define LARGEUR 255
#define MAX_VEHICULES 10

// Structure véhicule -> recuperer dans l'enonce
typedef struct voiture VEHICULE;
struct voiture {
    char direction;      // N, S, E, O
    int posx, posy;     // Position
    int vitesse;        // Vitesse
    char alignement;    // g ou d
    char type;          // v, c, m (voiture, camion, moto)
    char Carrosserie[4][30];
    int code_couleur;
    char etat;          // '1' actif, '0' inactif
    unsigned long int tps;
    struct voiture * NXT;
};

// Variables globales
wchar_t plan[HAUTEUR][LARGEUR];
VEHICULE vehicules[MAX_VEHICULES];
int nb_vehicules = 0;
VEHICULE joueur;



/* ================== Couleurs ================== */
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
        case RED:    printf("\033[1;31m"); break;
        case GREEN:  printf("\033[1;32m"); break;
        case BLUE:   printf("\033[1;34m"); break;
        case YELLOW: printf("\033[1;33m"); break;
        case BLACK:  printf("\033[0m");    break; // reset
    }
}




// Fonction de détection de touche non-bloquante -> recuperer dans l'enonce
char key_pressed() {
    struct termios oldterm, newterm;
    int oldfd;
    char c, result = 0;

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


// Effacer l'écran et positionner le curseur
void clear_screen() {
    printf("\033[2J");      // Efface l'écran
    printf("\033[H");       // Curseur en haut à gauche
}


// deplacer le curseur à la position x et y
// %d;%dH -> place le curseur à la ligne %d et à la colonne %d
void goto_xy(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}


// Charger le plan depuis un fichier
void charger_plan(const char* fichier) {
    FILE *f = fopen(fichier, "r");

    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", fichier);
        exit(1);
    }

    //pour avoir des caractères ASCI plus large
    wint_t wc;

    int y = 0, x = 0, wmax = 0;
    // On charge notre tableau
    while (y < HAUTEUR && (wc = fgetwc(f)) != WEOF) {
        if (wc == L'\n') {
            plan[y][x] = L'\0';
            if (x > wmax) wmax = x;
            x = 0;
            y++;
        } else if (x < LARGEUR - 1) {
            plan[y][x++] = (wchar_t)wc;
        }
    }

    // Si le fichier ne finit pas par un '\n'
    if (y < HAUTEUR) {
        plan[y][x] = L'\0';
    }

    fclose(f);
}

// Creation de vehicule
void charger_vehicule(VEHICULE *v, const char* fichier, char type)
{
    FILE *f = fopen(fichier, "r");

    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", fichier);
        return;
    }

    v->type = type;
    v->etat = '1';
    v->vitesse = 1;
    v->direction = 'N';

    for (int i = 0; i < 4; i++) {
        if (fgets(v->Carrosserie[i], 30, f)) {
            int len = strlen(v->Carrosserie[i]);
            if (len > 0 && v->Carrosserie[i][len-1] == '\n') {
                v->Carrosserie[i][len-1] = '\0';
            }
        } else {
            strcpy(v->Carrosserie[i], "");
        }
    }
    fclose(f);
}

// Vérifier les collisions
int collision_voiture(int cx, int cy) {
    int x = cx - 1; // coin haut-gauche
    int y = cy - 1;

    // bornes pour le carré [x..x+2], [y..y+2]
    if (x < 0 || x + 2 >= LARGEUR || y < 0 || y + 2 >= HAUTEUR) return 1;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            wint_t wc = plan[y + i][x + j];

            if (wc == L'─' || wc == L'┐'
             || wc == L'┌' || wc == L'│'
             || wc == L'├' || wc == L'┤'
             || wc == L'└' || wc == L'┘'
             || wc == L'║' || wc == L'═'
             || wc == L'╗' || wc == L'╔'
             || wc == L'╚' || wc == L'╝'
             || wc == L'├' || wc == L'╝')
                {
                return 1;
             }
        }
    }
    return 0;
}


// Afficher le plan
void afficher_plan() {
    goto_xy(0, 0);
    for (int i = 0; i < HAUTEUR; i++) {
        wprintf(L"%ls\n", plan[i]);
    }
}

// Afficher un véhicule
void afficher_vehicule(VEHICULE *v) {
    if (v->etat != '1') return;

    // Couleur selon le type
    if (v->type == 'v') printf("\033[31m");      // Rouge pour voiture
    else if (v->type == 'c') printf("\033[34m"); // Bleu pour camion
    else if (v->type == 'm') printf("\033[32m"); // Vert pour moto

    for (int i = 0; i < 4; i++) {
        goto_xy(v->posx, v->posy + i);
        printf("%s", v->Carrosserie[i]);
    }
    printf("\033[0m"); // Reset couleur
}

// Déplacer le joueur
void deplacer_joueur(char direction) {
    int new_x = joueur.posx;
    int new_y = joueur.posy;

    char ancienneDirectionn = joueur.direction;

    switch(direction) {
        case 'z': new_y--; joueur.direction = 'N'; break;
        case 's': new_y++; joueur.direction = 'S'; break;
        case 'q': new_x--; joueur.direction = 'O'; break;
        case 'd': new_x++; joueur.direction = 'E'; break;
    }

    // new_x/new_y = centre de la voiture
    if (!collision_voiture(new_x, new_y))
    {
        joueur.posx = new_x;
        joueur.posy = new_y;

        if (ancienneDirectionn != direction)
        {
            if (joueur.direction == 'E')
                charger_vehicule(&joueur, "voiture_D.txt", 'v');
            else if (joueur.direction == 'N')
                charger_vehicule(&joueur, "voiture_Z.txt", 'v');
            else if (joueur.direction == 'S')
                charger_vehicule(&joueur, "voiture_Z.txt", 'v');
            else
                charger_vehicule(&joueur, "voiture_D.txt", 'v');
        }
    }
}


// Affiche info position et déplacement
void afficher_instructions() {
    goto_xy(0, HAUTEUR + 2);
    printf("ZQSD: Déplacer la voiture rouge");
    goto_xy(0, HAUTEUR + 3);
    printf("ESC: Quitter");
    goto_xy(0, HAUTEUR + 4);
    printf("Position: (%d,%d) Direction: %c", joueur.posx, joueur.posy, joueur.direction);
}

void afficher_fichier(const char* fichier, enum COLOR color) {
    FILE *f = fopen(fichier, "r");
    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", fichier);
        exit(1);
    }

    setColor(color);
    if (color == YELLOW) {
        printf("\t\t\t");
    }

    wint_t wc;
    while ((wc = fgetwc(f)) != WEOF)
    {
        if (wc == L'─' || wc == L'┐' || wc == L'┌' || wc == L'│'
         || wc == L'├' || wc == L'┤' || wc == L'└' || wc == L'┘') {
            setColor(GREEN);
            putwchar(wc);
            setColor(color);
         } else {
             putwchar(wc);
         }
    }
    printf("\n");
    setColor(BLACK);
    fclose(f);
}

int main() {

    //l’Unicode (wchar_t)
    setlocale(LC_ALL, "");

    // Initialisation
    printf("Chargement du parking...\n");

    // Charger le plan
    charger_plan("parking.txt");

    // Initialiser le joueur
    charger_vehicule(&joueur, "voiture_Z.txt", 'v');
    joueur.posx = 74;
    joueur.posy = 36;
    joueur.code_couleur = 31; // Rouge

    printf("★ Bienvenue cher joueur ! ★\n");
    afficher_fichier("logo.txt", RED);
    printf("\n★ Pret à commencer l'aventure ? Votre destin vous attend ! ★\n\n");

    printf("★ Regle du jeu ★\n");
    printf("Jeu permettant de simuler le stationnement de voitures dans un parking payant\n\n");

    printf("Pour commencer veuillez choisir votre mode de jeu :\n");
    printf("[1] Fluide : moins de vehicules seront generes et tout roule normalement\n");
    printf("[2] Charge : plus de vehicules generes pouvant entrainer des bouchons ou meme des accidents dans le parking\n");


    while (1) {
        int mode;
        printf("Veuillez choisir votre mode de jeu : ");
        fflush(stdout);

        if (scanf("%i", &mode) != 1) {
            printf("Veuillez choisir un mode de jeu valide.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF) { }
            continue;
        }
        /* Purger le \n restant pour ne pas polluer le premier getchar() du jeu */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }

        switch (mode) {
            case 1: {
                // Boucle principale
                char touche;
                int running = 1;

                while (running) {
                    // Effacer et redessiner
                    clear_screen();

                    // Afficher le plan
                    afficher_plan();

                    // Afficher les véhicules
                    for (int i = 0; i < nb_vehicules; i++) {
                        afficher_vehicule(&vehicules[i]);
                    }

                    // Afficher le joueur
                    afficher_vehicule(&joueur);

                    // Afficher les instructions
                    afficher_instructions();

                    // Vérifier les touches
                    touche = key_pressed();
                    if (touche != 0) {
                        switch(touche) {
                            case 'z':
                            case 'q':
                            case 's':
                            case 'd':
                                deplacer_joueur(touche);
                                break;
                            case 27: // ESC
                                running = 0;
                                break;
                        }
                    }

                    usleep(50000);
                }

                clear_screen();
                printf("Merci d'avoir joué à Take IT !\n");
                return 0;
            }
            case 2:
                printf("Merci d'avoir joué au Take IT !\n");
                return 0;

            default:
                printf("Mode inconnu. Au revoir.\n");
                return 0;
        }
    }
}