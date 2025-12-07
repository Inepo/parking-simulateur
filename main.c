#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <locale.h>
#include <wchar.h>
#include <ncurses.h>//pour l'affichage

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

void lancer_musique() {
    system("play -q musique.wav repeat - &");
}

int est_mur(wchar_t wc)
{
    return (wc == L'─' || wc == L'┐'
         || wc == L'┌' || wc == L'│'
         || wc == L'├' || wc == L'┤'
         || wc == L'└' || wc == L'┘'
         || wc == L'║' || wc == L'═'
         || wc == L'╗' || wc == L'╔'
         || wc == L'╚' || wc == L'╝');
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

// deplacer le curseur à la position x et y
// %d;%dH -> place le curseur à la ligne %d et à la colonne %d
void goto_xy(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}


// Charger le plan depuis un fichier
void charger_parking(const char* fichier) {
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

    if (x < 0 || x + 2 >= LARGEUR || y < 0 || y + 2 >= HAUTEUR) return 1;

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            wint_t wc = plan[y + i][x + j];

            if (est_mur(wc)) return 1;
        }
    }

    return 0;
}

// Afficher le plan
void afficher_parking()
{
    for (int y = 0; y < HAUTEUR; y++) {
        for (int x = 0; x < LARGEUR && plan[y][x] != L'\0'; x++) {
            wchar_t wc = plan[y][x];

            if (est_mur(plan[y][x])) {
                attron(COLOR_PAIR(1));
            } else {
                attron(COLOR_PAIR(2));
            }

            mvprintw(y, x, "%lc", wc);
        }
    }

    attroff(COLOR_PAIR(1));
    attroff(COLOR_PAIR(2));
}

// Afficher un véhicule
void afficher_vehicule(VEHICULE *v)
{
    if (v->etat != '1') return;

    // Couleur selon le type
    if (v->type == 'v') attron(COLOR_PAIR(3)); // Rouge pour voiture
    else if (v->type == 'c') printf("\033[34m"); // Bleu pour camion
    else if (v->type == 'm') printf("\033[32m"); // Vert pour moto

    for (int i = 0; i < 4; i++) {
        //goto_xy(v->posx, v->posy + i);
        mvprintw(v->posy + i, v->posx, "%s", v->Carrosserie[i]);
    }

    if (v->type == 'v') attroff(COLOR_PAIR(3));
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
void afficher_instructions()
{
    attron(COLOR_PAIR(2));
    // sous le parking
    int ligne = HAUTEUR + 1;

    mvprintw(ligne,     0, "ZQSD: Deplacer la voiture rouge");
    mvprintw(ligne + 1, 0, "ESC: Quitter");
    mvprintw(ligne + 2, 0, "Position: (%d,%d)", joueur.posx, joueur.posy);

    attroff(COLOR_PAIR(2));
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
        putwchar(wc);
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
    charger_parking("parking.txt");

    // Initialiser le joueur
    charger_vehicule(&joueur, "voiture_Z.txt", 'v');
    joueur.posx = 74;
    joueur.posy = 36;
    joueur.code_couleur = 31; // Rouge

    afficher_fichier("logo.txt", RED);

    printf("★ Regle du jeu ★\n");
    printf("Jeu permettant de simuler le stationnement de voitures dans un parking payant\n\n");

    printf("Pour commencer veuillez choisir votre mode de jeu :\n");
    printf("[1] Fluide : moins de vehicules seront generes et tout roule normalement\n");
    printf("[2] Charge : plus de vehicules generes pouvant entrainer des bouchons ou meme des accidents dans le parking\n\n");


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

                initscr();
                start_color();
                use_default_colors();

                // cyan
                init_pair(1, COLOR_CYAN, -1);
                // jaune
                init_pair(2, COLOR_YELLOW, -1);
                // rouge
                init_pair(3, COLOR_RED, -1);

                while (running) {
                    // Effacer le contenue de l'ecran virtuel (fournit par ncurses)
                    clear();

                    afficher_parking();


                    for (int i = 0; i < nb_vehicules; i++) {
                        afficher_vehicule(&vehicules[i]);
                    }

                    // Afficher le joueur principal
                    afficher_vehicule(&joueur);

                    afficher_instructions();

                    // Dessine le contenu tu terminal virtuel dans le vrais terminal (fournit par ncurses)
                    refresh();

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
                            case 27: // touche échape
                                running = 0;
                                break;
                        }
                    }

                    //pour eviter les scitillements
                    usleep(50000);
                }

                //Ferme le mode ncurses pour remettre le terminal normalement
                endwin();
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