# Processamento Paralelo de Imagens

## Problemática e Objetivo

O projeto tem como objetivo a construção de um sistema de processamento paralelo para imagens, aplicando conceitos fundamentais de Sistemas Operacionais. Ele serve como um estudo prático de como processos e threads podem cooperar para otimizar o uso dos recursos do sistema, como memória e processamento.

A aplicação de técnicas de paralelismo em operações sobre pixels é ideal para aprimorar o desempenho, especialmente em imagens de grande porte. No entanto, a implementação dessas técnicas exige o uso de mecanismos de sincronização, como semáforos e mutexes, para garantir a consistência dos dados e evitar condições de corrida.

## Fundamentação Teórica

A solução explora a **comunicação entre processos (IPC)** através de **FIFOs** (named pipes) e **_shared memory_**, permitindo que processos independentes (`sender` e `worker`) troquem dados.

Dentro de um processo, a concorrência é alcançada com o uso de **threads**, que compartilham o mesmo espaço de memória. Essa abordagem é particularmente eficaz para processamento de imagens, onde as operações podem ser divididas e executadas em paralelo. Para garantir a integridade dos dados, o sistema utiliza **mutexes** para assegurar a exclusão mútua na fila de tarefas e **semáforos** para controlar o acesso e sinalizar eventos.

O projeto demonstra a aplicação de dois filtros de imagem adequados para paralelismo de dados:
* **Filtro Negativo**: Uma transformação linear simples onde `out = 255 – in`.
* **Limiarização com Fatiamento**: Mantém os valores de pixels dentro de uma faixa específica `[t1, t2]`, suprimindo os demais.

## Arquitetura da Solução

O sistema é composto por dois processos independentes que se comunicam para realizar o processamento de imagens:

1.  **Processo Emissor (`sender`)**: Responsável por carregar as imagens (formato PGM P5), extrair os metadados e enviar os dados de pixels para o `worker` por meio de memória compartilhada.

2.  **Processo Trabalhador (`worker`)**: Recebe os dados de imagem, instancia um *pool* de *threads* e distribui as tarefas em uma fila circular na memória compartilhada, que é protegida por semáforos e mutex. Cada thread processa uma parte da imagem, aplica o filtro especificado e, ao final, salva a imagem resultante em disco.

A sincronização entre o `sender` e o `worker` é garantida por:
* **Memória Compartilhada (`shm_open`)**: Uma fila circular (`unmgk_shm_queue`) que armazena as imagens a serem processadas.
* **Semáforos (`sem_open`)**: Usados para sincronizar o acesso à fila (mutex) e para sinalizar a presença de novas imagens (semáforo de itens).
* **FIFOs (`mkfifo`)**: Um *pipe* nomeado (`FIFO_ACK_UNMGCK_QUEUE`) é usado pelo `worker` para enviar confirmações de processamento de volta ao `sender`.

## Estrutura do Projeto

* `sender.c`: O produtor. Contém a lógica principal para carregar imagens e enviá-las para a fila de memória compartilhada.
* `worker.c`: O consumidor. Contém a lógica principal do `worker` que consome as imagens da fila, processa-as e envia uma confirmação de volta.
* `pthread_pool.c`: Implementação de um *thread pool* customizado para processamento paralelo das imagens.
* `shared_res.c`: Gerencia a criação e o acesso aos recursos de memória compartilhada, semáforos e FIFOs.
* `cli.c` e `cli_commands.c`: Implementam a interface de linha de comando do `sender`, utilizando a biblioteca `readline`.
* `Makefile`: Contém as regras de compilação e um atalho (`tmux_run`) para executar os processos `worker` e `sender` simultaneamente.

## Como Compilar e Executar

Este projeto utiliza `gcc` e as bibliotecas `pthread`, `math`, e `readline`. Para compilar, basta executar o seguinte comando:

```sh
make
````


### Requisitos

Certifique-se de que as bibliotecas de desenvolvimento necessárias estejam instaladas em seu sistema. Para sistemas baseados em Debian/Ubuntu, você pode usar:

```sh
sudo apt-get install libreadline-dev
```

-----

### Execução

O projeto é projetado para rodar os executáveis `sender` e `worker` simultaneamente. O `Makefile` já inclui um atalho para isso usando `tmux`:

```sh
make tmux_run
```

Isso abrirá uma sessão `tmux` com o `worker` e o `sender` em painéis separados.

Alternativamente, você pode rodar os executáveis em terminais separados:

**Terminal 1 (Worker):**

```sh
./build/worker
```

**Terminal 2 (Sender):**

```sh
./build/sender
```

Ao executar o `sender`, você verá o prompt `unmgk>` onde poderá digitar os comandos para processar as imagens.
