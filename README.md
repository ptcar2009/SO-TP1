# Simulação de um forno

## Para compilar

Para compilar para *release* (sem marcadores de *debug*) apenas rode o comando `make` na raiz do diretório. Esse comando irá criar um executável na pasta `bin/release`, com o nome `main`.

Para compilar para debugging, utilize o comando `make debug`, também a partir da raiz do diretório. Assim como o `make`, ele criará um executável que possui marcadores, `gdb`, além de uma cláusula de pre-compilação para não exigir argumentos na linha de comando, e simplesmente rodar com 3 iterações de forno.

## Para rodar

Caso queira rodar o programa diretamente, pode rodar o executável logo após ele ser compilado, utilizando (no caso da *release*) como argumento o número de vezes que cada pessoa vai ao forno.

Também é possível rodar o programa a partir do `Makefile`, rodando `make run`. Esse comando pega o número de iterações a partir do arquivo `.env`, onde tem uma variável `N_ITERATIONS` que é assinalada em tempo de execução.

## Para testar muitas vezes em paralelo

O comando `make test` inicializa 100 *background jobs* em paralelo, rodando o programa compilado em debug, e coloca a saída desses comandos em uma pasta `test`. É possível verificar se algum dos programas entrou em *deadlock* e verificando quantos *jobs* ainda estão rodando, e a saída  de cada job fica salva em um arquivo específico para ele.