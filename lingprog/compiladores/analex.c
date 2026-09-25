#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Mariana Marietti da Costa - 24140
// Rafaelly Maria Nascimento da Silva - 24153
// Samuel Rosa Parra - 24155


#define MAX_COMPRIMENTO 256
#define NUM_PALAVRAS 36


// cria o tipo tokens como enumeração, cada token tem um numero inteiro correspondente
typedef enum {
    programa, rotulo, tipo, variavel, procedimento,
    funcao, inicio, fim, atribuicao, se, entao, senao,
    enquanto, faca, vapara, mais, menos, vezes, dividir,
    igual, diferente, menor, menorouigual, maior, maiorouigual,
    e, ou, nao, abreparenteses, fechaparenteses, fechacolchetes,
    abrecolchetes, virgula, pontoevirgula, doispontos, ponto,
    numero, identificador, fimdearquivo
} token;


// vetor de strings que podem aparecer no código, cada string tem um token correspondente no vetor de tokens
char *palavras[NUM_PALAVRAS] = {
    "program","label","type","var","procedure",
    "function","begin","end",":=","if","then","else",
    "while","do","goto","+","-","*","div",
    "=","<>","<","<=",">",">=",
    "and","or","not","(",")","]",
    "[",",",";",":","."
};

// vetor de tokens correspondente ao vetor de strings que cada indice tem o tipo token criado acima
token tokens[NUM_PALAVRAS] = {
    programa, rotulo, tipo, variavel, procedimento,
    funcao, inicio, fim, atribuicao, se, entao, senao,
    enquanto, faca, vapara, mais, menos, vezes, dividir,
    igual, diferente, menor, menorouigual, maior, maiorouigual,
    e, ou, nao, abreparenteses, fechaparenteses, fechacolchetes,
    abrecolchetes, virgula, pontoevirgula, doispontos, ponto
};


// palavras[i] = tokens[i]
// Analex: recebe uma palavra e retorna o token correspondente
token analex(char *palavra) {
    for (int i = 0; i < NUM_PALAVRAS; i++)
        if (strcmp(palavra, palavras[i]) == 0)  // verifica se a palavra é uma palavra reservada ou símbolo
            return tokens[i];   // se for, retorna o token correspondente

    // se não for uma palavra reservada ou símbolo, verifica se é um número ou identificador
    for (int i = 0; palavra[i] != '\0'; i++)
        if (!isdigit((unsigned char)palavra[i])) // se tiver um caractere que não seja número, é um identificador
            return identificador;

    return numero; // se todos os caracteres forem números, é um número
}


// para classificar uma palavra do nosso arquivo como token, precisamos ler o arquivo, separar em palavras e somente depois classificar em tokens

// estrutura para guardar o estado do analisador léxico, com o arquivo aberto, a linha atual, a posição atual, o buffer para montar o token e a palavra atual para imprimir depois
typedef struct {
    FILE *arquivo;       // o arquivo aberto
    char  linha[256];    // a linha que está sendo lida agora
    int   pos;           // qual caractere da linha estou olhando agora, indice
    char  buf[256];      // onde vou montar o token
    char  palavraAtual[256]; // guarda o token depois de pronto
} Analisador;


// compilador lexico -> quebra em tokens

// separaToken: recebe a linha e a posição atual, e retorna o próximo token como string
char *separaToken(Analisador *an) {
    // enquanto o caractere atual for espaço, tab ou nova linha,
    while (an->linha[an->pos] && isspace((unsigned char)an->linha[an->pos]))
            // an pega o proximo caracter de linha e verifica se é espaço, tab ou nova linha, se for, pula o caractere
        an->pos++;  // pula o espaço

    // se chegar no fim da linha, ou seja, o caractere for nulo,
    if (!an->linha[an->pos])
        return NULL;    // retorna NULL para indicar que não tem mais token nessa linha

    int i = 0;  // indice para montar o token no buffer

    char c = an->linha[an->pos];     // olha o primeiro caractere para decidir o tipo do token

    // caso seja um número, continua lendo enquanto for número
    if (isdigit((unsigned char)c)) {
        while (isdigit((unsigned char)an->linha[an->pos]))  // enquanto o caractere for número
            an->buf[i++] = an->linha[an->pos++];   
            // na proxima posicao do buffer coloca o caractere atual da linha e depois incrementa a posição para ler o próximo caractere, continua lendo enquanto for número

    }
    // caso seja uma letra ou _, continua lendo enquanto for letra, número ou _
    else if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)an->linha[an->pos]) || an->linha[an->pos] == '_')
          // isalnum verifica se é letra ou número, ou seja, continua lendo enquanto for letra, número ou _
            an->buf[i++] = an->linha[an->pos++];  

    }
    // caso seja um símbolo, verifica se é um símbolo de dois caracteres, senão é um símbolo de um caractere
    else if (
        (c == ':' && an->linha[an->pos+1] == '=') ||
        (c == '<' && an->linha[an->pos+1] == '>') ||
        (c == '<' && an->linha[an->pos+1] == '=') ||
        (c == '>' && an->linha[an->pos+1] == '=')
    ) {
        an->buf[i++] = an->linha[an->pos++];
        an->buf[i++] = an->linha[an->pos++]; 

    }
    // caso seja um símbolo de um caractere, lê o símbolo
    else {
        an->buf[i++] = an->linha[an->pos++];
    }

    an->buf[i] = '\0';  // termina a string do token
    return an->buf;     // retorna o token como string
}

// ProximoToken: lê o próximo token do arquivo, usando separaToken para ler a linha e separar o token
token proximoToken(Analisador *an) {
    char *palavra;

    // enquanto separaToken retornar NULL, ou seja,
    // enquanto não tiver um token pronto para o que ele ta lendo, continua lendo ate ter um token pronto ou chegar no fim do arquivo
    while ((palavra = separaToken(an)) == NULL) {
        if (fgets(an->linha, MAX_COMPRIMENTO, an->arquivo) == NULL) // se chegar no fim do arquivo, ou seja, linha dor null,
            return fimdearquivo; // retorna o token de fim de arquivo
        an->pos = 0;    // se não, reseta a posição para o começo da linha e continua lendo
    }

    strcpy(an->palavraAtual, palavra);  // guarda a palavra atual para imprimir depois
    return analex(palavra);             // retorna o token correspondente a palavra lida
}


// chama proximo token até chegar no fim do arquivo, e imprime cada token encontrado
int main(void) {
    Analisador an;          // cria o analisador léxico
    an.pos = 0;             // inicializa a posição e a linha
    an.linha[0] = '\0';     // inicializa a palavra atual com string vazia

    // abre o arquivo para leitura
    an.arquivo = fopen("progtexte.txt", "r");
    if (!an.arquivo) {
        printf("Erro ao abrir arquivo\n");
        return 1;
    }

    // lê a primeira linha do arquivo
    fgets(an.linha, MAX_COMPRIMENTO, an.arquivo);

    token t;
    // enquando proximo token for diferente de fim de arquivo, imprime o token e a palavra atual
    while ((t = proximoToken(&an)) != fimdearquivo) {
        printf("token=%-3d  texto=\"%s\"\n", t, an.palavraAtual);
    }

    fclose(an.arquivo);
    return 0;
}
