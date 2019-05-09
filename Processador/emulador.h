//   (| = "ou" para bits) (& "e" para bits )

#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define RAM 100000000
#define TRUE 1
#define FALSE 0

using namespace std;

/* VARIÁVEIS */
typedef unsigned char byte;	                //8bits
typedef unsigned int palavra;               //32bits
typedef unsigned long int microinstrucao;   //64bits

/* ARMAZENAMENTO */
microinstrucao armazenamento[512];	//ARMAZENAMENTO DE MICROINSTRUÇÕES (dado pelo professor)
byte memoria[RAM]; 					//MEMORIA PRINCIPAL (RAM) DO EMULADOR
palavra barramentoB, barramentoC; 	//BARRAMENTOS
byte Z, N; 							//FLIP-FLOP PARA O DESLOCADOR

/* REGISTRADORES */
palavra MAR = 0, MDR = 0, PC = 0;                           // MEMORIA
byte MBR = 0;                                               // MEMORIA
palavra SP = 0, LV = 0, CPP = 0, TOS = 0, OPC = 0, H = 0;   // ULA
microinstrucao MIR;	                                        // Microinstrução atual

/* AUXILIARES */
byte aux_barramentoB, aux_operacao, aux_pulo, aux_memoria, aux_deslocador; 
palavra aux_gravar, MPC = 0; 

/* MÉTODOS DE INICIALIZAÇÃO */

//Lê o arquivo microprog.rom e carrega o microprograma para o armazenamento
void carregar_microprograma(){ 
    FILE *microprograma;
    microprograma = fopen("microprog.rom" , "rb");
	if (microprograma != NULL) {
		fread(armazenamento, sizeof(microinstrucao), 512, microprograma);
		fclose(microprograma);  
    }
}
//Lê o arquivo passado como argumento e carrega o programa na memória
void carregar_executavel(const char *arquivo){
	FILE *prog;
	palavra tamanho; //32bits
	byte tam_arquivo[4];

	prog = fopen(arquivo, "rb");
	if (prog != NULL) {
	//Carrega os primeiros 4 bytes que contém o tamanho do arquivo para um vetor
		fread(tam_arquivo, sizeof(byte), 4, prog); 
		memcpy(&tamanho, tam_arquivo, 4);//carrega esse vetor na variável tamanho.

		//Carrega os 20 primeiros bytes que contém a inicialização do programa para os primeiros 20 bytes da memória
		fread(memoria, sizeof(byte), 20, prog);

		//Carrega o programa na memória a partir da posição PC+1. 
		fread(&memoria[0x0401], sizeof(byte), tamanho-20, prog);
	}
	fclose(prog);
}

/* MICROINSTRUÇÃO */

//Onde será feita a separação da microinstrução e os auxiliares
void decodificar_microinstrucao(){
	aux_barramentoB  		=  MIR 		  & 0b1111;				//Qual dos registradores será usado no barramento B
	aux_memoria 			= (MIR >> 4)  & 0b111;				//Qual operação será feita com a memoria principal
	aux_gravar 				= (MIR >> 7)  & 0b111111111;		//Qual dos registradores será gravado o barramento C
	aux_operacao 			= (MIR >> 16) & 0b111111;			//Qual a operacaoção que será feita na ULA
	aux_deslocador 			= (MIR >> 22) & 0b11;				//Qual será a operação feita pelo deslocador
	aux_pulo				= (MIR >> 24) & 0b111;				//Se haverá pulo ou não
	MPC 					= (MIR >> 27) & 0b111111111;		//Qual será a próxima instrução		
}

//Faz a atribuição do barramento B
void atribuir_barramentoB(){
	//Carrega um registrador para o barramento B
	switch(aux_barramentoB){
		case 0: barramentoB = MDR;				break;
		case 1: barramentoB = PC;				break;
		//O caso 2 carrega o MBR com sinal fazendo a extensão de sinal, ou seja, copia-se o bit mais significativo do MBR para as 24 posições mais significativas do barramento B.
		case 2: barramentoB = MBR; 
			if(MBR & (0b10000000)) 
						barramentoB = barramentoB | (0b111111111111111111111111 << 8); //extensao de bit
						break;
		case 3: barramentoB = MBR;				break;
		case 4: barramentoB = SP;				break;
		case 5: barramentoB = LV;				break;
		case 6: barramentoB = CPP;				break;
		case 7: barramentoB = TOS;				break;
		case 8: barramentoB = OPC;				break;
		default: barramentoB = -1;				break;
	}	
}

void ULA(){
	switch(aux_operacao){
	//Cada operação da ULA é representado pela sequência dos bits de operação. 
	//Cada operação útil foi convertida para inteiro para facilitar a escrita do switch.
		case 12: barramentoC = H & barramentoB;			break;
		case 17: barramentoC = 1;						break;
		case 18: barramentoC = -1;						break;
		case 20: barramentoC = barramentoB;				break;
		case 24: barramentoC = H;						break;
		case 26: barramentoC = ~H;						break;
		case 28: barramentoC = H | barramentoB;			break;
		case 44: barramentoC = ~barramentoB;			break;
		case 53: barramentoC = barramentoB + 1;			break;
		case 54: barramentoC = barramentoB - 1;			break;
		case 57: barramentoC = H + 1;					break;
		case 59: barramentoC = -H; 						break; //complemento de 2
		case 60: barramentoC = H + barramentoB;			break;
		case 61: barramentoC = H + barramentoB + 1;		break;
		case 63: barramentoC = barramentoB - H;			break;
		default: 										break;
	}
	
	//Verifica o resultado da ula e atribui as variáveis zero e nzero
	if(barramentoC != 0) { 
		Z = FALSE;
		N = TRUE;
	} else {
		Z = TRUE;
		N = FALSE;
	}
	
	//Faz o deslocamento do aux_deslocador
	switch(aux_deslocador){
		//Faz o deslocamento em um bit para direita
		case 1: barramentoC = barramentoC >> 1;		break;
		//Faz o deslocamento em 8 bits para a esquerda
		case 2: barramentoC = barramentoC << 8;		break;
	}
}

//Grava o resultado através do barramento C
void atribuir_registradores() {
	//Pode atribuir vários registradores ao mesmo tempo dependendo se mi_gravar possui bit alto para o registrador correspondente
	if(aux_gravar & 0b000000001)   MAR = barramentoC;
	if(aux_gravar & 0b000000010)   MDR = barramentoC;
	if(aux_gravar & 0b000000100)   PC  = barramentoC;
	if(aux_gravar & 0b000001000)   SP  = barramentoC;
	if(aux_gravar & 0b000010000)   LV  = barramentoC;
	if(aux_gravar & 0b000100000)   CPP = barramentoC;
	if(aux_gravar & 0b001000000)   TOS = barramentoC;
	if(aux_gravar & 0b010000000)   OPC = barramentoC;
	if(aux_gravar & 0b100000000)   H   = barramentoC;
}
//Fetch, Read, Write 
void operar_memoria(){
	if(aux_memoria & 0b001) MBR = memoria[PC];								//FETCH

	//MDR recebe os 4 bytes referente a palavra MAR 
	if(aux_memoria & 0b010) memcpy(&MDR, &memoria[MAR*4], 4);	//READ
	
	//Os 4 bytes na memória da palavra MAR recebem o valor de MDR
	if(aux_memoria & 0b100) memcpy(&memoria[MAR*4], &MDR, 4);	//WRITE

}
//Faz a operação do pulo
void pular(){
	//Realiza o pulo se a saída da ULA for zero (| = "ou" para bits) (& "e" para bits )
	if(aux_pulo & 0b001) MPC = MPC | (Z << 8);

	//Realiza o pulo se a saída da ula for diferente de zero
	if(aux_pulo & 0b010) MPC = MPC | (N << 8);

	//Pula para a posição do MBR
	if(aux_pulo & 0b100) MPC = MPC | MBR;
}

/* PRINTAR INFORMAÇÕES */

//Mostra os valores em binário, formatados dependendo da necessidade
void binario(void *valor, int tipo) {   
	switch(tipo){
		//mostra o Enderereço da palavra
		case 1: 
		{
			printf(" ");
			byte aux;
			byte* valorAux = (byte*)valor;
				
			for(int i = 3; i >= 0; i--){
				aux = *(valorAux + i);
				for(int j = 0; j < 8; j++){
					printf("%d", (aux >> 7) & 0b1);
					aux = aux << 1;
				}
				printf(" ");
			}
		}
		break;

		// Printa os bits espaçando-os
		case 2: 
		{
			microinstrucao aux;
		
			aux = *((microinstrucao*)(valor));
			for(int j = 0; j < 36; j++){
				if ( j == 9 || j == 12 || j == 20 || j == 29 || j == 32) cout << "  ";

				printf("%ld", (aux >> 35) & 0b1);
				aux = aux << 1;
			}
		}
		break;

		//printa os bits não formatados
		case 3:
		{
			palavra aux;
			
			aux = *((palavra*)(valor));
			for(int j = 0; j < 32; j++){
				printf("%d", (aux >> 31) & 0b1);
				aux = aux << 1;
			}
		}
		break;	

	}
}
void atualizar(){
	system("clear");
		
	//Exibe a pilha de operandos quando o emulador já realizou a inicialização
	if (LV && SP) //Quando ambos forem utilizados, mostrar a pilha de operações
	{ 
		cout << "\n PILHA DE OPERAÇÕES: ";
		cout << " \n────────────────────────────────────────────────────────────";
		cout << "\n\t\t\t\t       ENDEREÇO";
		cout << "\n\t\t BINÁRIO\t\t  DE      INT";
		cout << "\n\t\t        \t\tPALAVRA\n";
		cout << "\n────────────────────────────────────────────────────────────\n";
		//Exibe a área delimitada por SP e LV para mostrar a pilha de operandos
		for (int i = SP; i >= LV; i--) {
			palavra valor;
			memcpy(&valor, &memoria[i*4], 4);
			binario(&valor , 1); cout << "\t"<< i; cout << "\t  " << (int)valor; cout << "\n";
		}
		cout << "────────────────────────────────────────────────────────────";
	}

	//Exibe os registradores
	cout << "\n REGISTRADORES:";
	cout << "\n────────────────────────────────────────────────────────────";
	cout << "\n\t\t\t  BINÁRIO\t           \n";
	cout << "\n    MAR :  ";		  	binario(&MAR , 3); cout << "      " << MAR; //endereço da proxima instrução
	cout << "\n    MDR :  ";   	 	  	binario(&MDR , 3); cout << "      " << MDR; //valor de MAR
	cout << "\n    PC  :  "; 	 	  	binario(&PC  , 3); cout << "      " << PC; //contador do programa
	cout << "\n    MBR :  ";			binario(&MBR , 3); printf ( "      0x%02x", (palavra)MBR & 0xFF); //ultima instrução executada em hex
 	cout << "\n    SP  :  ";		  	binario(&SP  , 3); cout << "      " << SP; //lv + num de variaveis
	cout << "\n    LV  :  ";		 	binario(&LV  , 3); cout << "      " << LV; //parte final da pilha (acho)
	cout << "\n    CPP :  ";		 	binario(&CPP , 3); cout << "      " << CPP; //contador de constantes, não é alterado
	cout << "\n    TOS :  ";		 	binario(&TOS , 3); cout << "      " << TOS; //valor do topo da pilha
	cout << "\n    OPC :  ";	  		binario(&OPC , 3); cout << "      " << OPC; 
	cout << "\n    H   :  ";		 	binario(&H   , 3); cout << "      " << H;
	cout << "\n";
	cout << "\n ENDEREÇO DA PRÓXIMA MICROINSTRUÇÃO";
	cout << "\n────────────────────────────────────────────────────────────";
	cout << "\n    MPC :  ";	  binario(&MPC , 3); printf ( "      0x%02x", MPC & 0xFF);

	//Exibe a microinstrução que a ula está operando atualmente
	cout << "\n\n MICROINSTRUÇÃO ATUAL";
	cout << "\n────────────────────────────────────────────────────────────";
	cout << "\n        Addr    JAM    ULA         C      Mem   B";
	cout << "\n     "; binario(&MIR, 2);

	getchar();
}
