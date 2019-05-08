#include <iostream>
#include "emulador.h"

using namespace std;

int main (int argc, const char *argv[]){
    setlocale(LC_ALL, "Portuguese");
    carregar_microprograma();
    carregar_executavel(argv[1]);

    while(true){
      atualizar();
      MIR = armazenamento[MPC];

      //--CONJUNTO DE OPERAÇÕES--//
      decodificar_microinstrucao();   //Separa a microinstrução
      atribuir_barramentoB();		      //Envia para o barramento B o registrador solicitado na microinstrução
      ULA();						              //Realiza as operações da ULA
      atribuir_registradores();	      //Encaminha o resultado da ULA para os registradores
      operar_memoria();			          //FEATCH, READ E WRITE
      pular();					              //Realiza os pulos se zero, não zero e MPC caso seja solicitado
    }
    return 0;
}


/*  - GLOSSARIO -

REGISTRADORES:
  MAR (registrador de endereço de memória de 32bits) - Endereço da proxima instrução executada
  MDR (registrador de dados de memória de 32 bits) - VALOR DO MAR
  PC  - Contém o endereço da instrução a ser buscada em seguida.
  MBR (Registrador de Buffer de Memória de 8bits, apontado por PC) - 
  Onde são armazenados em seqüência todos os bytes das instruções a serem executadas;
  SP  - Contém o endereço da palavra do topo da pilha
  LV  - Contém o endereço da primeira localização do quadro de variáveis locais
  CPP - Contém o endereço da primeira palavra do conjunto de constantes
  TOS - Registrador que indica o topo da pilha
  OPC - pode ser usado para guardar o endereço do código de operação de uma instrução de desvio
  H   - Acumulador (?)

ULA:
  Addr – Contém o endereço de uma microinstrução potencial seguinte.
  JAM – Determina como a próxima microinstrução é selecionada.
  ULA – Funções da ULA e do deslocador.
  C – Seleciona quais registradores são escritos a partir do barramento C.
  Mem – Funções de memória.
  B – Seleciona a fonte do barramento B
 */