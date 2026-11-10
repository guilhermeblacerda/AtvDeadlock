#ifndef BANKER_H
#define BANKER_H

#define MAX_CLIENTES 100
#define MAX_RECURSOS 20
#define MAX_COMANDOS 1000
#define TAM_LINHA 512

typedef struct {
    int n_clientes;
    int n_recursos;
    int **maximo;
    int **alocacao;
    int **necessidade;
    int *disponivel;
    FILE *saida;
} estado_t;

int **cria_matriz(int linhas, int colunas);
void libera_matriz(int **m, int linhas);

#endif