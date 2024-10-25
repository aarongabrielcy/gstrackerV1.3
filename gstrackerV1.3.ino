#define TINY_GSM_MODEM_SIM7600

#define MODEM_TX 17
#define MODEM_RX 18
#define SerialAT Serial1
#include <TinyGsmClient.h>
#define TINY_GSM_TEST_GPRS          true
#define TINY_GSM_TEST_TCP           false
#define TINY_GSM_TEST_GPS           false
#define TINY_GSM_TEST_TIME          true
#define TINY_GSM_TEST_TEMPERATURE   false

#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "internet.itelcel.com";
// const char apn[] = "ibasis.iot";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details to test TCP/SSL
const char server[] = "34.196.135.179";
bool gprs_state = false;
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
 
  String ccid, imei, imsi, cop, raw_imei, datetime_module;
  IPAddress local;
  int csq;

int year3 = 0,month3 = 0, day3 = 0, hour3 = 0, min3 = 0, sec3 = 0;
float timezone = 0, lat2 = 0,lon2= 0,speed2=0,alt2 = 0,accuracy2 = 0;
int vsat2= 0,usat2= 0,year2= 0,month2 = 0, day2 = 0, hour2 = 0,min2 = 0,sec2 = 0;
void setup() {
  // Set console baud rate
  Serial.begin(115200);

  // Set GSM module baud rate
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  gprs_state = gprs_config();
  //modem.enableGPS();
}

void light_sleep(uint32_t sec ) {
  esp_sleep_enable_timer_wakeup(sec * 1000000ULL);
  esp_light_sleep_start();
}
void info_modem(){
  ccid = modem.getSimCCID();
  Serial.print("CCID: ");
  Serial.println(ccid);
  imei = modem.getIMEI();
  raw_imei = formatIMEI(imei);
  Serial.print("IMEI: ");
  Serial.println(raw_imei);
  imsi = modem.getIMSI();
  Serial.print("IMSI: ");
  Serial.println(imsi);
  cop = modem.getOperator();
  Serial.print("Operator: " );
  Serial.println(cop);
  local = modem.localIP();
  Serial.print("Local IP: " );
  Serial.println(local);
  csq = modem.getSignalQuality();
  Serial.print("Signal quality: ");
  Serial.println(csq);   
}
bool gprs_config(){
  String ret;
  ret = modem.setNetworkMode(2);
  Serial.print("setNetworkMode: ");
  Serial.println(ret);
  uint8_t mode = modem.getGNSSMode();
  Serial.print("GNSS Mode: ");
  Serial.println(mode);

  modem.setGNSSMode(1, 1);
  light_sleep(1);

  String name = modem.getModemName();
  Serial.print("Modem Name: " );
  Serial.println(name);

  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);
  // Unlock your SIM card with a PIN if needed
  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }

  Serial.println("Waiting for network...");
  if (!modem.waitForNetwork(600000L)) {
    light_sleep(10);
    return false;
  }

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }
  
  Serial.print("Connecting to ");
  Serial.println(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    light_sleep(10);
    return false;
  }
  return true;
}
bool network_info(){
  bool res;
  res = modem.isGprsConnected();
  Serial.print("GPRS status: ");
  if(res){
    Serial.println("connected");
    return true;
  }
  Serial.println("NOT connected");  
  return false;
}
void loop() {
  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  if (!modem.init()) {
    Serial.println("Failed to restart modem, delaying 10s and retrying");
    return;
  }

if(!gprs_state){
  gprs_config();
}
if(network_info()){
  info_modem();
  delay(6000);
}
// Enviar comando AT para obtener los datos NMEA
  /*SerialAT.println(F("+CGPSNMEA?"));
  String nmeaData = modem.stream.readStringUntil('\n');  // Leer la respuesta NMEA
  
  // Imprimir la respuesta completa en el puerto serie
  Serial.println("Datos NMEA recibidos:");
  Serial.println(nmeaData);

  // Parsear la información relevante
  parseNMEAData(nmeaData);*/

#if TINY_GSM_TEST_TCP && defined TINY_GSM_MODEM_HAS_TCP
  Serial.println("############################## TINY_GSM_TEST_TCP ############################ ");

  TinyGsmClient client(modem, 0);
  const int port = 5200;
  Serial.print("Connecting to ");
  Serial.println(server);
  if (!client.connect(server, port)) {
    Serial.println("... failed");
  } else {
    client.println("STT;"+raw_imei+";3FFFFF;95;1.0.21;1;20241024;21:10:17;04BB4A02;334;20;3C1F;18;+20.905637;-89.645585;0.19;81.36;17;1;00000001;00000000;1;1;0929;4.1;14.19");

    // Wait for data to arrive
    uint32_t start = millis();
    while (client.connected() && !client.available() &&
           millis() - start < 30000L) {
      delay(100);
    };

    // Read data
    start = millis();
    while (client.connected() && millis() - start < 5000L) {
      while (client.available()) {
        Serial.write(client.read());
        start = millis();
      }
    }
  }
#endif

#if TINY_GSM_TEST_CALL && defined(CALL_TARGET)

  Serial.print("Calling:", CALL_TARGET);
  SerialAT.println("ATD"CALL_TARGET";");
  modem.waitResponse();
  light_sleep(20);
#endif

#if TINY_GSM_TEST_GPS && defined TINY_GSM_MODEM_HAS_GPS
  Serial.print("Enabling GPS/GNSS/GLONASS");
  modem.enableGPS();
  light_sleep(2);

  Serial.print("Requesting current GPS/GNSS/GLONASS location");
  //for (;;) {
    if (modem.getGPS(&lat2, &lon2, &speed2, &alt2, &vsat2, &usat2, &accuracy2,
                     &year2, &month2, &day2, &hour2, &min2, &sec2)) {
      String datetime_gnss = String(year2)+String(month2)+String(day2)+";"+hour2+":"+min2+":"+sec2+";"+String(lat2, 6)+";"+String(lon2, 8);
      Serial.print("speed: ");
      Serial.println(speed2);
      Serial.print("Satellite visible: ");
      Serial.println(vsat2);
      Serial.print("satellites in use: ");
      Serial.println(usat2);
      Serial.print("Altitude: ");
      Serial.println(alt2);
      Serial.print("Accuracy: ");
      Serial.println(accuracy2);
      Serial.print("dateTime GNSS: ");
      Serial.println(datetime_gnss);
      //break;
    } else {
      light_sleep(2);
    }
  //}
  Serial.print("Retrieving GPS/GNSS/GLONASS location again as a string");
  String gps_raw = modem.getGPSraw();
  Serial.print("GPS/GNSS Based Location String: ");
  Serial.println(gps_raw);
  //Serial.print("Disabling GPS");
  //modem.disableGPS();
#endif

#if TINY_GSM_TEST_TIME && defined TINY_GSM_MODEM_HAS_TIME
  Serial.println("############################## TINY_GSM_TEST_TIME ############################ ");
  
  for (int8_t i = 5; i; i--) {
    Serial.println("Requesting current network time");
    if (modem.getNetworkTime(&year3, &month3, &day3, &hour3, &min3, &sec3,
                             &timezone)) {
      /*Serial.print("DATE: ");
      String date = String(year3)+"/"+String(month3)+"/"+String(day3); 
      Serial.println(date);
      Serial.print("TIME: ");
      String time = String(hour3)+":"+String(min3)+":"+String(sec3); 
      Serial.println(time);*/
      datetime_module = getFormattedUTCDateTime(year3, month3, day3, hour3, min3, sec3, timezone); 
      Serial.print("DateTime Modem SIM: ");
      Serial.println(datetime_module);
      Serial.print("Timezone:" );
      Serial.println(timezone);
      break;
    } else {
      Serial.println("Couldn't get network time, retrying in 15s.");
      light_sleep(15);
    }
  }
  Serial.println("Retrieving time again as a string");
  String time = modem.getGSMDateTime(DATE_FULL);
  Serial.print("Current Network Time: ");
  Serial.println(time);
#endif

/*#if TINY_GSM_TEST_GPRS
  modem.gprsDisconnect();
  light_sleep(5);
  if (!modem.isGprsConnected()) {
    Serial.println("GPRS disconnected");
  } else {
    Serial.println("GPRS disconnect: Failed.");
  }
#endif*/

#if TINY_GSM_TEST_TEMPERATURE && defined TINY_GSM_MODEM_HAS_TEMPERATURE
  Serial.println("############################## TINY_GSM_TEST_TEMPERATURE ############################ ");
  float temp = modem.getTemperature();
  Serial.print("Chip temperature:");
  Serial.println(temp);
#endif

}
String formatIMEI(String input) {
  // Verifica que el string tenga al menos 10 caracteres
  if (input.length() >= 10) {
    // Devuelve los últimos 10 caracteres
    return input.substring(input.length() - 10);
  } else {
    // Si el string es menor que 10 caracteres, devuelve el string completo
    return input;
  }
}
String getFormattedUTCDateTime(int year, int month, int day, int hour, int min, int sec, float timezone) {
    // Ajustar la hora a UTC restando la diferencia de zona horaria
    hour -= int(timezone); // Convertimos timezone a un valor entero (horas)
    
    // Ajustar si la hora es negativa o pasa de 24 horas
    if (hour < 0) {
        hour += 24;
        day -= 1;  // Restamos un día si la hora es negativa

        // Ajustar el mes y año si el día es menor que 1
        if (day < 1) {
            month -= 1;
            if (month < 1) {
                month = 12;
                year -= 1;
            }
            // Ajustar el día con base en el nuevo mes
            day = daysInMonth(month, year);
        }
    } else if (hour >= 24) {
        hour -= 24;
        day += 1;  // Sumamos un día si la hora pasa de 24

        // Ajustar el mes y año si el día excede el número de días en el mes actual
        if (day > daysInMonth(month, year)) {
            day = 1;
            month += 1;
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }

    // Crear la cadena en formato "YYYYMMDD;HH:MM:SS"
    char buffer[20];
    sprintf(buffer, "%04d%02d%02d;%02d:%02d:%02d", year, month, day, hour, min, sec);

    // Devolver la cadena formateada
    return String(buffer);
}

// Función para obtener el número de días en un mes determinado
int daysInMonth(int month, int year) {
    if (month == 2) {
        // Verificar si es año bisiesto
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29;
        } else {
            return 28;
        }
    }
    // Meses con 31 días
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        return 31;
    }
    // Meses con 30 días
    return 30;
}
void parseNMEAData(String nmeaData) {
  if (nmeaData.indexOf("GPGSV") >= 0) {
    Serial.println("Satélites GPS en vista detectados:");
    Serial.println("Datos GPS GSV: " + nmeaData);  // Satélites en vista para GPS
  }

  if (nmeaData.indexOf("GLGSV") >= 0) {
    Serial.println("Satélites GLONASS en vista detectados:");
    Serial.println("Datos GLONASS GSV: " + nmeaData);  // Satélites en vista para GLONASS
  }

  if (nmeaData.indexOf("BDGSV") >= 0) {
    Serial.println("Satélites BeiDou en vista detectados:");
    Serial.println("Datos BeiDou GSV: " + nmeaData);  // Satélites en vista para BeiDou
  }

  if (nmeaData.indexOf("GNGSA") >= 0) {
    Serial.println("Satélites en uso detectados (GNSS Fix):");
    Serial.println("Datos GNGSA: " + nmeaData);  // Satélites en uso para el GNSS fix (podría incluir varios sistemas)
  }
}
