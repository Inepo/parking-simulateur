# Projet C

⚙️ Pour compiler un fichier unique :
`gcc main.c -o main`

▶️ Lancer le programme compilé :
`./main`


☞ Afin de pouvoir lancer avec ncurses (library pour l'affichage)

1 - Pour compiler (du moins du MacOS):
`clang main.c -Wall -Wextra -lncurses -o parking`

Pour windows il faut utiliser WLS :

`sudo apt install clang libncurses-dev`
`clang main.c -Wall -Wextra -lncurses -o parking`

2 - Lancer le programme compilé :
`./parking`
