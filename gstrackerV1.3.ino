// Definir el modelo del módem
#define TINY_GSM_MODEM_SIM7600

#include <TinyGsmClient.h>
#include <SoftwareSerial.h>

// Pines de conexión serial del módulo SIM7600G
#define MODEM_TX 17
#define MODEM_RX 18
#define SerialMon Serial
#define SerialAT Serial1

// Configuración de la conexión celular y del servidor
const char apn[] = "internet.itelcel.com"; // APN de la red celular
const char user[] = "";                    // Usuario del APN (si es necesario)
const char pass[] = "";                    // Contraseña del APN (si es necesario)
const char server[] = "34.196.135.179";    // IP del servidor
const int port = 5200;                     // Puerto del servidor

const char raw_imei[] = "2049191131";
const char raw_date[] = "20241023";
const char raw_time[] = "19:04:33";
const char raw_lat[] = "21.023694";
const char raw_lon[] = "89.584651";

// Tiempo entre envíos de datos
const unsigned long delayMillis = 20000;   // 20 segundos

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

unsigned long previousMillis = 0;

void setup() {
  // Inicialización de puertos seriales
  SerialMon.begin(115200);
  delay(10);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  // Inicializar el módem
  SerialMon.println("Iniciando el módem...");
  modem.restart();

  // Conectarse a la red GPRS usando el APN
  SerialMon.print("Conectando a la red celular...");
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println("Error al conectar a la red celular");
    while (true);
  }
  SerialMon.println("Conectado a la red celular");

  // Activar GNSS en el SIM7600
  SerialMon.println("Activando GNSS...");
  modem.enableGPS();
  delay(15000);  // Esperar a que GNSS se inicialice
}

void loop() {
  unsigned long currentMillis = millis();

  // Si ha pasado el tiempo configurado (20 segundos)
  if (currentMillis - previousMillis >= delayMillis) {
    previousMillis = currentMillis;

    // Variables para almacenar los datos GNSS
    float gps_latitude  = 0;
    float gps_longitude = 0;
    float gps_speed     = 0;
    float gps_altitude  = 0;
    int   gps_vsat      = 0;
    int   gps_usat      = 0;
    float gps_accuracy  = 0;
    int   gps_year      = 0;
    int   gps_month     = 0;
    int   gps_day       = 0;
    int   gps_hour      = 0;
    int   gps_minute    = 0;
    int   gps_second    = 0;
    float gps_course    = 0; // Variable para el course (rumbo o dirección)
    int   gps_fix       = 0; // Estado del fix (1: sin fix, 2: 2D fix, 3: 3

    // Solicitar la ubicación actual GNSS
    SerialMon.println("Solicitando ubicación GNSS...");
    if (modem.getGPS(&gps_latitude, &gps_longitude, &gps_speed, &gps_altitude,
                     &gps_vsat, &gps_usat, &gps_accuracy, &gps_year, &gps_month,
                     &gps_day, &gps_hour, &gps_minute, &gps_second)) {
      
      gps_course = modem.getGPSraw().toFloat();  // Obtener el course
      gps_fix = (gps_usat > 0) ? 3 : 0;  
      // Formatear la fecha y hora
      String date = String(gps_year) + String(gps_month < 10 ? "0" : "") + String(gps_month) + String(gps_day < 10 ? "0" : "") + String(gps_day);
      String time = String(gps_hour < 10 ? "0" : "") + String(gps_hour) + ":" + 
                    String(gps_minute < 10 ? "0" : "") + String(gps_minute) + ":" +
                    String(gps_second < 10 ? "0" : "") + String(gps_second);

      // Mostrar la información GNSS
      SerialMon.print("Latitud: "); SerialMon.print(gps_latitude, 8);
      SerialMon.print(", Longitud: "); SerialMon.print(gps_longitude, 8);
      SerialMon.print(", Velocidad: "); SerialMon.print(gps_speed);
      SerialMon.print(", Altitud: "); SerialMon.print(gps_altitude);
      SerialMon.print(", Satélites visibles: "); SerialMon.print(gps_vsat);
      SerialMon.print(", Satélites usados: "); SerialMon.println(gps_usat);

      // Verificar la conexión con el servidor TCP
      if (!client.connected()) {
        SerialMon.print("Conectando al servidor...");
        if (!client.connect(server, port)) {
          SerialMon.println("Error al conectar al servidor");
          return;
        }
        SerialMon.println("Conectado al servidor");
      }

      // Formatear la cadena a enviar
      String data = String(modem.getIMEI()) + ";" + String(date) + ";" + String(time) + ";" +
                    String(gps_latitude, 8) + ";" + String(gps_longitude, 8) + ";" +
                    String(gps_speed) + ";" + "0" + ";" + String(gps_vsat) + ";" + String(gps_usat) + ";";

      // Enviar los datos al servidor
      client.print(data);
      SerialMon.println("Datos enviados al servidor: " + data);

    } else {
      SerialMon.println("No se obtuvo ubicación GNSS, enviando información de la red celular...");

      // Obtener fecha y hora de la red celular
      String date, time;
      modem.sendAT("+CCLK?");
      String response = modem.stream.readStringUntil('\n');
      int start = response.indexOf("\"") + 1;
      int end = response.lastIndexOf("\"");
      if (start > 0 && end > start) {
        String gsmDateTime = response.substring(start, end);
        date = "20" + gsmDateTime.substring(0, 2) + gsmDateTime.substring(3, 5) + gsmDateTime.substring(6, 8); // YYYYMMDD
        time = gsmDateTime.substring(9, 11) + ":" + gsmDateTime.substring(12, 14) + ":" + gsmDateTime.substring(15, 17); // HH:MM:SS
      }
       Serial.print("IMEI: ");
       Serial.println(modem.getIMEI() );
      // Formatear cadena para enviar solo red celular cunado no tiene fix
       String data = "STT;"+String(raw_imei)+";3FFFFF;95;1.0.21;1;"+String(date)+";"+String(time)+";"+String(gps_latitude, 6) + ";"+String(gps_longitude, 6)+";"+String(gps_speed)+";"+String(gps_course)+";"+String(gps_usat)+";"+String(gps_fix) + ";00000001;00000000;1;1;0929;4.1;14.19";

    //String data = "STT;"+String(raw_imei)+";3FFFFF;95;1.0.21;1;"+raw_date+";"+raw_time+";0000B0E2;334;20;1223;11;+"+raw_lat+";-"+raw_lon+";"+speed+";"+course+";"+satellites+";"+fix+";00000001;00000000;1;1;0929;4.1;14.19";
      client.print(data);
      SerialMon.println("Datos enviados al servidor (sin GNSS): " + data);
    }
  }
}
