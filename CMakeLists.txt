cmake_minimum_required(VERSION 4.0)
project(stp C)

set(CMAKE_C_STANDARD 11)

add_executable(stp src/main.c
        library/vehicule.h
        library/configuration.h
        src/vehicule.c
        src/configuration.c
        library/jeu.h)
