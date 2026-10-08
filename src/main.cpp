#include <Arduino.h>

//Librerias
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

//  Pines ARDUINO
#define DHTPIN       2
#define DHTTYPE      DHT22
#define GAS_PIN      A0
#define LED_VERDE    6
#define LED_ROJO     7
#define BUZZER       8
#define VENTILADOR   9  

// Datos de Prueba
const float TEMP_ADVERTENCIA = 30.0;  
const float TEMP_INCENDIO    = 40.0; 
const int   GAS_ADVERTENCIA  = 400;   
const int   GAS_PELIGRO      = 800;

LiquidCrystal_I2C lcd(0x27, 16, 2); 
DHT dht(DHTPIN, DHTTYPE);

//Estados de la Habitacion
enum Estado { NORMAL, ADVERTENCIA, FUGA_GAS, INCENDIO };
Estado estado = NORMAL;
Estado estadoAnterior = INCENDIO; 


float temperatura = 25.0;
float humedad = 50.0;
int   gas = 0;
bool  ventilador = false;

unsigned long ultimaLectura = 0;
unsigned long ultimoCambioPantalla = 0;
bool pantallaAlterna = false;

// Escribimos en la pantalla
void escribirLinea(int fila, String texto) {
  while (texto.length() < 16) texto += " ";
  lcd.setCursor(0, fila);
  lcd.print(texto.substring(0, 16));
}

//leemos el resultado de los sensores
void leerSensores() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperatura = t;
  if (!isnan(h)) humedad = h;
  gas = analogRead(GAS_PIN);


//Condicionamos las alertas con el limite soportado
  if (temperatura >= TEMP_INCENDIO) {
    estado = INCENDIO;
  } else if (gas >= GAS_PELIGRO) {
    estado = FUGA_GAS;
  } else if (temperatura >= TEMP_ADVERTENCIA || gas >= GAS_ADVERTENCIA) {
    estado = ADVERTENCIA;
  } else {
    estado = NORMAL;
  }

  // El ventilador se activa solo en peligro
  ventilador = (estado == INCENDIO || estado == FUGA_GAS || estado == ADVERTENCIA);
  digitalWrite(VENTILADOR, ventilador ? HIGH : LOW);

  Serial.print("Temp: "); Serial.print(temperatura);
  Serial.print(" C | Hum: "); Serial.print(humedad);
  Serial.print(" % | Gas: "); Serial.print(gas);
  Serial.print(" | Estado: "); Serial.print(estado);
  Serial.print(" | Ventilador: "); Serial.println(ventilador ? "ON" : "OFF");
}

void actualizarPantalla() {
  switch (estado) {
    case NORMAL:
      escribirLinea(0, "SISTEMA NORMAL");
      if (pantallaAlterna) escribirLinea(1, "No hay peligros");
      else escribirLinea(1, "T:" + String(temperatura, 1) + "C H:" + String(humedad, 0) + "%");
      break;

    case ADVERTENCIA:
      escribirLinea(0, "ADVERTENCIA");
      if (gas >= GAS_ADVERTENCIA && !pantallaAlterna) escribirLinea(1, "Humo detectado");
      else escribirLinea(1, "Temp: " + String(temperatura, 1) + "C");
      break;

    case FUGA_GAS:
      escribirLinea(0, "PELIGRO! EVACUAR");
      if (pantallaAlterna) escribirLinea(1, "VENTILADOR: ON");
      else escribirLinea(1, "FUGA DE GAS");
      break;

    case INCENDIO:
      escribirLinea(0, "!INCENDIO!");
      if (pantallaAlterna) escribirLinea(1, "VENTILADOR: ON");
      else escribirLinea(1, "ALARMA ACTIVADA");
      break;
  }
}

void actualizarSalidas() {
  unsigned long ahora = millis();

  switch (estado) {
    case NORMAL:
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_ROJO, LOW);
      noTone(BUZZER);
      break;

    //Enciende Led rojo y hace un ruido
    case ADVERTENCIA:
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_ROJO, (ahora / 500) % 2);
      if (ahora % 1000 < 100) tone(BUZZER, 1000);
      else noTone(BUZZER);
      break;

    //Enciende led rojo con sonidos mas rapidos
    case FUGA_GAS:      
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_ROJO, (ahora / 150) % 2);
      if ((ahora / 200) % 2) tone(BUZZER, 2000);
      else noTone(BUZZER);
      break;

    //led rojo mas una sirena en incedios
    case INCENDIO:      
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_ROJO, HIGH);
      if ((ahora / 300) % 2) tone(BUZZER, 1500);
      else tone(BUZZER, 900);
      break;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(VENTILADOR, OUTPUT);

  dht.begin();
  lcd.init();
  lcd.backlight();
  escribirLinea(0, "Detector de");
  escribirLinea(1, "incendios...");
  delay(1500);
  lcd.clear();
}

void loop() {
  unsigned long ahora = millis();

  if (ahora - ultimaLectura >= 500) {
    ultimaLectura = ahora;
    leerSensores();
    actualizarPantalla();
  }

 
  if (ahora - ultimoCambioPantalla >= 1500) {
    ultimoCambioPantalla = ahora;
    pantallaAlterna = !pantallaAlterna;
    actualizarPantalla();
  }

  if (estado != estadoAnterior) {
    estadoAnterior = estado;
    lcd.clear();
    actualizarPantalla();
  }

  actualizarSalidas();
}
