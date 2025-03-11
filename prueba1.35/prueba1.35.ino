#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"
#include <Servo.h>
#include <WiFiS3.h>
#include "arduino_secrets.h"
#include <WiFiClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Creación del objeto para la pantalla OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Pines para la balanza
const int DOUT = A0;
const int CLK = A1;

// Inicialización del objeto HX711
HX711 balanza;

// Definición de servos
Servo servoCompuerta1;
Servo servoClasificador;
Servo servoCompuerta2;

// Contadores de clasificación
int Huevopequeno = 0;
int Huevomediano = 0;
int Huevogrande = 0;
int Huevojumbo = 0;

int totalHuevos = 0;

// Pines de los servos
const int pinCompuerta1 = 9;
const int pinClasificador = 10;
const int pinCompuerta2 = 11;

// Posiciones de los servos
const int posCerradaCompuerta1 = 0;
const int posAbiertaCompuerta1 = 100;

const int posClasificadorInicial = 60;
const int posClasificador10_20 = 30;
const int posClasificador20_30 = 0;
const int posClasificador30_40 = 90;
const int posClasificador40_up = 120;

const int posCerradaCompuerta2 = 90;
const int posAbiertaCompuerta2 = 0;

// Variables para el clasificador
int pesoClasificar = 0;
bool nuevoPeso = false; // Indica si ya se procesó el peso actual

// Credenciales Wi-Fi (arduino_secrets.h)
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// URL del servidor
const char* serverUrl = "http://vrito.local:5000/update";

void setup() {
  Serial.begin(9600);
  
  // Configuración de la balanza
  balanza.begin(DOUT, CLK);
  Serial.print("Lectura del valor del ADC: ");
  Serial.println(balanza.read());
  Serial.println("No ponga ningún objeto sobre la balanza");
  Serial.println("Destarando...");
  Serial.println("...");
  balanza.set_scale(388896.875);
  balanza.tare();

  // Inicialización de la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("No se encontró una pantalla OLED"));
    for (;;);  // Se queda aquí si no se encuentra la pantalla
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Listo para pesar");
  display.display();

  // Configuración inicial de los servos
  servoCompuerta1.attach(pinCompuerta1);
  servoClasificador.attach(pinClasificador);
  servoCompuerta2.attach(pinCompuerta2);
  servoCompuerta1.write(posCerradaCompuerta1);
  servoClasificador.write(posClasificadorInicial);
  servoCompuerta2.write(posCerradaCompuerta2);

  // Apertura inicial de la primera compuerta
  delay(10000);
  servoCompuerta1.write(posAbiertaCompuerta1);
  delay(300);
  servoCompuerta1.write(posCerradaCompuerta1);

  // Conexión a la red Wi-Fi
  Serial.println("Conectando a Wi-Fi");
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("comunicacion con modulo WiFi fallido!");
    while (true);
  }
  
  Serial.print("Conectando a la red: ");
  Serial.println(ssid);
  
  int status = WiFi.begin(ssid, pass);
  while (status != WL_CONNECTED) {
    Serial.println("Intentando conectar a Wi-Fi...");
    delay(500);
    status = WiFi.status();
  }
  
  Serial.println("Conexión a Wi-Fi exitosa");
  printWiFiStatus();
}

void loop() {
  static float pesoAnterior = 0;
  static unsigned long tiempoEstable = 0;
  static const unsigned long tiempoParaEstabilizar = 500;
  
  // Se obtiene el peso actual en gramos (solo 5 muestras)
  float pesoActual = balanza.get_units(5) * 1000;
  int pesoRedondeado = round(pesoActual);

  if (abs(pesoRedondeado - pesoAnterior) < 2) {
    if (millis() - tiempoEstable >= tiempoParaEstabilizar) {
      display.clearDisplay();
      display.setCursor(0, 10);
      display.setTextSize(2);
      display.print("Peso:");
      display.print(pesoRedondeado);
      display.println(" g");
      display.display();

      if (!nuevoPeso) {
        if (pesoRedondeado > 10) {
          pesoClasificar = pesoRedondeado;
          nuevoPeso = true;

          // Clasificación según el peso medido
          int posicionClasificador = posClasificadorInicial;
          if (pesoClasificar >= 10 && pesoClasificar < 53) {
            posicionClasificador = posClasificador10_20;
            Huevopequeno++;
          } else if (pesoClasificar >= 53 && pesoClasificar < 63) {
            posicionClasificador = posClasificador20_30;
            Huevomediano++;
          } else if (pesoClasificar >= 63 && pesoClasificar < 73) {
            posicionClasificador = posClasificador30_40;
            Huevogrande++;
          } else if (pesoClasificar >= 73) {
            posicionClasificador = posClasificador40_up;
            Huevojumbo++;
          }

          // Secuencia de movimiento de servos
          delay(200);
          servoClasificador.write(posicionClasificador);
          delay(200);
          
          servoCompuerta2.write(posAbiertaCompuerta2);
          delay(300);
          servoCompuerta2.write(posCerradaCompuerta2);
          delay(300);
          
          servoClasificador.write(posClasificadorInicial);
          delay(300);
          
          servoCompuerta1.write(posAbiertaCompuerta1);
          delay(300);
          servoCompuerta1.write(posCerradaCompuerta1);
          delay(300);

          totalHuevos++;

          // Enviar datos al servidor
          if (totalHuevos >= 12) {
            enviarDatosAlServidor(pesoClasificar);
            totalHuevos = 0;
            Huevopequeno = 0;
            Huevomediano = 0;
            Huevogrande = 0;
            Huevojumbo = 0;
          }

        } else {
          nuevoPeso = true;
        }
      }
    }
  } else {
    tiempoEstable = millis();
    nuevoPeso = false;
  }

  if (pesoRedondeado == 0) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.setTextSize(2);
    display.print("Peso:");
    display.print("0 g");
    display.display();
    nuevoPeso = false;
  }

  pesoAnterior = pesoRedondeado;
  delay(200);
}

void enviarDatosAlServidor(int peso) {
  WiFiClient client;
  if (client.connect("vrito.local", 5000)) {
    String jsonData = "{\"peso\":" + String(peso) +
                      ",\"pequeno\":" + String(Huevopequeno) +
                      ",\"mediano\":" + String(Huevomediano) +
                      ",\"grande\":" + String(Huevogrande) +
                      ",\"jumbo\":" + String(Huevojumbo) + "}";
    client.println("POST /update HTTP/1.1");
    client.println("Host: 172.20.130.10");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(jsonData.length()));
    client.println();
    client.println(jsonData);
    client.stop();
  } else {
    Serial.println("Conexión al servidor fallida");
  }
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
