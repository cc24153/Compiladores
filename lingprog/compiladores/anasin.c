#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "declaracoes.h"

// Mariana Marietti da Costa - 24140
// Rafaelly Maria Nascimento da Silva - 24153
// Samuel Rosa Parra - 24155


void erro(Analisador *an, const char *mensagem) {
    fprintf(stderr, "Erro sintatico na linha %d, coluna %d: %s. Token atual: \"%s\"\n",
            an->numeroLinha, an->colunaToken, mensagem, an->palavraAtual);  // imprime a mensagem de erro com a linha, coluna e token atual para ajudar a localizar o erro no código fonte
    exit(1);
}
#define ERRO(msg) erro(an, msg)


// funções auxiliares para o analisador sintático, usadas para manipular a tabela de símbolos, verificar tipos, etc:

// nomeClasse: recebe uma classe de simbolo e retorna uma string com o nome da classe para imprimir na tabela de simbolos
const char *nomeClasse(ClasseSimbolo classe){
    switch (classe)
    {
    case SIM_PROGRAMA:
        return "programa";
    case SIM_ENTRADA_SAIDA:
        return "entrada/saida";
    case SIM_TIPO:
        return "tipo";
    case SIM_VARIAVEL:
        return "variavel";
    case SIM_PROCEDIMENTO:
        return "procedimento";
    case SIM_FUNCAO:
        return "funcao";
    case SIM_PARAMETRO:
        return "parametro";
    default:
        return "desconhecido";
    }
}

// buscaSimbolo: recebe um nome e retorna o índice do símbolo na tabela de símbolos, ou -1 se não encontrar
int buscaSimbolo(Analisador *an, const char *nome) {
    for (int i = an->totalSimbolos - 1; i >= 0; i--)    // procura de trás para frente para encontrar o símbolo mais próximo (mais recente) com o nome dado, e que esteja ativo (ainda pode ser usado)
        if (an->simbolos[i].ativo && strcmp(an->simbolos[i].nome, nome) == 0)   // verifica se o símbolo está ativo e se o nome do símbolo é igual ao nome dado, usando strcmp para comparar strings
            return i;
    return -1;
}

// buscaSimboloNoEscopoAtual: recebe um nome e retorna o índice do símbolo na tabela de símbolos que está no escopo atual, ou -1 se não encontrar
  int buscaSimboloNoEscopoAtual(Analisador *an, const char *nome) {
    for (int i = an->totalSimbolos - 1; i >= 0; i--)    // procura de trás para frente para encontrar o símbolo mais próximo (mais recente) com o nome dado, que esteja ativo (ainda pode ser usado) e que esteja no escopo atual
        if (an->simbolos[i].ativo &&
            an->simbolos[i].escopo == an->escopoAtual &&
            strcmp(an->simbolos[i].nome, nome) == 0)
            return i;
    return -1;
}

// funções para manipular o escopo, usadas para entrar e sair de escopos em procedimentos e funções, e para verificar se um tipo é conhecido ou não
  void entraEscopo(Analisador *an) {
    an->escopoAtual++;  // incrementa o escopo atual para entrar em um novo escopo, usado para procedimentos e funções, e para verificar se um identificador está no escopo correto
}
  void saiEscopo(Analisador *an) {
    for (int i = 0; i < an->totalSimbolos; i++) // desativa todos os símbolos que estão no escopo atual, ou seja, que não podem mais ser usados, para garantir que identificadores declarados em escopos anteriores não sejam usados fora do escopo correto
        if (an->simbolos[i].escopo == an->escopoAtual)  // verifica se o símbolo está no escopo atual, ou seja, se foi declarado no escopo que está saindo
            an->simbolos[i].ativo = 0;      // desativa o símbolo para indicar que não pode mais ser usado, ou seja, que saiu do escopo

    if (an->escopoAtual > 0)    // decrementa o escopo atual para sair do escopo, garantindo que o escopo não fique negativo, e que o escopo 0 seja o escopo global onde estão os símbolos pré-definidos e os símbolos declarados no programa principal
        an->escopoAtual--;
}

// ehTipoConhecido: recebe um nome de tipo e verifica se é um tipo pré-definido ou um tipo declarado na tabela de símbolos, retornando 1 se for conhecido e 0 se não for
  int ehTipoConhecido(Analisador *an, const char *nome) {
    if (strcmp(nome, "integer") == 0 ||
        strcmp(nome, "real") == 0 ||
        strcmp(nome, "boolean") == 0 ||
        strcmp(nome, "char") == 0)
    {
        return 1;
    }

    int indice = buscaSimbolo(an, nome);
    return indice >= 0 && an->simbolos[indice].classe == SIM_TIPO; // verifica se o símbolo encontrado é um tipo, ou seja, se a classe do símbolo é SIM_TIPO, para garantir que o nome dado seja um tipo conhecido e não um identificador de variável, procedimento, etc
}

// adicionaSimbolo: recebe um nome, tipo e classe, e adiciona um símbolo na tabela de símbolos, verificando se o nome já foi declarado no escopo atual e se a tabela de símbolos não está cheia
  void adicionaSimbolo(Analisador *an, const char *nome, const char *tipoNome, ClasseSimbolo classe){
    if (buscaSimboloNoEscopoAtual(an, nome) >= 0)
    {
        char msg[MAX_COMPRIMENTO * 2];  // buffer para montar a mensagem de erro,
        snprintf(msg, sizeof(msg), "identificador '%s' ja declarado", nome);    // monta a mensagem de erro indicando que o identificador já foi declarado, usando snprintf para formatar a string com o nome do identificador
        erro(an, msg);  // imprime a mensagem de erro e encerra o programa,
    }

    // verifica se a tabela de símbolos está cheia antes de adicionar um novo símbolo, para evitar overflow e garantir que o programa não tente acessar índices inválidos na tabela de símbolos
    if (an->totalSimbolos >= MAX_SIMBOLOS) {
        erro(an, "tabela de simbolos cheia");
    }

    Simbolo *simbolo = &an->simbolos[an->totalSimbolos++];      // adiciona um novo símbolo na tabela de símbolos, usando o totalSimbolos como índice para o próximo símbolo a ser adicionado, e depois incrementa o totalSimbolos para indicar que um novo símbolo foi adicionado
    strncpy(simbolo->nome, nome, sizeof(simbolo->nome) - 1);    // copia o nome do símbolo para a struct do símbolo, limitando o tamanho para evitar overflow, e garantindo que seja uma string válida
    simbolo->nome[sizeof(simbolo->nome) - 1] = '\0';            // finaliza a string do nome do símbolo com o caractere nulo para que seja uma string válida, e para imprimir na tabela de símbolos e mensagens de erro
    strncpy(simbolo->tipo, tipoNome, sizeof(simbolo->tipo) - 1);    // copia o tipo do símbolo para a struct do símbolo, limitando o tamanho para evitar overflow, e garantindo que seja uma string válida
    simbolo->tipo[sizeof(simbolo->tipo) - 1] = '\0';            // finaliza a string do tipo do símbolo com o caractere nulo para que seja uma string válida, e para imprimir na tabela de símbolos e mensagens de erro
    simbolo->classe = classe;                                   // atribui a classe do símbolo para a struct do símbolo, usada para verificar se um identificador é uma variável, tipo, procedimento, etc, e para imprimir na tabela de símbolos
    simbolo->linha = an->numeroLinha;                           // armazena a linha onde o símbolo foi declarado, usando o número da linha atual do analisador léxico, para imprimir na tabela de símbolos e mensagens de erro
    simbolo->coluna = an->colunaToken;                          // armazena a coluna onde o símbolo foi declarado, usando a coluna do token atual do analisador léxico, para imprimir na tabela de símbolos e mensagens de erro
    simbolo->escopo = an->escopoAtual;                          // armazena o escopo onde o símbolo foi declarado, usando o escopo atual do analisador sintático, para verificar se um identificador está no escopo correto e para imprimir na tabela de símbolos
    simbolo->ativo = 1;                                         // marca o símbolo como ativo, ou seja, que ainda pode ser usado, para garantir que identificadores declarados em escopos anteriores não sejam usados fora do escopo correto
}

// adicionaPreDefinido: recebe um nome e uma classe, e adiciona um símbolo pré-definido na tabela de símbolos, usado para adicionar os tipos pré-definidos e os procedimentos de entrada/saída
  void adicionaPreDefinido(Analisador *an, const char *nome, ClasseSimbolo classe) {
    if (an->totalSimbolos >= MAX_SIMBOLOS)  // verifica se a tabela de símbolos está cheia antes de adicionar um novo símbolo pré-definido, para evitar overflow e garantir que o programa não tente acessar índices inválidos na tabela de símbolos
        erro(an, "tabela de simbolos cheia");

    Simbolo *simbolo = &an->simbolos[an->totalSimbolos++];  // adiciona um novo símbolo pré-definido na tabela de símbolos, usando o totalSimbolos como índice para o próximo símbolo a ser adicionado, e depois incrementa o totalSimbolos para indicar que um novo símbolo foi adicionado
    strncpy(simbolo->nome, nome, sizeof(simbolo->nome) - 1);    // copia o nome do símbolo pré-definido para a struct do símbolo, limitando o tamanho para evitar overflow, e garantindo que seja uma string válida
    simbolo->nome[sizeof(simbolo->nome) - 1] = '\0';        // finaliza a string do nome do símbolo pré-definido com o caractere nulo para que seja uma string válida, e para imprimir na tabela de símbolos e mensagens de erro
    strcpy(simbolo->tipo, "-");                             // os símbolos pré-definidos não têm um tipo específico, então atribui "-" para indicar que o tipo é desconhecido ou não aplicável, usado para imprimir na tabela de símbolos e mensagens de erro
    simbolo->classe = classe;                               // atribui a classe do símbolo pré-definido para a struct do símbolo, usada para verificar se um identificador é uma variável, tipo, procedimento, etc, e para imprimir na tabela de símbolos
    simbolo->linha = 0;                                     // os símbolos pré-definidos não têm uma linha específica onde foram declarados, então atribui 0 para indicar que a linha é desconhecida ou não aplicável, usado para imprimir na tabela de símbolos e mensagens de erro
    simbolo->coluna = 0;                                    // os símbolos pré-definidos não têm uma coluna específica onde foram declarados, então atribui 0 para indicar que a coluna é desconhecida ou não aplicável, usado para imprimir na tabela de símbolos e mensagens de erro
    simbolo->escopo = 0;                                    // os símbolos pré-definidos estão no escopo global, que é o escopo 0, usado para verificar se um identificador está no escopo correto e para imprimir na tabela de símbolos
    simbolo->ativo = 1;                                     // marca o símbolo pré-definido como ativo, ou seja, que ainda pode ser usado, para garantir que os símbolos pré-definidos possam ser usados em qualquer parte do programa  
}

// exigeDeclarado: recebe um nome e verifica se está declarado na tabela de símbolos, se não estiver, imprime uma mensagem de erro e encerra o programa
  void exigeDeclarado(Analisador *an, const char *nome){
    if (buscaSimbolo(an, nome) < 0) // se a função buscaSimbolo retornar um índice menor que 0, 
    // significa que o nome dado não foi encontrado na tabela de símbolos, 
    {
        // monta uma mensagem de erro indicando que o identificador não foi declarado,
        char msg[MAX_COMPRIMENTO * 2];
        snprintf(msg, sizeof(msg), "identificador '%s' nao declarado", nome);
        erro(an, msg);
    }
}


// imprimeTabelaSimbolos: imprime a tabela de símbolos no final da análise sintática, mostrando o nome, classe, tipo, linha e escopo de cada símbolo declarado no programa
  void imprimeTabelaSimbolos(Analisador *an) {
    printf("\nTabela de simbolos:\n");
    printf("%-20s %-15s %-15s %-8s %-8s\n", "Nome", "Classe", "Tipo", "Linha", "Escopo");
    printf("---------------------------------------------------------------------\n");

    for (int i = 0; i < an->totalSimbolos; i++)
    {
        printf("%-20s %-15s %-15s %-8d %-8d\n",
               an->simbolos[i].nome,
               nomeClasse(an->simbolos[i].classe),
               an->simbolos[i].tipo[0] ? an->simbolos[i].tipo : "-",
               an->simbolos[i].linha,
               an->simbolos[i].escopo);
    }
}


// funções de análise sintática, cada função corresponde a uma regra da gramática, e chama as funções correspondentes às regras que aparecem na regra dela


  void compila_programa(Analisador *an) {
    // verifica se o primeiro token é 'program', se não for, imprime uma mensagem de erro indicando que esperava 'program' no início do programa, e encerra o programa
    token t = proximoToken(an);
    if (t != programa){
        ERRO("esperava 'program' no inicio do programa");
    }

    // verifica se o próximo token é um identificador, que é o nome do programa, se não for, imprime uma mensagem de erro indicando que esperava um identificador após 'program', e encerra o programa
    t = proximoToken(an);
    if (t != identificador){
        ERRO("esperava identificador apos 'program'");
    }

    adicionaSimbolo(an, an->palavraAtual, "-", SIM_PROGRAMA);   // adiciona o nome do programa como um símbolo na tabela de símbolos, com tipo "-" para indicar que o tipo é desconhecido ou não aplicável, e classe SIM_PROGRAMA para indicar que é um programa

    // verifica se o próximo token é um símbolo de abertura de parênteses, indicando que há uma lista de arquivos de entrada/saída, se for, chama a função para compilar a lista de arquivos, se não for, continua para verificar o próximo token
    t = proximoToken(an);
    if (t == abreparenteses) {
        do {
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador na lista de arquivos");
            adicionaSimbolo(an, an->palavraAtual, "-", SIM_ENTRADA_SAIDA);

            t = proximoToken(an);
            if (t != virgula && t != fechaparenteses)
                ERRO("esperava ',' ou ')' na lista de arquivos");
        } while (t == virgula);

        t = proximoToken(an);
    }

    // verifica se o próximo token é um ponto e vírgula, indicando o final do cabeçalho do programa, se não for, imprime uma mensagem de erro indicando que esperava ';' após o cabeçalho do programa, e encerra o programa
    if (t != pontoevirgula){
        ERRO("esperava ';' apos cabecalho do programa");
    }
    
    compila_bloco(an);  // chama a função para compilar o bloco do programa, que é a parte principal do programa onde estão as declarações e os comandos

    // verifica se o próximo token é um ponto, indicando o final do programa, se não for, imprime uma mensagem de erro indicando que esperava '.' ao final do programa, e encerra o programa
    t = proximoToken(an);
    if (t != ponto){
        ERRO("esperava '.' ao final do programa");
    }

    // verifica se o próximo token é o fim de arquivo, indicando que não há mais conteúdo após o ponto final do programa, se não for, imprime uma mensagem de erro indicando que esperava fim de arquivo após '.', e encerra o programa
    t = proximoToken(an);
    if (t != fimdearquivo){
        ERRO("conteudo inesperado apos '.'");
    }

    printf("Programa sintaticamente correto!\n");
}

// preguica de comentar todos esses compila:

  void compila_bloco(Analisador *an) {
    token t = proximoToken(an);

    if (t == rotulo)
    {
        t = proximoToken(an);
        if (t != numero)
            ERRO("esperava numero apos 'label'");

        t = proximoToken(an);
        while (t == virgula)
        {
            t = proximoToken(an);
            if (t != numero)
                ERRO("esperava numero apos ',' em 'label'");
            t = proximoToken(an);
        }

        if (t != pontoevirgula)
            ERRO("esperava ';' ao final da secao 'label'");

        t = proximoToken(an);
    }

    if (t == tipo)
    {
        t = proximoToken(an);
        while (t == identificador)
        {
            char nomeTipo[MAX_COMPRIMENTO];
            strcpy(nomeTipo, an->palavraAtual);

            t = proximoToken(an);
            if (t != igual)
                ERRO("esperava '=' apos identificador em 'type'");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava nome de tipo apos '=' em 'type'");
            if (!ehTipoConhecido(an, an->palavraAtual))
                ERRO("tipo base nao declarado em 'type'");
            adicionaSimbolo(an, nomeTipo, an->palavraAtual, SIM_TIPO);

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' ao final da definicao de tipo");

            t = proximoToken(an);
        }
    }

    if (t == variavel)
    {
        t = proximoToken(an);
        while (t == identificador)
        {
            char ids[MAX_LISTA_IDS][MAX_COMPRIMENTO];
            int totalIds = 0;

            while (1)
            {
                if (totalIds >= MAX_LISTA_IDS)
                    ERRO("muitos identificadores na mesma declaracao");
                strcpy(ids[totalIds++], an->palavraAtual);

                t = proximoToken(an);
                if (t != virgula)
                    break;

                t = proximoToken(an);
                if (t != identificador)
                    ERRO("esperava identificador apos ',' em 'var'");
            }

            if (t != doispontos)
                ERRO("esperava ':' apos lista de variaveis");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava nome de tipo apos ':' em 'var'");
            if (!ehTipoConhecido(an, an->palavraAtual))
                ERRO("tipo nao declarado em 'var'");

            char tipoVar[MAX_COMPRIMENTO];
            strcpy(tipoVar, an->palavraAtual);
            for (int i = 0; i < totalIds; i++)
                adicionaSimbolo(an, ids[i], tipoVar, SIM_VARIAVEL);

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' ao final da declaracao de variavel");

            t = proximoToken(an);
        }
    }

    while (t == procedimento || t == funcao)
    {
        if (t == procedimento)
        {
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'procedure'");
            adicionaSimbolo(an, an->palavraAtual, "-", SIM_PROCEDIMENTO);
            entraEscopo(an);

            t = proximoToken(an);
            if (t == abreparenteses)
            {
                compila_parametros_formais(an);
                t = proximoToken(an);
            }

            if (t != pontoevirgula)
                ERRO("esperava ';' apos cabecalho de 'procedure'");

            compila_bloco(an);
            saiEscopo(an);

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos bloco de 'procedure'");

            t = proximoToken(an);
        }
        else
        {
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'function'");
            char nomeFuncao[MAX_COMPRIMENTO];
            strcpy(nomeFuncao, an->palavraAtual);
            adicionaSimbolo(an, nomeFuncao, "-", SIM_FUNCAO);
            int indiceFuncao = buscaSimbolo(an, nomeFuncao);
            entraEscopo(an);

            t = proximoToken(an);
            if (t == abreparenteses)
            {
                compila_parametros_formais(an);
                t = proximoToken(an);
            }

            if (t != doispontos)
                ERRO("esperava ':' apos parametros de 'function'");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava tipo de retorno apos ':' em 'function'");
            if (!ehTipoConhecido(an, an->palavraAtual))
                ERRO("tipo de retorno nao declarado em 'function'");
            strncpy(an->simbolos[indiceFuncao].tipo, an->palavraAtual, sizeof(an->simbolos[indiceFuncao].tipo) - 1);
            an->simbolos[indiceFuncao].tipo[sizeof(an->simbolos[indiceFuncao].tipo) - 1] = '\0';

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos cabecalho de 'function'");

            compila_bloco(an);
            saiEscopo(an);

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos bloco de 'function'");

            t = proximoToken(an);
        }
    }

    if (t != inicio)
        ERRO("esperava 'begin'");

    t = proximoToken(an);
    while (t != fim)
    {
        if (t == fimdearquivo)
            ERRO("fim de arquivo inesperado dentro de 'begin'");

        compila_comando(an, &t);

        if (t == pontoevirgula)
            t = proximoToken(an);
        else if (t != fim)
            ERRO("esperava ';' ou 'end' apos comando");
    }
}

  void registraListaParametros(Analisador *an, char ids[][MAX_COMPRIMENTO], int totalIds, const char *tipoParam) {
    if (!ehTipoConhecido(an, tipoParam))
        erro(an, "tipo nao declarado em parametros formais");

    for (int i = 0; i < totalIds; i++)
        adicionaSimbolo(an, ids[i], tipoParam, SIM_PARAMETRO);
}

  void compila_parametros_formais(Analisador *an) {
    token t;

    do
    {
        t = proximoToken(an);

        if (t == variavel)
            t = proximoToken(an);

        if (t == identificador)
        {
            char ids[MAX_LISTA_IDS][MAX_COMPRIMENTO];
            int totalIds = 0;

            while (1)
            {
                if (totalIds >= MAX_LISTA_IDS)
                    ERRO("muitos parametros na mesma declaracao");
                strcpy(ids[totalIds++], an->palavraAtual);

                t = proximoToken(an);
                if (t != virgula)
                    break;

                t = proximoToken(an);
                if (t != identificador)
                    ERRO("esperava identificador apos ',' em parametros");
            }

            if (t != doispontos)
                ERRO("esperava ':' apos lista de parametros");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava tipo em parametros formais");
            registraListaParametros(an, ids, totalIds, an->palavraAtual);

            t = proximoToken(an);
        }
        else if (t == funcao)
        {
            char ids[MAX_LISTA_IDS][MAX_COMPRIMENTO];
            int totalIds = 0;

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'function' em parametros formais");

            while (1)
            {
                if (totalIds >= MAX_LISTA_IDS)
                    ERRO("muitos parametros function na mesma declaracao");
                strcpy(ids[totalIds++], an->palavraAtual);

                t = proximoToken(an);
                if (t != virgula)
                    break;

                t = proximoToken(an);
                if (t != identificador)
                    ERRO("esperava identificador apos ',' em parametro function");
            }

            if (t != doispontos)
                ERRO("esperava ':' em parametro function");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava tipo de retorno em parametro function");
            if (!ehTipoConhecido(an, an->palavraAtual))
                ERRO("tipo de retorno nao declarado em parametro function");

            for (int i = 0; i < totalIds; i++)
                adicionaSimbolo(an, ids[i], an->palavraAtual, SIM_FUNCAO);

            t = proximoToken(an);
        }
        else if (t == procedimento)
        {
            char ids[MAX_LISTA_IDS][MAX_COMPRIMENTO];
            int totalIds = 0;

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'procedure' em parametros formais");

            while (1)
            {
                if (totalIds >= MAX_LISTA_IDS)
                    ERRO("muitos parametros procedure na mesma declaracao");
                strcpy(ids[totalIds++], an->palavraAtual);

                t = proximoToken(an);
                if (t != virgula)
                    break;

                t = proximoToken(an);
                if (t != identificador)
                    ERRO("esperava identificador apos ',' em parametro procedure");
            }

            for (int i = 0; i < totalIds; i++)
                adicionaSimbolo(an, ids[i], "-", SIM_PROCEDIMENTO);
        }
        else
        {
            ERRO("parametro formal invalido");
        }
    } while (t == pontoevirgula);

    if (t != fechaparenteses)
        ERRO("esperava ')' ao final dos parametros formais");
}
  void compila_comando(Analisador *an, token *t) {
    while (*t == numero)
    {
        *t = proximoToken(an);
        if (*t != doispontos)
            ERRO("esperava ':' apos rotulo numerico");
        *t = proximoToken(an);
    }

    compila_comando_sem_rotulo(an, t);
}
  void compila_comando_sem_rotulo(Analisador *an, token *t) {
    if (*t == identificador)
    {
        char nome[MAX_COMPRIMENTO];
        strcpy(nome, an->palavraAtual);
        exigeDeclarado(an, nome);

        *t = proximoToken(an);

        if (*t == abrecolchetes)
        {
            *t = proximoToken(an);
            compila_expressao(an, t);
            while (*t == virgula)
            {
                *t = proximoToken(an);
                compila_expressao(an, t);
            }
            if (*t != fechacolchetes)
                ERRO("esperava ']' apos indice de array");
            *t = proximoToken(an);
        }

        if (*t == atribuicao)
        {
            *t = proximoToken(an);
            compila_expressao(an, t);
        }
        else if (*t == abreparenteses)
        {
            *t = proximoToken(an);
            if (*t != fechaparenteses)
            {
                compila_expressao(an, t);
                while (*t == virgula)
                {
                    *t = proximoToken(an);
                    compila_expressao(an, t);
                }
            }

            if (*t != fechaparenteses)
                ERRO("esperava ')' ao final da chamada de procedimento");
            *t = proximoToken(an);
        }
        else
        {
            /* chamada de procedure sem parametros, como: r */
        }
    }
    else if (*t == vapara)
    {
        *t = proximoToken(an);
        if (*t != numero)
            ERRO("esperava numero apos 'goto'");
        *t = proximoToken(an);
    }
    else if (*t == inicio)
    {
        *t = proximoToken(an);
        while (*t != fim)
        {
            if (*t == fimdearquivo)
                ERRO("fim de arquivo inesperado em bloco composto");
            compila_comando(an, t);
            if (*t == pontoevirgula)
                *t = proximoToken(an);
            else if (*t != fim)
                ERRO("esperava ';' ou 'end' em bloco composto");
        }
        *t = proximoToken(an);
    }
    else if (*t == se)
    {
        *t = proximoToken(an);
        compila_expressao(an, t);

        if (*t != entao)
            ERRO("esperava 'then' apos condicao do 'if'");

        *t = proximoToken(an);
        compila_comando_sem_rotulo(an, t);

        if (*t == senao)
        {
            *t = proximoToken(an);
            compila_comando_sem_rotulo(an, t);
        }
    }
    else if (*t == enquanto)
    {
        *t = proximoToken(an);
        compila_expressao(an, t);

        if (*t != faca)
            ERRO("esperava 'do' apos condicao do 'while'");

        *t = proximoToken(an);
        compila_comando_sem_rotulo(an, t);
    }
}
  void compila_expressao(Analisador *an, token *t) {
    compila_expressao_simples(an, t);

    if (*t == igual || *t == diferente ||
        *t == menor || *t == menorouigual ||
        *t == maior || *t == maiorouigual)
    {
        *t = proximoToken(an);
        compila_expressao_simples(an, t);
    }
}
  void compila_expressao_simples(Analisador *an, token *t) {
    if (*t == mais || *t == menos)
        *t = proximoToken(an);

    compila_termo(an, t);

    while (*t == mais || *t == menos || *t == ou)
    {
        *t = proximoToken(an);
        compila_termo(an, t);
    }
}
  void compila_termo(Analisador *an, token *t) {
    compila_fator(an, t);

    while (*t == vezes || *t == dividir || *t == e)
    {
        *t = proximoToken(an);
        compila_fator(an, t);
    }
}
  void compila_fator(Analisador *an, token *t) {
    if (*t == identificador)
    {
        char nome[MAX_COMPRIMENTO];
        strcpy(nome, an->palavraAtual);
        exigeDeclarado(an, nome);

        *t = proximoToken(an);

        if (*t == abrecolchetes)
        {
            *t = proximoToken(an);
            compila_expressao(an, t);
            while (*t == virgula)
            {
                *t = proximoToken(an);
                compila_expressao(an, t);
            }
            if (*t != fechacolchetes)
                ERRO("esperava ']' apos indice em fator");
            *t = proximoToken(an);
        }
        else if (*t == abreparenteses)
        {
            *t = proximoToken(an);
            if (*t != fechaparenteses)
            {
                compila_expressao(an, t);
                while (*t == virgula)
                {
                    *t = proximoToken(an);
                    compila_expressao(an, t);
                }
            }

            if (*t != fechaparenteses)
                ERRO("esperava ')' ao final de chamada de funcao em fator");
            *t = proximoToken(an);
        }
    }
    else if (*t == abreparenteses)
    {
        *t = proximoToken(an);
        compila_expressao(an, t);

        if (*t != fechaparenteses)
            ERRO("esperava ')' apos expressao entre parenteses");
        *t = proximoToken(an);
    }
    else if (*t == nao)
    {
        *t = proximoToken(an);
        compila_fator(an, t);
    }
    else if (*t == numero)
    {
        *t = proximoToken(an);
    }
    else
    {
        ERRO("fator invalido: esperava id, numero, '(' ou 'not'");
    }
}


// função principal, que inicializa o analisador, adiciona os procedimentos predefinidos "read" e "write" na tabela de simbolos, 
// abre o arquivo de entrada, chama a função de compilar o programa, imprime a tabela de simbolos e fecha o arquivo
int main(int argc, char **argv) {
    const char *nomeArquivo = argc > 1 ? argv[1] : "exemplo.txt";   // permite passar o nome do arquivo como argumento, ou usar "exemplo.txt" por padrão
    Analisador an;
    memset(&an, 0, sizeof(an));                                     // inicializa o analisador com zeros, para garantir que os campos estejam limpos
    adicionaPreDefinido(&an, "read", SIM_PROCEDIMENTO);             // adiciona os procedimentos predefinidos "read" e "write" na tabela de simbolos, para que possam ser usados no programa sem precisar declarar
    adicionaPreDefinido(&an, "write", SIM_PROCEDIMENTO);

    an.arquivo = fopen(nomeArquivo, "r");   // tenta abrir o arquivo de entrada para leitura, e verifica se conseguiu abrir, caso contrário imprime uma mensagem de erro e encerra o programa
    if (!an.arquivo)
    {
        fprintf(stderr, "Erro ao abrir arquivo '%s'\n", nomeArquivo);
        return 1;
    }

    compila_programa(&an);  // chama a função de compilar o programa, que é a função principal do analisador sintático
    imprimeTabelaSimbolos(&an);

    fclose(an.arquivo);
    return 0;
}
