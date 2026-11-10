#include <stdio.h>
#include <stdlib.h>
#include "banker.h"

int **cria_matriz(int linhas, int colunas) {
    int **m = malloc(linhas * sizeof(int *));
    if (m == NULL) return NULL;
    for (int i = 0; i < linhas; i++) {
        m[i] = calloc(colunas, sizeof(int));
        if (m[i] == NULL) {
            for (int j = 0; j < i; j++) free(m[j]);
            free(m);
            return NULL;
        }
    }
    return m;
}

void libera_matriz(int **m, int linhas) {
    if (m == NULL) return;
    for (int i = 0; i < linhas; i++) free(m[i]);
    free(m);
}