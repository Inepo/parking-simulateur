//
// Created by Ines KADJI on 30/12/2025.
//

#ifndef STP_CONFIGURATION_H
#define STP_CONFIGURATION_H
#include "vehicule.h"

typedef enum {
    MANUELLE_COLLISION = 0,
    MANUELLE_SANS_COLLISION = 1,
    FLUIDE = 2,
    CHARGE = 3
} MODE_JEU;

char getToucheViaDirection(char direction);
char getDirectionOppose(char direction);
void afficherFichier(const char* fichier);
int isMur(wchar_t wc);
char key_pressed();
void colorerMur(VEHICULE *vehiculeCourant, wchar_t plan[HAUTEUR][LARGEUR]);
#endif //STP_CONFIGURATION_H