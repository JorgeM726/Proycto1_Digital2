#include "config.h"
// Incluye la biblioteca para controlar pines en el ESP32
#include <Arduino.h>
#include <ESP32Servo.h>


#define RXD2 5
#define TXD2 4



char temp[2];
char humidity[2];
char flux[3];
char alarma[1];

char dtemp[3];
char dhum[3];
char dflux[4];
char dalarma[2];

int Itemp = 0;
int Ihum = 0;
int Iflux = 0;
int Ialarma = 0;

int segundos;

String mistring4;

// Define los feeds de Adafruit IO
AdafruitIO_Feed *flujoFeed = io.feed("flujo");
AdafruitIO_Feed *tempFeed = io.feed("temp");
AdafruitIO_Feed *humedadFeed = io.feed("humedad");
AdafruitIO_Feed *tiempoFeed = io.feed("tiempo");

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2); // Corregido: Usar rx2pin y tx2pin
  
  // Establece los pines como salidas
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  while (!Serial); // Espera a que el monitor serial esté listo
  Serial.println("Connecting to Adafruit IO");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println(io.statusText());
  }

void loop() {
    io.run(); // Mantén la conexión con Adafruit IO

  
  Serial1.println("2");
  while(!Serial1.available());
  if (Serial1.available() > 0) {
    
    Serial1.readBytesUntil('\n', temp, 2);
    Serial.println("Recibido t");
    strncpy(dtemp,temp,2);
    dtemp[2]='\0';
    Serial.println(dtemp);
    tempFeed->save(dtemp);

    for (int i = 0; dtemp[i] != '\0'; ++i) {
    Itemp = Itemp * 10 + (dtemp[i] - '0');
}
memset(dtemp, 0, sizeof(dtemp));
    Serial.println(Itemp);
    Serial1.flush();
  }
  delay(2000); // Espera un segundo antes de enviar el siguiente valor
  Itemp = 0;

  
  
  Serial1.println("3");
   while(!Serial1.available());
  if (Serial1.available() > 0) {
  
    Serial1.readBytesUntil('\n', humidity, 2);
    Serial.println("Recibido h");
    
    strncpy(dhum,humidity,2);
    dhum[2]='\0';
    Serial.println(dhum);
    humedadFeed->save(dhum);
    for (int i = 0; dhum[i] != '\0'; ++i) {
    Ihum = Ihum * 10 + (dhum[i] - '0');
}
    

    Serial1.flush();
    
  }
  delay(2000); // Espera un segundo antes de enviar el siguiente valor

  Serial1.println("1");
  while(!Serial1.available());
  if (Serial1.available() > 0) {
    
    Serial1.readBytesUntil('\n', flux, 3);
    Serial.println("Recibidof ");
    strncpy(dflux,flux,3);
    dflux[3]='\0';
    Serial.println(dflux);
    flujoFeed->save(dflux);
    Serial1.flush();
  }
  delay(2000); // Espera un segundo antes de enviar el siguiente valor
  
  Serial1.println("4");
   while(!Serial1.available());
  if (Serial1.available() > 0) {
  
    Serial1.readBytesUntil('\n', alarma, 2);
    Serial.println("Alarma recibida");
    strncpy(dalarma,alarma,2);
    dalarma[1]='\0';
    
    tiempoFeed->save(dalarma);
     for (int i = 0; dalarma[i] != '\0'; ++i) {
    Ialarma = Ialarma * 10 + (dalarma[i] - '0');
}

    fal
    
    
    Serial1.flush();
  }
  delay(2000); // Espera un segundo antes de enviar el siguiente valor

  }