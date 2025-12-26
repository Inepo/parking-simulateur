#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <locale.h>
#include <wchar.h>
#include <time.h>
#include <ncurses.h>//pour l'affichage

#define HAUTEUR 50
#define LARGEUR 130
#define MAX_VEHICULES 10

// Structure véhicule -> recuperer dans l'enonce
typedef struct voiture VEHICULE;
struct voiture {
    char direction;      // N, S, E, O
    char precedente_direction;
    int count_meme_direction;
    int posx, posy;     // Position
    int vitesse;        // Vitesse
    char alignement;    // g ou d
    char type;          // v, c, m (voiture, camion, moto)
    char Carrosserie[4][30];
    int code_couleur;
    char etat;          // '1' actif, '0' inactif
    char statut;        // 'E' entrée, 'S' vers sortie, 'P' stationnée
    unsigned long int tps;
    char mode;
    time_t tps_stationnement;
    struct voiture * NXT;
    int last_x;
    int last_y;
    int phase_sortie;
};

typedef struct {
    int largeur;
    int hauteur;
    int offset_x;  // offset du centre par rapport au coin haut-gauche
    int offset_y;  // offset du centre par rapport au coin haut-gauche
} DIMENSIONS;

// Variables globales
wchar_t plan[HAUTEUR][LARGEUR];
VEHICULE vehicules[MAX_VEHICULES];
int nb_vehicules = 0;
VEHICULE joueur;
int avec_collision = 0;
int gain_total = 0;


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
    system("play -q ost_ft.wav repeat - &");
}

char get_direction_normalise(char direction) {
    if (direction == 'N') return 'z';
    if (direction == 'S') return 's';
    if (direction == 'O') return 'q';
    else return 'd';
}

int est_place_parking(VEHICULE* joueur)
{
    int x = joueur->posx;
    int y = joueur->posy;

    int haut_mur_parking = 0;
    int bas_mur_parking = 0;
    int face_mur_parking = 0;

    for(int i = 0; i<= 7; ++i) {
        if (plan[y + i][x] == L'─') {
            haut_mur_parking = 1;
        }

        if (plan[y - i][x] == L'─') {
            bas_mur_parking = 1;
        }

        if (i <= 5) {
             if (plan[y][x - i] == L'│' || plan[y][x + i] == L'│') {
                face_mur_parking = 1;
            }
        }
    }

    return (haut_mur_parking == 1 && bas_mur_parking == 1
        && face_mur_parking == 1);
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

DIMENSIONS get_dimensions_voiture(char direction) {
    DIMENSIONS dim;
    switch(direction) {
        case 'N':
        case 'S':
            dim.largeur = 3;
            dim.hauteur = 3;
            dim.offset_x = 1;
            dim.offset_y = 1;
            break;
        case 'E':
        case 'O':
            // Ajustement pour tenir compte de l'affichage réel
            dim.largeur = 8;  // au lieu de 6
            dim.hauteur = 2;
            dim.offset_x = 3;
            dim.offset_y = 1;
            break;
    }
    return dim;
}


// Vérifier les collisions
int collision_voiture(int cx, int cy, char direction) {
    DIMENSIONS dim = get_dimensions_voiture(direction);

    // Calculer le coin haut-gauche à partir du centre
    int x = cx - dim.offset_x;
    int y = cy - dim.offset_y;

    // Vérifier les limites du parking
    if (x < 0 || x + dim.largeur > LARGEUR ||
        y < 0 || y + dim.hauteur > HAUTEUR) {
        return 1;
        }

    // Vérifier chaque position que la voiture occuperait
    for (int i = 0; i < dim.hauteur; i++) {
        for (int j = 0; j < dim.largeur; j++) {
            wint_t wc = plan[y + i][x + j];

            if (est_mur(wc)) {
                return 1;
            }
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

    DIMENSIONS dim = get_dimensions_voiture(v->direction);
    int x = v->posx - dim.offset_x;
    int y = v->posy - dim.offset_y;

    // Couleur selon le type
    if (v->type == 'v') attron(COLOR_PAIR(3)); // Rouge pour voiture
    else if (v->type == 'c') printf("\033[34m"); // Bleu pour camion
    else if (v->type == 'm') printf("\033[32m"); // Vert pour moto

    for (int i = 0; i < 4; i++) {
        //goto_xy(v->posx, v->posy + i);
        for (int i = 0; i < dim.hauteur; i++) {
            if (v->Carrosserie[i][0] != '\0') {
                mvprintw(y + i, x, "%s", v->Carrosserie[i]);
            }
        }
    }

    if (v->type == 'v') attroff(COLOR_PAIR(3));
}

// Déplacer le joueur
void deplacer_joueur(char direction) {
    int new_x = joueur.posx;
    int new_y = joueur.posy;

    char ancienne_direction = joueur.direction;
    char nouvelle_direction;

    switch(direction) {
        case 'z':
            new_y--;
            nouvelle_direction = 'N';
            break;
        case 's':
            new_y++;
            nouvelle_direction = 'S';
            break;
        case 'q':
            new_x--;
            nouvelle_direction = 'O';
            break;
        case 'd':
            new_x++;
            nouvelle_direction = 'E';
            break;
    }

    // new_x/new_y = centre de la voiture
    if (avec_collision == 0) {
        if (collision_voiture(new_x, new_y, nouvelle_direction)) {
            // Collision détectée : on ne déplace PAS
            return;
        }
    }

    // Pas de collision (ou mode sans collision) : on peut déplacer
    joueur.posx = new_x;
    joueur.posy = new_y;

    // Changer la sprite si changement de direction
    if (ancienne_direction != nouvelle_direction) {
        joueur.direction = nouvelle_direction;

        if (direction == 'z' || direction == 's')
            charger_vehicule(&joueur, "voiture_Z.txt", 'v');
        else // E ou O
            charger_vehicule(&joueur, "voiture_D.txt", 'v');
    }
}


// Fonction utilitaire pour obtenir la direction opposée
char get_direction_opposee(char direction) {
    switch(direction) {
        case 'N': return 'S';
        case 'S': return 'N';
        case 'E': return 'O';
        case 'O': return 'E';
        default: return direction;
    }
}

void deplacer_joueur_auto() {
    int new_x = joueur.posx;
    int new_y = joueur.posy;

    char ancienne_direction = joueur.direction;
    char nouvelle_direction = ancienne_direction;

    // Calculer direction opposée (pour éviter de faire demi-tour)
    char dir_opposee;
    switch(ancienne_direction) {
        case 'N': dir_opposee = 'S'; break;
        case 'S': dir_opposee = 'N'; break;
        case 'E': dir_opposee = 'O'; break;
        case 'O': dir_opposee = 'E'; break;
        default: dir_opposee = ancienne_direction; break;
    }

    // Calculer la prochaine position selon la direction actuelle
    switch(ancienne_direction) {
        case 'N': new_y--; break;
        case 'S': new_y++; break;
        case 'O': new_x--; break;
        case 'E': new_x++; break;
    }

    // Vérifier si on peut continuer tout droit
    bool peut_continuer = !collision_voiture(new_x, new_y, ancienne_direction);
    bool doit_changer = (joueur.count_meme_direction >= 5 || !peut_continuer);

    if (doit_changer) {
        // Réinitialiser
        new_x = joueur.posx;
        new_y = joueur.posy;
        joueur.count_meme_direction = 0;

        char directions[] = {'N', 'S', 'E', 'O'};
        bool direction_trouvee = false;

        // Mélanger les directions pour un comportement moins prévisible
        for (int i = 3; i > 0; i--) {
            int j = rand() % (i + 1);
            char temp = directions[i];
            directions[i] = directions[j];
            directions[j] = temp;
        }

        // Phase 1 : Essayer toutes les directions SAUF l'opposée
        for (int i = 0; i < 4 && !direction_trouvee; i++) {
            char dir = directions[i];

            // Ne jamais faire demi-tour
            if (dir == dir_opposee) {
                continue;
            }

            int test_x = joueur.posx;
            int test_y = joueur.posy;

            switch(dir) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            if (!collision_voiture(test_x, test_y, dir)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir;
                direction_trouvee = true;
            }
        }

        // Phase 2 : Si vraiment bloqué, essayer même la direction opposée
        if (!direction_trouvee) {
            int test_x = joueur.posx;
            int test_y = joueur.posy;

            switch(dir_opposee) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            if (!collision_voiture(test_x, test_y, dir_opposee)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir_opposee;
            }
            // Sinon, reste sur place (bloqué)
        }

        // Mettre à jour la direction précédente seulement quand on change
        joueur.precedente_direction = ancienne_direction;
    } else {
        // Continue tout droit
        joueur.count_meme_direction++;
    }

    joueur.posx = new_x;
    joueur.posy = new_y;

    if (ancienne_direction != nouvelle_direction) {
        joueur.direction = nouvelle_direction;

        if (nouvelle_direction == 'N' || nouvelle_direction == 'S')
            charger_vehicule(&joueur, "voiture_Z.txt", 'v');
        else
            charger_vehicule(&joueur, "voiture_D.txt", 'v');
    }
}

void deplacer_joueur_auto2() {
    int new_x = joueur.posx;
    int new_y = joueur.posy;

    char ancienne_direction = joueur.direction;
    char nouvelle_direction = ancienne_direction;

    // Calculer direction opposée (pour éviter de faire demi-tour)
    char dir_opposee;
    switch(ancienne_direction) {
        case 'N': dir_opposee = 'S'; break;
        case 'S': dir_opposee = 'N'; break;
        case 'E': dir_opposee = 'O'; break;
        case 'O': dir_opposee = 'E'; break;
        default: dir_opposee = ancienne_direction; break;
    }

    // Calculer la prochaine position selon la direction actuelle
    switch(ancienne_direction) {
        case 'N': new_y--; break;
        case 'S': new_y++; break;
        case 'O': new_x--; break;
        case 'E': new_x++; break;
    }

    // Vérifier si on peut continuer tout droit
    bool peut_continuer = !collision_voiture(new_x, new_y, ancienne_direction);
    bool doit_changer = ((joueur.count_meme_direction <= 7 && ancienne_direction == 'N') || !peut_continuer);

    if (doit_changer) {
        // Réinitialiser
        new_x = joueur.posx;
        new_y = joueur.posy;
        joueur.count_meme_direction = 0;

        char directions[] = {'N', 'S', 'E', 'O'};
        bool direction_trouvee = false;

        // Mélanger les directions pour un comportement moins prévisible
        for (int i = 3; i > 0; i--) {
            int j = rand() % (i + 1);
            char temp = directions[i];
            directions[i] = directions[j];
            directions[j] = temp;
        }

        // Phase 1 : Essayer toutes les directions SAUF l'opposée
        for (int i = 0; i < 4 && !direction_trouvee; i++) {
            char dir = directions[i];

            // Ne jamais faire demi-tour
            if (dir == dir_opposee) {
                continue;
            }

            int test_x = joueur.posx;
            int test_y = joueur.posy;

            switch(dir) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            if (!collision_voiture(test_x, test_y, dir)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir;
                direction_trouvee = true;
            }
        }

        // Phase 2 : Si vraiment bloqué, essayer même la direction opposée
        if (!direction_trouvee) {
            int test_x = joueur.posx;
            int test_y = joueur.posy;

            switch(dir_opposee) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            if (!collision_voiture(test_x, test_y, dir_opposee)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir_opposee;
            }
            // Sinon, reste sur place (bloqué)
        }

        // Mettre à jour la direction précédente seulement quand on change
        joueur.precedente_direction = ancienne_direction;
    } else {
        // Continue tout droit
        joueur.count_meme_direction++;
    }

    joueur.posx = new_x;
    joueur.posy = new_y;

    if (ancienne_direction != nouvelle_direction) {
        joueur.direction = nouvelle_direction;

        if (nouvelle_direction == 'N' || nouvelle_direction == 'S')
            charger_vehicule(&joueur, "voiture_Z.txt", 'v');
        else
            charger_vehicule(&joueur, "voiture_D.txt", 'v');
    }
}

void deplacer_joueur_vers_sortie(int cible_x, int cible_y)
{
    int dx = cible_x - joueur.posx;
    int dy = cible_y - joueur.posy;

    char dir_opp = joueur.direction;
    switch (joueur.direction) {
        case 'N': dir_opp = 'S'; break;
        case 'S': dir_opp = 'N'; break;
        case 'E': dir_opp = 'O'; break;
        case 'O': dir_opp = 'E'; break;
    }

    char essais[4];
    int k = 0;

    if (abs(dx) >= abs(dy)) {
        if (dx > 0) essais[k++] = 'E';
        if (dx < 0) essais[k++] = 'O';
        if (dy < 0) essais[k++] = 'N';
        if (dy > 0) essais[k++] = 'S';
    } else {
        if (dy < 0) essais[k++] = 'N';
        if (dy > 0) essais[k++] = 'S';
        if (dx > 0) essais[k++] = 'E';
        if (dx < 0) essais[k++] = 'O';
    }

    int prev_x = joueur.last_x;
    int prev_y = joueur.last_y;

    char all[4] = {'N','S','E','O'};
    for (int i = 0; i < 4; i++) {
        bool deja = false;
        for (int j = 0; j < k; j++) if (essais[j] == all[i]) deja = true;
        if (!deja) essais[k++] = all[i];
    }

    char choix = 0;

    // Phase 2 : essayer directions (sans demi-tour + sans revenir en arrière)
    for (int i = 0; i < 4; i++) {
        char d = essais[i];
        if (d == dir_opp) continue;

        int nx = joueur.posx, ny = joueur.posy;
        if (d == 'N') ny--;
        if (d == 'S') ny++;
        if (d == 'E') nx++;
        if (d == 'O') nx--;

        if (nx == prev_x && ny == prev_y) continue;   // ✅ anti-retour

        if (!collision_voiture(nx, ny, d)) { choix = d; break; }
    }

    // Phase 3 : si bloqué, autoriser demi-tour mais pas si c'est juste revenir en arrière
    if (choix == 0) {
        int nx = joueur.posx, ny = joueur.posy;
        if (dir_opp == 'N') ny--;
        if (dir_opp == 'S') ny++;
        if (dir_opp == 'E') nx++;
        if (dir_opp == 'O') nx--;

        if (!(nx == prev_x && ny == prev_y) && !collision_voiture(nx, ny, dir_opp)) {
            choix = dir_opp;
        }
    }

    mvprintw(1, 2, "STATUT=%c POS=(%d,%d) dx=%d dy=%d choix=%c dir=%c",
             joueur.statut, joueur.posx, joueur.posy, dx, dy, choix ? choix : '-', joueur.direction);

    if (choix != 0) {
        // ✅ mémoriser la position AVANT de bouger
        joueur.last_x = joueur.posx;
        joueur.last_y = joueur.posy;

        joueur.direction = choix;

        char touche_direction = get_direction_normalise(choix);

        if (choix == 'N' || choix == 'S')
            charger_vehicule(&joueur, "voiture_Z.txt", 'v');
        else
            charger_vehicule(&joueur, "voiture_D.txt", 'v');

        deplacer_joueur(touche_direction);
    }
}

void afficher_gain()
{
    attron(COLOR_PAIR(1));

    mvprintw(1,104, "Gain total : %d",gain_total);

    attroff(COLOR_PAIR(1));
}

// Affiche info position et déplacement
void afficher_instructions()
{
    attron(COLOR_PAIR(2));
    // sous le parking
    int ligne = HAUTEUR + 1;

    mvprintw(1,104, "Gain total : %d",gain_total);

    mvprintw(ligne,     0, "ZQSD: Deplacer la voiture rouge");
    mvprintw(ligne + 1, 0, "ESC: Quitter");
    mvprintw(ligne + 2, 0, "Position: (%d,%d)", joueur.posx, joueur.posy);
    mvprintw(ligne + 3, 0, "Temps: %02lu:%02lu", joueur.tps / 60, joueur.tps % 60);

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

    lancer_musique();

    // Initialisation
    printf("Chargement du parking...\n");

    // Charger le plan
    charger_parking("parking.txt");

    // Initialiser le joueur
    charger_vehicule(&joueur, "voiture_D.txt", 'v');
    joueur.posx = 88;
    joueur.posy = 43;
    joueur.code_couleur = 31; // Rouge
    joueur.direction = 'O';

    afficher_fichier("logo.txt", RED);

    printf("★ Regle du jeu ★\n");
    printf("Jeu permettant de simuler le stationnement de voitures dans un parking payant\n\n");

    printf("Pour commencer veuillez choisir votre mode de jeu :\n");
    printf("[1] Sans collision - manuelle\n");
    printf("[2] Avec collision - manuelle\n");
    printf("[3] Auto\n\n");


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
                avec_collision = 0;

                initscr();
                start_color();
                use_default_colors();

                joueur.mode = 'M';
                unsigned long int tps_debut = time(NULL);
                joueur.tps = 0;

                // cyan
                init_pair(1, COLOR_CYAN, -1);
                // jaune
                init_pair(2, COLOR_YELLOW, -1);
                // rouge
                init_pair(3, COLOR_RED, -1);
                //violet
                init_pair(4, COLOR_MAGENTA, -1);

                unsigned long int prix = 0;

                while (running) {
                    // Effacer le contenue de l'ecran virtuel (fournit par ncurses)
                    clear();

                    joueur.tps = time(NULL) - tps_debut;

                    afficher_parking();


                    for (int i = 0; i < nb_vehicules; i++) {
                        afficher_vehicule(&vehicules[i]);
                    }

                    // Afficher le joueur principal
                    afficher_vehicule(&joueur);

                    afficher_gain();
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

                    if (joueur.posx >= 78 && joueur.posy <= 9) {
                        prix = joueur.tps * 2;
                        gain_total = gain_total + prix;
                    }

                    if (joueur.posx >= 80 && joueur.posy <= 9) {
                        running = 0;
                        break;
                    }
                }

                //Ferme le mode ncurses pour remettre le terminal normalement
                endwin();

                printf("\n\nVous avez passé un temps totale de %02lu:%02lu\n", joueur.tps / 60, joueur.tps % 60);
                printf("Vous devez régler %02lu,%02lu€\n", prix / 100, prix % 100);
                printf("\n\nMerci d'avoir joué à Take IT !\n");
                return 0;
            }
            case 2: {
                // Boucle principale
                char touche;
                int running = 1;
                avec_collision = 1;

                initscr();
                start_color();
                use_default_colors();

                unsigned long int tps_debut = time(NULL);
                joueur.tps = 0;
                joueur.mode = 'M';

                // cyan
                init_pair(1, COLOR_CYAN, -1);
                // jaune
                init_pair(2, COLOR_YELLOW, -1);
                // rouge
                init_pair(3, COLOR_RED, -1);
                //violet
                init_pair(4, COLOR_MAGENTA, -1);

                while (running) {
                    // Effacer le contenue de l'ecran virtuel (fournit par ncurses)
                    clear();

                    joueur.tps = time(NULL) - tps_debut;

                    afficher_parking();


                    for (int i = 0; i < nb_vehicules; i++) {
                        afficher_vehicule(&vehicules[i]);
                    }

                    // Afficher le joueur principal
                    afficher_vehicule(&joueur);

                    afficher_gain();
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

                    if (joueur.posx >= 80 && joueur.posy <= 9) {
                        running = 0;
                        break;
                    }

                    //pour eviter les scitillements
                    usleep(50000);
                }

                //Ferme le mode ncurses pour remettre le terminal normalement
                endwin();
                printf("Merci d'avoir joué à Take IT !\n");
                return 0;
            }

            case 3: {
                // Boucle principale
                int running = 1;
                avec_collision = 1;
                srand(time(NULL));

                unsigned long int tps_debut = time(NULL);
                joueur.tps = 0;
                joueur.mode = 'A';
                joueur.statut = 'E';
                joueur.tps_stationnement = 0;
                joueur.last_x = -1;
                joueur.last_y = -1;
                joueur.phase_sortie = 0;

                initscr();
                start_color();
                use_default_colors();

                // cyan
                init_pair(1, COLOR_CYAN, -1);
                // jaune
                init_pair(2, COLOR_YELLOW, -1);
                // rouge
                init_pair(3, COLOR_RED, -1);
                //violet
                init_pair(4, COLOR_MAGENTA, -1);

                while (running) {
                    // Effacer le contenue de l'ecran virtuel (fournit par ncurses)
                    clear();

                    joueur.tps = time(NULL) - tps_debut;

                    afficher_parking();


                    for (int i = 0; i < nb_vehicules; i++) {
                        afficher_vehicule(&vehicules[i]);
                    }

                    // Afficher le joueur principal
                    afficher_vehicule(&joueur);

                    afficher_gain();
                    afficher_instructions();


                    // Vérifier les touches
                    if (joueur.statut == 'E') {
                        deplacer_joueur_auto2();
                    } else if (joueur.statut == 'S') {

                    // Phase 0 : aller à l'intersection la plus proche sur x=45
                    if (joueur.phase_sortie == 0) {
                        // choisir y cible parmi {37,30,23,16} la plus proche de joueur.posy
                        int ys[5] = {37,30,23,16,8};
                        int bestY = ys[0];
                        int bestD = abs(joueur.posy - ys[0]);
                        for (int i=1;i<5;i++){
                            int d = abs(joueur.posy - ys[i]);
                            if (d < bestD) { bestD = d; bestY = ys[i]; }
                        }

                        deplacer_joueur_vers_sortie(45, bestY);

                        if (abs(joueur.posx - 45) + abs(joueur.posy - bestY) <= 1)
                            joueur.phase_sortie = 1;
                    }

                    // Phase 1 : monter sur la colonne x=45 vers y=16 (haut des rangées)
                    else if (joueur.phase_sortie == 1) {
                        deplacer_joueur_vers_sortie(45, 8);
                        if (abs(joueur.posx - 45) + abs(joueur.posy - 8) <= 1)
                            joueur.phase_sortie = 2;
                    }

                    // Phase 2 : aller vers la sortie
                    else {
                        deplacer_joueur_vers_sortie(80, 9);
                    }
                }



                    if(joueur.statut == 'E' && est_place_parking(&joueur)) {
                        joueur.statut = 'P';

                        joueur.tps_stationnement = time(NULL);
                    }

                    if (joueur.statut == 'P' && time(NULL) - joueur.tps_stationnement >= 10) {

                        // petit recul pour sortir de la place (optionnel)
                        char dir_opp = (joueur.direction == 'O') ? 'E' : 'O';
                        char touche_direction = get_direction_normalise(dir_opp);
                        deplacer_joueur(touche_direction);

                        joueur.last_x = joueur.posx;
                        joueur.last_y = joueur.posy;

                        joueur.statut = 'S';
                    }

                    if (joueur.posx >= 80 && joueur.posy <= 9) {
                        running = 0;
                        break;
                    }

                    // Dessine le contenu tu terminal virtuel dans le vrais terminal (fournit par ncurses)
                    refresh();

                    //pour eviter les scitillements
                    usleep(100000);
                }

                //Ferme le mode ncurses pour remettre le terminal normalement
                endwin();
                unsigned long int prix = joueur.tps * 2;

                printf("\n\nVous avez passé un temps totale de %02lu:%02lu\n", joueur.tps / 60, joueur.tps % 60);
                printf("Vous devez régler %02lu,%02lu€\n", prix / 100, prix % 100);
                printf("\n\nMerci d'avoir joué à Take IT !\n");

                char touche;

                touche = key_pressed();
                if (touche != 0) {
                    switch(touche) {
                        case 27: // touche échape
                            return 0;
                            break;
                    }
                }
            }

            default:
                printf("Mode inconnu. Au revoir.\n");
                return 0;
        }
    }
}