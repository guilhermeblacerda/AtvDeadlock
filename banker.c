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

static int separa_linha(char *linha, int *valores, int esperado) {
    int cont = 0;
    char *p = linha;
    while (*p != '\0' && cont < esperado) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n') break;
        char *fim;
        long v = strtol(p, &fim, 10);
        if (fim == p) return -1;
        if (v < 0) return -1;
        valores[cont++] = (int) v;
        p = fim;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == ',') p++;
        else if (*p == '\n' || *p == '\0') break;
        else return -1;
    }
    if (cont != esperado) return -1;
    return 0;
}

static int le_customer(const char *nome, int **maximo, int *n_clientes, int n_recursos) {
    FILE *f = fopen(nome, "r");
    if (f == NULL) {
        printf("Erro: nao abriu %s\n", nome);
        return 0;
    }

    char linha[TAM_LINHA];
    int valores[MAX_RECURSOS];
    int linha_atual = 0;

    while (fgets(linha, sizeof(linha), f) != NULL) {
        char *p = linha;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\0') continue;

        if (separa_linha(p, valores, n_recursos) != 0) {
            printf("Erro: linha %d de %s invalida\n", linha_atual + 1, nome);
            fclose(f);
            return 0;
        }
        if (linha_atual >= MAX_CLIENTES) {
            printf("Erro: clientes demais\n");
            fclose(f);
            return 0;
        }
        for (int j = 0; j < n_recursos; j++) {
            maximo[linha_atual][j] = valores[j];
        }
        linha_atual++;
    }
    fclose(f);

    if (linha_atual == 0) {
        printf("Erro: %s vazio\n", nome);
        return 0;
    }
    *n_clientes = linha_atual;
    return 1;
}

int main(int argc, char **argv) {
    int disponivel[MAX_RECURSOS];
    int n_recursos = 0;

    if (!le_argumentos(argc, argv, disponivel, &n_recursos)) {
        return 1;
    }

    int **maximo = cria_matriz(MAX_CLIENTES, n_recursos);
    if (maximo == NULL) {
        printf("Erro: sem memória\n");
        return 1;
    }

    int n_clientes = 0;
    if (!le_customer("customer.txt", maximo, &n_clientes, n_recursos)) {
        libera_matriz(maximo, MAX_CLIENTES);
        return 1;
    }

    libera_matriz(maximo, MAX_CLIENTES);
    return 0;
}