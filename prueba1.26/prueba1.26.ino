#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"
#include <Servo.h>
#include <WiFiS3.h>
#include "arduino_secrets.h"

// Dimensiones de la pantalla OLED
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
Servo servoCompuerta1;  // Primer servo (compuerta 1)
Servo servoClasificador; // Segundo servo (clasificador)
Servo servoCompuerta2;  // Tercer servo (compuerta 2)

int Huevopequeno = 0;
int Huevomediano = 0;
int Huevogrande = 0;
int Huevojumbo = 0;

// Pines de los servos
const int pinCompuerta1 = 9;
const int pinClasificador = 10;
const int pinCompuerta2 = 11;

// Posiciones de los servos
const int posCerradaCompuerta1 = 0;
const int posAbiertaCompuerta1 = 90;

const int posClasificadorInicial = 0;
const int posClasificador10_20 = 30;
const int posClasificador20_30 = 60;
const int posClasificador30_40 = 90;
const int posClasificador40_up = 120;

const int posCerradaCompuerta2 = 0;
const int posAbiertaCompuerta2 = 90;

// Variables para clasificador
int pesoClasificar = 0;
bool nuevoPeso = false; // Flag para indicar si hay un nuevo peso

// Definición de las credenciales Wi-Fi
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Inicialización del servidor web
WiFiServer server(80);

void setup() {
  // Configuración inicial de la balanza
  Serial.begin(9600);
  balanza.begin(DOUT, CLK);
  Serial.print("Lectura del valor del ADC: ");
  Serial.println(balanza.read());
  Serial.println("No ponga ningún objeto sobre la balanza");
  Serial.println("Destarando...");
  Serial.println("...");
  balanza.set_scale(388896.875);  // Ajustar escala si es necesario
  balanza.tare();  // El peso actual es considerado tara

  // Inicialización de la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("No se encontró una pantalla OLED"));
    for (;;);  // Se detiene aquí si no se encuentra la pantalla
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
  delay(2000);
  servoCompuerta1.write(posAbiertaCompuerta1);
  delay(2000);
  servoCompuerta1.write(posCerradaCompuerta1);

  // Conexión a la red Wi-Fi
  Serial.println("Conectando a Wi-Fi");
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  // Configurar IP estática
  IPAddress ip(192, 168, 1, 24);
  IPAddress gateway(192, 168, 1, 1);  // Dirección del gateway
  IPAddress subnet(255, 255, 255, 0); // Máscara de subred
  WiFi.config(ip, gateway, subnet);
  
  Serial.print("Creando punto de acceso con el nombre: ");
  Serial.println(ssid);
  
  int status = WiFi.beginAP(ssid, pass);  // Usar softAP para crear el AP
  if (status != WL_AP_LISTENING) {
    Serial.println("Creación del punto de acceso fallida");
    while (true);
  }

  delay(10000);
  server.begin();
  Serial.println("Servidor web iniciado");
  printWiFiStatus();
}

void loop() {
  static float pesoAnterior = 0;  // Almacena el peso de la iteración anterior
  static unsigned long tiempoEstable = 0;  // Tiempo en que el peso es estable
  static const unsigned long tiempoParaEstabilizar = 200;  // Tiempo requerido para considerar estable (en ms)
  
  float pesoActual = balanza.get_units(10) * 1000;  // Convertir a gramos
  int pesoRedondeado = round(pesoActual);

  // Verificar si el peso es estable (no cambia significativamente)
  if (abs(pesoRedondeado - pesoAnterior) < 2) {  // Cambios menores a 2 g se consideran estables
    if (millis() - tiempoEstable >= tiempoParaEstabilizar && pesoRedondeado > 0) {
      // El peso es estable y no es cero
      display.clearDisplay();
      display.setCursor(0, 10);
      display.setTextSize(2);
      display.print("Peso:");
      display.print(pesoRedondeado);
      display.println(" g");
      display.display();

      // Activar la clasificación si es un peso nuevo
      if (!nuevoPeso) {
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

        // Mover clasificador a la posición correspondiente
        delay(500);
        servoClasificador.write(posicionClasificador);
        delay(500);

        // Abrir la segunda compuerta
        servoCompuerta2.write(posAbiertaCompuerta2);
        delay(500);

        // Cerrar la segunda compuerta
        servoCompuerta2.write(posCerradaCompuerta2);
        delay(500);

        // Regresar clasificador a la posición inicial
        servoClasificador.write(posClasificadorInicial);
        delay(500);

        // Apertura de la primera compuerta para dejar pasar un objeto
        servoCompuerta1.write(posAbiertaCompuerta1);
        delay(1000);
        servoCompuerta1.write(posCerradaCompuerta1);
      }
    }
  } else {
    // Si el peso cambió, resetear el tiempo de estabilización
    tiempoEstable = millis();
    nuevoPeso = false;
  }

  // Si el peso llega a cero, mostrar 0 sin procesar más
  if (pesoRedondeado == 0) {
    display.clearDisplay();
    display.setCursor(0, 10);
    display.setTextSize(2);
    display.print("Peso:");
    display.print("0 g");
    display.display();
    nuevoPeso = false;  // Resetear el flag para el próximo peso
  }

  // Actualizar el peso anterior
  pesoAnterior = pesoRedondeado;

  // Pequeño retardo para estabilizar las lecturas
  delay(200);

  // Servidor web: manejar cliente
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Nuevo cliente conectado");
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Responder al cliente con el peso actual
   String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
response += "<!DOCTYPE HTML>\r\n<html>\r\n";
response += "<head>";
response += "<meta charset='UTF-8'>";
response += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
response += "<title>Medición de Peso</title>";
response += "<style>";
response += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; color: #333; padding: 20px; }";
response += "h1 { color: #007BFF; }";
response += "p { font-size: 18px; }";
response += "table { width: 60%; margin: auto; border-collapse: collapse; background: #fff; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1); }";
response += "th, td { padding: 12px; border-bottom: 1px solid #ddd; }";
response += "th { background-color: #007BFF; color: white; }";
response += "</style>";
response += "</head>\r\n";
response += "<body>";
response += "<h1>Medición de Peso</h1>\r\n";
response += "<p>Peso actual: <strong>" + String(pesoRedondeado) + " g</strong></p>\r\n";
response += "<table>";
response += "<tr><th>Tipo de Huevo</th><th>Cantidad</th></tr>";
response += "<tr><td>Pequeño</td><td>" + String(Huevopequeno) + "</td></tr>";
response += "<tr><td>Mediano</td><td>" + String(Huevomediano) + "</td></tr>";
response += "<tr><td>Grande</td><td>" + String(Huevogrande) + "</td></tr>";
response += "<tr><td>Jumbo</td><td>" + String(Huevojumbo) + "</td></tr>";
response += "</table>";
response += "</body></html>\r\n";

client.print(response);
client.stop();
Serial.println("Cliente desconectado");

  }
}

void printWiFiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.print("Para ver esta página, abre un navegador y ve a: http://");
  Serial.println(ip);
}