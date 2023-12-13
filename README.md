
# Desenvolvimento de um Sistema de Arquivos Simulando Padrão EXT3
## Objetivo
O objetivo deste projeto é desenvolver um sistema de arquivos que simule o padrão EXT3. O sistema será implementado em C ou C++, utilizando o compilador GNU GCC e chamadas de sistemas do padrão POSIX.

## Estrutura do Projeto
O código fonte do sistema de arquivos deve ser organizado nos seguintes arquivos:

fs.h: Declaração das funções que serão implementadas no arquivo fs.cpp.
fs.cpp: Implementação das funções declaradas em fs.h para manipular um arquivo em disco simulando um sistema de arquivo EXT3.
main.cpp: Contém um conjunto de testes unitários que serão utilizados para avaliar a correta implementação do sistema. Os testes fazem uso da biblioteca OpenSSL e Google Test.
sha256.cpp: Implementação de funções relacionadas ao algoritmo de hash SHA-256.
Execução e Compilação
Para compilar o projeto, é necessário ter as bibliotecas OpenSSL (libssl) 1.1 e Google Test instaladas. O comando de compilação pode ser realizado usando o GNU GCC da seguinte maneira:
*g++ -std=c++17 fs.cpp main.cpp sha256.cpp -o <nome do executável> -lgtest -lcrypto -lpthread*
O CMake também pode ser utilizado como alternativa ao comando acima.

## Testes
O arquivo main.cpp contém um conjunto de testes unitários que serão utilizados para avaliar a implementação do sistema de arquivos. Os testes são executados automaticamente e fazem uso da biblioteca OpenSSL e Google Test.

## Observações Importantes
Os arquivos fs-case*.bin são sistemas de arquivos inicializados pelo professor e são utilizados nos testes 4 a 6. Eles podem ser baixados e examinados com um editor hexadecimal.
O Moodle faz uma cópia desses arquivos a cada execução, modifica a cópia nos testes e apaga a cópia modificada.
Para voltar ao estado inicial dos arquivos, é possível utilizar a opção "Mostrar mais... -> Reset files". Nesse caso, qualquer modificação feita será perdida.
O Moodle identifica vazamentos de memória utilizando Valgrind, sendo que cada vazamento subtrai um ponto da nota.
Certifique-se de seguir as instruções e boas práticas durante o desenvolvimento e teste do sistema de arquivos.
