#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mariana Marietti da Costa - 24140
// Rafaelly Maria Nascimento da Silva - 24153
// Samuel Rosa Parra - 24155


#define MAX_COMPRIMENTO 256
#define MAX_SIMBOLOS 512
#define MAX_LISTA_IDS 128

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
    comentario,
    fimdearquivo,
    token_invalido
} token;

typedef enum
{
    SIM_PROGRAMA,
    SIM_ENTRADA_SAIDA,
    SIM_TIPO,
    SIM_VARIAVEL,
    SIM_PROCEDIMENTO,
    SIM_FUNCAO,
    SIM_PARAMETRO
} ClasseSimbolo;

typedef struct
{
    char nome[MAX_COMPRIMENTO];
    char tipo[MAX_COMPRIMENTO];
    ClasseSimbolo classe;
    int linha;
    int coluna;
    int escopo;
    int ativo;
} Simbolo;

typedef struct
{
    FILE *arquivo;
    char linha[MAX_COMPRIMENTO];
    int pos;
    int numeroLinha;
    int colunaToken;
    char buf[MAX_COMPRIMENTO];
    char palavraAtual[MAX_COMPRIMENTO];
    Simbolo simbolos[MAX_SIMBOLOS];
    int totalSimbolos;
    int escopoAtual;
} Analisador;

static const char *palavras[] = {
    "program", "label", "type", "var", "procedure",
    "function", "begin", "end", ":=", "if", "then", "else",
    "while", "do", "goto", "+", "-", "*", "div",
    "=", "<>", "<", "<=", ">", ">=",
    "and", "or", "not", "(", ")", "]",
    "[", ",", ";", ":", "."};

static const token tokens[] = {
    programa, rotulo, tipo, variavel, procedimento,
    funcao, inicio, fim, atribuicao, se, entao, senao,
    enquanto, faca, vapara, mais, menos, vezes, dividir,
    igual, diferente, menor, menorouigual, maior, maiorouigual,
    e, ou, nao, abreparenteses, fechaparenteses, fechacolchetes,
    abrecolchetes, virgula, pontoevirgula, doispontos, ponto};

static token proximoToken(Analisador *an);
static void compila_programa(Analisador *an);
static void compila_bloco(Analisador *an);
static void compila_parametros_formais(Analisador *an);
static void compila_comando(Analisador *an, token *t);
static void compila_comando_sem_rotulo(Analisador *an, token *t);
static void compila_expressao(Analisador *an, token *t);
static void compila_expressao_simples(Analisador *an, token *t);
static void compila_termo(Analisador *an, token *t);
static void compila_fator(Analisador *an, token *t);

static void erro(Analisador *an, const char *mensagem)
{
    fprintf(stderr, "Erro sintatico na linha %d, coluna %d: %s. Token atual: \"%s\"\n",
            an->numeroLinha, an->colunaToken, mensagem, an->palavraAtual);
    exit(1);
}

#define ERRO(msg) erro(an, msg)

static const char *nomeClasse(ClasseSimbolo classe)
{
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

static int buscaSimbolo(Analisador *an, const char *nome)
{
    for (int i = an->totalSimbolos - 1; i >= 0; i--)
        if (an->simbolos[i].ativo && strcmp(an->simbolos[i].nome, nome) == 0)
            return i;
    return -1;
}

static int buscaSimboloNoEscopoAtual(Analisador *an, const char *nome)
{
    for (int i = an->totalSimbolos - 1; i >= 0; i--)
        if (an->simbolos[i].ativo &&
            an->simbolos[i].escopo == an->escopoAtual &&
            strcmp(an->simbolos[i].nome, nome) == 0)
            return i;
    return -1;
}

static void entraEscopo(Analisador *an)
{
    an->escopoAtual++;
}

static void saiEscopo(Analisador *an)
{
    for (int i = 0; i < an->totalSimbolos; i++)
        if (an->simbolos[i].escopo == an->escopoAtual)
            an->simbolos[i].ativo = 0;

    if (an->escopoAtual > 0)
        an->escopoAtual--;
}

static int ehTipoConhecido(Analisador *an, const char *nome)
{
    if (strcmp(nome, "integer") == 0 ||
        strcmp(nome, "real") == 0 ||
        strcmp(nome, "boolean") == 0 ||
        strcmp(nome, "char") == 0)
    {
        return 1;
    }

    int indice = buscaSimbolo(an, nome);
    return indice >= 0 && an->simbolos[indice].classe == SIM_TIPO;
}

static void adicionaSimbolo(Analisador *an, const char *nome, const char *tipoNome, ClasseSimbolo classe)
{
    if (buscaSimboloNoEscopoAtual(an, nome) >= 0)
    {
        char msg[MAX_COMPRIMENTO * 2];
        snprintf(msg, sizeof(msg), "identificador '%s' ja declarado", nome);
        erro(an, msg);
    }

    if (an->totalSimbolos >= MAX_SIMBOLOS)
        erro(an, "tabela de simbolos cheia");

    Simbolo *simbolo = &an->simbolos[an->totalSimbolos++];
    strncpy(simbolo->nome, nome, sizeof(simbolo->nome) - 1);
    simbolo->nome[sizeof(simbolo->nome) - 1] = '\0';
    strncpy(simbolo->tipo, tipoNome, sizeof(simbolo->tipo) - 1);
    simbolo->tipo[sizeof(simbolo->tipo) - 1] = '\0';
    simbolo->classe = classe;
    simbolo->linha = an->numeroLinha;
    simbolo->coluna = an->colunaToken;
    simbolo->escopo = an->escopoAtual;
    simbolo->ativo = 1;
}

static void adicionaPreDefinido(Analisador *an, const char *nome, ClasseSimbolo classe)
{
    if (an->totalSimbolos >= MAX_SIMBOLOS)
        erro(an, "tabela de simbolos cheia");

    Simbolo *simbolo = &an->simbolos[an->totalSimbolos++];
    strncpy(simbolo->nome, nome, sizeof(simbolo->nome) - 1);
    simbolo->nome[sizeof(simbolo->nome) - 1] = '\0';
    strcpy(simbolo->tipo, "-");
    simbolo->classe = classe;
    simbolo->linha = 0;
    simbolo->coluna = 0;
    simbolo->escopo = 0;
    simbolo->ativo = 1;
}

static void exigeDeclarado(Analisador *an, const char *nome)
{
    if (buscaSimbolo(an, nome) < 0)
    {
        char msg[MAX_COMPRIMENTO * 2];
        snprintf(msg, sizeof(msg), "identificador '%s' nao declarado", nome);
        erro(an, msg);
    }
}

static void imprimeTabelaSimbolos(Analisador *an)
{
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

static token analex(const char *palavra)
{
    size_t total = sizeof(palavras) / sizeof(palavras[0]);
    for (size_t i = 0; i < total; i++)
        if (strcmp(palavra, palavras[i]) == 0)
            return tokens[i];

    if (isdigit((unsigned char)palavra[0]))
    {
        for (int i = 1; palavra[i] != '\0'; i++)
            if (!isdigit((unsigned char)palavra[i]))
                return token_invalido;
        return numero;
    }

    if (isalpha((unsigned char)palavra[0]) || palavra[0] == '_')
    {
        for (int i = 1; palavra[i] != '\0'; i++)
            if (!isalnum((unsigned char)palavra[i]) && palavra[i] != '_')
                return token_invalido;
        return identificador;
    }

    return token_invalido;
}

static int leLinha(Analisador *an)
{
    if (fgets(an->linha, MAX_COMPRIMENTO, an->arquivo) == NULL)
        return 0;
    an->pos = 0;
    an->numeroLinha++;
    return 1;
}

static int ignoraComentario(Analisador *an)
{
    if (an->linha[an->pos] == '{')
    {
        an->pos++;
        while (an->linha[an->pos] != '}')
        {
            if (an->linha[an->pos] == '\0')
            {
                if (!leLinha(an))
                {
                    strcpy(an->palavraAtual, "EOF");
                    erro(an, "comentario nao fechado");
                }
            }
            else
            {
                an->pos++;
            }
        }
        an->pos++;
        return 1;
    }

    if (an->linha[an->pos] == '(' && an->linha[an->pos + 1] == '*')
    {
        an->pos += 2;
        while (!(an->linha[an->pos] == '*' && an->linha[an->pos + 1] == ')'))
        {
            if (an->linha[an->pos] == '\0')
            {
                if (!leLinha(an))
                {
                    strcpy(an->palavraAtual, "EOF");
                    erro(an, "comentario nao fechado");
                }
            }
            else
            {
                an->pos++;
            }
        }
        an->pos += 2;
        return 1;
    }

    return 0;
}

static char *separaToken(Analisador *an)
{
    while (1)
    {
        while (an->linha[an->pos] && isspace((unsigned char)an->linha[an->pos]))
            an->pos++;

        if (an->linha[an->pos] == '\0')
        {
            if (!leLinha(an))
                return NULL;
            continue;
        }

        if (ignoraComentario(an))
            continue;

        break;
    }

    int i = 0;
    char c = an->linha[an->pos];
    an->colunaToken = an->pos + 1;

    if (isdigit((unsigned char)c))
    {
        while (isdigit((unsigned char)an->linha[an->pos]))
            an->buf[i++] = an->linha[an->pos++];
    }
    else if (isalpha((unsigned char)c) || c == '_')
    {
        while (isalnum((unsigned char)an->linha[an->pos]) || an->linha[an->pos] == '_')
            an->buf[i++] = (char)tolower((unsigned char)an->linha[an->pos++]);
    }
    else if ((c == ':' && an->linha[an->pos + 1] == '=') ||
             (c == '<' && an->linha[an->pos + 1] == '>') ||
             (c == '<' && an->linha[an->pos + 1] == '=') ||
             (c == '>' && an->linha[an->pos + 1] == '='))
    {
        an->buf[i++] = an->linha[an->pos++];
        an->buf[i++] = an->linha[an->pos++];
    }
    else
    {
        an->buf[i++] = an->linha[an->pos++];
    }

    an->buf[i] = '\0';
    return an->buf;
}

static token proximoToken(Analisador *an)
{
    char *palavra = separaToken(an);
    if (palavra == NULL)
    {
        strcpy(an->palavraAtual, "EOF");
        return fimdearquivo;
    }

    strncpy(an->palavraAtual, palavra, sizeof(an->palavraAtual) - 1);
    an->palavraAtual[sizeof(an->palavraAtual) - 1] = '\0';

    token t = analex(palavra);
    if (t == token_invalido)
        erro(an, "token invalido");
    return t;
}

static void compila_programa(Analisador *an)
{
    token t = proximoToken(an);
    if (t != programa)
        ERRO("esperava 'program' no inicio do programa");

    t = proximoToken(an);
    if (t != identificador)
        ERRO("esperava identificador apos 'program'");
    adicionaSimbolo(an, an->palavraAtual, "-", SIM_PROGRAMA);

    t = proximoToken(an);
    if (t == abreparenteses)
    {
        do
        {
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

    if (t != pontoevirgula)
        ERRO("esperava ';' apos cabecalho do programa");

    compila_bloco(an);

    t = proximoToken(an);
    if (t != ponto)
        ERRO("esperava '.' ao final do programa");

    t = proximoToken(an);
    if (t != fimdearquivo)
        ERRO("conteudo inesperado apos '.'");

    printf("Programa sintaticamente correto!\n");
}

static void compila_bloco(Analisador *an)
{
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

static void registraListaParametros(Analisador *an, char ids[][MAX_COMPRIMENTO], int totalIds, const char *tipoParam)
{
    if (!ehTipoConhecido(an, tipoParam))
        erro(an, "tipo nao declarado em parametros formais");

    for (int i = 0; i < totalIds; i++)
        adicionaSimbolo(an, ids[i], tipoParam, SIM_PARAMETRO);
}

static void compila_parametros_formais(Analisador *an)
{
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

static void compila_comando(Analisador *an, token *t)
{
    while (*t == numero)
    {
        *t = proximoToken(an);
        if (*t != doispontos)
            ERRO("esperava ':' apos rotulo numerico");
        *t = proximoToken(an);
    }

    compila_comando_sem_rotulo(an, t);
}

static void compila_comando_sem_rotulo(Analisador *an, token *t)
{
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

static void compila_expressao(Analisador *an, token *t)
{
    compila_expressao_simples(an, t);

    if (*t == igual || *t == diferente ||
        *t == menor || *t == menorouigual ||
        *t == maior || *t == maiorouigual)
    {
        *t = proximoToken(an);
        compila_expressao_simples(an, t);
    }
}

static void compila_expressao_simples(Analisador *an, token *t)
{
    if (*t == mais || *t == menos)
        *t = proximoToken(an);

    compila_termo(an, t);

    while (*t == mais || *t == menos || *t == ou)
    {
        *t = proximoToken(an);
        compila_termo(an, t);
    }
}

static void compila_termo(Analisador *an, token *t)
{
    compila_fator(an, t);

    while (*t == vezes || *t == dividir || *t == e)
    {
        *t = proximoToken(an);
        compila_fator(an, t);
    }
}

static void compila_fator(Analisador *an, token *t)
{
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

int main(int argc, char **argv)
{
    const char *nomeArquivo = argc > 1 ? argv[1] : "progtexte.txt";
    Analisador an;
    memset(&an, 0, sizeof(an));
    adicionaPreDefinido(&an, "read", SIM_PROCEDIMENTO);
    adicionaPreDefinido(&an, "write", SIM_PROCEDIMENTO);

    an.arquivo = fopen(nomeArquivo, "r");
    if (!an.arquivo)
    {
        fprintf(stderr, "Erro ao abrir arquivo '%s'\n", nomeArquivo);
        return 1;
    }

    compila_programa(&an);
    imprimeTabelaSimbolos(&an);

    fclose(an.arquivo);
    return 0;
}
