#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "declaracoes.h"

// VIDEO - PARTE 1 (MARIANA) - Escrita no arquivo MEPA
static void gera(Analisador *an, const char *fmt, ...) {
    if (!an->saida) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(an->saida, fmt, ap);
    va_end(ap);
}

static int novoRotulo(Analisador *an) { return ++an->proximoRotulo; }
static int nivelAcesso(Analisador *an, Simbolo *s) { (void)an; return s->escopo; }
int buscaSimbolo(Analisador *an, const char *nome);

static void tipoBase(Analisador *an, const char *tipo, char *saida) {
    strncpy(saida, tipo, MAX_COMPRIMENTO - 1); saida[MAX_COMPRIMENTO - 1] = '\0';
    for (int passo = 0; passo < 16; ++passo) {
        int i = buscaSimbolo(an, saida);
        if (i < 0 || an->simbolos[i].classe != SIM_TIPO) break;
        strncpy(saida, an->simbolos[i].tipo, MAX_COMPRIMENTO - 1); saida[MAX_COMPRIMENTO - 1] = '\0';
    }
}
static int mesmoTipo(Analisador *an, const char *a, const char *b) {
    char baseA[MAX_COMPRIMENTO], baseB[MAX_COMPRIMENTO];
    tipoBase(an, a, baseA);
    tipoBase(an, b, baseB);
    return strcmp(baseA, baseB) == 0;
}

static int tipoNumerico(Analisador *an, const char *tipo) {
    char base[MAX_COMPRIMENTO]; tipoBase(an, tipo, base);
    return strcmp(base, "integer") == 0 || strcmp(base, "real") == 0;
}
static int tipoCompativel(Analisador *an, const char *a, const char *b) {
    char baseA[MAX_COMPRIMENTO], baseB[MAX_COMPRIMENTO]; tipoBase(an, a, baseA); tipoBase(an, b, baseB);
    return strcmp(baseA, baseB) == 0 || (tipoNumerico(an, baseA) && tipoNumerico(an, baseB));
}
static void exigeTipo(Analisador *an, int condicao, const char *mensagem) { if (!condicao) erro(an, mensagem); }

static void emiteOperador(Analisador *an, token op) {
    switch (op) {
    case mais: gera(an,"SOMA\n"); break; case menos: gera(an,"SUBT\n"); break;
    case vezes: gera(an,"MULT\n"); break; case dividir: gera(an,"DIVI\n"); break;
    case e: gera(an,"CONJ\n"); break; case ou: gera(an,"DISJ\n"); break;
    case igual: gera(an,"CMIG\n"); break; case diferente: gera(an,"CMDG\n"); break;
    case menor: gera(an,"CMME\n"); break; case menorouigual: gera(an,"CMEG\n"); break;
    case maior: gera(an,"CMMA\n"); break; case maiorouigual: gera(an,"CMAG\n"); break;
    default: break;
    }
}

// Mariana Marietti da Costa - 24140
// Rafaelly Maria Nascimento da Silva - 24153
// Samuel Rosa Parra - 24155


// MARIANA

// analise sintatica é a parte do compilador que verifica se o código fonte está de acordo com a gramática da linguagem, ou seja, 
// e se os símbolos usados estão declarados e no escopo correto, para garantir que o programa seja sintaticamente correto antes de passar para as próximas etapas do compilador
// se a estrutura do código está correta (funcoes de compila), 

// a diferenca p analex é que ele se preocupa apenas em separar os tokens, verificando se os tokens são válidos

// *an sera o estado atual do analisador

void erro(Analisador *an, const char *mensagem) {   // recebe an que é o anasin e a mensagem
    // stderr é o fluxo de saída de erro, usado para imprimir mensagens de erro, para que possam ser redirecionadas para um arquivo ou para a tela, e para diferenciar das mensagens de saída normal do programa
    fprintf(stderr, "Erro sintatico na linha %d, coluna %d: %s. Token atual: \"%s\"\n",
        an->numeroLinha, an->colunaToken, mensagem, an->palavraAtual);  // imprime a mensagem de erro com a linha, coluna e token atual para ajudar a localizar o erro no código fonte
    exit(1);
}
#define ERRO(msg) erro(an, msg) 
// ERRO é um macro, ou seja, é uma substituição de texto, que chama a função erro com o analisador e a mensagem dada, 
// para facilitar a escrita de mensagens de erro em todo o código, sem precisar passar o analisador toda vez
// vai pegar o erro e definir ERRO sendo ele, ou seja, quando escrever ERRO("mensagem"), vai substituir por erro(an, "mensagem"), para facilitar a escrita de mensagens de erro em todo o código, sem precisar passar o analisador toda vez


// funções auxiliares para o analisador sintático, usadas para manipular a tabela de símbolos, verificar tipos, etc:

// funções para manipular o escopo, o escopo é um conceito que representa a região do código onde um identificador é válido, ou seja, onde ele pode ser usado, para garantir que identificadores declarados em escopos anteriores não sejam usados fora do escopo correto, e para garantir que um identificador seja declarado antes de ser usado no mesmo escopo
void entraEscopo(Analisador *an) {
    if (an->escopoAtual >= 62) erro(an, "limite de escopos excedido");
    an->escopoAtual++;
    an->deslocamentoPorEscopo[an->escopoAtual] = 0;  // zera o deslocamento das variaveis no novo escopo
}
void saiEscopo(Analisador *an) {
    for (int i = 0; i < an->totalSimbolos; i++) // desativa todos os símbolos que estão no escopo atual, ou seja, que não podem mais ser usados, para garantir que identificadores declarados em escopos anteriores não sejam usados fora do escopo correto
        if (an->simbolos[i].escopo == an->escopoAtual)  // verifica se o símbolo está no escopo atual, ou seja, se foi declarado no escopo que está saindo
            an->simbolos[i].ativo = 0;      // desativa o símbolo para indicar que não pode mais ser usado, ou seja, que saiu do escopo

    if (an->escopoAtual > 0)    // decrementa o escopo atual para sair do escopo, garantindo que o escopo não fique negativo, e que o escopo 0 seja o escopo global onde estão os símbolos pré-definidos e os símbolos declarados no programa principal
        an->escopoAtual--;
}

// nomeClasse: recebe uma classe de simbolo e retorna uma string com o nome da classe para imprimir na tabela de simbolos
const char *nomeClasse(ClasseSimbolo classe){
    switch (classe) {
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
    case SIM_ROTULO:
        return "rotulo";
    default:
        return "desconhecido";
    }
}

// buscaSimbolo: recebe um nome e retorna o índice do símbolo na tabela de símbolos, ou -1 se não encontrar
// usado para verificar se um identificador já foi declarado em qualquer escopo, para garantir que um identificador seja declarado antes de ser usado, e para obter informações sobre o símbolo, como tipo e classe, para verificar o uso correto do identificador no código fonte
int buscaSimbolo(Analisador *an, const char *nome) {
    for (int i = an->totalSimbolos - 1; i >= 0; i--)    // procura de trás para frente para encontrar o símbolo mais próximo (mais recente) com o nome dado, e que esteja ativo (ainda pode ser usado)
        if (an->simbolos[i].ativo && strcmp(an->simbolos[i].nome, nome) == 0)   // verifica se o símbolo está ativo e se o nome do símbolo é igual ao nome dado, usando strcmp para comparar strings
            return i;
    return -1;
}
// buscaSimboloNoEscopoAtual: recebe um nome e retorna o índice do símbolo na tabela de símbolos que está no escopo atual, ou -1 se não encontrar
// usado para verificar se um identificador já foi declarado no escopo atual, para evitar declarações duplicadas, e para garantir que um identificador seja declarado antes de ser usado no mesmo escopo
int buscaSimboloNoEscopoAtual(Analisador *an, const char *nome) {
    for (int i = an->totalSimbolos - 1; i >= 0; i--)    // procura de trás para frente para encontrar o símbolo mais próximo (mais recente) com o nome dado, que esteja ativo (ainda pode ser usado) e que esteja no escopo atual
        if (an->simbolos[i].ativo && an->simbolos[i].escopo == an->escopoAtual && strcmp(an->simbolos[i].nome, nome) == 0)
            return i;
    return -1;
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
    // retorna 1 se o nome for um tipo pré-definido ou um tipo declarado na tabela de símbolos, 
    // e 0 se não for, para verificar se um tipo usado no código fonte é conhecido, ou seja, se foi declarado ou é um tipo pré-definido, para garantir que o programa seja sintaticamente correto
}

// adicionaSimbolo: recebe um nome, tipo e classe, e adiciona um símbolo na tabela de símbolos, verificando se o nome já foi declarado no escopo atual e se a tabela de símbolos não está cheia
void adicionaSimbolo(Analisador *an, const char *nome, const char *tipoNome, ClasseSimbolo classe){
    if (buscaSimboloNoEscopoAtual(an, nome) >= 0)   // nao pode haver dois simbolos com o mesmo nome no mesmo escopo, entao verifica se o nome dado já foi declarado no escopo atual usando a função buscaSimboloNoEscopoAtual, e se encontrar um símbolo com o mesmo nome, imprime uma mensagem de erro indicando que o identificador já foi declarado, formatando a string com o nome do identificador, e encerra o programa
    {
        char msg[MAX_COMPRIMENTO * 2];  // buffer para montar a mensagem de erro,
        // snprintf é uma função de formatação de string, que funciona como sprintf mas com um limite de tamanho para evitar overflow, usada para montar a mensagem de erro indicando que o identificador já foi declarado, formatando a string com o nome do identificador
        snprintf(msg, sizeof(msg), "identificador '%s' ja declarado", nome);    // monta a mensagem de erro indicando que o identificador já foi declarado, usando snprintf para formatar a string com o nome do identificador
        erro(an, msg);  // imprime a mensagem de erro e encerra o programa,
    }

    // verifica se a tabela de símbolos está cheia antes de adicionar um novo símbolo, para evitar overflow e garantir que o programa não tente acessar índices inválidos na tabela de símbolos
    if (an->totalSimbolos >= MAX_SIMBOLOS) {
        erro(an, "tabela de simbolos cheia");
    }

    Simbolo *simbolo = &an->simbolos[an->totalSimbolos++];      // usando o totalSimbolos como índice para o próximo símbolo a ser adicionado, e depois incrementa o totalSimbolos para indicar que um novo símbolo foi adicionado
    
    strncpy(simbolo->nome, nome, sizeof(simbolo->nome) - 1);    // copia o nome do símbolo para a struct do símbolo, limitando o tamanho para evitar overflow, e garantindo que seja uma string válida
    simbolo->nome[sizeof(simbolo->nome) - 1] = '\0';            // finaliza a string do nome do símbolo com o caractere nulo para que seja uma string válida, e para imprimir na tabela de símbolos e mensagens de erro
    
    strncpy(simbolo->tipo, tipoNome, sizeof(simbolo->tipo) - 1);    // copia o tipo do símbolo para a struct do símbolo, limitando o tamanho para evitar overflow, e garantindo que seja uma string válida
    simbolo->tipo[sizeof(simbolo->tipo) - 1] = '\0';            // finaliza a string do tipo do símbolo com o caractere nulo para que seja uma string válida, e para imprimir na tabela de símbolos e mensagens de erro
    
    simbolo->classe = classe;                                   // atribui a classe do símbolo para a struct do símbolo, usada para verificar se um identificador é uma variável, tipo, procedimento, etc, e para imprimir na tabela de símbolos
    simbolo->linha = an->numeroLinha;                           // armazena a linha onde o símbolo foi declarado, usando o número da linha atual do analisador léxico, para imprimir na tabela de símbolos e mensagens de erro
    simbolo->coluna = an->colunaToken;                          // armazena a coluna onde o símbolo foi declarado, usando a coluna do token atual do analisador léxico, para imprimir na tabela de símbolos e mensagens de erro
    simbolo->escopo = an->escopoAtual;                          // armazena o escopo onde o símbolo foi declarado, usando o escopo atual do analisador sintático, para verificar se um identificador está no escopo correto e para imprimir na tabela de símbolos
    simbolo->ativo = 1;
    simbolo->deslocamento = (classe == SIM_VARIAVEL || classe == SIM_PARAMETRO) ? an->deslocamentoPorEscopo[an->escopoAtual]++ : -1;
    simbolo->rotuloCodigo = (classe == SIM_ROTULO || classe == SIM_PROCEDIMENTO || classe == SIM_FUNCAO) ? novoRotulo(an) : -1;
    simbolo->quantidadeParametros = 0;
    simbolo->referencia = 0;
    simbolo->donoEscopo = an->escopoAtual > 0 ? an->simboloRotinaPorEscopo[an->escopoAtual] : -1;  // registra a rotina dona do escopo; -1 indica o escopo global
}
// adicionaPreDefinido: recebe um nome e uma classe, e adiciona um símbolo pré-definido na tabela de símbolos, usado para adicionar os tipos pré-definidos e os procedimentos de entrada/saída
// ele faz isso toda vez que o programa é executado, para garantir que os símbolos pré-definidos estejam sempre na tabela de símbolos, mesmo que o programa fonte não declare nenhum tipo ou procedimento, e para garantir que sejam reconhecidos como tipos e procedimentos conhecidos durante a análise sintática
// um simboo pre definido é um símbolo que já está na tabela de símbolos antes de começar a compilar o programa, para representar os tipos pré-definidos (integer, real, boolean, char) e os procedimentos de entrada/saída (read, write), para que possam ser usados no programa sem precisar declarar, e para garantir que sejam reconhecidos como tipos e procedimentos conhecidos durante a análise sintática
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
// exigir que o simbolo esteja declarado
void exigeDeclarado(Analisador *an, const char *nome){
    // se nao ta na tabela de simbolos, entao o identificador nao foi declarado, entao imprime uma mensagem de erro indicando que o identificador não foi declarado, formatando a string com o nome do identificador, e encerra o programa
    if (buscaSimbolo(an, nome) < 0) // se a função buscaSimbolo retornar um índice menor que 0, significa que o nome dado não foi encontrado na tabela de símbolos, 
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
    printf("%-20s %-15s %-15s %-8s %-8s\n", "Nome", "Natureza", "Tipo", "Linha", "Escopo");
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


// RAFAELLY

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
    
    gera(an, "INPP\n");
    an->rotuloPrincipal = novoRotulo(an);
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
    gera(an, "PARA\n");

    printf("Programa sintaticamente correto!\n");
}


void compila_bloco(Analisador *an) {
    token t = proximoToken(an);

    if (t == rotulo)
    {
        t = proximoToken(an);
        if (t != numero)
            ERRO("esperava numero apos 'label'");

        do {
            if (t != numero) ERRO("esperava numero em 'label'");
            adicionaSimbolo(an, an->palavraAtual, "-", SIM_ROTULO);
            t = proximoToken(an);
            if (t != virgula) break;
            t = proximoToken(an);
        } while (1);

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
            // VIDEO - PARTE 1 (MARIANA) - Alocacao das variaveis
            gera(an, "AMEM %d\n", totalIds);

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' ao final da declaracao de variavel");

            t = proximoToken(an);
        }
    }

    if (an->escopoAtual == 0)
        gera(an, "DSVS L%d\n", an->rotuloPrincipal);
    else
        gera(an, "DSVS L%d\n", an->rotuloCorpoPorEscopo[an->escopoAtual]);

    while (t == procedimento || t == funcao)
    {
        if (t == procedimento)
        {
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'procedure'");
            char nomeProc[MAX_COMPRIMENTO];
            strcpy(nomeProc, an->palavraAtual);
            adicionaSimbolo(an, nomeProc, "-", SIM_PROCEDIMENTO);
            int idxProc = buscaSimbolo(an, nomeProc);
            entraEscopo(an);
            int corpoProc = novoRotulo(an);
            an->rotuloCorpoPorEscopo[an->escopoAtual] = corpoProc;
            an->simboloRotinaPorEscopo[an->escopoAtual] = idxProc;
            gera(an, "L%d: NADA\nENPR %d\n", an->simbolos[idxProc].rotuloCodigo, an->escopoAtual);

            t = proximoToken(an);
            if (t == abreparenteses)
            {
                compila_parametros_formais(an);
                t = proximoToken(an);
            }

            if (t != pontoevirgula)
                ERRO("esperava ';' apos cabecalho de 'procedure'");

            int escopoProc = an->escopoAtual;
            compila_bloco(an);
            gera(an, "RTPR %d, %d\n", escopoProc, an->simbolos[idxProc].quantidadeParametros);
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
            int escopoFunc = an->escopoAtual;
            int corpoFunc = novoRotulo(an);
            an->rotuloCorpoPorEscopo[escopoFunc] = corpoFunc;
            an->simboloRotinaPorEscopo[escopoFunc] = indiceFuncao;
            gera(an, "L%d: NADA\nENPR %d\n", an->simbolos[indiceFuncao].rotuloCodigo, escopoFunc);

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

            char tipoRetorno[MAX_COMPRIMENTO];
            strcpy(tipoRetorno, an->simbolos[indiceFuncao].tipo);
            adicionaSimbolo(an, nomeFuncao, tipoRetorno, SIM_VARIAVEL);
            int resultadoFuncao = buscaSimboloNoEscopoAtual(an, nomeFuncao);
            int parametrosFunc = an->simbolos[indiceFuncao].quantidadeParametros;
            an->simbolos[resultadoFuncao].deslocamento = -(parametrosFunc + 3);
            an->deslocamentoPorEscopo[escopoFunc]--;
            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos cabecalho de 'function'");

            compila_bloco(an);
            gera(an, "RTPR %d, %d\n", escopoFunc, parametrosFunc);
            saiEscopo(an);

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos bloco de 'function'");

            t = proximoToken(an);
        }
    }

    if (t != inicio)
        ERRO("esperava 'begin'");

    if (an->escopoAtual == 0) gera(an, "L%d: NADA\n", an->rotuloPrincipal);
    else gera(an, "L%d: NADA\n", an->rotuloCorpoPorEscopo[an->escopoAtual]);
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
    int locais = an->deslocamentoPorEscopo[an->escopoAtual];
    // VIDEO - PARTE 1 (MARIANA) - Desalocacao ao fechar o bloco
    if (locais) gera(an, "DMEM %d\n", locais);
}

void registraListaParametros(Analisador *an, char ids[][MAX_COMPRIMENTO], int totalIds, const char *tipoParam) {
    if (!ehTipoConhecido(an, tipoParam))
        erro(an, "tipo nao declarado em parametros formais");

    for (int i = 0; i < totalIds; i++)
        adicionaSimbolo(an, ids[i], tipoParam, SIM_PARAMETRO);
    if (an->escopoAtual > 0)
        an->simbolos[an->simboloRotinaPorEscopo[an->escopoAtual]].quantidadeParametros += totalIds;
}

// SAMUEL

void compila_parametros_formais(Analisador *an) {
    token t;

    do
    {
        t = proximoToken(an);

        int porReferencia = (t == variavel);
        if (porReferencia)
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
            for (int i = 0; i < totalIds; ++i) {
                int p = buscaSimboloNoEscopoAtual(an, ids[i]);
                an->simbolos[p].referencia = porReferencia;
            }

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
    int rotina = an->simboloRotinaPorEscopo[an->escopoAtual];
    int total = an->simbolos[rotina].quantidadeParametros;
    int ordinal = 0;
    for (int i = 0; i < an->totalSimbolos; ++i) {
        Simbolo *p = &an->simbolos[i];
        if (p->donoEscopo == rotina && p->classe == SIM_PARAMETRO) {
            p->deslocamento = ordinal - total - 2;
            ordinal++;
        }
    }
    an->deslocamentoPorEscopo[an->escopoAtual] = 0;
}

// VIDEO - PARTE 3 (SAMUEL) - Argumentos e passagem de endereco com CREN
static void compilaArgumentos(Analisador *an, token *t, int rotina) {
    int ordinal = 0;
    while (*t != fechaparenteses) {
        int indiceParametro = -1;
        int visto = 0;
        for (int i = 0; i < an->totalSimbolos; ++i)
            if (an->simbolos[i].donoEscopo == rotina && an->simbolos[i].classe == SIM_PARAMETRO) {
                if (visto++ == ordinal) { indiceParametro = i; break; }
            }
        if (indiceParametro < 0) erro(an, "quantidade de argumentos excede os parametros");
        if (an->simbolos[indiceParametro].referencia) {
            if (*t != identificador) erro(an, "parametro var requer uma variavel");
            int atual = buscaSimbolo(an, an->palavraAtual);
            if (atual < 0 || an->simbolos[atual].deslocamento == -1 ||
                (an->simbolos[atual].classe != SIM_VARIAVEL && an->simbolos[atual].classe != SIM_PARAMETRO)) erro(an, "argumento var invalido");
            if (!mesmoTipo(an, an->simbolos[atual].tipo, an->simbolos[indiceParametro].tipo)) erro(an, "tipo incompativel em argumento var");
            if (an->simbolos[atual].referencia)
                gera(an, "CRVL %d, %d\n", nivelAcesso(an, &an->simbolos[atual]), an->simbolos[atual].deslocamento);
            else
                gera(an, "CREN %d, %d\n", nivelAcesso(an, &an->simbolos[atual]), an->simbolos[atual].deslocamento);
            *t = proximoToken(an);
        } else {
            compila_expressao(an, t);
            if (!tipoCompativel(an, an->tipoExpressao, an->simbolos[indiceParametro].tipo)) erro(an, "tipo incompativel em argumento");
        }
        ordinal++;
        if (*t != virgula) break;
        *t = proximoToken(an);
        if (*t == fechaparenteses) erro(an, "esperava argumento apos ','");
    }
    int esperados = an->simbolos[rotina].quantidadeParametros;
    if (ordinal != esperados) erro(an, "quantidade de argumentos diferente da declaracao");
}

// VIDEO - PARTE 3 (SAMUEL) - Chamada de rotina com CHPR
static void compila_chamada(Analisador *an, token *t, int rotina) {
    Simbolo *simbolo = &an->simbolos[rotina];
    if (simbolo->classe == SIM_FUNCAO) gera(an, "AMEM 1\n");

    if (*t == abreparenteses) {
        *t = proximoToken(an);
        compilaArgumentos(an, t, rotina);
        if (*t != fechaparenteses) erro(an, "esperava ')' ao final da chamada");
        *t = proximoToken(an);
    } else if (simbolo->quantidadeParametros != 0) {
        erro(an, "chamada exige argumentos");
    }

    gera(an, "CHPR L%d\n", simbolo->rotuloCodigo);
    if (simbolo->classe == SIM_FUNCAO) strcpy(an->tipoExpressao, simbolo->tipo);
}

void compila_comando(Analisador *an, token *t) {
    while (*t == numero)
    {
        char rotulo[MAX_COMPRIMENTO]; strcpy(rotulo, an->palavraAtual);
        int idx = buscaSimbolo(an, rotulo);
        if (idx < 0 || an->simbolos[idx].classe != SIM_ROTULO) ERRO("rotulo usado nao foi declarado");
        gera(an, "L%d: NADA\n", an->simbolos[idx].rotuloCodigo);
        *t = proximoToken(an);
        if (*t != doispontos) ERRO("esperava ':' apos rotulo numerico");
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
        int indiceIdentificador = buscaSimbolo(an, nome);
        Simbolo *simbolo = &an->simbolos[indiceIdentificador];
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
            erro(an, "arrays nao sao suportados pela linguagem deste compilador");
        }

        if (*t == atribuicao)
        {
            *t = proximoToken(an);
            compila_expressao(an, t);
            if (simbolo->classe != SIM_VARIAVEL && simbolo->classe != SIM_PARAMETRO)
                erro(an, "destino invalido na atribuicao");
            if (!tipoCompativel(an, simbolo->tipo, an->tipoExpressao))
                erro(an, "tipos incompativeis na atribuicao");
            if (simbolo->referencia)
                gera(an, "ARMI %d, %d\n", nivelAcesso(an, simbolo), simbolo->deslocamento);
            else
                gera(an, "ARMZ %d, %d\n", nivelAcesso(an, simbolo), simbolo->deslocamento);
        }
        else if (*t == abreparenteses)
        {
            if (simbolo->classe != SIM_PROCEDIMENTO)
                erro(an, "chamada de procedimento invalida");
            int ehRead = indiceIdentificador == 0;
            int ehWrite = indiceIdentificador == 1;
            if (!ehRead && !ehWrite) {
                compila_chamada(an, t, indiceIdentificador);
                return;
            }
            *t = proximoToken(an);
            if (*t == fechaparenteses) erro(an, "chamada exige argumentos");
            if (ehRead) {
                do {
                    if (*t != identificador) ERRO("read espera identificadores");
                    char destino[MAX_COMPRIMENTO];
                    strcpy(destino, an->palavraAtual);
                    exigeDeclarado(an, destino);
                    int idx = buscaSimbolo(an, destino);
                    if (an->simbolos[idx].classe != SIM_VARIAVEL && an->simbolos[idx].classe != SIM_PARAMETRO)
                        erro(an, "read espera variaveis ou parametros");
                    gera(an, "LEIT\n");
                    if (an->simbolos[idx].referencia)
                        gera(an, "ARMI %d, %d\n", nivelAcesso(an, &an->simbolos[idx]), an->simbolos[idx].deslocamento);
                    else
                        gera(an, "ARMZ %d, %d\n", nivelAcesso(an, &an->simbolos[idx]), an->simbolos[idx].deslocamento);
                    *t = proximoToken(an);
                    if (*t == virgula) *t = proximoToken(an); else break;
                } while (1);
            } else {
                compila_expressao(an, t);
                gera(an, "IMPR\n");
                while (*t == virgula) {
                    *t = proximoToken(an);
                    compila_expressao(an, t);
                    gera(an, "IMPR\n");
                }
            }

            if (*t != fechaparenteses)
            ERRO("esperava ')' ao final da chamada de procedimento");
            *t = proximoToken(an);
        }
        else
        {
            if (simbolo->classe != SIM_PROCEDIMENTO)
                erro(an, "chamada de procedimento invalida");
            if (indiceIdentificador <= 1) erro(an, "chamada exige argumentos");
            compila_chamada(an, t, indiceIdentificador);
        }
    }
    else if (*t == vapara)
    {
        *t = proximoToken(an);
        if (*t != numero)
            ERRO("esperava numero apos 'goto'");
        int idxRotulo = buscaSimbolo(an, an->palavraAtual);
        if (idxRotulo < 0 || an->simbolos[idxRotulo].classe != SIM_ROTULO) ERRO("rotulo usado nao foi declarado");
        if (an->simbolos[idxRotulo].escopo == an->escopoAtual)
            gera(an, "DSVS L%d\n", an->simbolos[idxRotulo].rotuloCodigo);
        else
            gera(an, "DSVR L%d, %d, %d\n", an->simbolos[idxRotulo].rotuloCodigo, an->simbolos[idxRotulo].escopo, an->escopoAtual);
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
        int rotuloSenao = novoRotulo(an), rotuloFim = novoRotulo(an);
        *t = proximoToken(an);
        compila_expressao(an, t);
        exigeTipo(an, tipoCompativel(an, an->tipoExpressao, "boolean"), "condicao do if deve ser boolean");

        if (*t != entao)
            ERRO("esperava 'then' apos condicao do 'if'");

        // VIDEO - PARTE 2 (RAFAELLY) - Desvios do if/else
        gera(an, "DSVF L%d\n", rotuloSenao);
        *t = proximoToken(an);
        compila_comando_sem_rotulo(an, t);

        if (*t == senao)
        {
            gera(an, "DSVS L%d\nL%d: NADA\n", rotuloFim, rotuloSenao);
            *t = proximoToken(an);
            compila_comando_sem_rotulo(an, t);
            gera(an, "L%d: NADA\n", rotuloFim);
        } else { gera(an, "L%d: NADA\n", rotuloSenao); }
    }
    else if (*t == enquanto)
    {
        int rotuloInicio = novoRotulo(an), rotuloFim = novoRotulo(an);
        gera(an, "L%d: NADA\n", rotuloInicio);
        *t = proximoToken(an);
        compila_expressao(an, t);
        exigeTipo(an, tipoCompativel(an, an->tipoExpressao, "boolean"), "condicao do while deve ser boolean");

        if (*t != faca)
            ERRO("esperava 'do' apos condicao do 'while'");

        // VIDEO - PARTE 2 (RAFAELLY) - Saida e retorno ao teste do while
        gera(an, "DSVF L%d\n", rotuloFim);
        *t = proximoToken(an);
        compila_comando_sem_rotulo(an, t);
        gera(an, "DSVS L%d\nL%d: NADA\n", rotuloInicio, rotuloFim);
    }
}
void compila_expressao(Analisador *an, token *t) {
    compila_expressao_simples(an, t);

    if (*t == igual || *t == diferente ||
        *t == menor || *t == menorouigual ||
        *t == maior || *t == maiorouigual)
    {
        token op = *t;
        *t = proximoToken(an);
        char esquerda[MAX_COMPRIMENTO]; strcpy(esquerda, an->tipoExpressao);
        compila_expressao_simples(an, t);
        if (op == menor || op == menorouigual || op == maior || op == maiorouigual)
            exigeTipo(an, tipoNumerico(an, esquerda) && tipoNumerico(an, an->tipoExpressao), "comparacao ordenada exige operandos numericos");
        else
            exigeTipo(an, tipoCompativel(an, esquerda, an->tipoExpressao), "tipos incompativeis na comparacao");
        emiteOperador(an, op);
        strcpy(an->tipoExpressao, "boolean");
    }
}
void compila_expressao_simples(Analisador *an, token *t) {
    token sinal = token_invalido;
    if (*t == mais || *t == menos) { sinal = *t; *t = proximoToken(an); }

    compila_termo(an, t);
    if (sinal != token_invalido) { exigeTipo(an, tipoNumerico(an, an->tipoExpressao), "sinal unario exige tipo numerico"); if (sinal == menos) gera(an, "INVR\n"); }

    while (*t == mais || *t == menos || *t == ou)
    {
        token op = *t;
        char esquerda[MAX_COMPRIMENTO]; strcpy(esquerda, an->tipoExpressao);
        *t = proximoToken(an);
        compila_termo(an, t);
        if (op == ou) {
            exigeTipo(an, tipoCompativel(an, esquerda, "boolean") && tipoCompativel(an, an->tipoExpressao, "boolean"), "operador 'or' exige operandos boolean");
            strcpy(an->tipoExpressao, "boolean");
        } else {
            exigeTipo(an, tipoNumerico(an, esquerda) && tipoNumerico(an, an->tipoExpressao), "operador aritmetico exige operandos numericos");
            if (mesmoTipo(an, esquerda, "real") || mesmoTipo(an, an->tipoExpressao, "real")) strcpy(an->tipoExpressao, "real");
        }
        emiteOperador(an, op);
    }
}
void compila_termo(Analisador *an, token *t) {
    compila_fator(an, t);

    while (*t == vezes || *t == dividir || *t == e)
    {
        token op = *t;
        char esquerda[MAX_COMPRIMENTO]; strcpy(esquerda, an->tipoExpressao);
        *t = proximoToken(an);
        compila_fator(an, t);
        if (op == e) {
            exigeTipo(an, tipoCompativel(an, esquerda, "boolean") && tipoCompativel(an, an->tipoExpressao, "boolean"), "operador 'and' exige operandos boolean");
            strcpy(an->tipoExpressao, "boolean");
        } else {
            exigeTipo(an, tipoNumerico(an, esquerda) && tipoNumerico(an, an->tipoExpressao), "operador aritmetico exige operandos numericos");
            if (mesmoTipo(an, esquerda, "real") || mesmoTipo(an, an->tipoExpressao, "real")) strcpy(an->tipoExpressao, "real");
        }
        emiteOperador(an, op);
    }
}
void compila_fator(Analisador *an, token *t) {
    if (*t == identificador)
    {
        char nome[MAX_COMPRIMENTO];
        strcpy(nome, an->palavraAtual);
        exigeDeclarado(an, nome);
        int indiceValor = buscaSimbolo(an, nome);
        if (indiceValor >= 0 && an->simbolos[indiceValor].classe != SIM_VARIAVEL &&
            an->simbolos[indiceValor].classe != SIM_PARAMETRO && an->simbolos[indiceValor].classe != SIM_FUNCAO)
            erro(an, "identificador nao pode ser usado em expressao");
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
            erro(an, "arrays nao sao suportados pela linguagem deste compilador");
        }
        else if (*t == abreparenteses)
        {
            Simbolo *valor = &an->simbolos[indiceValor];
            int rotina = indiceValor;
            // Na recursao, o nome tambem identifica a celula de retorno.
            if (valor->classe == SIM_VARIAVEL && valor->deslocamento < 0)
                rotina = valor->donoEscopo;
            if (an->simbolos[rotina].classe != SIM_FUNCAO)
                erro(an, "identificador nao e uma funcao");
            compila_chamada(an, t, rotina);
        } else if (indiceValor >= 0 && an->simbolos[indiceValor].deslocamento != -1) {
            if (an->simbolos[indiceValor].referencia)
                gera(an, "CRVI %d, %d\n", nivelAcesso(an, &an->simbolos[indiceValor]), an->simbolos[indiceValor].deslocamento);
            else
                gera(an, "CRVL %d, %d\n", nivelAcesso(an, &an->simbolos[indiceValor]), an->simbolos[indiceValor].deslocamento);
            strncpy(an->tipoExpressao, an->simbolos[indiceValor].tipo, sizeof(an->tipoExpressao)-1);
        } else if (indiceValor >= 0 && an->simbolos[indiceValor].classe == SIM_FUNCAO) {
            compila_chamada(an, t, indiceValor);
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
        exigeTipo(an, tipoCompativel(an, an->tipoExpressao, "boolean"), "operador 'not' exige operando boolean");
        gera(an, "CRCT 0\nCMIG\n");
        strcpy(an->tipoExpressao, "boolean");
    }
    else if (*t == numero)
    {
        gera(an, "CRCT %s\n", an->palavraAtual);
        strcpy(an->tipoExpressao, "integer");
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
    const char *nomeArquivo = argc > 1 ? argv[1] : "exemplo.txt";
    const char *nomeSaida = argc > 2 ? argv[2] : "saida.mepa";
    Analisador an;
    memset(&an, 0, sizeof(an));                                     // inicializa o analisador com zeros, para garantir que os campos estejam limpos
    adicionaPreDefinido(&an, "read", SIM_PROCEDIMENTO);             // adiciona os procedimentos predefinidos "read" e "write" na tabela de simbolos, para que possam ser usados no programa sem precisar declarar
    adicionaPreDefinido(&an, "write", SIM_PROCEDIMENTO);
    an.saida = fopen(nomeSaida, "w");
    if (!an.saida) { fprintf(stderr, "Erro ao criar arquivo MEPA '%s'\n", nomeSaida); return 1; }

    an.arquivo = fopen(nomeArquivo, "r");   // tenta abrir o arquivo de entrada para leitura, e verifica se conseguiu abrir, caso contrário imprime uma mensagem de erro e encerra o programa
    if (!an.arquivo)
    {
        fprintf(stderr, "Erro ao abrir arquivo '%s'\n", nomeArquivo);
        fclose(an.saida);
        return 1;
    }

    compila_programa(&an);  // chama a função de compilar o programa, que é a função principal do analisador sintático
    imprimeTabelaSimbolos(&an);

    fclose(an.arquivo);
    fclose(an.saida);
    return 0;
}
