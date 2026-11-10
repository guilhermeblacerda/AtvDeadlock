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

static int le_argumentos(int argc, char **argv, int *disponivel, int *n_recursos) {
    if (argc < 2) {
        printf("Erro: informe a quantidade de recursos\n");
        return 0;
    }
    *n_recursos = argc - 1;
    if (*n_recursos > MAX_RECURSOS) {
        printf("Erro: recursos demais (max %d)\n", MAX_RECURSOS);
        return 0;
    }
    for (int i = 0; i < *n_recursos; i++) {
        char *fim;
        long v = strtol(argv[i + 1], &fim, 10);
        if (fim == argv[i + 1] || *fim != '\0' || v < 0) {
            printf("Erro: valor invalido '%s'\n", argv[i + 1]);
            return 0;
        }
        disponivel[i] = (int) v;
    }
    return 1;
}

int main(int argc, char **argv) {
    int disponivel[MAX_RECURSOS];
    int n_recursos = 0;

    if (!le_argumentos(argc, argv, disponivel, &n_recursos)) {
        return 1;
    }

    return 0;
}