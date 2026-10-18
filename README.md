# 🚗 Simulation de Parking en C

Réalisation d'une simulation de parking en **C** proposant **5 modes de jeu** : deux modes d'entraînement à la conduite, deux simulations automatiques (fluide et chargée) et un mode spécial mêlant conduite manuelle et trafic automatique.

---

## Sommaire

- [Aperçu](#-aperçu)
- [Modes de jeu](#-modes-de-jeu)
- [Installation](#-installation)
- [Compilation et lancement](#-compilation-et-lancement)
- [Commandes](#-commandes)

---

## Aperçu

Le programme représente un parking vu de dessus, avec ses places, ses allées et ses obstacles. Selon le mode choisi, vous prenez le volant d'une voiture, vous observez des voitures automatiques circuler, ou vous faites les deux en même temps.

---

## Modes de jeu

### [1] Entraînement — Mode facile
L'utilisateur contrôle le véhicule sans risque de collision : il peut toucher les obstacles sans perdre. Idéal pour prendre en main les commandes.

### [2] Entraînement — Mode difficile
L'utilisateur contrôle le véhicule avec gestion des collisions : le moindre contact avec un obstacle met fin à la partie (**game over**).

### [3] Fluide
Simulation d'un parking avec **5 véhicules automatiques** qui circulent, se garent et repartent.

### [4] Chargé
Simulation d'un parking avec **30 véhicules automatiques** : bouchons, attentes et klaxons.

### [5] Spécial
Simulation d'un parking avec **5 véhicules automatiques** et **1 véhicule contrôlé manuellement** par l'utilisateur, qui doit se frayer un chemin au milieu du trafic.

| # | Mode | Contrôle | Véhicules automatiques | Collisions |
|---|------|----------|:----------------------:|------------|
| 1 | Entraînement — facile | Manuel | 0 | Sans risque |
| 2 | Entraînement — difficile | Manuel | 0 | Game over |
| 3 | Fluide | Automatique | 5 | — |
| 4 | Chargé | Automatique | 30 | — |
| 5 | Spécial | Mixte | 5 (+1 manuel) | — |

---

## Installation

### Prérequis

- Un compilateur C (`gcc` ou `clang`)
- `make`
- Un système Linux / macOS (ou WSL sous Windows)
<!-- Si vous utilisez une bibliothèque (ncurses, SDL…), ajoutez-la ici, par exemple : -->
<!-- - `ncurses` : `sudo apt install libncurses-dev` -->

### Récupérer le projet

```bash
git clone <url-du-depot>
cd <nom-du-projet>
```

---

## Compilation et lancement

Avec le Makefile :

```bash
make
./parking
```

Ou directement avec `gcc` :

```bash
gcc -Wall -Wextra -o parking src/*.c
./parking
```

Pour nettoyer les fichiers compilés :

```bash
make clean
```

Au lancement, un menu permet de choisir l'un des 5 modes de jeu.

---

## Commandes

| Touche | Action |
|--------|--------|
| `↑` / `Z` | Avancer |
| `↓` / `S` | Reculer |
| `←` / `Q` | Tourner à gauche |
| `→` / `D` | Tourner à droite |
| `P` | Klaxon |
| `Échap` / `X` | Quitter / retour au menu |

---
