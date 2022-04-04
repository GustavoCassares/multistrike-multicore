//update from v2: Implementation of first ign signal

#include <Preferences.h>
Preferences EEPROM;


//Pin Declaration:
//Inputs:
const uint8_t ign1InPin=34; //GPIO34 - Board D34
const uint8_t ign2InPin=35; //GPIO35 - Board D35   
const uint8_t ign3InPin=36; //GPIO36 - Board UP

//Outputs:
const uint8_t ignOut1Pin=16; //GPIO16 - Board RX2
const uint8_t ignOut2Pin=17; //GPIO17 - Board TX2
const uint8_t ignOut3Pin=18; //GPIO18 - Board D18
const uint8_t ignOut4Pin=19; //GPIO19 - Board D19
const uint8_t ignOut5Pin=21; //GPIO21 - Board D21
const uint8_t ignOut6Pin=22; //GPIO22 - Board D22

//State Variables:
static uint8_t ign1InState = LOW;
static uint8_t ign2InState = LOW;
static uint8_t ign3InState = LOW;
static uint8_t ign1OutState = LOW;
static uint8_t ign2OutState = LOW;
static uint8_t ign3OutState = LOW;
static uint8_t ign4OutState = LOW;
static uint8_t ign5OutState = LOW;
static uint8_t ign6OutState = LOW;
volatile static uint8_t risingEdgeFlag1=LOW;
volatile static uint8_t risingEdgeFlag2=LOW;
volatile static uint8_t risingEdgeFlag3=LOW;
static int cycleCounter1=0;
static int cycleCounter2=0;
static int cycleCounter3=0;
static int cycleCounter4=0;
static int cycleCounter5=0;
static int cycleCounter6=0;
int primaryCycles=3;
int secondaryCycles=3;
int secondCoilEnable=0;
static uint8_t taskCoreZero=0;
int i=LOW;

//Time Variables:
int currentMicros1=0;
int currentMicros2=0;
int currentMicros3=0;
int previousTime1=0;
int previousTime2=0;
int previousTime3=0;
int previousTime4=0;
int previousTime5=0;
int previousTime6=0;

int phasingTime=200; // time from 1st to second spark in microsseconds
int burstDwellTime=1000; // Dwell time for 3rd to n spark
int burstInterval=500; // Coil rest time from second to n spark

//CommVariables:
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars]; 
boolean newData = false;

//Function declarations
void ParameterExhibition();
void WriteToEEPROM();
void PinModeDeclaration();
void ParseCommData();
void DataShow();
void IRAM_ATTR Ign1SignalRise();
void SerialComm(void * pvParameters);
void ReadFromEEPROM();
void MainIgn1();
void ActuateOutput();



//------------------------------------------------------------------------------------------------------------
void ParameterExhibition()
{
  Serial.print("secondCoilEnable:");
  Serial.println(secondCoilEnable);
  Serial.print("phasingTime:");
  Serial.println(phasingTime);
  Serial.print("primaryCycles:");
  Serial.println(primaryCycles);
  Serial.print("secondaryCycles:");
  Serial.println(secondaryCycles);
  Serial.print("burstInterval:");
  Serial.println(burstInterval);
  Serial.print("burstDwellTime:");
  Serial.println(burstDwellTime);
}

//---------------------------------------------------------------------------------------------------------------

void WriteToEEPROM()
{
  // the first parameter is the address, and must be shorter than 15 chars.
  EEPROM.putInt("secondCoilEna", secondCoilEnable);
  EEPROM.putInt("phasingTime", phasingTime);
  EEPROM.putInt("primaryCycles", primaryCycles);
  EEPROM.putInt("secondaryCycles", secondaryCycles);
  EEPROM.putInt("burstDwellTime", burstDwellTime);
  EEPROM.putInt("burstInterval", burstInterval);
}

//---------------------------------------------------------------------------------------------------------------
void PinModeDeclaration()
{
  pinMode(ign1InPin,INPUT);
  pinMode(ign2InPin,INPUT);
  pinMode(ign3InPin,INPUT);
  pinMode(ignOut1Pin,OUTPUT);
  pinMode(ignOut2Pin,OUTPUT);
  pinMode(ignOut3Pin,OUTPUT);
  pinMode(ignOut4Pin,OUTPUT);
  pinMode(ignOut5Pin,OUTPUT);
  pinMode(ignOut6Pin,OUTPUT);

}

//-----------------------------------------------------------------------------------------------------------------

void ParseCommData()
{
  char * strtokIndx= strtok(tempChars,","); // this is used by strtok() as an index
  secondCoilEnable=atoi(strtokIndx);

  strtokIndx = strtok(NULL,",");      // get the first part - the string
  phasingTime=atoi(strtokIndx);

  strtokIndx = strtok(NULL,",");      // get the first part - the string
  primaryCycles=constrain(atoi(strtokIndx),0,5);

  strtokIndx = strtok(NULL,",");      // get the first part - the string
  secondaryCycles=constrain(atoi(strtokIndx),0,5);

  strtokIndx = strtok(NULL,",");      // get the first part - the string
  burstInterval=atoi(strtokIndx);

  strtokIndx = strtok(NULL,",");      // get the first part - the string
  burstDwellTime =constrain(atoi(strtokIndx),0,5000);
}


//-------------------------------------------------------------------------------------------------------------


void DataShow()
{
  //Data Show:
  if (newData == true) {
    ParameterExhibition();
    newData = false;
  }
}
//-----------------------------------------------------------------------------------------------------------------

void IRAM_ATTR Ign1SignalRise()
{
  risingEdgeFlag1=true;
  currentMicros1=micros();
  previousTime2=currentMicros1;

  //Serial.println("ISR");

}

//------------------------------------------------------------------------------------------------

void IRAM_ATTR Ign2SignalRise()
{
  risingEdgeFlag2=true;
  currentMicros2=micros();
  previousTime4=currentMicros2;

  //Serial.println("ISR");

}

//------------------------------------------------------------------------------------------------


void IRAM_ATTR Ign3SignalRise()
{
  risingEdgeFlag3=true;
  currentMicros3=micros();
  previousTime6=currentMicros3;

  //Serial.println("ISR");

}

//------------------------------------------------------------------------------------------------



void SerialComm(void * pvParameters)
{
  while (true)
  {
    //Serial.print("Main Loop: Executing on core ");
    //Serial.println(xPortGetCoreID());


    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    int rc=0;
    int cntComma = 0;

    // if (Serial.available() > 0) {
    while (Serial.available() > 0 && newData == false) {
      rc = Serial.read();

      if (recvInProgress == true) {
        if (rc == ',') {
          cntComma++;
        }

        if (rc != endMarker) {
          receivedChars[ndx] = rc;
          ndx++;
          if (ndx >= numChars) {
            ndx = numChars - 1;
          }
        }
        else {
          receivedChars[ndx] = '\0'; // terminate the string
          recvInProgress = false;
          ndx = 0;
          newData = true;
        }
      }

      else if (rc == startMarker) {
        recvInProgress = true;
      }
    }

    if (newData == true) {
      if (cntComma == 5) {
        strcpy(tempChars, receivedChars);
        ParseCommData();
        DataShow(); 
        WriteToEEPROM();
        DataShow();
      }
      else {
        Serial.println("Wrong input!");
      }
      newData = false;
    }
    vTaskDelay(100); 
  }
}

//------------------------------------------------------------------------------------------------------------
void ReadFromEEPROM()
{
  //the second parameter of "getInt" is the default value in case nothing is written in EEPROM.
  secondCoilEnable = EEPROM.getInt("secondCoilEna", secondCoilEnable);
  phasingTime      = EEPROM.getInt("phasingTime", phasingTime);
  primaryCycles    = EEPROM.getInt("primaryCycles", primaryCycles);
  secondaryCycles  = EEPROM.getInt("secondaryCycles", secondaryCycles);
  burstDwellTime   = EEPROM.getInt("burstDwellTime", burstDwellTime);
  burstInterval    = EEPROM.getInt("burstInterval", burstInterval);
}   



//--------------------------------------------------------------------------------------------------
void setup()
{
  PinModeDeclaration();
  Serial.begin(115200);
  delay (1000);
  //"eeprom" is a namespace and can be changed as desired.
  //false refers to RW mode (not read only)
  EEPROM.begin("eeprom", false);
  ReadFromEEPROM();

  ParameterExhibition();

  xTaskCreatePinnedToCore(
      SerialComm,   /* função que implementa a tarefa */
      "SerialComm", /* nome da tarefa */
      10000,      /* número de palavras a serem alocadas para uso com a pilha da tarefa */
      NULL,       /* parâmetro de entrada para a tarefa (pode ser NULL) */
      100,          /* prioridade da tarefa (0 a N) */
      NULL,       /* referência para a tarefa (pode ser NULL) */
      taskCoreZero);         /* Núcleo que executará a tarefa */

  delay(500); //tempo para a tarefa iniciar

  attachInterrupt(ign1InPin, Ign1SignalRise, RISING);
  attachInterrupt(ign2InPin, Ign2SignalRise, RISING);
  attachInterrupt(ign3InPin, Ign3SignalRise, RISING);

}


//----------------------------------------------------------------------------------------------

void PhaseIgn1()   //Função para copiar o sinal de entrada na borda de subida, defasar a primeira borda de descida e depois comandar o número de pulsos desejados, com intervalo e acionamento determinados
{

  static volatile uint8_t previousInputState;
  static volatile uint8_t fallingEdgeFlag1;
  static volatile uint8_t burstModeFlag1;




  //previousTime2=currentMicros1;



  if (ign1InState==HIGH)
  {   
    if (currentMicros1-previousTime2>=phasingTime)
    {
      ign2OutState=HIGH; 
      previousInputState=HIGH; 
    }
    else
    {
      ign2OutState=LOW;
    }   
  }
  else
  {
    //ign2OutState=ign1InState; // Durante o acionamento através da ECU, o sinal de saída do uC permanece tambem acionado


    if (previousInputState==HIGH) //Comparação a fim de identificar a borda de descida do sinal de entrada (ECU)
    {
      fallingEdgeFlag1=HIGH; //Flag que sinaliza a parte do ciclo após borda de descida do sinal de entrada
      previousTime2=currentMicros1; // Reset do intervalo de tempo que permite o phasing após borda de descida do sinal de entrada
      previousInputState=LOW;
    }    

    if (fallingEdgeFlag1==HIGH) //Rotina enquanto a flag que sinaliza a borda de descida do sinal original está acionada
    {
      if (currentMicros1-previousTime2>=phasingTime) //Comparação de tempo entre borda de descida e phasing da segunda bobina em relação à primeira
      {
        ign2OutState=LOW; // Após final do tempo de phasing da segunda bobina em relação à primeira, o sinal de saída é desligado
        previousTime2=currentMicros1; // Reset de tempo que permite contagem após borda de descida do sinal defasado
        fallingEdgeFlag1=LOW; // Reset do flag que sinaliza o período de defasagem do primeiro sinal de saída
        burstModeFlag1=HIGH; // Flag que sinaliza a parte do ciclo após borda de descida do primeiro sinal de saída
      }
      else
      {
        ign2OutState=HIGH; // Permanece o sinal de saída acionado enquanto o primeiro tempo de defasagem não é alcançado
        fallingEdgeFlag1=HIGH; // Mantém o flag pós borda de descida do sinal de entrada ativo
      }
    }
    if (burstModeFlag1==HIGH && fallingEdgeFlag1==LOW) // Rotina durante parte secundária do ciclo de acionamento
    {
      if (cycleCounter2<=secondaryCycles-2) // Limita o número de ciclos de acionamento da bobina
      {
        if (ign2OutState==LOW) // Condição para que a contagem de tempo de "descanso" da bobina funcione
        {
          if (currentMicros1-previousTime2>=burstInterval) // Delimita o intervalo de tempo de "descanso" da bobina
          {
            ign2OutState=HIGH; //Aciona a bobina após intervalo de descanso
            previousTime2=currentMicros1; // Reset do intervalo de tempo
          }    
        }
        else if (currentMicros1-previousTime2>=burstDwellTime) // Delimita o dwell secundario da bobina
        {
          ign2OutState=LOW; // Desaciona a bobina após dwell secundario
          previousTime2=currentMicros1; // Reset do intervalo de tempo
          cycleCounter2++; // Contador de ciclos de acionamento secundário
        }    
      }
      else
      {
        cycleCounter2=0; // Reset do contador após numero de ciclos desejado alcançado
        burstModeFlag1=LOW; // Reset da flag que sinaliza a segunda parte do ciclo de acionamento
      }


    }

  }



}



//-------------------------------------------------------------------------------------------------------------



void MainIgn1()   //Função para copiar o sinal de entrada na borda de subida e depois comandar o número de pulsos desejados, com intervalo e numero de ciclos determinados
{
  static volatile uint8_t fallingEdgeFlag1;
  static volatile uint8_t burstModeFlag1;
  static volatile uint8_t previousInputState;
  currentMicros1=micros();



  //i=ign1InState;  //variável que guarda o estado de entrada do ciclo anterior, a fim de ser possível sinalizar a troca de estado
  //  ign1InState= digitalRead(ign1InPin); // Atualiza o estado da variável de entrada


  if (ign1InState==HIGH)
  { 
    ign1OutState=ign1InState; // Durante o acionamento através da ECU, o sinal de saída do uC permanece tambem acionado
    previousInputState=HIGH;    
  }
  else
  {

    if (previousInputState==HIGH) //Comparação a fim de identificar a borda de descida do sinal de entrada (ECU)
    {   

      fallingEdgeFlag1=HIGH; //Flag que sinaliza a parte do ciclo após borda de descida do sinal de entrada
      previousTime1=currentMicros1; // Reset do intervalo de tempo que permite o phasing após borda de descida do sinal de entrada
      previousInputState=LOW;
    }    

    if (fallingEdgeFlag1==HIGH) //Rotina enquanto a flag que sinaliza a borda de descida do sinal original está acionada
    {

      ign1OutState=LOW; // Após final do tempo de phasing da segunda bobina em relação à primeira, o sinal de saída é desligado
      previousTime1=currentMicros1; // Reset de tempo que permite contagem após borda de descida do sinal defasado
      fallingEdgeFlag1=LOW; // Reset do flag que sinaliza o período de defasagem do primeiro sinal de saída
      burstModeFlag1=HIGH; // Flag que sinaliza a parte do ciclo após borda de descida do primeiro sinal de saída  
    }
    if (burstModeFlag1==HIGH && fallingEdgeFlag1==LOW) // Rotina durante parte secundária do ciclo de acionamento
    {
      if (cycleCounter1<=primaryCycles-2) // Limita o número de ciclos de acionamento da bobina
      {
        if (ign1OutState==LOW) // Condição para que a contagem de tempo de "descanso" da bobina funcione
        {
          if (currentMicros1-previousTime1>=burstInterval) // Delimita o intervalo de tempo de "descanso" da bobina
          {
            ign1OutState=HIGH; //Aciona a bobina após intervalo de descanso
            previousTime1=currentMicros1; // Reset do intervalo de tempo
          }    
        }
        else if (currentMicros1-previousTime1>=burstDwellTime) // Delimita o dwell secundario da bobina
        {
          ign1OutState=LOW; // Desaciona a bobina após dwell secundario
          previousTime1=currentMicros1; // Reset do intervalo de tempo
          cycleCounter1++; // Contador de ciclos de acionamento secundário
        }    
      }
      else
      {
        cycleCounter1=0; // Reset do contador após numero de ciclos desejado alcançado
        i=LOW;
        burstModeFlag1=LOW; // Reset da flag que sinaliza a segunda parte do ciclo de acionamento

      }


    }

  }



}



//-------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------

void PhaseIgn2()   //Função para copiar o sinal de entrada na borda de subida, defasar a primeira borda de descida e depois comandar o número de pulsos desejados, com intervalo e acionamento determinados
{

  static volatile uint8_t previousInputState;
  static volatile uint8_t fallingEdgeFlag2;
  static volatile uint8_t burstModeFlag2;




  //previousTime2=currentMicros1;



  if (ign2InState==HIGH)
  {   
    if (currentMicros2-previousTime4>=phasingTime)
    {
      ign4OutState=HIGH; 
      previousInputState=HIGH; 
    }
    else
    {
      ign4OutState=LOW;
    }   
  }
  else
  {
    

    if (previousInputState==HIGH) //Comparação a fim de identificar a borda de descida do sinal de entrada (ECU)
    {
      fallingEdgeFlag2=HIGH; //Flag que sinaliza a parte do ciclo após borda de descida do sinal de entrada
      previousTime4=currentMicros2; // Reset do intervalo de tempo que permite o phasing após borda de descida do sinal de entrada
      previousInputState=LOW;
    }    

    if (fallingEdgeFlag2==HIGH) //Rotina enquanto a flag que sinaliza a borda de descida do sinal original está acionada
    {
      if (currentMicros2-previousTime4>=phasingTime) //Comparação de tempo entre borda de descida e phasing da segunda bobina em relação à primeira
      {
        ign4OutState=LOW; // Após final do tempo de phasing da segunda bobina em relação à primeira, o sinal de saída é desligado
        previousTime4=currentMicros2; // Reset de tempo que permite contagem após borda de descida do sinal defasado
        fallingEdgeFlag2=LOW; // Reset do flag que sinaliza o período de defasagem do primeiro sinal de saída
        burstModeFlag2=HIGH; // Flag que sinaliza a parte do ciclo após borda de descida do primeiro sinal de saída
      }
      else
      {
        ign4OutState=HIGH; // Permanece o sinal de saída acionado enquanto o primeiro tempo de defasagem não é alcançado
        fallingEdgeFlag2=HIGH; // Mantém o flag pós borda de descida do sinal de entrada ativo
      }
    }
    if (burstModeFlag2==HIGH && fallingEdgeFlag2==LOW) // Rotina durante parte secundária do ciclo de acionamento
    {
      if (cycleCounter4<=secondaryCycles-2) // Limita o número de ciclos de acionamento da bobina
      {
        if (ign4OutState==LOW) // Condição para que a contagem de tempo de "descanso" da bobina funcione
        {
          if (currentMicros2-previousTime4>=burstInterval) // Delimita o intervalo de tempo de "descanso" da bobina
          {
            ign4OutState=HIGH; //Aciona a bobina após intervalo de descanso
            previousTime4=currentMicros2; // Reset do intervalo de tempo
          }    
        }
        else if (currentMicros2-previousTime4>=burstDwellTime) // Delimita o dwell secundario da bobina
        {
          ign4OutState=LOW; // Desaciona a bobina após dwell secundario
          previousTime4=currentMicros2; // Reset do intervalo de tempo
          cycleCounter4++; // Contador de ciclos de acionamento secundário
        }    
      }
      else
      {
        cycleCounter4=0; // Reset do contador após numero de ciclos desejado alcançado
        burstModeFlag2=LOW; // Reset da flag que sinaliza a segunda parte do ciclo de acionamento
      }


    }

  }



}



//-------------------------------------------------------------------------------------------------------------


void PhaseIgn3()   //Função para copiar o sinal de entrada na borda de subida, defasar a primeira borda de descida e depois comandar o número de pulsos desejados, com intervalo e acionamento determinados
{

  static volatile uint8_t previousInputState;
  static volatile uint8_t fallingEdgeFlag3;
  static volatile uint8_t burstModeFlag3;




  //previousTime2=currentMicros1;



  if (ign3InState==HIGH)
  {   
    if (currentMicros3-previousTime6>=phasingTime)
    {
      ign6OutState=HIGH; 
      previousInputState=HIGH; 
    }
    else
    {
      ign6OutState=LOW;
    }   
  }
  else
  {
    

    if (previousInputState==HIGH) //Comparação a fim de identificar a borda de descida do sinal de entrada (ECU)
    {
      fallingEdgeFlag3=HIGH; //Flag que sinaliza a parte do ciclo após borda de descida do sinal de entrada
      previousTime6=currentMicros3; // Reset do intervalo de tempo que permite o phasing após borda de descida do sinal de entrada
      previousInputState=LOW;
    }    

    if (fallingEdgeFlag3==HIGH) //Rotina enquanto a flag que sinaliza a borda de descida do sinal original está acionada
    {
      if (currentMicros3-previousTime6>=phasingTime) //Comparação de tempo entre borda de descida e phasing da segunda bobina em relação à primeira
      {
        ign6OutState=LOW; // Após final do tempo de phasing da segunda bobina em relação à primeira, o sinal de saída é desligado
        previousTime6=currentMicros3; // Reset de tempo que permite contagem após borda de descida do sinal defasado
        fallingEdgeFlag3=LOW; // Reset do flag que sinaliza o período de defasagem do primeiro sinal de saída
        burstModeFlag3=HIGH; // Flag que sinaliza a parte do ciclo após borda de descida do primeiro sinal de saída
      }
      else
      {
        ign6OutState=HIGH; // Permanece o sinal de saída acionado enquanto o primeiro tempo de defasagem não é alcançado
        fallingEdgeFlag3=HIGH; // Mantém o flag pós borda de descida do sinal de entrada ativo
      }
    }
    if (burstModeFlag3==HIGH && fallingEdgeFlag3==LOW) // Rotina durante parte secundária do ciclo de acionamento
    {
      if (cycleCounter6<=secondaryCycles-2) // Limita o número de ciclos de acionamento da bobina
      {
        if (ign6OutState==LOW) // Condição para que a contagem de tempo de "descanso" da bobina funcione
        {
          if (currentMicros3-previousTime6>=burstInterval) // Delimita o intervalo de tempo de "descanso" da bobina
          {
            ign6OutState=HIGH; //Aciona a bobina após intervalo de descanso
            previousTime6=currentMicros3; // Reset do intervalo de tempo
          }    
        }
        else if (currentMicros3-previousTime6>=burstDwellTime) // Delimita o dwell secundario da bobina
        {
          ign6OutState=LOW; // Desaciona a bobina após dwell secundario
          previousTime6=currentMicros3; // Reset do intervalo de tempo
          cycleCounter6++; // Contador de ciclos de acionamento secundário
        }    
      }
      else
      {
        cycleCounter6=0; // Reset do contador após numero de ciclos desejado alcançado
        burstModeFlag3=LOW; // Reset da flag que sinaliza a segunda parte do ciclo de acionamento
      }


    }

  }



}



//-------------------------------------------------------------------------------------------------------------



void MainIgn2()   //Função para copiar o sinal de entrada na borda de subida e depois comandar o número de pulsos desejados, com intervalo e numero de ciclos determinados
{
  static volatile uint8_t fallingEdgeFlag2;
  static volatile uint8_t burstModeFlag2;
  static volatile uint8_t previousInputState;
  currentMicros2=micros();



  //i=ign1InState;  //variável que guarda o estado de entrada do ciclo anterior, a fim de ser possível sinalizar a troca de estado
  //  ign1InState= digitalRead(ign1InPin); // Atualiza o estado da variável de entrada


  if (ign2InState==HIGH)
  { 
    ign3OutState=ign2InState; // Durante o acionamento através da ECU, o sinal de saída do uC permanece tambem acionado
    previousInputState=HIGH;    
  }
  else
  {

    if (previousInputState==HIGH) //Comparação a fim de identificar a borda de descida do sinal de entrada (ECU)
    {   

      fallingEdgeFlag2=HIGH; //Flag que sinaliza a parte do ciclo após borda de descida do sinal de entrada
      previousTime3=currentMicros2; // Reset do intervalo de tempo que permite o phasing após borda de descida do sinal de entrada
      previousInputState=LOW;
    }    

    if (fallingEdgeFlag2==HIGH) //Rotina enquanto a flag que sinaliza a borda de descida do sinal original está acionada
    {

      ign3OutState=LOW; // Após final do tempo de phasing da segunda bobina em relação à primeira, o sinal de saída é desligado
      previousTime3=currentMicros2; // Reset de tempo que permite contagem após borda de descida do sinal defasado
      fallingEdgeFlag2=LOW; // Reset do flag que sinaliza o período de defasagem do primeiro sinal de saída
      burstModeFlag2=HIGH; // Flag que sinaliza a parte do ciclo após borda de descida do primeiro sinal de saída  
    }
    if (burstModeFlag2==HIGH && fallingEdgeFlag2==LOW) // Rotina durante parte secundária do ciclo de acionamento
    {
      if (cycleCounter3<=primaryCycles-2) // Limita o número de ciclos de acionamento da bobina
      {
        if (ign3OutState==LOW) // Condição para que a contagem de tempo de "descanso" da bobina funcione
        {
          if (currentMicros2-previousTime3>=burstInterval) // Delimita o intervalo de tempo de "descanso" da bobina
          {
            ign3OutState=HIGH; //Aciona a bobina após intervalo de descanso
            previousTime3=currentMicros2; // Reset do intervalo de tempo
          }    
        }
        else if (currentMicros2-previousTime3>=burstDwellTime) // Delimita o dwell secundario da bobina
        {
          ign3OutState=LOW; // Desaciona a bobina após dwell secundario
          previousTime3=currentMicros2; // Reset do intervalo de tempo
          cycleCounter3++; // Contador de ciclos de acionamento secundário
        }    
      }
      else
      {
        cycleCounter3=0; // Reset do contador após numero de ciclos desejado alcançado
        i=LOW;
        burstModeFlag2=LOW; // Reset da flag que sinaliza a segunda parte do ciclo de acionamento

      }


    }

  }



}



//-------------------------------------------------------------------------------------------------------------

void MainIgn3()   //Função para copiar o sinal de entrada na borda de subida e depois comandar o número de pulsos desejados, com intervalo e numero de ciclos determinados
{
  static volatile uint8_t fallingEdgeFlag3;
  static volatile uint8_t burstModeFlag3;
  static volatile uint8_t previousInputState;
  currentMicros3=micros();



  //i=ign1InState;  //variável que guarda o estado de entrada do ciclo anterior, a fim de ser possível sinalizar a troca de estado
  //  ign1InState= digitalRead(ign1InPin); // Atualiza o estado da variável de entrada


  if (ign3InState==HIGH)
  { 
    ign5OutState=ign3InState; // Durante o acionamento através da ECU, o sinal de saída do uC permanece tambem acionado
    previousInputState=HIGH;    
  }
  else
  {

    if (previousInputState==HIGH) //Comparação a fim de identificar a borda de descida do sinal de entrada (ECU)
    {   

      fallingEdgeFlag3=HIGH; //Flag que sinaliza a parte do ciclo após borda de descida do sinal de entrada
      previousTime5=currentMicros3; // Reset do intervalo de tempo que permite o phasing após borda de descida do sinal de entrada
      previousInputState=LOW;
    }    

    if (fallingEdgeFlag3==HIGH) //Rotina enquanto a flag que sinaliza a borda de descida do sinal original está acionada
    {

      ign5OutState=LOW; // Após final do tempo de phasing da segunda bobina em relação à primeira, o sinal de saída é desligado
      previousTime5=currentMicros3; // Reset de tempo que permite contagem após borda de descida do sinal defasado
      fallingEdgeFlag3=LOW; // Reset do flag que sinaliza o período de defasagem do primeiro sinal de saída
      burstModeFlag3=HIGH; // Flag que sinaliza a parte do ciclo após borda de descida do primeiro sinal de saída  
    }
    if (burstModeFlag3==HIGH && fallingEdgeFlag3==LOW) // Rotina durante parte secundária do ciclo de acionamento
    {
      if (cycleCounter5<=primaryCycles-2) // Limita o número de ciclos de acionamento da bobina
      {
        if (ign5OutState==LOW) // Condição para que a contagem de tempo de "descanso" da bobina funcione
        {
          if (currentMicros3-previousTime5>=burstInterval) // Delimita o intervalo de tempo de "descanso" da bobina
          {
            ign5OutState=HIGH; //Aciona a bobina após intervalo de descanso
            previousTime5=currentMicros3; // Reset do intervalo de tempo
          }    
        }
        else if (currentMicros3-previousTime5>=burstDwellTime) // Delimita o dwell secundario da bobina
        {
          ign5OutState=LOW; // Desaciona a bobina após dwell secundario
          previousTime5=currentMicros3; // Reset do intervalo de tempo
          cycleCounter5++; // Contador de ciclos de acionamento secundário
        }    
      }
      else
      {
        cycleCounter5=0; // Reset do contador após numero de ciclos desejado alcançado
        i=LOW;
        burstModeFlag3=LOW; // Reset da flag que sinaliza a segunda parte do ciclo de acionamento

      }


    }

  }



}



//-------------------------------------------------------------------------------------------------------------







void ActuateOutput()
{
  digitalWrite(ignOut1Pin, ign1OutState);
  digitalWrite(ignOut2Pin, ign2OutState);
  digitalWrite(ignOut3Pin, ign3OutState);
  digitalWrite(ignOut4Pin, ign4OutState);
  digitalWrite(ignOut5Pin, ign5OutState);
  digitalWrite(ignOut6Pin, ign6OutState);
}

//------------------------------------------------------------------------------------------------

void loop()
{

  if (risingEdgeFlag1==true)
  {   ign1InState=digitalRead(ign1InPin);
    // i=true;
    MainIgn1();



    if(secondCoilEnable==true)
    {

      PhaseIgn1();
    }
  }
//---------------------

   if (risingEdgeFlag2==true)
  {   ign2InState=digitalRead(ign2InPin);
    // i=true;
    MainIgn2();



    if(secondCoilEnable==true)
    {

      PhaseIgn2();
    }
  }


//--------------------

   if (risingEdgeFlag3==true)
  {   ign3InState=digitalRead(ign3InPin);
    // i=true;
    MainIgn3();



    if(secondCoilEnable==true)
    {

      PhaseIgn3();
    }
  }


//--------------------


  ActuateOutput();
  //Serial.print("Main Loop: Executing on core ");
  // Serial.println(uxTaskPriorityGet(NULL));
  //Serial.println(xPortGetCoreID());


}

//-----------------------------------------------------------------------------------------------------------
