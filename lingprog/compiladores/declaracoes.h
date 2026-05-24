#ifndef DECLARACOES_H   // faz com que o conteúdo do arquivo seja incluído apenas uma vez durante a compilacao
#define DECLARACOES_H   // bandeira/carimbo que define que o arquivo já foi incluído

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// arquivo declaracoes serve para declarar coisas usadas no analisador léxico e sintático,
// para evitar dependências circulares entre os arquivos anasin.c e analisador.c, e para organizar melhor o código

// Mariana Marietti da Costa - 24140
// Rafaelly Maria Nascimento da Silva - 24153
// Samuel Rosa Parra - 24155


#define MAX_COMPRIMENTO 256 // tamanho maximo de uma linha, palavra ou token (número de bits que um token pode ocupar)
#define NUM_PALAVRAS 36 // número de palavras reservadas, usado para comparar o token separado com as palavras reservadas

#define MAX_SIMBOLOS 512        // tamanho maximo da tabela de simbolos, usado para verificar se a tabela de simbolos está cheia
#define MAX_LISTA_IDS 128       // tamanho maximo de identificadores em uma mesma declaracao, usado para verificar se tem muitos identificadores na mesma declaracao


// enum de tokens, cada token tem um numero inteiro correspondente, usado para comparar o token separado com as palavras reservadas e simbolos
// programa tem o token 0
typedef enum {
    programa, rotulo, tipo, variavel, procedimento, funcao,
    inicio, fim, atribuicao, se, entao, senao, enquanto, faca,
    vapara, mais, menos, vezes, dividir, igual, diferente, menor,
    menorouigual, maior, maiorouigual, e, ou, nao, abreparenteses, 
    fechaparenteses, fechacolchetes, abrecolchetes, virgula, pontoevirgula,
    doispontos, ponto, numero, identificador, comentario, fimdearquivo,
    token_invalido
} token;


// enum de classes de simbolos para a tabela de simbolos, usado para verificar se um identificador é uma variavel, tipo, procedimento, etc
typedef enum {
    SIM_PROGRAMA,
    SIM_ENTRADA_SAIDA,
    SIM_TIPO,
    SIM_VARIAVEL,
    SIM_PROCEDIMENTO,
    SIM_FUNCAO,
    SIM_PARAMETRO
} ClasseSimbolo;

// struct de simbolo para a tabela de simbolos, cada simbolo tem um nome, tipo, classe, linha e coluna onde foi declarado, escopo onde foi declarado 
// e se está ativo ou não (ou seja, se ainda pode ser usado ou já saiu do escopo)
typedef struct {
    char nome[MAX_COMPRIMENTO]; // nome do simbolo, usado para comparar com os identificadores no código
    char tipo[MAX_COMPRIMENTO]; // tipo do simbolo, usado para verificar se o tipo de uma variavel é conhecido, ou o tipo de retorno de uma função é conhecido, etc
    ClasseSimbolo classe;       // classe do simbolo, usado para verificar se um identificador é uma variavel, tipo, procedimento, etc
    int linha;                  // linha onde o simbolo foi declarado, usado para imprimir na tabela de simbolos
    int coluna;                 // coluna onde o simbolo foi declarado, usado para imprimir na tabela de simbolos
    int escopo;                 // escopo onde o simbolo foi declarado, usado para verificar se um identificador está no escopo correto, escopo é um numero inteiro que começa em 0 para o escopo global, e é incrementado para cada novo escopo (procedimento ou função) e decrementado ao sair do escopo
    int ativo;                  // indica se o simbolo está ativo (ainda pode ser usado) ou não (já saiu do escopo)
} Simbolo;


// struct de analisador, usado para ler o arquivo de entrada, armazenar a linha atual, posição atual, buffer do token separado, 
// palavra atual (string do token separado), tabela de simbolos, total de simbolos e escopo atual
typedef struct {
    FILE *arquivo;                  // o arquivo aberto, usado para ler o código fonte
    char linha[MAX_COMPRIMENTO];    // a linha que está sendo lida agora, usada para separar os tokens e para imprimir na tabela de simbolos
    int pos;                        // qual caractere da linha estou olhando agora, indice usado para separar os tokens e para imprimir na tabela de simbolos
    int numeroLinha;                // numero da linha atual, usado para imprimir na tabela de simbolos
    int colunaToken;                // coluna onde o token separado começa, usado para imprimir na tabela de simbolos
    char buf[MAX_COMPRIMENTO];      // onde vou montar o token separado, usado para separar os tokens e para imprimir na tabela de simbolos
    char palavraAtual[MAX_COMPRIMENTO]; // guarda a string do token separado, usada para passar a string do token para o analisador sintático e para imprimir na tabela de simbolos
    Simbolo simbolos[MAX_SIMBOLOS];     // tabela de simbolos, usada para armazenar os simbolos declarados no programa e para verificar se um identificador é declarado, se o tipo de uma variavel é conhecido, etc
    int totalSimbolos;              // total de simbolos na tabela de simbolos, usado para verificar se a tabela de simbolos está cheia e para imprimir a tabela de simbolos
    int escopoAtual;                // escopo atual, usado para verificar se um identificador está no escopo correto e para imprimir na tabela de simbolos
} Analisador;


// como se fosse uma interface: sao assinaturas de funcoes usadas tanto no analisador lexico quanto no sintatico, 
// para evitar dependencias circulares entre os arquivos anasin.c e analisador.c, e para organizar melhor o codigo

// função de erro, usada para imprimir mensagens de erro sintático com a linha, coluna e token atual, e para encerrar o programa
void erro(Analisador *an, const char *mensagem);

//funciona como uma interface, o analisador lexico chama as funcoes abaixo para separar o token, comparar com as palavras reservadas e passar a string do token para o analisador sintático
char *separaToken(Analisador *analisa);
token proximoToken(Analisador *analisa);

// funces do compilador sintático 
void compila_programa(Analisador *an);
void compila_bloco(Analisador *an);
void compila_parametros_formais(Analisador *an);
void compila_comando(Analisador *an, token *t);
void compila_comando_sem_rotulo(Analisador *an, token *t);
void compila_expressao(Analisador *an, token *t);
void compila_expressao_simples(Analisador *an, token *t);
void compila_termo(Analisador *an, token *t);
void compila_fator(Analisador *an, token *t);


#endif // fim do arquivo declaracoes.h
