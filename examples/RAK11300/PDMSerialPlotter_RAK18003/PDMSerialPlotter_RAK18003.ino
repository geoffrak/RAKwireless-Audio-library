/**
 * @file PDMSerialPlotter_RAK18003.ino
 * @author rakwireless.com
 * @brief This example reads audio data from the PDM microphones, and prints
 * out the samples to the Serial console. The Serial Plotter built into the
 * Arduino IDE can be used to plot the audio data (Tools -> Serial Plotter)
 * @note This example need use the RAK18003 module.
 * @version 0.1
 * @date 2022-06-6
 * @copyright Copyright (c) 2022
 */
 
#include "audio.h"

TPT29555   Expander1(0x23);
TPT29555   Expander2(0x25);

Channel_Mode channels = left; // stereo or left or right
int frequency = 16000;
// buffer to read samples into, each sample is 16-bits
short sampleBuffer[BUFFER_SIZE];

volatile uint8_t read_flag = 0;
void onPDMdata();
void RAK18003Init(void);

void setup() {
  
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
  
  // configure the data receive callback
  PDM.onReceive(onPDMdata);
  // initialize PDM with:
  // - one channel (mono mode)
  // - a 16 kHz sample rate
  if (!PDM.begin(channels, frequency)) {
    Serial.println("Failed to start PDM!");
    while (1);
  }
  
  Serial.println("=====================================");
  Serial.println("Audio test");
  Serial.println("=====================================");
}

void loop() {
  // wait for samples to be read
  if (read_flag == 1) {
    read_flag = 0;
    // print samples to the serial monitor or plotter
    for (int i = 0; i < BUFFER_SIZE; i++) {
      Serial.println(sampleBuffer[i]);
    }
  }
}
void onPDMdata() {
  // read into the sample buffer
  PDM.read(sampleBuffer, sizeof(sampleBuffer));
  read_flag = 1;
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

  // if(Expander1.digitalRead(0) == 1)  //Check SD card
  // {
  //   Serial.println("There is no SD card on the RAK18003 board, please check !");     
  // }
  
  if(Expander1.digitalRead(1) == 0)  //Check if the microphone board is connected on the RAK18003
  {
    Serial.println("There is no microphone board, please check !");     
  }

  // if(Expander1.digitalRead(4) == 0)  //Check if the RAK18060 AMP board is connected on the RAK18003
  // {
  //   Serial.println("There is no RAK18060 AMP board, please check !");     
  // }

  // if(Expander1.digitalRead(8) == 0)  //Check if the RAK18080 DSPG board is connected on the RAK18003
  // {
  //   Serial.println("There is no RAK18080 DSPG board, please check !");     
  // }
  
}