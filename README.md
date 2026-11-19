# Algoritmo de Banker

Implementação 5 da disciplina de Infraestrutura de Software (CESAR School): algoritmo de Banker para evitar deadlock, escrito em C, a partir da versão apresentada na página P-45 do livro *Operating System Concepts* (Silberschatz et al., 10ª edição).

O número de clientes e o número de tipos de recurso não são fixos: o número de clientes vem das linhas do `customer.txt` e o número de recursos vem dos argumentos da linha de comando.

## Requisitos

- Linux, Unix ou macOS
- Compilador C com suporte a C11 (`cc`, `gcc` ou `clang`)
- `make`

## Compilação

```sh
make
```

Gera o executável `banker` na raiz do projeto. Para remover os arquivos compilados:

```sh
make clean
```

## Execução

```sh
./banker 10 5 7
```

Cada argumento é o número de instâncias de um tipo de recurso. O exemplo acima define três recursos, com 10, 5 e 7 instâncias.

O programa lê `customer.txt` e `commands.txt` do diretório atual e grava o resultado em `result.txt`, também no diretório atual.

## Arquivos de entrada

### customer.txt

Uma linha por cliente, com a demanda máxima de cada recurso separada por vírgula. O número de linhas é o número de clientes (numerados a partir de 0).

```
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```

### commands.txt

Um comando por linha:

| Comando | Significado |
| --- | --- |
| `RQ <cliente> <r1> <r2> ...` | cliente solicita recursos |
| `RL <cliente> <r1> <r2> ...` | cliente libera recursos |
| `*` | imprime o estado atual (MAXIMUM, ALLOCATION, NEED e AVAILABLE) |

```
RQ 0 0 2 1
RQ 2 3 0 2
RL 0 0 1 1
*
RQ 4 4 0 0
```

Linhas em branco são ignoradas em `commands.txt`. Em `customer.txt` são aceitas linhas em branco no início e no fim do arquivo, mas não entre clientes. Finais de linha `\n` e `\r\n` são aceitos nos dois arquivos.

## Saída

O arquivo `result.txt` recebe uma linha por comando:

| Situação | Mensagem |
| --- | --- |
| Requisição atendida | `Allocate to customer 0 the resources 0 2 1` |
| Liberação feita | `Release from customer 0 the resources 0 1 1` |
| Pedido maior que a necessidade restante | `The customer 2 request 0 1 0 was denied because exceed its maximum allocation` |
| Pedido maior que o disponível | `The resources 3 3 2 are not enough to customer 4 request 4 0 0 ` |
| Pedido que levaria a estado inseguro | `The customer 0 request 0 2 0 was denied because result in an unsafe state` |
| Liberação maior que o que o cliente possui | `The customer 0 release 2 0 was denied because exceed its current allocation` |
| Comando `*` | tabela `MAXIMUM`, `ALLOCATION`, `NEED` e linha `AVAILABLE` |

Para os arquivos de exemplo do enunciado, `./banker 10 5 7` produz:

```
Allocate to customer 0 the resources 0 2 1
Allocate to customer 2 the resources 3 0 2
Allocate to customer 1 the resources 2 0 0
Allocate to customer 4 the resources 0 0 2
Release from customer 0 the resources 0 1 1
Allocate to customer 3 the resources 2 1 1
MAXIMUM   |   ALLOCATION  |   NEED
7 5 3     |   0 1 0       |   7 4 3 
3 2 2     |   2 0 0       |   1 2 2 
9 0 2     |   3 0 2       |   6 0 0 
2 2 2     |   2 1 1       |   0 1 1 
4 3 3     |   0 0 2       |   4 3 1 
AVAILABLE 3 3 2 
The resources 3 3 2 are not enough to customer 4 request 4 0 0 
Allocate to customer 1 the resources 1 0 2
The customer 0 request 0 2 0 was denied because result in an unsafe state
The customer 2 request 0 1 0 was denied because exceed its maximum allocation
```

## Como cada requisição é decidida

1. Se o pedido é maior que a necessidade restante do cliente (`NEED`), ele é negado por exceder o máximo declarado.
2. Se o pedido é maior que o disponível (`AVAILABLE`), o cliente não é atendido e a mensagem mostra o disponível.
3. Caso contrário, o pedido é aplicado de forma provisória (`AVAILABLE`, `ALLOCATION` e `NEED` são atualizados) e o algoritmo de segurança roda: ele procura repetidamente um cliente cuja necessidade cabe no vetor `work` e devolve a esse vetor o que o cliente possui. Se todos os clientes conseguem terminar, o estado é seguro e o pedido é concedido. Caso contrário, o estado anterior é restaurado e o pedido é negado.

## Tratamento de erros

O programa valida tudo antes de começar a executar os comandos. Quando encontra um problema, escreve uma mensagem em `stderr` no formato `banker: error: ...`, encerra com status 1 e não cria o `result.txt`. Os casos tratados são:

- nenhum argumento, ou argumento que não é um inteiro não negativo;
- número de argumentos diferente do número de valores por linha em `customer.txt` (para mais ou para menos);
- número de recursos em um comando diferente do número de argumentos;
- `customer.txt` ou `commands.txt` inexistente, sem permissão de leitura, vazio ou que é um diretório;
- valores inválidos ou vazios em `customer.txt` (letras, negativos, vírgula dupla ou final) e linha em branco entre clientes;
- bytes NUL em `customer.txt` ou `commands.txt` (arquivo binário ou UTF-16);
- comando desconhecido, cliente inexistente, valores faltando ou sobrando e texto depois de `*` em `commands.txt`;
- falha ao criar ou gravar o `result.txt`.

Se algum cliente declara um máximo maior que o total de instâncias de um recurso, o programa emite um aviso em `stderr` e continua: esse cliente nunca poderá terminar, então qualquer pedido que dependa dele será negado por estado inseguro.

## Testes

```sh
make test
```

Compila e roda os casos de `tests/fixtures`, que cobrem o exemplo do enunciado, finais de linha CRLF, linhas em branco, dois tipos de recurso e cada um dos erros listados acima. Cada caso tem `args`, os arquivos de entrada e `expected.txt` (saída esperada byte a byte) ou `error` (trecho esperado na mensagem de erro, com status 1 e sem `result.txt`).

```sh
make sanitize
```

Recompila com AddressSanitizer e UndefinedBehaviorSanitizer, roda a mesma bateria e limpa os arquivos gerados.

## Estrutura

```
Makefile
src/
  main.c      fluxo principal e execução dos comandos
  banker.c    estado do banco, requisição, liberação e algoritmo de segurança
  banker.h
  input.c     leitura e validação de argumentos, customer.txt e commands.txt
  input.h
  output.c    formatação das mensagens e da tabela do result.txt
  output.h
  error.c     mensagens de erro e aviso em stderr
  error.h
tests/
  run.sh      executa os casos de tests/fixtures
  fixtures/   entradas e saídas esperadas
```

## Fluxo de branches

O repositório segue Gitflow simplificado: `main` recebe apenas versões entregues, `develop` integra o trabalho e cada parte foi desenvolvida em uma branch `feature/*` própria (núcleo do algoritmo, leitura de entrada, saída, Makefile, testes e documentação), integrada com `merge --no-ff`.
