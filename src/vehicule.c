//
// Created by Ines KADJI on 30/12/2025.
//

#include "../library/vehicule.h"

#include <stdlib.h>
#include <wchar.h>
#include <string.h>

#include "../library/configuration.h"

// Methode élémentaire pour connaitre les dimension d'un vehicule
DIMENSIONS getDimensionsVehicule(char direction) {
    DIMENSIONS dim = {0};

    switch(direction) {
        case 'N':
        case 'S':
            dim.largeur = 4;
            dim.hauteur = 4;
            dim.offset_x = 2;
            dim.offset_y = 2;
            break;
        case 'E':
        case 'O':
            dim.largeur = 10;
            dim.hauteur = 3;
            dim.offset_x = 4;
            dim.offset_y = 2;
            break;
    }


    return dim;
}

// Verifier si tout les vehicules sont sortis
int isToutVehiculeSorti(int nb_vehicules, VEHICULE *tab_vehicules) {
    for (int i = 0; i < nb_vehicules; i++) {
        if (tab_vehicules[i].statut != 'F') {
            return 0;
        }
    }
    return 1;
}


// Verifier si le vehicule est dans une place de parking
int isDansPlaceParking(VEHICULE *voiture, wchar_t plan[HAUTEUR][LARGEUR])
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
        for (int i = 0; i < 5; i++) {
            voiture->coord_mur_place[i].x = x_mur;
            voiture->coord_mur_place[i].y = y_debut + i;
        }

        return 1;

    }

    return 0;
}

void klaxonMultiple() {
    // Vérifier si le processus play est déjà en cours
    int ret = system("pgrep -f 'klaxonBouchon.wav' > /dev/null 2>&1");

    if (ret == 0) {
        return;  // Le son est déjà en train de jouer
    }

    system("play -q assets/klaxonBouchon.wav &");
}

// Creation de vehicule
void chargerVehicule(VEHICULE *vehicule)
{
    const char* fichier = NULL;

    if (vehicule->type == 'v') {
        if (vehicule->direction == 'N' || vehicule->direction == 'S') {
            fichier = "assets/voiture_Z.txt";
        }
        else {
            fichier = "assets/voiture_D.txt";
        }
    }

    else if (vehicule->type == 'm') {
        if (vehicule->direction == 'N' || vehicule->direction == 'S') {
            fichier = "assets/moto_Z.txt";
        }
        else {
            fichier = "assets/moto_D.txt";
        }
    }

    else if (vehicule->type == 'c') {
        if (vehicule->direction == 'N') {
            fichier = "assets/camion_Z.txt";
        }
        else if (vehicule->direction == 'S') {
            fichier = "assets/camion_S.txt";
        }
        else if (vehicule->direction == 'O') {
            fichier = "assets/camion_Q.txt";
        }
        else {
            fichier = "assets/camion_D.txt";
        }
    }

    FILE *f;

    if (fichier != NULL) {
        f = fopen(fichier, "r");

        if (!f) {
            printf("Erreur: impossible d'ouvrir %s\n", fichier);
            return;
        }
    }
    else {
        printf("Erreur");
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (fgets(vehicule->Carrosserie[i], 30, f)) {
            int len = strlen(vehicule->Carrosserie[i]);
            if (len > 0 && vehicule->Carrosserie[i][len-1] == '\n') {
                vehicule->Carrosserie[i][len-1] = '\0';
            }
        } else {
            strcpy(vehicule->Carrosserie[i], "");
        }
    }

    fclose(f);
}

// Verifier s'il y a un vehicule juste devant
int isVehiculeDevant(COORDONEE coord, char direction, VEHICULE* vehicule, int nb_vehicules, VEHICULE* tab_vehicules) {

    // Distance de sécurité
    int distance_check = 6;

    if (vehicule->type == 'c') {
        distance_check = 8;
    }

    DIMENSIONS dim_actuel = getDimensionsVehicule(direction);

    // Calcul zone occupée par le véhicule
    int x_actuel = coord.x - dim_actuel.offset_x;
    int y_actuel = coord.y - dim_actuel.offset_y;


    for (int i = 0; i < nb_vehicules; i++) {
        VEHICULE *autre = &tab_vehicules[i];

        if (autre->direction == getDirectionOppose(vehicule->direction)) {
            distance_check = 3;
        }

        if (!autre->active || autre == vehicule) continue;

        DIMENSIONS dim_autre = getDimensionsVehicule(autre->direction);

        int ax = autre->position.x - dim_autre.offset_x;
        int ay = autre->position.y - dim_autre.offset_y;


        switch(direction) {
            case 'N':
                // Véhicule devant = plus haut (y plus petit)
                if (ay < y_actuel && ay >= y_actuel - distance_check) {
                    // Vérifier chevauchement horizontal
                    if (!(ax + dim_autre.largeur <= x_actuel ||
                          ax >= x_actuel + dim_actuel.largeur)) {
                        return 1;
                    }
                }
                break;

            case 'S':
                // Véhicule devant = plus bas (y plus grand)
                if (ay > y_actuel && ay <= y_actuel + distance_check) {
                    // Vérifier chevauchement horizontal
                    if (!(ax + dim_autre.largeur <= x_actuel ||
                          ax >= x_actuel + dim_actuel.largeur)) {
                        return 1;
                    }
                }
                break;

            case 'O':
                // Véhicule devant = plus à gauche (x plus petit)
                if (ax < x_actuel && ax >= x_actuel - distance_check) {
                    // Vérifier chevauchement vertical
                    if (!(ay + dim_autre.hauteur <= y_actuel ||
                          ay >= y_actuel + dim_actuel.hauteur)) {
                        return 1;
                    }
                }
                break;

            case 'E':
                // Véhicule devant = plus à droite (x plus grand)
                if (ax > x_actuel && ax <= x_actuel + distance_check) {
                    // Vérifier chevauchement vertical
                    if (!(ay + dim_autre.hauteur <= y_actuel ||
                          ay >= y_actuel + dim_actuel.hauteur)) {
                        return 1;
                    }
                }
                break;
        }
    }

    return 0;
}

// Afficher un véhicule sur la console
void afficherVehicule(VEHICULE *v)
{
    DIMENSIONS dim = getDimensionsVehicule(v->direction);
    int x = v->position.x - dim.offset_x;
    int y = v->position.y - dim.offset_y;

    // Couleur selon le type
    if (v->type == 'v') attron(COLOR_PAIR(3)); // Rouge pour voiture
    else if (v->type == 'c') attron(COLOR_PAIR(4)); // Magenta pour camion
    else if (v->type == 'm') attron(COLOR_PAIR(6)); // Bleu pour moto

    for (int i = 0; i < 4; i++) {
        for (int i = 0; i < dim.hauteur; i++) {
            if (v->Carrosserie[i][0] != '\0') {
                mvprintw(y + i, x, "%s", v->Carrosserie[i]);
            }
        }
    }

    if (v->type == 'v') attroff(COLOR_PAIR(3));
}


// Vérifier les collisions
int isCollision(COORDONEE coord, char direction,
    VEHICULE* vehicule, int avec_collision,wchar_t plan[HAUTEUR][LARGEUR],
    int nb_vehicules, VEHICULE* tab_vehicules) {

    if (avec_collision == 1) {
        return 0;
    }

    DIMENSIONS dim = getDimensionsVehicule(direction);

    // Calculer le coin haut gauche à partir du centre
    int x = coord.x - dim.offset_x;
    int y = coord.y - dim.offset_y;

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

    if (isVehiculeDevant(coord, direction, vehicule, nb_vehicules, tab_vehicules)) {
        return 1;
    }

    return 0;
}


// Permet de savoir s'il y a un vehicule à une place
int isPlaceOccupe(COORDONEE coord, char direction, int nb_vehicules, VEHICULE* tab_vehicules) {
    // On vérifie sur 3O caractères
    int distance_check = 32;

    for (int i = 0; i < nb_vehicules; i++) {
        VEHICULE *autre = &tab_vehicules[i];

        if (!autre->active) continue;

        int ax = autre->position.x;
        int ay = autre->position.y;

        if (direction == 'O') {
            if (ax < coord.x && ax >= coord.x - distance_check) {
                int diff_y = abs(ay - coord.y);
                if (diff_y <= 3) {
                    return 1;
                }
            }
        }
        else if (direction == 'E') {
            if (ax > coord.x && ax <= coord.x + distance_check) {
                int diff_y = abs(ay - coord.y);

                if (diff_y <= 3) {
                    return 1;
                }
            }
        }
    }

    return 0;
}


// Déplacer le vehicule manuellement
void deplacerVehicule(char direction, VEHICULE* vehicule, int avec_collision, wchar_t plan[HAUTEUR][LARGEUR], int nb_vehicules, VEHICULE* tab_vehicules) {
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
        COORDONEE coord;
        coord.x = new_x;
        coord.y = new_y;
        if (isCollision(coord, nouvelle_direction, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules)) {
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

        chargerVehicule(vehicule);
    }
}

// Déplacer un vehicule automatiquement dans un mode multi
void deplacerMultiVehiculeAuto(VEHICULE *vehicule, int avec_collision, int nb_vehicules, VEHICULE *tab_vehicules, wchar_t plan[HAUTEUR][LARGEUR]) {
    int new_x = vehicule->position.x;
    int new_y = vehicule->position.y;

    char ancienne_direction = vehicule->direction;
    char nouvelle_direction = ancienne_direction;

    // Calculer direction opposée (pour éviter de faire demi tour)
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
    COORDONEE coord;
    coord.x = new_x;
    coord.y = new_y;
    bool peut_continuer = (new_x != 89) && !isCollision(coord, ancienne_direction, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules);

    if (new_y != 53 && isPlaceOccupe(coord, ancienne_direction, nb_vehicules, tab_vehicules)) {
        peut_continuer = false;
    }

    bool doit_changer = ((vehicule->count_meme_direction <= 6 && ancienne_direction == 'N') || !peut_continuer);


    if (doit_changer) {

        new_x = vehicule->position.x;
        new_y = vehicule->position.y;
        vehicule->count_meme_direction = 0;

        char directions[] = {'N', 'S', 'E', 'O'};
        bool direction_trouvee = false;

        // Melanger les directions
        for (int i = 3; i > 0; i--) {
            int j = rand() % (i + 1);
            char temp = directions[i];
            directions[i] = directions[j];
            directions[j] = temp;
        }

        // Phase 1 : Essayer toutes les directions sauf l'opposée
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


            if (test_x == 89) {
                continue;
            }

            coord.x = test_x;
            coord.y = test_y;

            if ((dir == 'O' || dir == 'E')) {
                if (!isCollision(coord, dir, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules) && (test_y != 53 && !isPlaceOccupe(coord, dir, nb_vehicules, tab_vehicules))) {
                    new_x = test_x;
                    new_y = test_y;
                    nouvelle_direction = dir;
                    direction_trouvee = true;
                }
            }
            else if (!isCollision(coord, dir, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir;
                direction_trouvee = true;
            }
        }

        // Phase 2 : Si vraiment bloqué pendant 25 tours, essayer même la direction opposée
        if (vehicule->nb_sans_bouger >= 18 && !direction_trouvee) {
            klaxonMultiple();
        }


        if (vehicule->nb_sans_bouger == 30 && !direction_trouvee) {
            int test_x = vehicule->position.x;
            int test_y = vehicule->position.y;

            switch(dir_opposee) {
                case 'N': test_y--; break;
                case 'S': test_y++; break;
                case 'O': test_x--; break;
                case 'E': test_x++; break;
            }

            coord.x = test_x;
            coord.y = test_y;

            if (test_x != 89 && !isCollision(coord, dir_opposee, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules)) {
                new_x = test_x;
                new_y = test_y;
                nouvelle_direction = dir_opposee;
            }

            vehicule->nb_sans_bouger = 0;
        }
        else {
            vehicule->nb_sans_bouger++;
        }
    } else {
        // Continue tout droit
        vehicule->count_meme_direction++;
    }

    vehicule->position.x = new_x;
    vehicule->position.y = new_y;

    if (ancienne_direction != nouvelle_direction) {
        vehicule->direction = nouvelle_direction;
        chargerVehicule(vehicule);
    }
}


// déplacer le vehicule vers une coordonée, nottament pour sortir du parking
void deplacerVehiculeVers(COORDONEE coord, VEHICULE* vehicule, int avec_collision, wchar_t plan[HAUTEUR][LARGEUR], int nb_vehicules, VEHICULE* tab_vehicules)
{
    int dx = coord.x - vehicule->position.x;
    int dy = coord.y - vehicule->position.y;

    char dir_opp = vehicule->direction;
    switch (vehicule->direction) {
        case 'N': dir_opp = 'S'; break;
        case 'S': dir_opp = 'N'; break;
        case 'E': dir_opp = 'O'; break;
        case 'O': dir_opp = 'E'; break;
    }

    char essais[4];
    int k = 0;

    // Si le véhicule est en sortie, JAMAIS vers le sud la sortie est en haut (N)
    bool interdit_sud = (vehicule->statut == 'S');

    if (abs(dx) >= abs(dy)) {
        if (dx > 0) essais[k++] = 'E';
        if (dx < 0) essais[k++] = 'O';
        if (dy < 0) essais[k++] = 'N';
        if (dy > 0 && !interdit_sud) essais[k++] = 'S';
    } else {
        if (dy < 0) essais[k++] = 'N';
        if (dy > 0 && !interdit_sud) essais[k++] = 'S';
        if (dx > 0) essais[k++] = 'E';
        if (dx < 0) essais[k++] = 'O';
    }

    int prev_x = vehicule->ancienne_position.x;
    int prev_y = vehicule->ancienne_position.y;

    char all[4] = {'N','S','E','O'};
    for (int i = 0; i < 4; i++) {
        if (interdit_sud && all[i] == 'S') continue;

        bool deja = false;
        for (int j = 0; j < k; j++) if (essais[j] == all[i]) deja = true;
        if (!deja) essais[k++] = all[i];
    }

    char choix = 0;

    // Phase 2 : essayer directions (sans demi tour + sans revenir en arrière)
    for (int i = 0; i < 4; i++) {
        char d = essais[i];
        if (d == dir_opp) continue;

        int nx = vehicule->position.x, ny = vehicule->position.y;
        if (d == 'N') ny--;
        if (d == 'S') ny++;
        if (d == 'E') nx++;
        if (d == 'O') nx--;

        if (nx == prev_x && ny == prev_y) continue;

        COORDONEE coord;
        coord.x = nx;
        coord.y = ny;

        if (!isCollision(coord, d, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules)) { choix = d; break; }
    }

    // Phase 3 : si bloqué, autoriser demi-tour mais pas si c'est juste revenir en arrière
    // ET JAMAIS vers le sud en sortie
    if (choix == 0) {
        if (!(interdit_sud && dir_opp == 'S')) {
            int nx = vehicule->position.x, ny = vehicule->position.y;
            if (dir_opp == 'N') ny--;
            if (dir_opp == 'S') ny++;
            if (dir_opp == 'E') nx++;
            if (dir_opp == 'O') nx--;

            COORDONEE coord;
            coord.x = nx;
            coord.y = ny;

            if (!(nx == prev_x && ny == prev_y) && !isCollision(coord, dir_opp, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules)) {
                choix = dir_opp;
            }
        }
    }

    if (choix != 0) {
        vehicule->ancienne_position.x = vehicule->position.x;
        vehicule->ancienne_position.y = vehicule->position.y;

        vehicule->direction = choix;

        char touche_direction = getToucheViaDirection(choix);

        chargerVehicule(vehicule);

        deplacerVehicule(touche_direction, vehicule, avec_collision, plan, nb_vehicules, tab_vehicules);
    }
}

void sortirPlaceParking(VEHICULE *vehiculeCourant, int avec_collision, wchar_t plan[HAUTEUR][LARGEUR], int nb_vehicules, VEHICULE* tab_vehicules) {
    // petit recul pour sortir de la place (optionnel)
    char dir_opp = (vehiculeCourant->direction == 'O') ? 'E' : 'O';
    char touche_direction = getToucheViaDirection(dir_opp);
    deplacerVehicule(touche_direction, vehiculeCourant, avec_collision, plan, nb_vehicules, tab_vehicules);

    vehiculeCourant->ancienne_position.x = vehiculeCourant->position.x;
    vehiculeCourant->ancienne_position.y = vehiculeCourant->position.y;

    vehiculeCourant->statut = 'S';
    vehiculeCourant->tps_stationnement = 0;
}
