#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <WiFiClientSecure.h>

const char* ssid = "MEGACABLE-2.4G-B9AA";       
const char* password = "ZNk82bSsuR";   
const String ntfy_url = "http://ntfy.sh/mini_cactus_MAGNUM";
const String script_url = "https://script.google.com/macros/s/AKfycbyOJBGiAcYC6jDxt9Uev23V6fAc231h5NqMmbI-8bmLiu3hwoLrATM_WE-jP2NnJuGJnA/exec";

#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP  10

#define DHTPIN 5
#define DHTTYPE DHT11

const int pinLED = 13;
const int pinSensorSuelo = 35;
const int pinReleBomba = 16;
const float UMBRAL_TEMP = 28.0;

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, HIGH);
  pinMode(pinReleBomba, OUTPUT);
  digitalWrite(pinReleBomba, HIGH);  

  lcd.init();
  lcd.backlight();
  dht.begin();
  
  delay(4000); 
  
  int HSuelo = analogRead(pinSensorSuelo);
  bool alertaTierra = (HSuelo > 3000);

  bool riego = false;

if (alertaTierra)
 riego = true;
  Serial.print("Lectura suelo ADC: ");
  Serial.println(HSuelo);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suelo ADC: ");
  lcd.print(HSuelo);
  
 
  if (alertaTierra) {
    Serial.println("Estado: TIERRA SECA");
    lcd.setCursor(0, 1);
    lcd.print("Tierra seca");
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print("Regando...");
    Serial.println("Estado: REGANDO");
    digitalWrite(pinReleBomba, LOW);
    delay(4000);
    digitalWrite(pinReleBomba, HIGH);
    Serial.println("Estado: RIEGO COMPLETADO");
    lcd.setCursor(0, 1);
    lcd.print("ya no tengo sed");
  } else {
    Serial.println("Estado: HUMEDAD nice");
    lcd.setCursor(0, 1);
    lcd.print("Humedad nice");
  }
  
  float temperatura = dht.readTemperature();
  float humedadAmbiente = dht.readHumidity();
  
  lcd.setCursor(0, 0);
  if (isnan(temperatura)) {
    lcd.print("acomoda tu DHT11  ");
    temperatura = 0.0; 
  } else {
    lcd.print("T:");
    lcd.print(temperatura, 1);
    lcd.print("C H:");
    lcd.print((int)humedadAmbiente);
    lcd.print("% ");
  }
  
  lcd.setCursor(0, 1);
  if (alertaTierra) {
    lcd.print("Seca / regado   ");
  } else {
    lcd.print(" Humedad ");
  }
  
  delay(3500); 
  
  bool alertaCalor = (temperatura >= UMBRAL_TEMP);
  
  WiFi.begin(ssid, password);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    intentos++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 1);
    lcd.print("datos Guardados.");
    
  
    WiFiClientSecure clientSeguro;
    clientSeguro.setInsecure();
    
    HTTPClient httpSheets;
    httpSheets.begin(clientSeguro, script_url);
    httpSheets.addHeader("Content-Type", "application/x-www-form-urlencoded");
    httpSheets.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    String datos = "adc=" + String(HSuelo) + "&clima=" + String(temperatura) + "&riego=" + String(riego? "si" : "no");
    httpSheets.POST(datos);
    httpSheets.end();
    
    if (alertaTierra || alertaCalor) {
      HTTPClient httpNtfy;
      httpNtfy.begin(ntfy_url);
      httpNtfy.addHeader("mini_cactus_MAGNUM", "ALERTA, me muerooo");
      httpNtfy.addHeader("Tags", "cactus,warning"); 
      String mensaje = "Lectura del suelo: " + String( HSuelo) + "\n";
      if (alertaTierra) mensaje += "Dame agua Humana";
      if (alertaCalor) mensaje += " Me asoleo no ves? " + String(temperatura, 1) + " C ";
      httpNtfy.POST(mensaje);
      httpNtfy.end();
      
      lcd.setCursor(0, 1);
      lcd.print("Abre la aplicacion");
    } else {
      lcd.setCursor(0, 1);
      lcd.print("registro guardado en la bitacora ");
    }
  } else {
    lcd.setCursor(0, 1);
    lcd.print("revisa Wi-Fi     ");
  }
  
  delay(3000);
  
  lcd.clear();
  lcd.print("Deep Sleep...");
  delay(1000);
  lcd.noBacklight(); 
  

  digitalWrite(pinLED, LOW);
  pinMode(pinLED, INPUT);
  

  esp_sleep_enable_timer_wakeup((alertaTierra ? 1800ULL : TIME_TO_SLEEP) * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() { 
}