//
// Created by Ines KADJI on 30/12/2025.
//
#include <time.h>
#include <wchar.h>
#include <ncurses.h>//pour l'affichage
#include "jeu.h"

#ifndef STP_VEHICULE_H
#define STP_VEHICULE_H

#define HAUTEUR 60

#define LARGEUR 160

typedef struct {
    int x;
    int y;
} COORDONEE;

typedef struct voiture {
    char direction;      // N, S, E, O
    int count_meme_direction;   //nb même direction d'affilée
    COORDONEE position;     // Position
    char type;          // v, c, m (voiture, camion, moto)
    char Carrosserie[4][30]; //TODO
    int code_couleur;   // couleur de la voiture
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
    int nb_sans_bouger;         //nombre de fois que le vehicule n'a pas bougé pour eviter les demi tour inutile
    bool klaxon;
}VEHICULE;

typedef struct {
    int largeur;   // largeur du vehicule
    int hauteur;   // hauteur du vehicule
    int offset_x;  // offset du centre par rapport au coin haut-gauche
    int offset_y;  // offset du centre par rapport au coin haut-gauche
} DIMENSIONS;

int isToutVehiculeSorti(int nb_vehicules, VEHICULE *tab_vehicules);
int isDansPlaceParking(VEHICULE *voiture, wchar_t plan[HAUTEUR][LARGEUR]);
void chargerVehicule(VEHICULE *vehicule);
void afficherVehicule(VEHICULE *v, MODE_JEU modeJeu);
int deplacerVehicule(char direction, VEHICULE* vehicule, int avec_collision, wchar_t plan[HAUTEUR][LARGEUR], int nb_vehicules, VEHICULE* tab_vehicules);
void deplacerMultiVehiculeAuto(VEHICULE *vehicule, int nb_vehicules, VEHICULE *tab_vehicules, wchar_t plan[HAUTEUR][LARGEUR]);
int isPlaceOccupe(COORDONEE coord, char direction, int nb_vehicules, VEHICULE* tab_vehicules);
void deplacerVehiculeVers(COORDONEE coord, VEHICULE* vehicule,
    int avec_collision, wchar_t plan[HAUTEUR][LARGEUR], int nb_vehicules, VEHICULE* tab_vehicules);
void sortirPlaceParking(VEHICULE *vehiculeCourant, int avec_collision, wchar_t plan[HAUTEUR][LARGEUR], int nb_vehicules, VEHICULE* tab_vehicules);

#endif //STP_VEHICULE_H