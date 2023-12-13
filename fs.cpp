#include "fs.h"
#include <cmath>
#include <fstream>
#include <cstring>
#include <bitset>

using namespace std;

int tamVectorInodes, tamVectorBlocks, idInodeLivre;
char blockSize, numBlocks, numInodes, tamMapaBits, n[3];     

void initFs(string fsFileName, int blockSize, int numBlocks, int numInodes){

    /**
    * @brief Inicializa um sistema de arquivos que simula EXT3
    * @param fsFileName nome do arquivo que contém sistema de arquivos que simula EXT3 (caminho do arquivo no sistema de arquivos local)
    * @param blockSize tamanho em bytes do bloco
    * @param numBlocks quantidade de blocos
    * @param numInodes quantidade de inodes
    */

    fstream file{fsFileName, ios::out | ios::trunc | ios::binary};
    INODE inodeBarra{};

    tamMapaBits = ceil(numBlocks/8.0);

    int size = 3 + ceil(numBlocks/8.0) + numInodes*sizeof(INODE) + 1 + blockSize * numBlocks;

    char *mapa = new char[tamMapaBits]{};
    char a = 0x00;

    if(!file.is_open()){
        exit(EXIT_FAILURE);
    }

    // Preenche o arquivo com bytes nulos para reservar espaço
    for(auto i = 0; i < size; i++){
        file.write(&a , 1) ;
    }

    // Escreve informações iniciais no arquivo
    file.seekp(0, ios::beg);
    file.write((char *)&blockSize, 1);
    file.write((char *)&numBlocks, 1);
    file.write((char *)&numInodes, 1);

    mapa[0] = 0x01;
    file.write(mapa, tamMapaBits);
    
    // Configura as informações do inode para o diretório raiz
    inodeBarra.IS_USED = 0x01;
    inodeBarra.IS_DIR = 0x01;
    inodeBarra.NAME[0] = '/';

    file.write((char *) &inodeBarra, sizeof(INODE)); // Escreve o inode do diretório raiz no arquivo
    file.close();
}

void addFile(string fsFileName, string filePath, string fileContent){

    /**
    * @brief Adiciona um novo arquivo dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
    * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
    * @param filePath caminho completo novo arquivo dentro sistema de arquivos que simula EXT3.
    * @param fileContent conteúdo do novo arquivo
    */

    fstream file(fsFileName, ios::in | ios::out | ios::binary);
    INODE inode{};
    bitset<8> mapa;
    
    file.read((char *) &n, 3);     
    
    // Recupera os valores do sistema de arquivos
    blockSize = n[0];
    numBlocks = n[1];
    numInodes = n[2];

    // Calcula tamanhos e contadores  
    tamVectorInodes = numInodes * sizeof(INODE); 
    tamVectorBlocks = numBlocks * blockSize;   
    tamMapaBits = ceil(numBlocks / 8.0);   

    int contadorDiretorios = 0, idBarra = 0, qtdFilhos = 1, blocosOcupados = 0; 
    int tamFileContent = fileContent.length();
    int initialPos = tamMapaBits + 3;

    char idBlocoLivre = 0, tamDiretorio = 0, qtdBlocoUsado = 0, nomeDiretorio[10], localDoArquivo[10];
    char Blocos = ceil(tamFileContent / (double)blockSize);
    char dadosFile[fileContent.length()];
    char inodeOcupado[2] = {0x01, 0x00};
    char dadosBlocos[tamVectorBlocks];     
    char blocosDeDiretorio[] = {1, 2};

    file.seekg((initialPos), ios::beg);

    // Procura um inode livre
    for (int i = 0; i < numInodes; i++){
        file.read((char *)&inode, sizeof(INODE)); 

        if (inode.IS_USED == 0){
            idInodeLivre = i;
            break;
        }
    }
    
    for (size_t i = 0; i < filePath.length(); i++){
        if (filePath.at(i) == '/'){
            idBarra = i;
            contadorDiretorios++;
        }
    }

    // Configura nomes de diretório e caminho
    bool blocoNomesDiretorio = false, blocoLocalDoArquivo = false;
    int i = 0;

    while (i < sizeof(localDoArquivo) && i < filePath.length()){
        if (contadorDiretorios == 1){
            localDoArquivo[i] = (i + 1) < filePath.length() ? filePath.at(i + 1) : 0;
            nomeDiretorio[i] = filePath.at(i) == '/' ? '/' : 0;
        }
        else{
            nomeDiretorio[i] = ((i + 1) < filePath.length() && filePath.at(i + 1) != '/' && !blocoNomesDiretorio) ? filePath.at(i + 1) : 0;
            blocoNomesDiretorio = nomeDiretorio[i] == 0;

            localDoArquivo[i] = ((idBarra + 1) < filePath.length() && filePath.at(idBarra + 1) != '/' && !blocoLocalDoArquivo) ? filePath.at(idBarra + 1) : 0;
            blocoLocalDoArquivo = localDoArquivo[i] == 0;

            idBarra++;
        }
        i++;
    }

    file.seekg((initialPos + tamVectorInodes + 1), ios::beg);
    file.read((char *) &dadosBlocos, sizeof(dadosBlocos)); 

    // Atualiza informações do diretório pai
    if (nomeDiretorio[0] != '/'){
        file.seekg((initialPos), ios::beg);

        for (int i = 0; i < numInodes; i++){
            file.read((char *)&inode, sizeof(INODE));

            if (strcmp(inode.NAME, nomeDiretorio) == 0){
                file.seekg((initialPos + i * sizeof(INODE) + 12), ios::beg);
                file.read(&tamDiretorio, 1); 
                file.seekp(-1, ios::cur);

                tamDiretorio++;

                file.write(&tamDiretorio, 1); 
                file.seekp((initialPos + tamVectorInodes + blockSize * inode.DIRECT_BLOCKS[0] + 1), ios::beg);
                file.write((char *)&idInodeLivre, 1); 
                file.seekp((initialPos + i * sizeof(INODE) + sizeof(INODE)), ios::beg);
            }
        }
    }

    // Encontra o próximo bloco livre
    file.seekg((initialPos), ios::beg);

    for (int i = 0; i < numInodes; i++){
        file.read((char *)&inode, sizeof(INODE)); 

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++){
            if (inode.DIRECT_BLOCKS[j] > idBlocoLivre){
                idBlocoLivre = inode.DIRECT_BLOCKS[j];
            }
        }
    }

    idBlocoLivre++;

    // Escreve informações do inode do arquivo
    file.seekp((initialPos + sizeof(INODE) * idInodeLivre), ios::beg);
    file.write((char *)&inodeOcupado, 2);
    file.write((char *)&localDoArquivo, 10);
    file.write((char *)&tamFileContent, 1); 

    // Escreve os blocos de dados do arquivo
    for (size_t i = 0; i < Blocos; i++){
        file.write((char *)&idBlocoLivre, 1); 
        idBlocoLivre++;
    }

    for (size_t i = 0; i < fileContent.length(); i++){
        dadosFile[i] = fileContent.at(i);
    }

    // Ajustes para salvar índice e conteúdo em blocos
    if (nomeDiretorio[0] != '/'){
        file.seekp((initialPos + tamVectorInodes + 1), ios::beg);
        file.write((char *) &blocosDeDiretorio, 2); 
    
        file.seekp((initialPos + tamVectorInodes + blockSize * (idBlocoLivre - Blocos) + 1), ios::beg);
        file.write((char *)&dadosFile, fileContent.length()); 
        qtdFilhos = 2;
    }else{
        char blocosDeDiretorio[] = {1, 0};

        file.seekp((initialPos + tamVectorInodes + 1), ios::beg);
        file.write((char *)&blocosDeDiretorio, 2);                        
        file.write((char *)&dadosFile, fileContent.length()); 
    }

    // Ajuste para o número de filhos
    file.seekp((initialPos + 12), ios::beg);
    file.write((char *)&qtdFilhos, 1); 
    file.seekg((initialPos), ios::beg);

    for (int i = 0; i < numInodes; i++){
        file.read((char *)&inode, sizeof(INODE)); 

        for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++){
            if (inode.DIRECT_BLOCKS[j] > qtdBlocoUsado){
                qtdBlocoUsado = inode.DIRECT_BLOCKS[j];
            }
        }
    }

    // Calcula o valor em hexadecimal do número de blocos preenchidos  
    for (size_t i = 0; i <= qtdBlocoUsado; i++){
        mapa.set(i);
    }

    // Converte o mapa de bits em um valor inteiro
    unsigned long BintoHex = mapa.to_ulong();

    // Escreve o valor hexadecimal no sistema de arquivos
    file.seekp(3, ios::beg);
    file.write((char *)&BintoHex, 1); 

    file.close();
}

void addDir(string fsFileName, string dirPath){
    
    /**
    * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
    * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
    * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
    */

    fstream file(fsFileName, ios::in | ios::out | ios::binary);
    INODE inode;
    bitset<8> mapa;

    file.read((char *)&n, 3); 
    blockSize = n[0];
    numBlocks = n[1];
    numInodes = n[2];

    tamVectorInodes = numInodes * sizeof(INODE); 
    tamVectorBlocks = numBlocks * blockSize;     
    tamMapaBits = ceil((numBlocks - 1)/8.0);   
    
    int qtdInodesOcupados = -1, Blocos = 1, valorNoBlocoRaiz = 2, idBlocoLivre = 0; 
    int initialPos = tamMapaBits + 3;

    char Diretorio[] = {0x01, 0x01};
    char dadosBlocos[tamVectorBlocks];
    char nomeDiretorio[10]; 

    file.seekg((initialPos + tamVectorInodes + 1), ios::beg);
    file.read((char *)&dadosBlocos, sizeof(dadosBlocos)); 

    // Encontra o primeiro bloco livre
    for (size_t i = 0; i < sizeof(dadosBlocos); i += blockSize){
        if (dadosBlocos[i] == 0){
            idBlocoLivre = i / 2;
            break;
        }
    }
    
    file.seekg((initialPos), ios::beg);

    // Procura um inode livre
    for (int i = 0; i < numInodes; i++){
        file.read((char *)&inode, sizeof(INODE)); 

        if (inode.IS_USED == 0){
            idInodeLivre = i;
            break;
        }
    }

    // Define o nome do diretório
    for (size_t i = 0; i < sizeof(nomeDiretorio); i++){
        if (i + 1 < dirPath.length() && i + 1 < dirPath.length()){
            nomeDiretorio[i] = dirPath.at(i + 1);
        }
        else{
            nomeDiretorio[i] = 0;
        }
    }

    // Escreve informações do diretório
    file.seekp((initialPos + idInodeLivre * sizeof(INODE)), ios::beg);
    file.write((char *)&Diretorio, 2); 
    file.write((char *)&nomeDiretorio, 10);     
    file.seekp(1, ios::cur);
    file.write((char *)&idBlocoLivre, 1); 
    file.seekg((initialPos), ios::beg);

    // Conta o número de inodes preenchidos e blocos usados
    for (int i = 0; i < numInodes; i++){
        file.read((char *)&inode, sizeof(INODE)); 

        if (inode.IS_USED == 1){
            for (int i = 0; i < 3; i++){
                if (inode.DIRECT_BLOCKS[i] > 0){
                    Blocos++;
                }
            }
            qtdInodesOcupados++;
        }
    }


    // Calcula o valor do mapa de bits
    for (size_t i = 0; i < Blocos; i++){
        mapa.set(i);
    }

    // Atualiza o mapa de bits e informações de diretório raiz
    file.seekp(3, ios::beg);
    unsigned long mapaValue = mapa.to_ulong();
    file.write((char *)&mapaValue, 1); 
    file.seekp((initialPos + 12), ios::beg);
    file.write((char *)&qtdInodesOcupados, 1); 
    file.seekp((initialPos + tamVectorInodes + 2), ios::beg);
    file.write((char *)&valorNoBlocoRaiz, 1); 

    file.close();
}

void remove(string fsFileName, string path){

    /**
    * @brief Remove um arquivo ou diretório (recursivamente) de um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
    * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
    * @param path caminho completo do arquivo ou diretório a ser removido.
    */

    fstream file(fsFileName, ios::in | ios::out | ios::binary);
    INODE inode;
    INODE newInode;
    bitset<8> mapa; 

    file.read((char *)&n, 3);     
    
    // Atribuindo valores a partir do array n
    blockSize = n[0];
    numBlocks = n[1];
    numInodes = n[2];
      
    tamVectorInodes = numInodes * sizeof(INODE); 
    tamVectorBlocks = numBlocks * blockSize;   
    tamMapaBits = ceil(numBlocks / 8.0);   

    int idLocalDoArquivo = 0, idDiretorio = 0, contadorDiretorios = 0, idBarra = 0,  contadorInodeOcupado = -1;
    int initialPos = tamMapaBits + 3; 

    char nomeDiretorio[10], localDoArquivo[10], idInodeOcupado = 0, tamDiretorio = 0, inodeLivre[22];

    // Encontra barras no caminho e divide nomes de diretório
    for (size_t i = 0; i < path.length(); i++){
        if (path.at(i) == '/'){
            contadorDiretorios++;
            idBarra = i;
        }
    }

    // Iterando sobre o caminho do arquivo    
    bool blocoNomesDiretorio = false, blocoLocalDoArquivo = false;
    int i = 0;

    while (i < sizeof(localDoArquivo) && i < path.length()){
        if (contadorDiretorios == 1){
            localDoArquivo[i] = (i + 1) < path.length() ? path.at(i + 1) : 0;
            nomeDiretorio[i] = ((i + 1) < path.length() && path.at(i) == '/') ? '/' : 0;
        }
        else{
            nomeDiretorio[i] = ((i + 1) < path.length() && path.at(i + 1) != '/' && !blocoNomesDiretorio) ? path.at(i + 1) : 0;
            blocoNomesDiretorio = nomeDiretorio[i] == 0;

            localDoArquivo[i] = ((idBarra + 1) < path.length() && path.at(idBarra + 1) != '/' && !blocoLocalDoArquivo) ? path.at(idBarra + 1) : 0;
            blocoLocalDoArquivo = localDoArquivo[i] == 0;

            idBarra++;
        }
        i++;
    }

    file.seekg((initialPos), ios::beg);

    // Procura índices de inodes pelo nome    
    for (int i = 0; i < numInodes; i++){
        file.read((char *)&inode, sizeof(INODE)); 

        if (strcmp(inode.NAME, localDoArquivo) == 0){
            idLocalDoArquivo = i;
        }
        if (strcmp(inode.NAME, nomeDiretorio) == 0){
            idDiretorio = i;
        }
    }

    // Preenche o array vazio de inode
    for (size_t i = 0; i < sizeof(INODE); i++){
        inodeLivre[i] = 0;
    }

    // Remove inode do caminho do arquivo
    file.seekp((initialPos + sizeof(INODE) * idLocalDoArquivo), ios::beg);
    file.write((char *)&inodeLivre, sizeof(INODE));

    // Atualiza o tamanho do diretório pai
    file.seekg((initialPos + idDiretorio * sizeof(INODE) + 12), ios::beg);
    file.read((char *)&tamDiretorio, 1);
    file.seekp(-1, ios::cur);
    tamDiretorio--;
    file.write((char *)&tamDiretorio, 1); 

    mapa.set(0);  // O primeiro bloco é usado

    // Atualiza o mapa de bits com os blocos usados
    file.seekg((initialPos), ios::beg);
    for (int i = 0; i < numInodes; i++){
        file.read((char *)&newInode, sizeof(INODE)); 

        for (size_t j = 0; j < sizeof(newInode.DIRECT_BLOCKS); j++){
            if (newInode.DIRECT_BLOCKS[j] != 0){
                mapa.set(newInode.DIRECT_BLOCKS[j]);
            }
        }
    }

    // Atualiza o mapa de bits no arquivo
    file.seekp(3, ios::beg);
    unsigned long BintoHex = mapa.to_ulong();
    file.write((char *)&BintoHex, 1); 

    file.seekg((initialPos), ios::beg);

    for (int i = 0; i < numInodes; i++){
        file.read((char *)&newInode, sizeof(INODE)); 

        if (newInode.IS_USED == 1){
            idInodeOcupado = i;
            contadorInodeOcupado++;
        }
    }

    // Atualiza o índice do inode usado se houver apenas 1 inode usado
    if (contadorInodeOcupado == 1){
        file.seekp((initialPos + tamVectorInodes + 1), ios::beg);
        file.write((char *)&idInodeOcupado, 1);
    }

    file.close();
}

void move(string fsFileName, string oldPath, string newPath){

    /**
    * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
    * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
    * @param oldPath caminho completo do arquivo ou diretório a ser movido.
    * @param newPath novo caminho completo do arquivo ou diretório.
    */
   
    fstream file(fsFileName, ios::in | ios::out | ios::binary);
    INODE inode;
    INODE inodeToRemoveBlock;
    INODE newInode;
    bitset<8> mapa;

    file.read((char *)&n, 3); 

    blockSize = n[0];
    numBlocks = n[1];
    numInodes = n[2];
      
    tamVectorInodes = numInodes * sizeof(INODE); 
    tamVectorBlocks = numBlocks * blockSize;   
    tamMapaBits = ceil(numBlocks / 8.0);     

    int idLocalDoArquivo = 0, idOldDiretorio = 0, idNewDiretorio = 0, contadorDiretorios = 0, idBarra = 0, increaseid = 0, usedBlocks = 0;
    int initialPos = tamMapaBits + 3;

    char nomeDiretorio[10], localDoArquivo[10], newNomeDiretorio[10], newlocalDoArquivo[10], tamDiretorio = 0, idBlocoLivre = 0; 

    // Encontra barras no caminho antigo
    for (size_t i = 0; i < oldPath.length(); i++){
        if (oldPath.at(i) == '/'){
            contadorDiretorios++;
            idBarra = i;
        }
    }

    // Divide nomes de diretório no caminho antigo
    bool blocoNomesDiretorio = false, blocoLocalDoArquivo = false;
    int i = 0;

    while (i < sizeof(localDoArquivo) && i < oldPath.length()){
        if (contadorDiretorios == 1){
            localDoArquivo[i] = (i + 1) < oldPath.length() ? oldPath.at(i + 1) : 0;
            nomeDiretorio[i] = ((i + 1) < oldPath.length() && oldPath.at(i) == '/') ? '/' : 0;
        }
        else{
            nomeDiretorio[i] = ((i + 1) < oldPath.length() && oldPath.at(i + 1) != '/' && !blocoNomesDiretorio) ? oldPath.at(i + 1) : 0;
            blocoNomesDiretorio = nomeDiretorio[i] == 0;

            localDoArquivo[i] = ((idBarra + 1) < oldPath.length() && oldPath.at(idBarra + 1) != '/' && !blocoLocalDoArquivo) ? oldPath.at(idBarra + 1) : 0;
            blocoLocalDoArquivo = localDoArquivo[i] == 0;

            idBarra++;
        }
        i++;
    }

    contadorDiretorios = 0;
    idBarra = 0;                            

    // Encontra barras no novo caminho
    for (size_t i = 0; i < newPath.length(); i++){
        if (newPath.at(i) == '/'){
            contadorDiretorios++;
            idBarra = i;
        }
    }

    // Divide nomes de diretório no novo caminho
    bool blocknewNomeDiretorio = false, blocknewlocalDoArquivo = false; 
    i = 0;

    while (i < sizeof(newlocalDoArquivo) && i < newPath.length()){
        if (contadorDiretorios == 1){
            newlocalDoArquivo[i] = (i + 1) < newPath.length() ? newPath.at(i + 1) : 0;
            newNomeDiretorio[i] = ((i + 1) < newPath.length() && newPath.at(i) == '/') ? '/' : 0;
        }
        else{
            newNomeDiretorio[i] = ((i + 1) < newPath.length() && newPath.at(i + 1) != '/' && !blocknewNomeDiretorio) ? newPath.at(i + 1) : 0;
            blocknewNomeDiretorio = newNomeDiretorio[i] == 0;

            newlocalDoArquivo[i] = ((idBarra + 1) < newPath.length() && newPath.at(idBarra + 1) != '/' && !blocknewlocalDoArquivo) ? newPath.at(idBarra + 1) : 0;
            blocknewlocalDoArquivo = newlocalDoArquivo[i] == 0;

            idBarra++;
        }
        i++;
    }
  
    file.seekg((initialPos), ios::beg);

    char* names[] = {localDoArquivo, nomeDiretorio, newNomeDiretorio};
    int* indices[] = {&idLocalDoArquivo, &idOldDiretorio, &idNewDiretorio};

    // Procura índices de inodes pelos nomes no caminho antigo e novo
    for (int i = 0; i < numInodes; i++){
        file.read((char *)&inode, sizeof(INODE)); 

        for (int j = 0; j < 3; j++){
            if (strcmp(inode.NAME, names[j]) == 0){
                *indices[j] = i;
            }
        }
    }

    // Renomeia o arquivo/diretório se os nomes forem diferentes
    if (strcmp(localDoArquivo, newlocalDoArquivo))
    {
        file.seekp((initialPos + idLocalDoArquivo * sizeof(INODE) + 2), ios::beg);
        for (size_t i = 0; i < sizeof(newlocalDoArquivo); i++)
        {
            char newName = newlocalDoArquivo[i];
            file.write((char *)&newName, 1);
        }
    }else{
        // Remove do diretório antigo e adiciona ao novo
        file.seekg((initialPos + idOldDiretorio * sizeof(INODE) + 12), ios::beg);
        file.read((char *)&tamDiretorio, 1); 
        tamDiretorio--;
        file.seekp(-1, ios::cur);
        file.write((char *)&tamDiretorio, 1); 
        
        file.seekg((initialPos + idOldDiretorio * sizeof(INODE)),ios::beg);
        file.read((char *)&inodeToRemoveBlock, sizeof(INODE));

        for (size_t i = 0; i < sizeof(inodeToRemoveBlock.DIRECT_BLOCKS); i++){
            if (inodeToRemoveBlock.DIRECT_BLOCKS[i] != 0){
                usedBlocks++;
            }
        }
        if (inodeToRemoveBlock.NAME[0] == '/'){
            usedBlocks++;
        }

        char listOfValuesOfBlock[usedBlocks * blockSize] = {0};

        // Copia os blocos do diretório antigo para o novo
        for (size_t i = 0; i < usedBlocks; i++){
            char n[blockSize];
  
            file.seekg((initialPos + tamVectorInodes + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * blockSize), ios::beg);
            file.read((char *)&n, blockSize);

            for (size_t j = 0; j < blockSize; j++){
                listOfValuesOfBlock[j + increaseid] = n[j];
            }

            increaseid = blockSize;
        }

        // Remove referências antigas
        for (size_t i = 0; i < sizeof(listOfValuesOfBlock) - 1; i++){
            if (listOfValuesOfBlock[i + 1] != 0){
                if (listOfValuesOfBlock[i] == idLocalDoArquivo){
                    listOfValuesOfBlock[i] = listOfValuesOfBlock[i + 1];
                }
                if (i != 0 && listOfValuesOfBlock[i] == listOfValuesOfBlock[i - 1]){
                    listOfValuesOfBlock[i] = listOfValuesOfBlock[i + 1];
                }
            }
        }

        // Escreve de volta os blocos no diretório antigo
        increaseid = 0;
        for (size_t i = 0; i < usedBlocks; i++){
            file.seekp((initialPos + tamVectorInodes + 1 + inodeToRemoveBlock.DIRECT_BLOCKS[i] * blockSize), ios::beg);

            for (size_t j = 0; j < blockSize; j++){
                char tempValue = listOfValuesOfBlock[j + increaseid];
                file.write((char *)&tempValue, 1); 
            }
            increaseid = blockSize;
        }

        // Se o tamanho do diretório for muito pequeno, limpa o bloco de inode
        if (usedBlocks * 2 > tamDiretorio){
            for (size_t i = tamDiretorio - 1; i < sizeof(inodeToRemoveBlock.DIRECT_BLOCKS); i++){
                inodeToRemoveBlock.DIRECT_BLOCKS[i] = 0;
            }

            file.seekp((initialPos + idOldDiretorio * sizeof(INODE)), ios::beg);
            file.write((char *)&inodeToRemoveBlock, sizeof(INODE)); 
        }

        tamDiretorio = 0;
        file.seekg((initialPos + idNewDiretorio * sizeof(INODE) + 12), ios::beg);
        file.read((char *)&tamDiretorio, 1);
        tamDiretorio++;
        file.seekp(-1, ios::cur);
        file.write((char *)&tamDiretorio, 1); 
        
        file.seekg((initialPos + idNewDiretorio * sizeof(INODE)), ios::beg);
        file.read((char *)&newInode, sizeof(INODE));

        // Encontra o último bloco usado no novo diretório
        int lastBlockUsed = 0;
        for (size_t i = 0; i < sizeof(newInode.DIRECT_BLOCKS); i++){
            if (newInode.DIRECT_BLOCKS[i] != 0 && idLocalDoArquivo != newInode.DIRECT_BLOCKS[i]){
                lastBlockUsed = i;
            }
        }

        // Calcula o índice do bloco no novo diretório
        int tempSize = tamDiretorio - 1;
        while (tempSize > blockSize){
            tempSize -= blockSize;
        }
        int idOfblocosDeDiretorio = newInode.DIRECT_BLOCKS[lastBlockUsed] * 2 + tempSize;

        if (tamDiretorio > blockSize){
            // Encontra o próximo bloco livre para o diretório
            file.seekg((initialPos), ios::beg);

            for (int i = 0; i < numInodes; i++){
                file.read((char *)&inode, sizeof(INODE)); 

                for (size_t j = 0; j < sizeof(inode.DIRECT_BLOCKS); j++){
                    if (inode.DIRECT_BLOCKS[j] > idBlocoLivre){
                        idBlocoLivre = inode.DIRECT_BLOCKS[j];
                    }
                }
            }
            idBlocoLivre++;

            // Atualiza a lista de blocos usados no novo diretório
            idOfblocosDeDiretorio = idBlocoLivre * 2;

            file.seekg((initialPos + idNewDiretorio * sizeof(INODE)), ios::beg);
            file.read((char *)&newInode, sizeof(INODE));
            for (size_t i = 1; i < sizeof(newInode.DIRECT_BLOCKS); i++){
                if (newInode.DIRECT_BLOCKS[i] == 0){
                    newInode.DIRECT_BLOCKS[i] = idBlocoLivre;
                    break;
                }
            }
            file.seekp((initialPos + idNewDiretorio * sizeof(INODE)), ios::beg);
            file.write((char *)&newInode, sizeof(INODE));
        }

        // Adiciona referência ao bloco no novo diretório
        file.seekp((initialPos + tamVectorInodes + idOfblocosDeDiretorio + 1), ios::beg);
        file.write((char *)&idLocalDoArquivo, 1); 

        // Inicializando o array de mapa de bits
        mapa.set(0);

        // Lê inodes e atualizando o mapa de bits
        file.seekg(initialPos, ios::beg);
        for (int i = 0; i < numInodes; i++){
            file.read((char *)&newInode, sizeof(INODE));

            for (size_t j = 0; j < sizeof(newInode.DIRECT_BLOCKS); j++){
                if (newInode.DIRECT_BLOCKS[j] != 0){
                    mapa.set(newInode.DIRECT_BLOCKS[j]);
                }
            }
        }

        // Calculando o valor binário para hexadecimal
        unsigned long BintoHex = mapa.to_ulong();

        // Adicionando o número de blocos preenchidos (mapa) ao arquivo
        file.seekp(3, ios::beg);
        file.write((char *)&BintoHex, 1);
    }
    file.close();
}