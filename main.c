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

typedef struct {
    int x;
    int y;
} COORDONEE;

typedef struct voiture {
    char direction;      // N, S, E, O
    int count_meme_direction;   //nb même direction d'affilée
    COORDONEE position;     // Position
    int vitesse;        // Vitesse
    char type;          // v, c, m (voiture, camion, moto)
    char Carrosserie[4][30]; //TODO
    int code_couleur;   // couleur de la voiture
    char etat;          // '1' actif, '0' inactif
    char statut;        // 'E' entrée, 'S' vers sortie, 'P' stationnée 'F' fini
    unsigned long int tps;          // temps passé dans le parking
    char mode;          // 'M' manuelle, 'A' automatique
    time_t tps_stationnement;       // durée réel de stationnement
    int tps_stationnement_alloue;   // durée de stationnement allouée à la voiture (si auto)
    struct voiture * NXT;               //TODO
    COORDONEE ancienne_position;    // derniere position
    int phase_sortie;               // 0 : intersection la plus proche, 1 monter vers y=16
    COORDONEE coord_mur_place[4];  // coordonnée du mur de couleur de le place
    unsigned long start_time;  // timestamp de départ
    int active;                // 0 = pas encore lancée, 1 = en cours

}VEHICULE;

typedef struct {
    int largeur;   // largeur du vehicule
    int hauteur;   // hauteur du vehicule
    int offset_x;  // offset du centre par rapport au coin haut-gauche
    int offset_y;  // offset du centre par rapport au coin haut-gauche
} DIMENSIONS;

 typedef enum {
    MANUELLE_COLLISION = 0,
    MANUELLE_SANS_COLLISION = 1,
    AUTOMATIQUE_SOLO = 2,
    AUTOMATIQUE_MULTI = 3,
} MODE_JEU;



#define HAUTEUR 50

#define LARGEUR 130

#define MAX_VEHICULES 3



wchar_t plan[HAUTEUR][LARGEUR];

int nb_vehicules;

VEHICULE joueur;

int avec_collision;

int gain_total = 0;

unsigned long int chronometre;

MODE_JEU modeJeu;

VEHICULE *tab_vehicules;


void lancer_musique() {
    system("play -q ost_ft.wav repeat - &");
}

char get_direction_normalise(char direction) {
    if (direction == 'N') return 'z';
    if (direction == 'S') return 's';
    if (direction == 'O') return 'q';

    return 'd';
}

// Verifier si tout les vehicules sont sortis
int isToutVehiculeSorti() {
    for (int i = 0; i < nb_vehicules; i++) {
        if (tab_vehicules[i].statut != 'F') {
            return 0;
        }
    }
    return 1;
}

// Fonction utilitaire pour obtenir la direction opposée
char getDirectionOpposee(char direction) {
    switch(direction) {
        case 'N': return 'S';
        case 'S': return 'N';
        case 'E': return 'O';
        case 'O': return 'E';
        default: return direction;
    }
}

int isDansPlaceParking(VEHICULE *voiture)
{
    int x = voiture->position.x;
    int y = voiture->position.y;

    int haut_mur_parking = 0;
    int bas_mur_parking = 0;
    int x_mur = -1;

    // Vérifier les murs horizontaux, en haut et en bas
    for(int i = 0; i <= 7; ++i) {
        if (plan[y + i][x] == L'─') {
            haut_mur_parking = 1;
        }
        if (plan[y - i][x] == L'─') {
            bas_mur_parking = 1;
        }
    }

    // Trouver le mur vertical, en face
    for (int i = 0; i <= 6; i++) {
        if (plan[y][x - i] == L'█') {
            x_mur = x - i;
            break;
        }
        if (plan[y][x + i] == L'█') {
            x_mur = x + i;
            break;
        }
    }

    if (haut_mur_parking && bas_mur_parking && x_mur != -1)
    {
        // Trouver le premier mur de la colonne en remontant de 1
        int y_debut = y;
        while (y_debut > 0 && plan[y_debut - 1][x_mur] == L'█') {
            y_debut--;
        }

        // On set les coordonnées du mur en partant du premier et en descendant de 4
        for (int i = 0; i < 4; i++) {
            voiture->coord_mur_place[i].x = x_mur;
            voiture->coord_mur_place[i].y = y_debut + i;
        }

        return 1;

    }

    return 0;
}

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

// Charger le plan depuis un fichier
void chargerParking() {
    FILE *f = fopen("parking.txt", "r");

    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", "parking.txt");
        exit(1);
    }

    //pour avoir des caractères ASCI plus large
    wint_t wc;

    int y = 0, x = 0, wmax = 0;

    // On charge notre matrice
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
void chargerVehicule(VEHICULE *v, const char* fichier, char type)
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

DIMENSIONS getDimensionsVehicule(char direction, char type) {
    DIMENSIONS dim;
    if (type == 'v') {
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
                dim.largeur = 8;
                dim.hauteur = 2;
                dim.offset_x = 3;
                dim.offset_y = 1;
                break;
        }
    }

    return dim;
}

int isPlaceOccupe(int cx, int cy, char direction) {
    // On vérifie sur 3O caractères
    int distance_check = 30;

    for (int i = 0; i < nb_vehicules; i++) {
        VEHICULE *autre = &tab_vehicules[i];

        if (!autre->active) continue;

        int ax = autre->position.x;
        int ay = autre->position.y;

        if (direction == 'O') {
            if (ax < cx && ax >= cx - distance_check) {
                int diff_y = abs(ay - cy);
                if (diff_y <= 5) {
                    return 1;
                }
            }
        }
        else if (direction == 'E') {
            if (ax > cx && ax <= cx + distance_check) {
                int diff_y = abs(ay - cy);

                if (diff_y <= 5) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

// Vérifier les collisions
//TODO : avoir le type de voiture
int isCollision(int cx, int cy, char direction) {
    DIMENSIONS dim = getDimensionsVehicule(direction, 'v');

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

            if (isMur(wc)) {
                return 1;
            }
        }
    }

    return 0;
}

// Afficher le plan sur la console
void afficherParking()
{
    for (int y = 0; y < HAUTEUR; y++) {
        for (int x = 0; x < LARGEUR && plan[y][x] != L'\0'; x++) {
            wchar_t wc = plan[y][x];

            if (isMur(plan[y][x])) {
                if (plan[y][x] == L'█') attron(COLOR_PAIR(5));
                else attron(COLOR_PAIR(1));
            } else {
                attron(COLOR_PAIR(2));
            }

            mvprintw(y, x, "%lc", wc);
            attrset(A_NORMAL);
        }
    }
}

// Afficher un véhicule sur la console
void afficherVehicule(VEHICULE *v)
{
    if (v->etat != '1') return;

    DIMENSIONS dim = getDimensionsVehicule(v->direction, v->type);
    int x = v->position.x - dim.offset_x;
    int y = v->position.y - dim.offset_y;

    // Couleur selon le type
    if (v->type == 'v') attron(COLOR_PAIR(3)); // Rouge pour voiture
    else if (v->type == 'c') printf("\033[34m"); // Bleu pour camion
    else if (v->type == 'm') printf("\033[32m"); // Vert pour moto

    for (int i = 0; i < 4; i++) {
        for (int i = 0; i < dim.hauteur; i++) {
            if (v->Carrosserie[i][0] != '\0') {
                mvprintw(y + i, x, "%s", v->Carrosserie[i]);
            }
        }
    }

    if (v->type == 'v') attroff(COLOR_PAIR(3));
}

// Déplacer le vehicule manuellement
void deplacerVehicule(char direction, VEHICULE* vehicule) {
    int new_x = vehicule->position.x;
    int new_y = vehicule->position.y;

    char ancienne_direction = vehicule->direction;
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
        if (isCollision(new_x, new_y, nouvelle_direction)) {
            // Collision détectée : on ne déplace PAS
            return;
        }
    }

    // Pas de collision (ou mode sans collision) : on peut déplacer
    vehicule->position.x = new_x;
    vehicule->position.y = new_y;

    // Changer l'affichage si changement de direction
    if (ancienne_direction != nouvelle_direction) {
        vehicule->direction = nouvelle_direction;

        if (direction == 'z' || direction == 's')
            chargerVehicule(vehicule, "voiture_Z.txt", 'v');
        else // E ou O
            chargerVehicule(vehicule, "voiture_D.txt", 'v');
    }
}


// Déplacer un vehicule automatiquement dans un mode multi
void deplacerMultiVehiculeAuto(VEHICULE *vehicule) {
    int new_x = vehicule->position.x;
    int new_y = vehicule->position.y;

    char ancienne_direction = vehicule->direction;
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
    bool peut_continuer = !isCollision(new_x, new_y, ancienne_direction);
    bool doit_changer = ((vehicule->count_meme_direction <= 6 && ancienne_direction == 'N') || !peut_continuer);

    if (doit_changer) {
        // Réinitialiser
        new_x = vehicule->position.x;
        new_y = vehicule->position.y;
        vehicule->count_meme_direction = 0;

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

            if (dir == dir_opposee) {
                continue;
            }

            int test_x = vehicule->position.x;
            int test_y = vehicule->position.y;

            switch(dir) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            if ((dir == 'O' || dir == 'E')) {
                if (!isCollision(test_x, test_y, dir) && !isPlaceOccupe(test_x, test_y, dir)) {
                    new_x = test_x;
                    new_y = test_y;
                    nouvelle_direction = dir;
                    direction_trouvee = true;
                }
            }
            else if (!isCollision(test_x, test_y, dir)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir;
                direction_trouvee = true;
            }
        }

        // Phase 2 : Si vraiment bloqué, essayer même la direction opposée
        if (!direction_trouvee) {
            int test_x = vehicule->position.x;
            int test_y = vehicule->position.y;

            switch(dir_opposee) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            if (!isCollision(test_x, test_y, dir_opposee)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir_opposee;
            }
        }
    } else {
        // Continue tout droit
        vehicule->count_meme_direction++;
    }

    vehicule->position.x = new_x;
    vehicule->position.y = new_y;

    if (ancienne_direction != nouvelle_direction) {
        vehicule->direction = nouvelle_direction;

        if (nouvelle_direction == 'N' || nouvelle_direction == 'S')
            chargerVehicule(vehicule, "voiture_Z.txt", 'v');
        else
            chargerVehicule(vehicule, "voiture_D.txt", 'v');
    }
}


// Déplacer un vehicule automatiquement dans un mode solo
void deplacerVehiculeAuto(VEHICULE *vehicule) {
    int new_x = vehicule->position.x;
    int new_y = vehicule->position.y;

    char ancienne_direction = vehicule->direction;
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
    bool peut_continuer = !isCollision(new_x, new_y, ancienne_direction);
    bool doit_changer = ((vehicule->count_meme_direction <= 6 && ancienne_direction == 'N') || !peut_continuer);

    if (doit_changer)
    {
        new_x = vehicule->position.x;
        new_y = vehicule->position.y;
        vehicule->count_meme_direction = 0;

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

            if (dir == dir_opposee) {
                continue;
            }

            int test_x = vehicule->position.x;
            int test_y = vehicule->position.y;

            switch(dir) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            if (!isCollision(test_x, test_y, dir)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir;
                direction_trouvee = true;
            }
        }

        // Phase 2 : Si vraiment bloqué, essayer même la direction opposée
        if (!direction_trouvee) {
            int test_x = vehicule->position.x;
            int test_y = vehicule->position.y;

            switch(dir_opposee) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            //if (!isCollision(test_x, test_y, dir_opposee)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir_opposee;
            //}
        }
    } else {
        // Continue tout droit
        vehicule->count_meme_direction++;
    }

    vehicule->position.x = new_x;
    vehicule->position.y = new_y;

    if (ancienne_direction != nouvelle_direction) {
        vehicule->direction = nouvelle_direction;

        if (nouvelle_direction == 'N' || nouvelle_direction == 'S')
            chargerVehicule(vehicule, "voiture_Z.txt", 'v');
        else
            chargerVehicule(vehicule, "voiture_D.txt", 'v');
    }
}


// déplacer le vehicule vers une coordonée, nottament pour sortir du parking
void deplacerVehiculeVers(int cible_x, int cible_y, VEHICULE* vehicule)
{
    int dx = cible_x - vehicule->position.x;
    int dy = cible_y - vehicule->position.y;

    char dir_opp = vehicule->direction;
    switch (vehicule->direction) {
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

    int prev_x = vehicule->ancienne_position.x;
    int prev_y = vehicule->ancienne_position.y;

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

        int nx = vehicule->position.x, ny = vehicule->position.y;
        if (d == 'N') ny--;
        if (d == 'S') ny++;
        if (d == 'E') nx++;
        if (d == 'O') nx--;

        if (nx == prev_x && ny == prev_y) continue;

        if (!isCollision(nx, ny, d)) { choix = d; break; }
    }

    // Phase 3 : si bloqué, autoriser demi-tour mais pas si c'est juste revenir en arrière
    if (choix == 0) {
        int nx = vehicule->position.x, ny = vehicule->position.y;
        if (dir_opp == 'N') ny--;
        if (dir_opp == 'S') ny++;
        if (dir_opp == 'E') nx++;
        if (dir_opp == 'O') nx--;

        if (!(nx == prev_x && ny == prev_y) && !isCollision(nx, ny, dir_opp)) {
            choix = dir_opp;
        }
    }

    if (choix != 0) {
        vehicule->ancienne_position.x = vehicule->position.x;
        vehicule->ancienne_position.y = vehicule->position.y;

        vehicule->direction = choix;

        char touche_direction = get_direction_normalise(choix);

        if (choix == 'N' || choix == 'S')
            chargerVehicule(vehicule, "voiture_Z.txt", 'v');
        else
            chargerVehicule(vehicule, "voiture_D.txt", 'v');

        deplacerVehicule(touche_direction, vehicule);
    }
}


// affichage du hain sur la console (temps par voiture * 2)
void afficher_gain_multi()
{
    attron(COLOR_PAIR(2));

    unsigned long int prix = 0;

    for (int p = 0; p < nb_vehicules; ++p) {
        prix = prix + (tab_vehicules[p].tps * 2);
    }

    mvprintw(1,104, "Gain total : %02lu,%02lu€",prix / 100, prix % 100);

    attroff(COLOR_PAIR(3));
}


// affichage des instructions notament en mode solo ou manuelle
void afficherInstructions(VEHICULE *vehicule)
{
    attron(COLOR_PAIR(2));
    // sous le parking
    int ligne = HAUTEUR + 1;

    int temps = time(NULL) - chronometre;

    mvprintw(ligne,     0, "ZQSD: Deplacer la voiture rouge");
    mvprintw(ligne + 1, 0, "ESC: Quitter");
    mvprintw(ligne + 2, 0, "Position: (%d,%d)", vehicule->position.x, vehicule->position.y);
    mvprintw(ligne + 3, 0, "Temps: %02lu:%02lu", temps / 60, temps % 60);

    attroff(COLOR_PAIR(2));
}


// affichage d'un fichier texte sur la console, notamment le logo
void afficherFichier(const char* fichier) {
    FILE *f = fopen(fichier, "r");
    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", fichier);
        exit(1);
    }

    //Utilise la couleur rouge
    printf("\033[1;31m");

    wint_t wc;
    while ((wc = fgetwc(f)) != WEOF)
    {
        putwchar(wc);
    }

    printf("\n");

    //Remet la couleur noire
    printf("\033[0m");
    fclose(f);
}


// paramettrage du mode de jeu
void parametreModeJeu()
{
    VEHICULE vehicule;

    switch (modeJeu) {
        case 0:
            avec_collision = 0;

            vehicule.position.x = 86;
            vehicule.position.y = 35;
            vehicule.code_couleur = 31; // Rouge
            vehicule.direction = 'O';
            vehicule.mode = 'M'; //Manuelle
            vehicule.statut = 'E'; //Entre dans le parking
            vehicule.tps = 0;
            nb_vehicules = 1;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            tab_vehicules[0] = vehicule;

            chargerVehicule(&tab_vehicules[0], "voiture_D.txt", 'v');

            break;
        case 1 :
            avec_collision = 1;

            vehicule.position.x = 86;
            vehicule.position.y = 35;
            vehicule.code_couleur = 31; // Rouge
            vehicule.direction = 'O';
            vehicule.mode = 'M'; //Manuelle
            vehicule.statut = 'E'; //Entre dans le parking
            vehicule.tps = 0;
            nb_vehicules = 1;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            tab_vehicules[0] = vehicule;

            chargerVehicule(&tab_vehicules[0], "voiture_D.txt", 'v');

            break;
        case 2 :
            avec_collision = 0;

            vehicule.position.x = 86;
            vehicule.position.y = 35;
            vehicule.code_couleur = 31; // Rouge
            vehicule.direction = 'O'; // Ouest <- vers la gauche
            vehicule.mode = 'A'; // Manuelle
            vehicule.statut = 'E'; // Entre dans le parking
            vehicule.tps = 0;
            vehicule.tps_stationnement = 0;
            vehicule.ancienne_position.x = -1;
            vehicule.ancienne_position.y = -1;
            vehicule.phase_sortie = 0;

            //attribue aléatoirement un temps (entre 10 et 30 secondes) de stationnement
            vehicule.tps_stationnement_alloue = 10 + rand() % 21;

            nb_vehicules = 1;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            tab_vehicules[0] = vehicule;

            chargerVehicule(&tab_vehicules[0], "voiture_D.txt", 'v');

            break;
        case 3:
            avec_collision = 0;

            nb_vehicules = 3;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            for (int indexVehicule = 0; indexVehicule < nb_vehicules; indexVehicule++)
            {
                vehicule.position.x = 86;
                vehicule.position.y = 35;
                vehicule.code_couleur = 31; // Rouge
                vehicule.direction = 'O'; // Ouest <- vers la gauche
                vehicule.mode = 'A'; // Manuelle
                vehicule.statut = 'E'; // Entre dans le parking
                vehicule.tps = 0;
                vehicule.tps_stationnement = 0;
                vehicule.ancienne_position.x = -1;
                vehicule.ancienne_position.y = -1;
                vehicule.phase_sortie = 0;
                vehicule.start_time = chronometre + (indexVehicule * 3); // 0s pour la 1ère, 3s pour la 2e
                vehicule.active = 0;

                //attribue aléatoirement un temps (entre 10 et 30 secondes) de stationnement
                vehicule.tps_stationnement_alloue = 10 + rand() % 21;

                tab_vehicules[indexVehicule] = vehicule;

                chargerVehicule(&tab_vehicules[indexVehicule], "voiture_D.txt", 'v');
            }

            break;
    }
}



int main() {

    //l’Unicode (wchar_t)
    setlocale(LC_ALL, "");

    //lancer_musique();


    // Charger le plan
    chargerParking();

    afficherFichier("logo.txt");

    printf("★ Regle du jeu ★\n");
    printf("Jeu permettant de simuler le stationnement de voitures dans un parking payant\n\n");

    printf("Pour commencer veuillez choisir votre mode de jeu :\n");
    printf("[1] Sans collision - manuelle\n");
    printf("[2] Avec collision - manuelle\n");
    printf("[3] Auto\n");
    printf("[4] Fluide : circulation calme et agréable, sans bouchons.\n\n");


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

        switch (mode)
        {
            case 1: modeJeu = MANUELLE_COLLISION; break;
            case 2: modeJeu = MANUELLE_SANS_COLLISION; break;
            case 3: modeJeu = AUTOMATIQUE_SOLO; break;
            case 4: modeJeu = AUTOMATIQUE_MULTI; break;
            default:
                printf("Mode inconnu. Au revoir.\n");
                return 0;
        }

        //srand(time(NULL));
        chronometre = time(NULL);

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
        //vert
        init_pair(5, COLOR_GREEN, -1);

        parametreModeJeu();

        char touche;
        int running = 1;

        bool modeAuto = (modeJeu == AUTOMATIQUE_MULTI || modeJeu == AUTOMATIQUE_SOLO);

        while (running)
        {
            // Effacer le contenue de l'ecran virtuel (fournit par ncurses)
            clear();

            afficherParking();

            unsigned long now = time(NULL);

            VEHICULE *vehiculeCourant;

            for (int indexVehicule = 0; indexVehicule < nb_vehicules; indexVehicule++)
            {
                vehiculeCourant = &tab_vehicules[indexVehicule];

                vehiculeCourant->tps = time(NULL) - chronometre;

                // activation au bon moment
                if (!vehiculeCourant->active) {
                    if (now >= vehiculeCourant->start_time) {
                        vehiculeCourant->active = 1;
                    } else {
                        continue;
                    }
                }

                if (!modeAuto)
                {
                    // Vérifier les touches
                    touche = key_pressed();
                    if (touche != 0) {
                        switch(touche) {
                            case 'z':
                            case 'q':
                            case 's':
                            case 'd':
                                deplacerVehicule(touche, vehiculeCourant);
                                break;
                            case 27:
                                running = 0;
                                break;
                        }
                    }
                }

                else
                {
                    if (vehiculeCourant->statut == 'E')
                    {
                        if (modeJeu == AUTOMATIQUE_SOLO) deplacerVehiculeAuto(vehiculeCourant);
                        else deplacerMultiVehiculeAuto(vehiculeCourant);
                    }
                    else if (vehiculeCourant->statut == 'S') {
                        // Phase 0 : aller à l'intersection la plus proche sur x=45
                        if (vehiculeCourant->phase_sortie == 0)
                        {
                            // choisir y cible parmi {37,30,23,16} la plus proche de vehiculeCourant->posy
                            int ys[5] = {29,24,19,14,7};
                            int bestY = ys[0];
                            int bestD = abs(vehiculeCourant->position.y - ys[0]);
                            for (int i=1;i<5;i++){
                                int d = abs(vehiculeCourant->position.y - ys[i]);
                                if (d < bestD) { bestD = d; bestY = ys[i]; }
                            }

                            deplacerVehiculeVers(47, bestY, vehiculeCourant);

                            if (abs(vehiculeCourant->position.x - 47) + abs(vehiculeCourant->position.y - bestY) <= 1)
                                vehiculeCourant->phase_sortie = 1;
                        }

                        // Phase 1 : monter sur la colonne x=45 vers y=16 (haut des rangées)
                        else if (vehiculeCourant->phase_sortie == 1)
                        {
                            deplacerVehiculeVers(47, 8, vehiculeCourant);
                            if (abs(vehiculeCourant->position.x - 47) + abs(vehiculeCourant->position.y - 8) <= 1)
                                vehiculeCourant->phase_sortie = 2;
                        }

                        else deplacerVehiculeVers(80,7, vehiculeCourant);
                    }
                }

                if (vehiculeCourant->statut == 'P' &&
                    (!modeAuto || (time(NULL) - vehiculeCourant->tps_stationnement < vehiculeCourant->tps_stationnement_alloue))) {
                    if (!isDansPlaceParking(vehiculeCourant))vehiculeCourant->statut = 'E';
                    else {
                        COORDONEE coord;
                        for (int i = 0; i < 4; ++i) {
                            coord = vehiculeCourant->coord_mur_place[i];
                            if (plan[coord.y][coord.x] == L'█') {
                                attron(COLOR_PAIR(3));
                                mvprintw(coord.y, coord.x, "%lc", plan[coord.y][coord.x]);
                                attroff(COLOR_PAIR(3));
                            }
                        }
                    }
                }

                if(isDansPlaceParking(vehiculeCourant) &&
                    (!modeAuto || vehiculeCourant->statut == 'E')) {
                    vehiculeCourant->statut = 'P';
                    vehiculeCourant->tps_stationnement = time(NULL);
                }

                if (vehiculeCourant->position.x >= 80 && vehiculeCourant->position.y <= 9) {
                    vehiculeCourant->statut = 'F';
                }
                else {
                    afficherVehicule(vehiculeCourant);
                }


                if (modeAuto && vehiculeCourant->statut == 'P' && time(NULL) - vehiculeCourant->tps_stationnement >= 10) {

                    // petit recul pour sortir de la place (optionnel)
                    char dir_opp = (vehiculeCourant->direction == 'O') ? 'E' : 'O';
                    char touche_direction = get_direction_normalise(dir_opp);
                    deplacerVehicule(touche_direction, vehiculeCourant);

                    vehiculeCourant->ancienne_position.x = vehiculeCourant->position.x;
                    vehiculeCourant->ancienne_position.y = vehiculeCourant->position.y;

                    vehiculeCourant->statut = 'S';
                }

                if (isToutVehiculeSorti()) {
                    running = 0;
                    break;
                }
            }

            afficher_gain_multi();
            afficherInstructions(vehiculeCourant);

            refresh();

            usleep(70000);
        }

        //Ferme le mode ncurses pour remettre le terminal normalement
        endwin();
        unsigned long int prix = 0;

        for (int p = 0; p < nb_vehicules ; ++p) {
            prix = prix + (tab_vehicules[p].tps * 2);
        }

        int temps = time(NULL) - chronometre;

        printf("\n\nVous avez passé un temps totale de %02d:%02d\n", temps / 60, temps % 60);
        printf("Vous devez régler %02lu,%02lu€\n", prix / 100, prix % 100);
        printf("\n\nMerci d'avoir joué à Take IT !\n");

        // on desaloue l'espace mémoire de notre tableau dynamique
        free(tab_vehicules);
        tab_vehicules = NULL;

        return 0;
    }
}