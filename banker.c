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

static int le_commands(const char *nome, comando_t *cmds, int *n_cmds, int n_clientes, int n_recursos) {
    FILE *f = fopen(nome, "r");
    if (f == NULL) {
        printf("Erro: nao abriu %s\n", nome);
        return 0;
    }

    char linha[TAM_LINHA];
    int total = 0;
    int num_linha = 0;

    while (fgets(linha, sizeof(linha), f) != NULL) {
        num_linha++;
        char *p = linha;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\0') continue;

        char tipo[8];
        if (sscanf(p, "%7s", tipo) != 1) {
            printf("Erro: linha %d vazia\n", num_linha);
            fclose(f);
            return 0;
        }
        p = p + strlen(tipo);
        while (*p == ' ' || *p == '\t') p++;

        if (total >= MAX_COMANDOS) {
            printf("Erro: comandos demais\n");
            fclose(f);
            return 0;
        }

        if (strcmp(tipo, "*") == 0) {
            cmds[total].tipo = CMD_STATUS;
            cmds[total].cliente = -1;
            total++;
        } else if (strcmp(tipo, "RQ") == 0 || strcmp(tipo, "RL") == 0) {
            int valores[MAX_RECURSOS + 1];
            int qtd = 0;
            char *q = p;
            while (*q != '\0' && *q != '\n' && qtd < n_recursos + 1) {
                while (*q == ' ' || *q == '\t') q++;
                if (*q == '\0' || *q == '\n') break;
                char *fim;
                long v = strtol(q, &fim, 10);
                if (fim == q) break;
                valores[qtd++] = (int) v;
                q = fim;
            }
            if (qtd != n_recursos + 1) {
                printf("Erro: linha %d com %d valores (esperado %d)\n",
                       num_linha, qtd, n_recursos + 1);
                fclose(f);
                return 0;
            }
            int cli = valores[0];
            if (cli < 0 || cli >= n_clientes) {
                printf("Erro: linha %d cliente %d invalido\n", num_linha, cli);
                fclose(f);
                return 0;
            }
            cmds[total].tipo = (strcmp(tipo, "RQ") == 0) ? CMD_RQ : CMD_RL;
            cmds[total].cliente = cli;
            for (int j = 0; j < n_recursos; j++) {
                cmds[total].recursos[j] = valores[j + 1];
            }
            total++;
        } else {
            printf("Erro: linha %d comando '%s' desconhecido\n", num_linha, tipo);
            fclose(f);
            return 0;
        }
    }
    fclose(f);

    if (total == 0) {
        printf("Erro: %s vazio\n", nome);
        return 0;
    }
    *n_cmds = total;
    return 1;
}

static void prepara_estado(estado_t *e, int **maximo, int **alocacao, int **necessidade,
                           int *disponivel, int n_clientes, int n_recursos) {
    e->n_clientes = n_clientes;
    e->n_recursos = n_recursos;
    e->maximo = maximo;
    e->alocacao = alocacao;
    e->necessidade = necessidade;
    e->disponivel = disponivel;

    for (int i = 0; i < n_clientes; i++) {
        for (int j = 0; j < n_recursos; j++) {
            e->alocacao[i][j] = 0;
            e->necessidade[i][j] = e->maximo[i][j];
        }
    }
}

static int estado_seguro(estado_t *e) {
    int n = e->n_clientes;
    int m = e->n_recursos;

    int *trabalho = malloc(m * sizeof(int));
    int *terminou = calloc(n, sizeof(int));
    if (trabalho == NULL || terminou == NULL) {
        free(trabalho);
        free(terminou);
        return 0;
    }

    for (int j = 0; j < m; j++) {
        trabalho[j] = e->disponivel[j];
    }

    int achou = 1;
    while (achou) {
        achou = 0;
        for (int i = 0; i < n; i++) {
            if (terminou[i]) continue;
            int pode = 1;
            for (int j = 0; j < m; j++) {
                if (e->necessidade[i][j] > trabalho[j]) {
                    pode = 0;
                    break;
                }
            }
            if (pode) {
                for (int j = 0; j < m; j++) {
                    trabalho[j] += e->alocacao[i][j];
                }
                terminou[i] = 1;
                achou = 1;
            }
        }
    }

    int seguro = 1;
    for (int i = 0; i < n; i++) {
        if (!terminou[i]) {
            seguro = 0;
            break;
        }
    }

    free(trabalho);
    free(terminou);
    return seguro;
}

static void faz_pedido(estado_t *e, int cliente, int *pedido) {
    int m = e->n_recursos;

    for (int j = 0; j < m; j++) {
        if (pedido[j] > e->necessidade[cliente][j]) {
            fprintf(e->saida, "The customer %d request", cliente);
            for (int k = 0; k < m; k++) fprintf(e->saida, " %d", pedido[k]);
            fprintf(e->saida, " was denied because exceed its maximum allocation\n");
            return;
        }
    }

    for (int j = 0; j < m; j++) {
        if (pedido[j] > e->disponivel[j]) {
            fprintf(e->saida, "The resources");
            for (int k = 0; k < m; k++) fprintf(e->saida, " %d", e->disponivel[k]);
            fprintf(e->saida, " are not enough to customer %d request", cliente);
            for (int k = 0; k < m; k++) fprintf(e->saida, " %d", pedido[k]);
            fprintf(e->saida, "\n");
            return;
        }
    }

    for (int j = 0; j < m; j++) {
        e->disponivel[j] -= pedido[j];
        e->alocacao[cliente][j] += pedido[j];
        e->necessidade[cliente][j] -= pedido[j];
    }

    if (estado_seguro(e)) {
        fprintf(e->saida, "Allocate to customer %d the resources", cliente);
        for (int k = 0; k < m; k++) fprintf(e->saida, " %d", pedido[k]);
        fprintf(e->saida, "\n");
    } else {
        for (int j = 0; j < m; j++) {
            e->disponivel[j] += pedido[j];
            e->alocacao[cliente][j] -= pedido[j];
            e->necessidade[cliente][j] += pedido[j];
        }
        fprintf(e->saida, "The customer %d request", cliente);
        for (int k = 0; k < m; k++) fprintf(e->saida, " %d", pedido[k]);
        fprintf(e->saida, " was denied because result in an unsafe state\n");
    }
}

int main(int argc, char **argv) {
    int disponivel[MAX_RECURSOS];
    int n_recursos = 0;

    if (!le_argumentos(argc, argv, disponivel, &n_recursos)) {
        return 1;
    }

    int **maximo = cria_matriz(MAX_CLIENTES, n_recursos);
    int **alocacao = cria_matriz(MAX_CLIENTES, n_recursos);
    int **necessidade = cria_matriz(MAX_CLIENTES, n_recursos);
    if (maximo == NULL || alocacao == NULL || necessidade == NULL) {
        printf("Erro: sem memoria\n");
        libera_matriz(maximo, MAX_CLIENTES);
        libera_matriz(alocacao, MAX_CLIENTES);
        libera_matriz(necessidade, MAX_CLIENTES);
        return 1;
    }

    int n_clientes = 0;
    if (!le_customer("customer.txt", maximo, &n_clientes, n_recursos)) {
        libera_matriz(maximo, MAX_CLIENTES);
        libera_matriz(alocacao, MAX_CLIENTES);
        libera_matriz(necessidade, MAX_CLIENTES);
        return 1;
    }

    comando_t *cmds = malloc(sizeof(comando_t) * MAX_COMANDOS);
    if (cmds == NULL) {
        printf("Erro: sem memoria\n");
        libera_matriz(maximo, MAX_CLIENTES);
        libera_matriz(alocacao, MAX_CLIENTES);
        libera_matriz(necessidade, MAX_CLIENTES);
        return 1;
    }

    int n_cmds = 0;
    if (!le_commands("commands.txt", cmds, &n_cmds, n_clientes, n_recursos)) {
        free(cmds);
        libera_matriz(maximo, MAX_CLIENTES);
        libera_matriz(alocacao, MAX_CLIENTES);
        libera_matriz(necessidade, MAX_CLIENTES);
        return 1;
    }

    estado_t estado;
    prepara_estado(&estado, maximo, alocacao, necessidade, disponivel, n_clientes, n_recursos);

    free(cmds);
    libera_matriz(maximo, MAX_CLIENTES);
    libera_matriz(alocacao, MAX_CLIENTES);
    libera_matriz(necessidade, MAX_CLIENTES);
    return 0;
}