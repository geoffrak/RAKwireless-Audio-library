/** 
 * @file VoiceConfigWisDM.ino 
 * @author rakwireless.com
 * @brief This example shows how to use the RAK voice recognition suite to identify 
 * specific voice command words to obtain device information through lora. 
 * If there is no problem with your module, the key voice command words will be printed
 * on the serial port after the device is initialized.Saying "Hey RAKstar" to it will wake 
 * it up into voice recognition and the blue and green LEDs on the WisBlock Base board will light up. 
 * You can say other command words to it while the light is on to make it work for you.
 * @note This example requires a core that supports lora.
 * @version 0.1 
 * @date 2022-08-6   
 * @copyright Copyright (c) 2022
*/
#include "audio.h"
#include <Arduino.h>
#include <LoRaWan-RAK4630.h> //http://librarymanager/All#SX126x
#include <SPI.h>
#include <CDSpotter.h>
#include "RAKwireless_Demo_pack_WithTxt_Lv0.h"

#define DSPOTTER_MODEL           g_lpdwRAKwireless_Demo_pack_WithTxt_Lv0
#define  COMMAND_GROUP_CHOOSE    4
#define COMMAND_STAGE_TIMEOUT    6000                    // The minimum recording time in ms when there is no result at command stage.
#define COMMAND_STAGE_REPEAT     1                       // When it is 1, sample code will recognize repeatly at command stage till to timeout.
                                                         // Otherwise, it will switch to trigger stage immediately after command recognized.

TPT29555   Expander1(0x23);
TPT29555   Expander2(0x25);

// default number of output channels
static const char channels = 1;

// default PCM output frequency
static const int frequency = 16000;

#define SAMPLE_READ_NUM     64
// Buffer to read samples into, each sample is 16-bits
short sampleBuffer[SAMPLE_READ_NUM];

// Number of audio samples read
volatile int samplesRead;
//VR
static CDSpotter g_hDSpotter;
int g_nCurrentStage = 0;
uint32_t *g_ptr;//For license

void RAK18003Init(void);
void voice_case(int element);
void setup() {
  int nMempoolSize = 0, nErr = -1;
  byte *pMemPool = 0;
  int nActiveCommandGroup = COMMAND_GROUP_CHOOSE;

  //Serial.println("Before PDM init");
  //rp2040
   //Serial.println("Before PDM init");
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, HIGH);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_BLUE, HIGH);

  // Initialize Serial for debug output
  time_t timeout = millis();
  Serial.begin(115200);
  while (!Serial)
  {
    if ((millis() - timeout) < 3000)
    {
      delay(100);
    }
    else
    {
      break;
    }
  }
 
  RAK18003Init();  
  // Configure the data receive callback
  PDM.onReceive(onPDMdata);
  // Initialize PDM with:
  // - one channel (mono mode)
  // - 16 kHz sample rate
  if (!PDM.begin(right, frequency)) {
    Serial.print("Failed to start PDM!");
    while (1);
  }

  //Get License from flash
  Audio_Handle.begin();
  g_ptr = Audio_Handle.GetLicenseAddr();
  Serial.println(CDSpotter::GetSerialNumber());
  //DSpotter
  nMempoolSize = CDSpotter::GetMemoryUsage(DSPOTTER_MODEL,60);
  Serial.print("DSpotter mem usage");
  Serial.println(nMempoolSize);
  pMemPool = (byte*)malloc(sizeof(byte)*nMempoolSize);
  if(!pMemPool)
    Serial.print("allocate DSpotter mempool failed");
  //nErr = g_hDSpotter.Init(g_lpdwLicense,sizeof(g_lpdwLicense),DSPOTTER_MODEL,60,pMemPool,nMempoolSize);
  nErr = g_hDSpotter.Init(g_ptr,LICEENSE_LENGTH * sizeof(uint32_t),DSPOTTER_MODEL,60,pMemPool,nMempoolSize);
  if(nErr!=CDSpotter::Success)
  {
    Serial.print("DSpotter err: ");
    Serial.println(nErr);
    if(nErr==CDSpotter::LicenseFailed)
    {
      Serial.println("License err, please check register under device ID for License");
      Serial.println(CDSpotter::GetSerialNumber());
    }
  }
  else
  {
    Serial.println("DSpotter init success");
  }

  //Add Set Group model API
  nErr = g_hDSpotter.SetActiveCommandGroup(nActiveCommandGroup);
  if(nErr!=CDSpotter::Success){
  Serial.println("Set active commands group failed, using default");
  nActiveCommandGroup = 1;
  }
  //show command
  for (int nStage = 0; nStage < 2; nStage++)
  {
    char szCommand[64];
    int nID;
    int nGroupChoose;
    if(nStage==0)
    {
        Serial.println("The list of Trigger words: ");
      nGroupChoose = 0;
    }
      else
    {
        Serial.println("The list of Command words: ");
      nGroupChoose = nActiveCommandGroup;
    }
    for(int i = 0; i < g_hDSpotter.GetCommandCount(nGroupChoose); i++)
    {
      g_hDSpotter.GetCommand(nGroupChoose, i, szCommand, sizeof(szCommand), &nID);
      Serial.print(szCommand);
      Serial.print(" , ID = ");
      Serial.println(nID);
    }
    Serial.println("");
  }
  Serial.println("");
  //set 2 stage timeout
  g_hDSpotter.SetCommandStageProperty(COMMAND_STAGE_TIMEOUT, COMMAND_STAGE_REPEAT == 1);
  //Start VR
  g_hDSpotter.Start();
  Serial.println("Enter Trigger state");
  
  lora_init();
  digitalWrite(LED_BLUE, LOW);
  digitalWrite(LED_GREEN, LOW);
}

void loop() {
  int nRes = -1,nDataSize = 0;
  int nFil, nSG, nScore, nID;
  int nStage = 0;
  short sSample[512];
  char pCommand[64];
    
  //audio data lost resource not enough to do VR
//  if(g_hDSpotter.GetRecordLostCount()>0) //need remark this line too
//    Serial.println("drop data");///LEDblink();

  //Do VR
  nRes = g_hDSpotter.DoRecognition(&nStage);

  //VR return Key word get keyword info
  if(nRes==CDSpotter::Success)
  {
    Serial.print("Detect ID: ");
    g_hDSpotter.GetRecogResult(&nID, pCommand, sizeof(pCommand), &nScore, &nSG, &nFil);
    Serial.print(pCommand);
    Serial.print(" ");
    Serial.println(nID);
    Serial.print("SG: ");
    Serial.print(nSG);
    Serial.print(" Energy: ");
    Serial.print(nFil);
    Serial.print(" Score: ");
    Serial.println(nScore);
    voice_case(nID);
  }
  //Check VR stage
  if (nStage != g_nCurrentStage)
  {
    g_nCurrentStage  = nStage; 
    if (nStage == CDSpotter::TriggerStage)
    {
      digitalWrite(LED_BLUE, LOW);
      digitalWrite(LED_GREEN, LOW);
      Serial.println("VR Switch to Trigger Stage");
    }
    else if (nStage == CDSpotter::CommandStage)
    {
      digitalWrite(LED_BLUE, HIGH);
      digitalWrite(LED_GREEN, HIGH);
      Serial.println("VR Switch to Command Stage");
    }
  }
  // Clear the read count
  samplesRead = 0;
  
}
/**
   Callback function to process the data from the PDM microphone.
   NOTE: This callback is executed as part of an ISR.
   Therefore using `Serial` to print messages inside this function isn't supported.
 * */
void onPDMdata() {
  // Query the number of available bytes
  int nLeftRBBufferSize;
  //int bytesAvailable = PDM.available();
  
  //Serial.println(bytesAvailable);
  //delay(1000);
  // Read into the sample buffer
  PDM.read(sampleBuffer, SAMPLE_READ_NUM*2);

  // 16-bit, 2 bytes per sample
  //samplesRead = bytesAvailable / 2;
  //put to Ringbuffer and wait for do VR
  g_hDSpotter.PutRecordData(sampleBuffer, SAMPLE_READ_NUM);
  
}
void RAK18003Init(void)
{
  if(!Expander1.begin())
  {
    Serial.println("Did not find IO Expander Chip1");   
  }

  if(!Expander2.begin())
  {
    Serial.println("Did not find IO Expander Chip2");      
  }  
  Expander1.pinMode(0, INPUT);    //SD check
  Expander1.pinMode(1, INPUT);    //MIC check
  Expander1.pinMode(2, INPUT);    //MIC CTR1
  Expander1.pinMode(3, INPUT);    //MIC CTR2
  Expander1.pinMode(4, INPUT);    //AMP check
  Expander1.pinMode(5, INPUT);    //AMP CTR1
  Expander1.pinMode(6, INPUT);    //AMP CTR2
  Expander1.pinMode(7, INPUT);    //AMP CTR3
  Expander1.pinMode(8, INPUT);    //DSP check
  Expander1.pinMode(9, INPUT);    //DSP CTR1  DSP int 
  Expander1.pinMode(10, INPUT);   //DSP CTR2  DSP ready
  Expander1.pinMode(11, OUTPUT);  //DSP CTR3  DSP reset
  Expander1.pinMode(12, INPUT);   //DSP CTR4  not use
  Expander1.pinMode(13, INPUT);   //DSP CTR5  not use
  Expander1.pinMode(14, INPUT);   //NOT USE
  Expander1.pinMode(15, INPUT);   //NOT USE

//  Expander1.digitalWrite(14, 0);    //set chip 1 not use pin output low
//  Expander1.digitalWrite(15, 0);    //set chip 1 not use pin output low
    
  Expander2.pinMode(0, OUTPUT);  //CORE  SPI CS1 for DSPG CS
  Expander2.pinMode(1, OUTPUT);  //CORE  SPI CS2   
  Expander2.pinMode(2, OUTPUT);  //CORE  SPI CS3
  Expander2.pinMode(3, OUTPUT);  //PDM switch CTR    1 to dsp   0 to core
  Expander2.pinMode(4, INPUT);  //not use
  Expander2.pinMode(5, INPUT);  //not use
  Expander2.pinMode(6, INPUT);  //not use
  Expander2.pinMode(7, INPUT);  //not use
  Expander2.pinMode(8, INPUT);  //not use
  Expander2.pinMode(9, INPUT);  //not use
  Expander2.pinMode(10, INPUT); //not use
  Expander2.pinMode(11, INPUT); //not use
  Expander2.pinMode(12, INPUT); //not use
  Expander2.pinMode(13, INPUT); //not use
  Expander2.pinMode(14, INPUT); //not use
  Expander2.pinMode(15, INPUT); //not use 

  Expander2.digitalWrite(0, 1);  //set SPI CS1 High 
  Expander2.digitalWrite(1, 1);  //set SPI CS2 High  
  Expander2.digitalWrite(2, 1);  //set SPI CS3 High  
  Expander2.digitalWrite(3,0);    //set the PDM data direction from MIC to WisCore

  if(Expander1.digitalRead(1) == 0)  //Check if the microphone board is connected on the RAK18003
  {
    Serial.println("There is no microphone board, please check !");     
  }
}
void voice_case(int element)
{   
  String str;
  char send_buff[40];
  memset(send_buff,0,sizeof(send_buff));
  /*
   * You can add tasks that you want under each case.
   */
  switch (element)
  {
    case 4000:  //Connect
      lora_init();
      Serial.println("Connect");
      break;
    case 4001:  //Disconnect
    
      Serial.println("Disconnect");
      break;
    case 4002:  //Report Location
      str="Report Location";  
      str.toCharArray(send_buff,str.length()+1);
      send_lora_frame((uint8_t *)&send_buff,sizeof(send_buff));
      Serial.println("Report Location");
      break;
    case 4003:  //Get Status
      str = "Get Status";      
      str.toCharArray(send_buff,str.length()+1);
      send_lora_frame((uint8_t *)&send_buff,sizeof(send_buff));
      Serial.println("Get Status");
      break;
    case 4004:  //Report Battery
      str = "Report Battery";      
      str.toCharArray(send_buff,str.length()+1);
      send_lora_frame((uint8_t *)&send_buff,sizeof(send_buff));
      Serial.println("Report Battery");
      break;
    case 4005:  //Report Environment
      str = "Report Environment";      
      str.toCharArray(send_buff,str.length()+1);
      send_lora_frame((uint8_t *)&send_buff,sizeof(send_buff));
      Serial.println("Report Environment");
      break;
    case 4006:  //Reset
    
      Serial.println("Reset");
      break;
    default:
      break;
  }
}
