# Nome do Projeto 🚀

#### Descricao do Projeto

---

#### 🛠️ Pré-requisitos

Antes de começar, você precisa garantir que o compilador de C e o editor de código estejam devidamente instalados no seu computador.

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

#### 💻 Como Rodar o Projeto

##### 3. Executando o Código

Existem duas formas principais de rodar o programa:

###### Opção A: Utilizando a extensão Code Runner (Mais Fácil)
* Abra o seu arquivo `.c`.
* Clique no ícone de **Play (Run Code)** localizado no canto superior direito do VS Code, ou utilize o atalho de teclado:
  * **Windows/Linux:** `Ctrl + Alt + N`
  * **macOS:** `Cmd + Option + N`

###### Opção B: Compilação Manual via Terminal Integrado
Se preferir usar o terminal interno do VS Code (`Ctrl + '` ou `Cmd + '`), digite os seguintes comandos:

1. **Compilar o arquivo:**
   ```bash
   gcc main.c -o main
   ```
2. **Executar o programa gerado:**
   *No Windows:*
   ```bash
   .\main.exe
   ```
   *No Linux/macOS:*
   ```bash
   ./main
   ```

---
