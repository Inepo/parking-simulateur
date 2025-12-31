#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <locale.h>
#include <wchar.h>
#include <time.h>

#ifdef __APPLE__
    #include <ncurses.h>
#else
    #include <ncursesw/ncurses.h>
#endif

#include "../library/vehicule.h"
#include "../library/configuration.h"
#include "../library/jeu.h"


wchar_t plan[HAUTEUR][LARGEUR];

int nb_vehicules;

int avec_collision;

int gain_save = 0;

int game_over;

unsigned long int chronometre;

MODE_JEU modeJeu;

VEHICULE *tab_vehicules;

VEHICULE *joueur;

int payeJoueur = 0;



void lancer_musique() {
    system("play -q -v 0.5 assets/ost_ft.wav &");

}

void arreter_musique() {
    system("pkill -f 'ost_ft.wav'");
}

void klaxon() {
    system("play -q -v 0.5 assets/klaxon.wav &");
}

void gameOver() {
    system("play -q -v 0.5 assets/gameover.wav &");
}



// Charger le plan depuis un fichier
void chargerParking() {
    FILE *f = fopen("assets/parking.txt", "r");

    if (!f) {
        printf("Erreur: impossible d'ouvrir %s\n", "assets/parking.txt");
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




// 0 -> collision detecté et pas de mouvement
// 1 -> collision detécté et mouvement
int deplacer(VEHICULE* vehiculeCourant, int *running) {
    if (modeJeu == MANUELLE_COLLISION || modeJeu == MANUELLE_SANS_COLLISION || (vehiculeCourant->mode == 'S'))
    {
        int collision = vehiculeCourant->mode == 'S' || avec_collision;
        // Vérifier les touches
        char touche = key_pressed();
            if (touche != 0) {
                switch(touche) {
                    case 'z':
                    case 'q':
                    case 's':
                    case 'd':
                        return deplacerVehicule(touche, vehiculeCourant, collision, plan, nb_vehicules, tab_vehicules);
                    case 'p':
                        klaxon();
                        break;
                    case 27:
                        *running = 0;
                        break;
                }
            }
    }

    else
    {
        if (vehiculeCourant->statut == 'E')
        {
            deplacerMultiVehiculeAuto(vehiculeCourant, nb_vehicules, tab_vehicules, plan);
        }
        else if (vehiculeCourant->statut == 'S') {
            // Phase 0 : aller à l'intersection la plus proche sur x=46
            if (vehiculeCourant->phase_sortie == 0)
            {
                // Si on n'est pas encore sur x=46, y aller d'abord
                if (vehiculeCourant->position.x != 46) {
                    int ys[7] = {45,39,33,27,21,15,7};
                    int bestY = ys[0];
                    int bestD = abs(vehiculeCourant->position.y - ys[0]);

                    for (int i=0; i<7; i++){
                        int d = abs(vehiculeCourant->position.y - ys[i]);
                        if (d < bestD) {
                            bestD = d;
                            bestY = ys[i];
                        }
                    }

                    COORDONEE coord;
                    coord.x = 46;
                    coord.y = bestY;

                    deplacerVehiculeVers(coord, vehiculeCourant,avec_collision,plan, nb_vehicules, tab_vehicules);
                }

                // Une fois sur x=46, se positionner sur une intersection
                else {
                    int ys[7] = {45,39,33,27,21,15,7};
                    int bestY = ys[0];
                    int bestD = abs(vehiculeCourant->position.y - ys[0]);

                    for (int i=0; i<7; i++){
                        int d = abs(vehiculeCourant->position.y - ys[i]);
                        if (d < bestD) {
                            bestD = d;
                            bestY = ys[i];
                        }
                    }

                    // Forcer uniquement Nord ou Sud pour rester sur x=46
                    if (vehiculeCourant->position.y > bestY) {
                        // Aller vers le Nord
                        vehiculeCourant->direction = 'N';
                        chargerVehicule(vehiculeCourant);
                        deplacerVehicule('z', vehiculeCourant, avec_collision, plan, nb_vehicules, tab_vehicules);
                    }
                    else if (vehiculeCourant->position.y < bestY) {
                        // Aller vers le Sud
                        vehiculeCourant->direction = 'S';
                        chargerVehicule(vehiculeCourant);
                        deplacerVehicule('s', vehiculeCourant, avec_collision, plan, nb_vehicules, tab_vehicules);
                    }

                    // Vérifier si on a atteint l'intersection
                    if (abs(vehiculeCourant->position.y - bestY) <= 1) {
                        vehiculeCourant->phase_sortie = 1;
                    }
                }
            }

            // Phase 1 : monter sur la colonne x=46 vers y=8
            else if (vehiculeCourant->phase_sortie == 1)
            {
                // Forcer à rester sur x=46 en allant uniquement Nord
                if (vehiculeCourant->position.x == 46) {
                    vehiculeCourant->direction = 'N';
                    chargerVehicule(vehiculeCourant);
                    deplacerVehicule('z', vehiculeCourant, avec_collision, plan, nb_vehicules, tab_vehicules);

                    if (vehiculeCourant->position.y <= 8) {
                        vehiculeCourant->phase_sortie = 2;
                    }
                }
                else {
                    // Si on a dévié de x=46, y retourner d'abord
                    COORDONEE coord;
                    coord.x = 46;
                    coord.y = vehiculeCourant->position.y;
                    deplacerVehiculeVers(coord, vehiculeCourant, avec_collision, plan, nb_vehicules, tab_vehicules);
                }
            }

            else {
                COORDONEE coord;
                coord.x = 80;
                coord.y = 7;
                deplacerVehiculeVers(coord, vehiculeCourant, avec_collision, plan, nb_vehicules, tab_vehicules);
            }
        }
    }

    return 0;
}




void libererVehicule(VEHICULE *vehiculeCourant) {
    vehiculeCourant->statut = 'F';

    gain_save = gain_save + (vehiculeCourant->tps * 2);

    VEHICULE *nouveau = malloc((nb_vehicules - 1) * sizeof(VEHICULE));
    int k = 0;

    for (int i = 0; i < nb_vehicules; i++) {
        if (&tab_vehicules[i] != vehiculeCourant) {
            nouveau[k++] = tab_vehicules[i];
        }
    }

    free(tab_vehicules);
    tab_vehicules = nouveau;
    nb_vehicules--;
}




void afficherTemps() {
    int temps = time(NULL) - chronometre;

    attron(COLOR_PAIR(2));
    mvprintw(5, 116, "Temps: %02d:%02d", temps / 60, temps % 60);
    attroff(COLOR_PAIR(2));
    refresh();
}


// affichage du hain sur la console (temps par voiture * 2)
void afficherGain()
{
    attron(COLOR_PAIR(2));

    unsigned long int prix = 0;

    for (int p = 0; p < nb_vehicules; ++p) {
        if (tab_vehicules[p].mode == 'S') {
            payeJoueur = tab_vehicules[p].tps * 2;
        }
        prix = prix + (tab_vehicules[p].tps * 2);
    }

    prix = prix + gain_save;

    mvprintw(3,113, "Gain total : %02lu,%02lu€",prix / 100, prix % 100);

    attroff(COLOR_PAIR(2));
}


// affichage des instructions notament en mode solo ou manuelle
void afficherInstructions(VEHICULE *vehicule)
{
    attron(COLOR_PAIR(2));
    // sous le parking
    int ligne = HAUTEUR - 1;

    mvprintw(ligne,     0, "ZQSD: Deplacer la voiture rouge");
    mvprintw(ligne + 1, 0, "ESC: Quitter");
    mvprintw(ligne + 2, 0, "Position: (%d,%d)", vehicule->position.x, vehicule->position.y);

    attroff(COLOR_PAIR(2));
}





// paramettrage du mode de jeu
void parametreModeJeu()
{
    VEHICULE vehicule;

    char types[] = {'v', 'm', 'c'};

    switch (modeJeu) {
        case 0:
            avec_collision = 0;

            vehicule.position.x = 86;
            vehicule.position.y = 53;
            vehicule.direction = 'O';
            vehicule.mode = 'M'; //Manuelle
            vehicule.statut = 'E'; //Entre dans le parking
            vehicule.tps = 0;
            vehicule.type = 'v'; //voiture
            vehicule.count_meme_direction = 0;
            vehicule.klaxon = false;

            nb_vehicules = 1;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            tab_vehicules[0] = vehicule;

            chargerVehicule(&tab_vehicules[0]);

            break;
        case 1 :
            avec_collision = 1;

            vehicule.position.x = 86;
            vehicule.position.y = 53;
            vehicule.direction = 'O';
            vehicule.mode = 'M'; //Manuelle
            vehicule.statut = 'E'; //Entre dans le parking
            vehicule.tps = 0;
            vehicule.type = 'v'; //voiture
            vehicule.nb_sans_bouger = 0;
            vehicule.klaxon = false;

            nb_vehicules = 1;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            tab_vehicules[0] = vehicule;

            chargerVehicule(&tab_vehicules[0]);

            break;
        case 2:
            avec_collision = 0;

            nb_vehicules = 5;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            for (int indexVehicule = 0; indexVehicule < nb_vehicules; indexVehicule++)
            {
                vehicule.position.x = 86;
                vehicule.position.y = 53;
                vehicule.direction = 'O'; // Ouest <- vers la gauche
                vehicule.mode = 'A'; // Manuelle
                vehicule.statut = 'E'; // Entre dans le parking
                vehicule.tps = 0;
                vehicule.tps_stationnement = 0;
                vehicule.ancienne_position.x = -1;
                vehicule.ancienne_position.y = -1;
                vehicule.phase_sortie = 0;
                vehicule.start_time = chronometre + (indexVehicule * 2); // 0s pour la 1ère, 3s pour la 2e
                vehicule.active = 0;
                vehicule.type = types[rand() % 3];
                vehicule.nb_sans_bouger = 0;
                vehicule.klaxon = false;

                //attribue aléatoirement un temps (entre 10 et 30 secondes) de stationnement
                vehicule.tps_stationnement_alloue = 10 + rand() % 21;

                tab_vehicules[indexVehicule] = vehicule;

                chargerVehicule(&tab_vehicules[indexVehicule]);
            }

            break;
        case 3:
            avec_collision = 0;

            nb_vehicules = 30;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }

            for (int indexVehicule = 0; indexVehicule < nb_vehicules; indexVehicule++)
            {
                vehicule.position.x = 86;
                vehicule.position.y = 53;
                vehicule.direction = 'O'; // Ouest <- vers la gauche
                vehicule.mode = 'A'; // Manuelle
                vehicule.statut = 'E'; // Entre dans le parking
                vehicule.tps = 0;
                vehicule.tps_stationnement = 0;
                vehicule.ancienne_position.x = -1;
                vehicule.ancienne_position.y = -1;
                vehicule.phase_sortie = 0;
                vehicule.nb_sans_bouger = 0;
                vehicule.klaxon = false;

                vehicule.start_time = chronometre + (indexVehicule * 2); // 0s pour la 1ère, 3s pour la 2e
                vehicule.active = 0;
                vehicule.type = types[rand() % 3];

                //attribue aléatoirement un temps (entre 10 et 30 secondes) de stationnement
                vehicule.tps_stationnement_alloue = 10 + rand() % 21;

                tab_vehicules[indexVehicule] = vehicule;

                chargerVehicule(&tab_vehicules[indexVehicule]);
            }
            break;

        case 4:
            avec_collision = 0;

            nb_vehicules = 6;

            tab_vehicules = malloc(nb_vehicules * sizeof(VEHICULE));

            if (tab_vehicules == NULL) {
                printf("Erreur: malloc\n");
                exit(1);
            }


            for (int indexVehicule = 0; indexVehicule < nb_vehicules - 1; indexVehicule++)
            {
                vehicule.position.x = 86;
                vehicule.position.y = 53;
                vehicule.direction = 'O'; // Ouest <- vers la gauche
                vehicule.mode = 'A'; // Manuelle
                vehicule.statut = 'E'; // Entre dans le parking
                vehicule.tps = 0;
                vehicule.tps_stationnement = 0;
                vehicule.ancienne_position.x = -1;
                vehicule.ancienne_position.y = -1;
                vehicule.phase_sortie = 0;
                vehicule.nb_sans_bouger = 0;
                vehicule.klaxon = false;

                vehicule.start_time = chronometre + (indexVehicule * 2); // 0s pour la 1ère, 3s pour la 2e
                vehicule.active = 0;
                vehicule.type = types[rand() % 3];

                //attribue aléatoirement un temps (entre 10 et 30 secondes) de stationnement
                vehicule.tps_stationnement_alloue = 10 + rand() % 21;

                tab_vehicules[indexVehicule] = vehicule;

                chargerVehicule(&tab_vehicules[indexVehicule]);
            }

            vehicule.position.x = 86;
            vehicule.position.y = 53;
            vehicule.direction = 'O';
            vehicule.mode = 'S'; //Manuelle et special
            vehicule.statut = 'E'; //Entre dans le parking
            vehicule.tps = 0;
            vehicule.type = 'v'; //voiture
            vehicule.nb_sans_bouger = 0;
            vehicule.klaxon = false;

            vehicule.start_time = chronometre + (nb_vehicules * 1.5);
            vehicule.active = 0;

            tab_vehicules[nb_vehicules - 1] = vehicule;

            joueur = &tab_vehicules[nb_vehicules - 1];

            chargerVehicule(&tab_vehicules[nb_vehicules - 1]);
            break;
    }

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
    //bleu
    init_pair(6, COLOR_BLUE, -1);
    //Orange
    init_pair(7, 159, COLOR_BLACK);
}



void afficherInfoModeJeu() {
    attron(COLOR_PAIR(2));
    attron(A_BOLD);

    switch (modeJeu) {
        case 0:
            mvprintw(11, 108, "Mode entraînement facile activé");
            mvprintw(12, 123, "—");
            mvprintw(13, 119, "détends-toi !");
            mvprintw(15, 111, "Le jeu veille sur toi");
            mvprintw(16, 123, "—");
            mvprintw(17, 103, "le décor empêche toute escapade imprévue.");
            break;
        case 1:
            mvprintw(11, 107, "Mode entraînement difficile activé");
            mvprintw(12, 123, "—");
            mvprintw(13, 119, "attention !");
            mvprintw(15, 109, "Le jeu ne veille plus sur toi");
            mvprintw(16, 123, "—");
            mvprintw(17, 105, "toute escapade imprévue est game over.");
            break;
        case 2:
            mvprintw(12, 114, "Mode fluide activé");
            mvprintw(13, 123, "—");
            mvprintw(14, 112, "trafic fluide garanti !");
            mvprintw(15, 108, "se garer devient un jeu d’enfant");
            break;
        case 3:
            mvprintw(11, 114, "Mode chargé activé");
            mvprintw(12, 123, "—");
            mvprintw(13, 114, "le parking déborde !");
            mvprintw(14, 112, "Beaucoup de véhicules…");
            mvprintw(16, 111, "prépare-toi aux bouchons.");
            break;
        case 4:
            mvprintw(11, 114, "Mode Spécial activé");
            mvprintw(12, 123, "—");
            mvprintw(13, 118, "Version BETA");
            mvprintw(14, 111, "Conduis parmi les voitures");
            mvprintw(15, 120, "autonomes !");
            mvprintw(16, 110, "⚠ Fonctionnalité expérimentale");
            break;
    }

    attroff(COLOR_PAIR(2));
    attroff(A_BOLD);
}



int main() {

    //l’Unicode (wchar_t)
    setlocale(LC_ALL, "");

    system("clear");

    lancer_musique();

    srand(time(NULL));

    // Charger le plan
    chargerParking();

    printf("\n");
    afficherFichier("assets/logo.txt");

    printf("\033[1;37m");  // Blanc

    printf("★ Regle du jeu ★\n");
    printf("Jeu permettant de simuler le stationnement de voitures dans un parking payant\n\n\n\n");

    printf("\033[4;31m");// Rouge souligné
    printf("Mode Parking ON :");
    printf("\033[0m"); //RESET
    printf("\033[1;31m");  // Rouge
    printf(" vitesse plafonnée à 3 px/s.\n");
    printf("Même à fond, votre véhicule refuse d’obéir… et c’est voulu\n\n\n\n");

    printf("\033[4;37m");  // Blanc souligne
    printf("Pour commencer veuillez choisir votre mode de jeu :\n");
    printf("[1] Entrainement — mode facile.\n");
    printf("[2] Entrainement — mode difficle.\n");
    printf("[3] Fluide — trafic léger, se garer devient un jeu d’enfant !\n");
    printf("[4] Chargé — beaucoup de véhicules… attention aux bouchons !\n");
    printf("[5] Spécial — tu n’es plus seul... d’autres voitures roulent toutes seules !\n\n\n\n");

    printf("\033[0m"); // RESET
    printf("\033[1;37m");  // Blanc

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
            case 3: modeJeu = FLUIDE; break;
            case 4: modeJeu = CHARGE; break;
            case 5: modeJeu = SPECIAL; break;
            default:
                printf("Mode inconnu. Au revoir.\n");
                return 0;
        }

        //srand(time(NULL));
        chronometre = time(NULL);

        initscr();
        start_color();
        use_default_colors();

        parametreModeJeu();

        int running = 1;

        bool modeAuto = (modeJeu == FLUIDE || modeJeu == CHARGE || modeJeu == SPECIAL);

        while (running)
        {
            // Effacer le contenue de l'ecran virtuel (fournit par ncurses)
            erase();

            afficherParking();
            afficherInfoModeJeu();

            unsigned long now = time(NULL);

            VEHICULE *vehiculeCourant;


            for (int indexVehicule = 0; indexVehicule < nb_vehicules; indexVehicule++)
            {

                vehiculeCourant = &tab_vehicules[indexVehicule];

                vehiculeCourant->tps = time(NULL) - chronometre;



                // activation au bon moment
                if (!vehiculeCourant->active)
                {
                    if (now >= vehiculeCourant->start_time) {
                        vehiculeCourant->active = 1;
                    } else {
                        continue;
                    }
                }



                game_over = deplacer(vehiculeCourant, &running);

                if (game_over == 1 && (modeJeu == MANUELLE_SANS_COLLISION || modeJeu == SPECIAL)) {
                    running = 0;
                    break;
                }


                if (vehiculeCourant->statut == 'P' &&
                    (!modeAuto || (time(NULL) - vehiculeCourant->tps_stationnement < vehiculeCourant->tps_stationnement_alloue)))
                {
                    colorerMur(vehiculeCourant, plan);
                }


                if(isDansPlaceParking(vehiculeCourant,plan) &&
                    (!modeAuto || vehiculeCourant->statut != 'P'))
                {
                    vehiculeCourant->statut = 'P';
                    vehiculeCourant->tps_stationnement = time(NULL);
                }


                if (vehiculeCourant->position.x >= 80 && vehiculeCourant->position.y <= 9)
                {
                    libererVehicule(vehiculeCourant);
                }
                else {
                    afficherVehicule(vehiculeCourant, modeJeu);
                }


                if (modeAuto && vehiculeCourant->statut == 'P' && time(NULL) - vehiculeCourant->tps_stationnement >= 10)
                {
                    sortirPlaceParking(vehiculeCourant,avec_collision,plan,nb_vehicules,vehiculeCourant);
                }

            }

            afficherGain();

            if (!modeAuto) {
                afficherInstructions(vehiculeCourant);
            }

            if (modeJeu == SPECIAL && joueur->active) {
                afficherInstructions(joueur);
            }

            afficherTemps();

            refresh();

            usleep(70000);

            if (isToutVehiculeSorti(nb_vehicules, tab_vehicules))
            {
                running = 0;
                break;
            }
        }

        //Ferme le mode ncurses pour remettre le terminal normalement
        endwin();

        /* Reset écran */
        printf("\033[2J");   // clear
        printf("\033[H");    // cursor
        fflush(stdout);

        int temps = time(NULL) - chronometre;

        if (game_over == 1 && (modeJeu == MANUELLE_SANS_COLLISION || modeJeu == SPECIAL)) {
            arreter_musique();
            gameOver();
            printf("\n\n");
            afficherFichier("assets/game_over.txt");
            printf("\033[1;31m");// Rouge souligné
            printf("Spoiler — les murs ne bougent pas. Toi si. Réessaie !");
            printf("\n\n\n");
            printf("\033[1;37m");  // Blanc

            if (modeJeu == MANUELLE_SANS_COLLISION) {
                printf("Vous avez passé un temps totale de \033[1;35m%02d:%02d\033[0m\n", temps / 60, temps % 60);
            }
            else {
                temps = joueur->tps;

                printf("\033[1;37m");  // Blanc
                printf("\n\nVous avez passé un temps totale de \033[1;35m%02d:%02d\033[0m\n", temps / 60, temps % 60);
                printf("\033[1;37m");  // Blanc
                printf("Vous devez régler \033[1;35m%02d,%02d€\033[0m\n", payeJoueur / 100, payeJoueur % 100);
            }
        }
        else {
            printf("\033[1;37m");  // Blanc
            printf("\n\nVous avez passé un temps totale de \033[1;35m%02d:%02d\033[0m\n", temps / 60, temps % 60);
            printf("\033[1;37m");  // Blanc
            printf("Vous devez régler \033[1;35m%02d,%02d€\033[0m\n", gain_save / 100, gain_save % 100);
        }

        printf("\033[1;37m");  // Blanc
        printf("\n\nMerci d'avoir joué à ");

        printf("\033[4;31m");// Rouge souligné
        printf("VROU.vroumm");

        printf("\033[0m"); // RESET
        printf("\033[1;37m");  // Blanc souligne
        printf(" !\n\n\n\n");

        // on desaloue l'espace mémoire de notre tableau dynamique
        free(tab_vehicules);
        tab_vehicules = NULL;

        arreter_musique();

        return 0;
    }
}