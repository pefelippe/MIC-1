#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM 20 //20 bytes de inicialização

int swap_bytes(short int valor);                        //trocar bytes 00 73
int busca_indice(char *str, char **lista, int tamanho); //indice interno
int existe(char *str, char **lista, int tamanho);       //verifica se existe a variavel
int retorna_offset(char *label, char **lista_labels, int *lista_offsets, int tamanho); //returna o offset da instrução executada

int main(int argc, char *argv[]){
    if(argc  < 3 || argc > 3){
        printf("Erro ao abrir ou escrever os arquivos. Tente novamente. \n");
        exit(1);
    }

    char buffer[TAM];   // Guardará temporariamente as intruções, operandos, etc
    char codigo_hex = 0x0;  // Guardará o código hexa da instrução utilizada
    char **lista_vars = (char **) malloc(sizeof(char *));   // Guardará a lista de variáveis locais
    char **lista_labels = (char **) malloc(sizeof(char *)); // guarda os labels
    int *lista_offsets = (int *) malloc(sizeof(int));   // guarda os offsets das labels
    int indice = 0; // indice interno (para variáveis locais)
    int indice_offset = 0; // indice para a lista de offsets, "tamanho da lista"
    int offset = 0; // offset da instrução atual

    int tamanho_cod_ini = 20;	// 20 bytes de inicialização

    short int init = 0x73; //2 bytes
    init = swap_bytes(init); //inverte o 0x73
    short int padding = 0; //2 bytes
    int cpp = 0x0006;   //Contador de constantes, começa com 6
    int lv = 0x1001;    //endereço da primeira localização do quadro de variáveis locais, começa 4097
    int pc = 0x400;     //Contador do Programa, começa em 1024

    memset(buffer, 0x0, TAM);   // Zera o que tem em buffer para retirar (possíveis) lixos

    /* ABRINDO O ASM E GERANDO O .EXE */
    FILE *source = fopen(argv[1], "r");     //lê o asm
    FILE *executavel = fopen(argv[2], "w"); //cria o .exe

    if(source == NULL){
        printf("Erro: arquivo %s não encontrado.\n", argv[1]);
        exit(1);
    }

    if(executavel == NULL){
        printf("Erro ao tentar abrir %s. Cheque suas permissões de escrita.\n", argv[2]);
        exit(1);
    }

    /* Mapeando variáveis */
    while(fscanf(source, "%s", buffer) && !feof(source)){ //lê as strings e coloca no buffer até feof
        if(!strcmp(buffer, "istore") || !strcmp(buffer, "iload") || !strcmp(buffer, "iinc")){ //nesses é obrigatorio vir uma variavel dps
            //Se ao ler o arquivo, encontrar uma desses comandos, vai pegar a variavel seguinte
            fscanf(source, "%s", buffer);   // Pega variável
            if(!existe(buffer, lista_vars, indice)){ //verificar e a variavel ja existe
                *(lista_vars + indice) = (char *) malloc(sizeof(char) * strlen(buffer)); // Aloca espaço do tamanho do nome da string
                strcpy(*(lista_vars + indice), buffer);
                indice++;  // Vai para o próxima variável
                lista_vars = realloc(lista_vars, sizeof(char *) * (indice + 1)); //realloca para guardar a nova variavel
            }
        }
    }

   rewind(source); // Volta para o início do arquivo

    /* Mapeamento de labels */
    while(fscanf(source, "%s", buffer) && !feof(source)){ //ler o source até o fim e salvar as variaveis no buffer
        if(!strcmp(buffer, "nop") || \
        !strcmp(buffer, "iadd") || \
        !strcmp(buffer, "isub") || \
        !strcmp(buffer, "iand") || \
        !strcmp(buffer, "ior") || \
        !strcmp(buffer, "dup") || \
        !strcmp(buffer, "pop") || \
        !strcmp(buffer, "swap") || \
        !strcmp(buffer, "wide") || \
        !strcmp(buffer, "ireturn")){
            offset++;
        } 
        else if(!strcmp(buffer, "bipush") || \
        !strcmp(buffer, "iload") || \
        !strcmp(buffer, "istore")){
            fscanf(source, "%s", buffer);
            offset += 2;
        }
        else if(!strcmp(buffer, "ldc_w") || \
        !strcmp(buffer, "goto") || \
        !strcmp(buffer, "iflt") || \
        !strcmp(buffer, "ifeq") || \
        !strcmp(buffer, "if_icmpeq") || \
        !strcmp(buffer, "invokevirtual")){
            fscanf(source, "%s", buffer);
            offset += 3;
        }
        else if(!strcmp(buffer, "iinc")){
			// Instruções que somam 2 bytes
            fscanf(source, "%s", buffer);
            fscanf(source, "%s", buffer);
            offset += 3;
        } else{
            if(!existe(buffer, lista_labels, indice_offset)){
                *(lista_labels + indice_offset) = (char *) malloc(sizeof(char) * strlen(buffer));
                strcpy(*(lista_labels + indice_offset), buffer);
                *(lista_offsets + indice_offset) = offset;

                indice_offset++;
                lista_offsets = realloc(lista_offsets, sizeof(int) * (indice_offset + 1));
                lista_labels = realloc(lista_labels, sizeof(char *) * (indice_offset + 1));
            }           
        }
   }

    rewind(source);   // Volta para o início do arquivo
    
    tamanho_cod_ini += offset;
    
    /* Escrevendo no Arquivo */
    fwrite(&tamanho_cod_ini, sizeof(int), 1, executavel);
    fwrite(&init, sizeof(short int), 1, executavel);
    fwrite(&padding, sizeof(short int), 1, executavel);
    fwrite(&cpp, sizeof(int), 1, executavel);
    fwrite(&lv, sizeof(int), 1, executavel);
    fwrite(&pc, sizeof(int), 1, executavel);
    lv += indice;	// Soma LV ao número de variáveis e vira SP
    fwrite(&lv, sizeof(int), 1, executavel); //SP
    
    offset = 0; // Recontagem de offsets

    /* Gerando o binário */
    while(fscanf(source, "%s", buffer) && !feof(source)){
        if(!strcmp(buffer, "nop")){
            codigo_hex = 0x01;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "iadd")){
            codigo_hex = 0x02;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "isub")){
            codigo_hex = 0x05;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "iand")){
            codigo_hex = 0x08;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "ior")){
            codigo_hex = 0x0b;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "dup")){
            codigo_hex = 0x0e;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "pop")){
            codigo_hex = 0x10;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "swap")){
            codigo_hex = 0x13;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        }  else if(!strcmp(buffer, "bipush")){
            codigo_hex = 0x19;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            fscanf(source, "%s", buffer);
            char valor = atoi(buffer);
            fwrite(&valor, sizeof(char), 1, executavel);
            offset += 2;
        } else if(!strcmp(buffer, "iload")){
            codigo_hex = 0x1c;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            fscanf(source, "%s", buffer);
            char varnum = busca_indice(buffer, lista_vars, indice);
            fwrite(&varnum, sizeof(char), 1, executavel);
            offset += 2;
        } else if(!strcmp(buffer, "istore")){
            codigo_hex = 0x22;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            fscanf(source, "%s", buffer);
            char varnum = busca_indice(buffer, lista_vars, indice);
            fwrite(&varnum, sizeof(char), 1, executavel);
            offset += 2;
        } else if(!strcmp(buffer, "wide")){
            codigo_hex = 0x28;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        } else if(!strcmp(buffer, "ldc_w")){
			// precisa de index, 2 bytes
            codigo_hex = 0x32;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            // TODO: Checar validade
            fscanf(source, "%s", buffer);
            short int constante = atoi(buffer);
            fwrite(&constante, sizeof(short int), 1, executavel);
            offset += 2;
        } else if(!strcmp(buffer, "iinc")){
			// precisa de varnum, 1 byte, e const, 1 byte
            codigo_hex = 0x36;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);

            fscanf(source, "%s", buffer);
            char varnum = busca_indice(buffer, lista_vars, indice);
            fscanf(source, "%s", buffer);
            char constante = atoi(buffer);

            fwrite(&varnum, sizeof(char), 1, executavel);
            fwrite(&constante, sizeof(char), 1, executavel);
            offset += 3;
        } else if(!strcmp(buffer, "goto")){
			// precisa de offset, 2 bytes
            codigo_hex = 0x3c;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            // Cálculo offset
            fscanf(source, "%s", buffer);
            int offset_label = retorna_offset(buffer, lista_labels, lista_offsets, indice_offset);
            short int pulo = offset_label - offset;
            pulo = swap_bytes(pulo);
            fwrite(&pulo, sizeof(short int), 1, executavel);
            offset += 3;    // 3 porque gravamos 2 bytes além do byte da instrução
        } else if(!strcmp(buffer, "iflt")){
			// precisa de offset, 2 bytes
            codigo_hex = 0x43;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);

            fscanf(source, "%s", buffer);
            int offset_label = retorna_offset(buffer, lista_labels, lista_offsets, indice_offset);
            short int pulo = offset_label - offset;
            pulo = swap_bytes(pulo);
            fwrite(&pulo, sizeof(short int), 1, executavel);
            offset += 3;
        } else if(!strcmp(buffer, "ifeq")){
			// precisa de offset, 2 bytes
            codigo_hex = 0x47;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);

            fscanf(source, "%s", buffer);
            int offset_label = retorna_offset(buffer, lista_labels, lista_offsets, indice_offset);
            short int pulo = offset_label - offset;
            pulo = swap_bytes(pulo);
            fwrite(&pulo, sizeof(short int), 1, executavel);
            offset += 3;
        } else if(!strcmp(buffer, "if_icmpeq")){
			// precisa de offset, 2 bytes
            codigo_hex = 0x4b;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);

            fscanf(source, "%s", buffer);
            int offset_label = retorna_offset(buffer, lista_labels, lista_offsets, indice_offset);
            short int pulo = offset_label - offset;
            pulo = swap_bytes(pulo);
            fwrite(&pulo, sizeof(short int), 1, executavel);
            offset += 3;
        } else if(!strcmp(buffer, "invokevirtual")){
			// precisa de disp, 2 bytes
            codigo_hex = 0x55;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);

            fscanf(source, "%s", buffer);
            int offset_label = retorna_offset(buffer, lista_labels, lista_offsets, indice_offset);
            short int pulo = offset_label - offset;
            pulo = swap_bytes(pulo);
            fwrite(&pulo, sizeof(short int), 1, executavel);
            offset += 3;
        } else if(!strcmp(buffer, "ireturn")){
            codigo_hex = 0x6b;
            fwrite(&codigo_hex, sizeof(char), 1, executavel);
            offset++;
        }
    }

    fclose(source);
    fclose(executavel);
    return 0;
}

/* Implementação */
int swap_bytes(short int valor)  //troca os bits invertidos 
{
    short int resultado;
    resultado = valor >> 8; 
    resultado += (valor & 0xFF) << 8;
    return resultado;
}

int busca_indice(char *str, char **lista, int tamanho) // Retorna o número da variável
{
    for(int i = 0; i < tamanho; i++){
        if(!strcmp(str, *(lista + i)))
            return i;
    }
    return -1;
}

int existe(char *str, char **lista, int tamanho)    // Checa se váriavel já está na lista
{
    for(int i = 0; i < tamanho; i++){
        if(!strcmp(str, *(lista + i)))
            return 1;
    }
    return 0;
}

int retorna_offset(char *label, char **lista_labels, int *lista_offsets, int tamanho)
{
    for(int i = 0; i < tamanho; i++){
        if(!strcmp(label, *(lista_labels + i)))
            return *(lista_offsets + i);
    }
    return 256; // caso não encontre, retorne 2 bytes
}