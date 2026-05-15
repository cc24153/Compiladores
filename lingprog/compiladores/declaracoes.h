#ifndef DECLARACOES_H
#define DECLARACOES_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_COMPRIMENTO 256
#define NUM_PALAVRAS 36


// cada palavra reservada vira um numero inteiro, o que facilita a comparação depois
typedef enum
{
    programa, rotulo, tipo, variavel, procedimento,
    funcao, inicio, fim, atribuicao, se, entao, senao,
    enquanto, faca, vapara, mais, menos, vezes, dividir,
    igual, diferente, menor, menorouigual, maior, maiorouigual,
    e, ou, nao, abreparenteses, fechaparenteses, fechacolchetes,
    abrecolchetes, virgula, pontoevirgula, doispontos, ponto, numero,
    identificador, fimdearquivo
} token;

// funcao reutilizavel
typedef struct
{
    FILE *arquivo;
    char linha[MAX_COMPRIMENTO];        // linha atual lida
    int pos;                            // posição prox char na linha
    char buf[MAX_COMPRIMENTO];          // buffer onde o token separado vai ser escrito
    char palavraAtual[MAX_COMPRIMENTO]; // palavra atual, usada para passar a string do token para o analisador sintático
} Analisador;


// funcoes do compilador lexico
char *separaToken(Analisador *analisa);
token proximoToken(Analisador *analisa);
token analex(char *palavra);
int qualNumero(char *str);
char *qualId(char *id);


#endif
