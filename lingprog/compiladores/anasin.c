#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_COMPRIMENTO 256
#define NUM_PALAVRAS 36

// cria o tipo tokens como enumeração, cada token tem um numero inteiro correspondente
typedef enum
{
    programa,
    rotulo,
    tipo,
    variavel,
    procedimento,
    funcao,
    inicio,
    fim,
    atribuicao,
    se,
    entao,
    senao,
    enquanto,
    faca,
    vapara,
    mais,
    menos,
    vezes,
    dividir,
    igual,
    diferente,
    menor,
    menorouigual,
    maior,
    maiorouigual,
    e,
    ou,
    nao,
    abreparenteses,
    fechaparenteses,
    fechacolchetes,
    abrecolchetes,
    virgula,
    pontoevirgula,
    doispontos,
    ponto,
    numero,
    identificador,
    fimdearquivo
} token;

// vetor de strings que podem aparecer no código, cada string tem um token correspondente no vetor de tokens
char *palavras[NUM_PALAVRAS] = {
    "program", "label", "type", "var", "procedure",
    "function", "begin", "end", ":=", "if", "then", "else",
    "while", "do", "goto", "+", "-", "*", "div",
    "=", "<>", "<", "<=", ">", ">=",
    "and", "or", "not", "(", ")", "]",
    "[", ",", ";", ":", "."};

// vetor de tokens correspondente ao vetor de strings que cada indice tem o tipo token criado acima
token tokens[NUM_PALAVRAS] = {
    programa, rotulo, tipo, variavel, procedimento,
    funcao, inicio, fim, atribuicao, se, entao, senao,
    enquanto, faca, vapara, mais, menos, vezes, dividir,
    igual, diferente, menor, menorouigual, maior, maiorouigual,
    e, ou, nao, abreparenteses, fechaparenteses, fechacolchetes,
    abrecolchetes, virgula, pontoevirgula, doispontos, ponto};

// palavras[i] = tokens[i]
// Analex: recebe uma palavra e retorna o token correspondente
token analex(char *palavra)
{
    for (int i = 0; i < NUM_PALAVRAS; i++)
        if (strcmp(palavra, palavras[i]) == 0) // verifica se a palavra é uma palavra reservada ou símbolo
            return tokens[i];                  // se for, retorna o token correspondente

    // se não for uma palavra reservada ou símbolo, verifica se é um número ou identificador
    for (int i = 0; palavra[i] != '\0'; i++)
        if (!isdigit((unsigned char)palavra[i])) // se tiver um caractere que não seja número, é um identificador
            return identificador;

    return numero; // se todos os caracteres forem números, é um número
}

// estrutura
typedef struct
{
    FILE *arquivo;          // o arquivo aberto
    char linha[256];        // a linha que está sendo lida agora
    int pos;                // qual caractere da linha estou olhando agora, indice
    char buf[256];          // onde vou montar o token
    char palavraAtual[256]; // guarda o token depois de pronto
} Analisador;

// compilador sintático -> ve se a estrutura do programa tá correta, se tem os tokens na ordem certa, etc
// maligno quer q retorne o tipo do token

token compila_programa(Analisador *an)
{
    token t;

    /* Espera a palavra-chave 'program' */
    t = proximoToken(an);
    if (t != programa)
        ERRO("esperava 'program' no inicio do programa");

    /* Nome do programa */
    t = proximoToken(an);
    if (t != identificador)
        ERRO("esperava identificador apos 'program'");

    /* '(' para a lista de arquivos de E/S */
    t = proximoToken(an);
    if (t != abreparenteses)
        ERRO("esperava '(' apos o nome do programa");

    /* Lista de identificadores separados por ',' */
    do
    {
        t = proximoToken(an);
        if (t != identificador)
            ERRO("esperava identificador na lista de arquivos");

        t = proximoToken(an);
        if (t != virgula && t != fechaparenteses)
            ERRO("esperava ',' ou ')' na lista de arquivos");
    } while (t == virgula);

    /* ';' antes do bloco principal */
    t = proximoToken(an);
    if (t != pontoevirgula)
        ERRO("esperava ';' apos ')'");

    /* Compila o bloco principal */
    compila_bloco(an);

    /* '.' encerrando o programa */
    t = proximoToken(an);
    if (t != ponto)
        ERRO("esperava '.' ao final do programa");

    /* Não deve haver nada depois do '.' */
    t = proximoToken(an);
    if (t != fimdearquivo)
        ERRO("conteudo inesperado apos '.'");

    printf("Programa sintaticamente correto!\n");
}

void compila_bloco(Analisador *an)
{
    token t = proximoToken(an);

    /* ---- seção de rótulos: label num {, num} ; ---- */
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

    /* ---- seção de tipos: type id = id ; {id = id ;} ---- */
    if (t == tipo)
    {
        t = proximoToken(an);

        while (t == identificador)
        {
            t = proximoToken(an);
            if (t != igual)
                ERRO("esperava '=' apos identificador em 'type'");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava nome de tipo apos '=' em 'type'");

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' ao final da definicao de tipo");

            t = proximoToken(an);
        }
    }

    /* ---- seção de variáveis: var id {, id} : tipo ; {id...} ---- */
    if (t == variavel)
    {
        t = proximoToken(an);

        while (t == identificador)
        {
            /* lê lista de identificadores */
            while (1)
            {
                t = proximoToken(an);
                if (t == virgula)
                {
                    t = proximoToken(an);
                    if (t != identificador)
                        ERRO("esperava identificador apos ',' em 'var'");
                }
                else
                {
                    break; /* saiu da lista */
                }
            }

            /* ':' separando nomes do tipo */
            if (t != doispontos)
                ERRO("esperava ':' apos lista de variaveis");

            /* nome do tipo */
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava nome de tipo apos ':' em 'var'");

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' ao final da declaracao de variavel");

            t = proximoToken(an);
        }
    }

    /* ---- declarações de procedures e functions ---- */
    while (t == procedimento || t == funcao)
    {

        if (t == procedimento)
        {
            /* procedure id [( params )] ; bloco ; */
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'procedure'");

            t = proximoToken(an);
            if (t == abreparenteses)
            {
                compila_parametros_formais(an); /* consome até o ')' */
                t = proximoToken(an);
            }

            if (t != pontoevirgula)
                ERRO("esperava ';' apos cabecalho de 'procedure'");

            compila_bloco(an); /* bloco interno da procedure */

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos bloco de 'procedure'");

            t = proximoToken(an);
        }
        else
        {
            /* function id [( params )] : tipo ; bloco ; */
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'function'");

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

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos cabecalho de 'function'");

            compila_bloco(an);

            t = proximoToken(an);
            if (t != pontoevirgula)
                ERRO("esperava ';' apos bloco de 'function'");

            t = proximoToken(an);
        }
    }

    /* ---- corpo principal: begin ... end ---- */
    if (t != inicio)
        ERRO("esperava 'begin'");

    t = proximoToken(an);
    while (t != fim)
    {
        if (t == fimdearquivo)
            ERRO("fim de arquivo inesperado dentro de 'begin'");

        compila_comando(an, &t);

        if (t == pontoevirgula)
        {
            t = proximoToken(an);
        }
        else if (t != fim)
        {
            ERRO("esperava ';' ou 'end' apos comando");
        }
    }
    /* 'end' já foi consumido pelo while; o chamador avança se precisar */
}

void compila_parametros_formais(Analisador *an)
{
    token t;

    do
    {
        t = proximoToken(an);

        /* parâmetro por referência */
        if (t == variavel)
            t = proximoToken(an);

        if (t == identificador)
        {
            /* lista de identificadores */
            t = proximoToken(an);
            while (t == virgula)
            {
                t = proximoToken(an);
                if (t != identificador)
                    ERRO("esperava identificador apos ',' em parametros");
                t = proximoToken(an);
            }

            if (t != doispontos)
                ERRO("esperava ':' apos lista de parametros");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava tipo em parametros formais");

            t = proximoToken(an);
        }
        else if (t == funcao)
        {
            /* parâmetro do tipo function */
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'function' em parametros");

            t = proximoToken(an);
            while (t == virgula)
            {
                t = proximoToken(an);
                if (t != identificador)
                    ERRO("esperava identificador apos ',' em parametros");
                t = proximoToken(an);
            }

            if (t != doispontos)
                ERRO("esperava ':' em parametro function");

            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava tipo de retorno em parametro function");

            t = proximoToken(an);
        }
        else if (t == procedimento)
        {
            /* parâmetro do tipo procedure */
            t = proximoToken(an);
            if (t != identificador)
                ERRO("esperava identificador apos 'procedure' em parametros");

            t = proximoToken(an);
            while (t == virgula)
            {
                t = proximoToken(an);
                if (t != identificador)
                    ERRO("esperava identificador apos ',' em parametros");
                t = proximoToken(an);
            }
        }
        else
        {
            ERRO("parametro formal invalido");
        }

    } while (t == pontoevirgula);

    if (t != fechaparenteses)
        ERRO("esperava ')' ao final dos parametros formais");
}

void compila_comando(Analisador *an, token *t)
{
    /* Rótulos numéricos opcionais: 42 : comando */
    while (*t == numero)
    {
        *t = proximoToken(an);
        if (*t != doispontos)
            ERRO("esperava ':' apos rotulo numerico");
        *t = proximoToken(an);
    }

    compila_comando_sem_rotulo(an, t);
}

void compila_comando_sem_rotulo(Analisador *an, token *t)
{

    if (*t == identificador)
    {
        /* id [ '[' expr {, expr} ']' ] ':=' expr
           ou id '(' expr {, expr} ')'           */

        *t = proximoToken(an);

        /* acesso a array no lado esquerdo: id[...] */
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
            /* atribuição: id := expr */
            *t = proximoToken(an);
            compila_expressao(an, t);
        }
        else if (*t == abreparenteses)
        {
            /* chamada de procedimento: id(expr {, expr}) */
            do
            {
                *t = proximoToken(an);
                compila_expressao(an, t);
            } while (*t == virgula);

            if (*t != fechaparenteses)
                ERRO("esperava ')' ao final da chamada de procedimento");
            *t = proximoToken(an);
        }
        else
        {
            ERRO("esperava ':=' ou '(' apos identificador em comando");
        }
    }
    else if (*t == vapara)
    {
        /* goto numero */
        *t = proximoToken(an);
        if (*t != numero)
            ERRO("esperava numero apos 'goto'");
        *t = proximoToken(an);
    }
    else if (*t == inicio)
    {
        /* begin ... end  (bloco de comandos composto) */
        *t = proximoToken(an);
        while (*t != fim)
        {
            if (*t == fimdearquivo)
                ERRO("fim de arquivo inesperado em bloco composto");
            compila_comando(an, t);
            if (*t == pontoevirgula)
                *t = proximoToken(an);
        }
        *t = proximoToken(an); /* consome 'end' */
    }
    else if (*t == se)
    {
        /* if expr then comando [else comando] */
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
        /* while expr do comando */
        *t = proximoToken(an);
        compila_expressao(an, t);

        if (*t != faca)
            ERRO("esperava 'do' apos condicao do 'while'");

        *t = proximoToken(an);
        compila_comando_sem_rotulo(an, t);
    }
    /* Comando vazio é válido em Pascal; não precisa de else aqui */
}

void compila_expressao(Analisador *an, token *t)
{
    compila_expressao_simples(an, t);

    /* operadores relacionais */
    if (*t == igual || *t == diferente ||
        *t == menor || *t == menorouigual ||
        *t == maior || *t == maiorouigual)
    {

        *t = proximoToken(an);
        compila_expressao_simples(an, t);
    }
}

void compila_expressao_simples(Analisador *an, token *t)
{
    /* sinal unário opcional */
    if (*t == mais || *t == menos)
        *t = proximoToken(an);

    compila_termo(an, t);

    /* operadores aditivos */
    while (*t == mais || *t == menos || *t == ou)
    {
        *t = proximoToken(an);
        compila_termo(an, t);
    }
}

void compila_termo(Analisador *an, token *t)
{
    compila_fator(an, t);

    while (*t == vezes || *t == dividir || *t == e)
    {
        *t = proximoToken(an);
        compila_fator(an, t);
    }
}

void compila_fator(Analisador *an, token *t)
{
    if (*t == identificador)
    {
        *t = proximoToken(an);

        /* acesso a array: id[...] */
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

            /* chamada de função: id(...) */
        }
        else if (*t == abreparenteses)
        {
            do
            {
                *t = proximoToken(an);
                compila_expressao(an, t);
            } while (*t == virgula);

            if (*t != fechaparenteses)
                ERRO("esperava ')' ao final de chamada de funcao em fator");
            *t = proximoToken(an);
        }
    }
    else if (*t == abreparenteses)
    {
        /* expressão entre parênteses */
        *t = proximoToken(an);
        compila_expressao(an, t);

        if (*t != fechaparenteses)
            ERRO("esperava ')' apos expressao entre parenteses");
        *t = proximoToken(an);
    }
    else if (*t == nao)
    {
        /* 'not' fator  (chamada recursiva) */
        *t = proximoToken(an);
        compila_fator(an, t);
    }
    else if (*t == numero)
    {
        /* literal numérico */
        *t = proximoToken(an);
    }
    else
    {
        ERRO("fator invalido: esperava id, numero, '(' ou 'not'");
    }
}
