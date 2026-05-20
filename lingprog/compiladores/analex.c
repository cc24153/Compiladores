#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "declaracoes.h"

// Mariana Marietti da Costa - 24140
// Rafaelly Maria Nascimento da Silva - 24153
// Samuel Rosa Parra - 24155


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


// compilador lexico -> quebra em tokens:

static int leLinha(Analisador *an) {
    if (fgets(an->linha, MAX_COMPRIMENTO, an->arquivo) == NULL)
        return 0;   // se chegar no fim do arquivo, retorna 0 para indicar que não tem mais linhas para ler
    
    an->pos = 0;        // inicializa a posição para o início da linha
    an->numeroLinha++;  // incrementa o número da linha para imprimir na tabela de símbolos e mensagens de erro
    return 1;
}

static int ignoraComentario(Analisador *an) {
    // comentário entre chaves
    if (an->linha[an->pos] == '{')
        an->pos++;                  // pula a chave de abertura
        while (an->linha[an->pos] != '}')   // enquanto não encontrar a chave de fechamento
        {
            // se chegar no fim da linha sem encontrar a chave de fechamento, lê a próxima linha
            if (an->linha[an->pos] == '\0')
            {
                if (!leLinha(an))   // se chegar no fim do arquivo, retorna 0 para indicar que não tem mais linhas para ler
                {
                    strcpy(an->palavraAtual, "EOF");    // copia "EOF" (end of file) para a palavra atual para imprimir na mensagem de erro
                    erro(an, "comentario nao fechado");
                }
            }
            // se não chegar no fim da linha, continua procurando a chave de fechamento
            else
            {
                an->pos++;
            }
        }
        an->pos++;  // pula a chave de fechamento
        return 1;
    }

    // comentário entre (* *)
    if (an->linha[an->pos] == '(' && an->linha[an->pos + 1] == '*')
    {
        an->pos += 2;   // pula a sequência de abertura
        while (!(an->linha[an->pos] == '*' && an->linha[an->pos + 1] == ')'))   // enquanto não encontrar a sequência de fechamento
        {
            // se chegar no fim da linha sem encontrar a sequência de fechamento, lê a próxima linha
            if (an->linha[an->pos] == '\0')
            {
                if (!leLinha(an))
                {
                    strcpy(an->palavraAtual, "EOF");
                    erro(an, "comentario nao fechado");
                }
            }
            // se não chegar no fim da linha, continua procurando a sequência de fechamento
            else
            {
                an->pos++;
            }
        }
        an->pos += 2;  // pula a sequência de fechamento
        return 1;
    }

    return 0;   // se não for um comentário, retorna 0 para indicar que não ignorou nenhum comentário
}

// separaToken: recebe a linha e a posição atual, e retorna o próximo token como string
char *separaToken(Analisador *an) {
    // enquanto o caractere atual for espaço em branco, tabulação ou nova linha, ou se tiver um comentário, ignora e continua procurando o próximo token
    while (1) 
    {
        // enquanto o caractere atual for espaço em branco, tabulação ou nova linha,
        while (an->linha[an->pos] && isspace((unsigned char)an->linha[an->pos]))
            an->pos++;  // pula os caracteres em branco

        // se chegar no fim da linha, lê a próxima linha
        if (an->linha[an->pos] == '\0')
        {
            if (!leLinha(an))
                return NULL;
            continue;
        }

        // se tiver um comentário, ignora o comentário e continua procurando o próximo token
        if (ignoraComentario(an))
            continue;

        break;
    }

    int i = 0;  // índice para montar o token separado no buffer
    char c = an->linha[an->pos];    // caractere atual para verificar se é número, letra ou símbolo
    an->colunaToken = an->pos + 1;  // armazena a coluna onde o token separado começa para imprimir na tabela de símbolos e mensagens de erro

    // se o caractere atual for um dígito, monta um token de número, 
    if (isdigit((unsigned char)c))
    {
        while (isdigit((unsigned char)an->linha[an->pos]))  // enquanto o caractere atual for um dígito,
            an->buf[i++] = an->linha[an->pos++];            // adiciona o dígito ao buffer e avança para o próximo caractere
    }
    // se o caractere atual for uma letra ou underscore, monta um token de identificador ou palavra reservada,
    else if (isalpha((unsigned char)c) || c == '_')
    {
        while (isalnum((unsigned char)an->linha[an->pos]) || an->linha[an->pos] == '_') // enquanto o caractere atual for uma letra, dígito ou underscore,
            an->buf[i++] = (char)tolower((unsigned char)an->linha[an->pos++]);          // adiciona o caractere ao buffer (convertendo para minúsculo para comparar com as palavras reservadas) e avança para o próximo caractere
    }
    // se o caractere atual for um símbolo, monta um token de símbolo,
    else if ((c == ':' && an->linha[an->pos + 1] == '=') ||
             (c == '<' && an->linha[an->pos + 1] == '>') ||
             (c == '<' && an->linha[an->pos + 1] == '=') ||
             (c == '>' && an->linha[an->pos + 1] == '='))
    {
        an->buf[i++] = an->linha[an->pos++];
        an->buf[i++] = an->linha[an->pos++];
    }
    // se nao for nada, o token é o símbolo de um caractere, como '+', '-', '*', etc, então adiciona o símbolo ao buffer e avança para o próximo caractere
    else
    {
        an->buf[i++] = an->linha[an->pos++];
    }

    an->buf[i] = '\0';  // finaliza a string do token separado com o caractere nulo para que seja uma string válida, e para imprimir na tabela de símbolos e mensagens de erro
    return an->buf;
}

// proximoToken: chama separaToken para obter o próximo token como string, e depois chama analex para obter o token correspondente, 
// e armazena a string do token separado na palavra atual para imprimir na tabela de símbolos e mensagens de erro
token proximoToken(Analisador *an) {
    char *palavra = separaToken(an);    // chama separaToken para obter o próximo token como string
    if (palavra == NULL)                // se palavra for NULL, significa que chegou no fim do arquivo, 
    {
        strcpy(an->palavraAtual, "EOF");    // copia "EOF" (end of file) para a palavra atual para imprimir na mensagem de erro
        return fimdearquivo;
    }
    {
        strcpy(an->palavraAtual, "EOF");    // se palavra for uma string vazia, significa que chegou no fim do arquivo,
        return fimdearquivo;
    }

    strncpy(an->palavraAtual, palavra, sizeof(an->palavraAtual) - 1);   // copia a string do token separado para a palavra atual, limitando o tamanho para evitar overflow, e garantindo que seja uma string válida
    an->palavraAtual[sizeof(an->palavraAtual) - 1] = '\0';              // finaliza a string da palavra atual com o caractere nulo para que seja uma string válida, e para imprimir na tabela de símbolos e mensagens de erro

    token t = analex(palavra);  // chama analex para obter o token correspondente à string do token separado
    if (t == token_invalido)
        erro(an, "token invalido");
    return t;   // retorna o token correspondente para o analisador sintático usar na análise do código fonte
}


// execução principal deve começar pelo analisador sintático
// chama proximo token até chegar no fim do arquivo, e imprime cada token encontrado
// int main(void) {
//     Analisador an;          // cria o analisador léxico
//     an.pos = 0;             // inicializa a posição e a linha
//     an.linha[0] = '\0';     // inicializa a palavra atual com string vazia

//     // abre o arquivo para leitura
//     an.arquivo = fopen("exemplo.txt", "r");
//     if (!an.arquivo) {
//         printf("Erro ao abrir arquivo\n");
//         return 1;
//     }

//     // lê a primeira linha do arquivo
//     fgets(an.linha, MAX_COMPRIMENTO, an.arquivo);

//     token t;
//     // enquando proximo token for diferente de fim de arquivo, imprime o token e a palavra atual
//     while ((t = proximoToken(&an)) != fimdearquivo) {
//         printf("token=%-3d  texto=\"%s\"\n", t, an.palavraAtual);
//     }

//     fclose(an.arquivo);
//     return 0;
// }
