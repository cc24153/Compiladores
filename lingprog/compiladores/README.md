# Compilador Pascal para MEPA

Compilador didático de um subconjunto de Pascal, desenvolvido em C. O projeto realiza análise léxica, sintática e semântica, mantém uma tabela de símbolos com controle de escopo e gera código para a máquina virtual MEPA.

O código gerado é salvo em `saida.mepa` por padrão. A implementação trabalha com variáveis escalares e construções como atribuições, expressões, condicionais, laços, procedimentos, funções e comandos `read`/`write`. Arrays não são suportados.

---

### Pré-requisitos

##### 1. Instalar o Compilador (GCC)
O VS Code é apenas um editor de texto e precisa de um compilador externo para rodar o código C.

*   **Windows:** Baixe e instale o **MinGW** (certifique-se de marcar o pacote `mingw32-base-bin` durante a instalação).
    > ⚠️ **Importante:** Após a instalação, você deve adicionar o caminho da pasta `bin` (ex: `C:\MinGW\bin`) às **Variáveis de Ambiente (PATH)** do Windows para que o sistema reconheça o comando `gcc`.
*   **Linux (Ubuntu/Debian):** Abra o terminal e instale o pacote essencial build:
    ```bash
    sudo apt update && sudo apt install build-essential
    ```
*   **macOS:** Instale as ferramentas de linha de comando do Xcode rodando no terminal:
    ```bash
    xcode-select --install
    ```

##### 2. Extensões do VS Code
Abra a aba de extensões do VS Code (`Ctrl + Shift + X` ou `Cmd + Shift + X`) e instale:
*   **C/C++** (Desenvolvida pela Microsoft) – Oferece suporte a IntelliSense, debugging e formatação de código.
*   **Code Runner** (Opcional, mas altamente recomendada) – Permite executar o código com apenas um clique ou atalho.

---

### Como Rodar

##### Compilar

Compile os dois arquivos `.c` juntos, pois `anasin.c` usa o analisador léxico implementado em `analex.c`:

```bash
gcc -Wall -Wextra anasin.c analex.c -o compilador
```

No macOS, também é possível usar `clang` no lugar de `gcc`.

##### Executar

Passe o arquivo Pascal como primeiro argumento. O segundo argumento, que é opcional, define o nome do arquivo MEPA de saída:

```bash
./compilador exemplo.txt saida.mepa
```

Se o nome de saída for omitido, o compilador cria `saida.mepa` no diretório atual:

```bash
./compilador exemplo.txt
```

No Windows, execute `compilador.exe` em vez de `./compilador`:

```powershell
.\compilador.exe exemplo.txt saida.mepa
```

Após a compilação, `saida.mepa` contém as instruções geradas. Abra esse arquivo com o interpretador MEPA para executar o programa.

> **Observação:** `exemplo.txt` contém duas declarações de `f` no mesmo escopo. Como o compilador rejeita identificadores duplicados, remova ou renomeie uma delas antes de usar esse exemplo como entrada.

---

### Arquivos principais

- `analex.c`: separa o código-fonte em tokens;
- `anasin.c`: analisa a sintaxe e a semântica, mantém a tabela de símbolos e gera código MEPA;
- `declaracoes.h`: define tokens, símbolos e estruturas compartilhadas;
- `exemplo.txt`: exemplo de código-fonte Pascal.
