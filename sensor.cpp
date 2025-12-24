#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

// ---------------- PINES ----------------
#define SENSOR_PIN 34        // Sensor capacitivo
#define LED_PIN 2            // LED integrado

// ---------------- WIFI ----------------
#define WIFI_SSID "nokia_1100"
#define WIFI_PASSWORD "wifisinclave"

// ---------------- FIREBASE ----------------
#define API_KEY "AIzaSyCEvyKVl3Bhan5J_J_zTHlYdeQCug30ge4"
#define DATABASE_URL "https://motorcycle-telemetry-4cf4c-default-rtdb.firebaseio.com/"

// ---------------- NTP ----------------
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600;   // Colombia
const int daylightOffset_sec = 0;

// ---------------- FIREBASE OBJETOS ----------------
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ---------------- TIEMPO ----------------
unsigned long lastRead = 0;
const unsigned long interval = 15000; // 15 segundos

// ---------------- LED FUNCIONES ----------------
// Función simple: enciende LED el tiempo indicado y apaga
void parpadear(int tiempoEncendido) {
  digitalWrite(LED_PIN, HIGH);
  delay(tiempoEncendido);
  digitalWrite(LED_PIN, LOW);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  delay(3000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("ESP32 INICIANDO");

  // -------- WIFI --------
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  // -------- NTP --------
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Hora sincronizada");

  // -------- FIREBASE --------
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Signup anónimo
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase signup OK");
  } else {
    Serial.printf("Firebase signup ERROR: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase inicializado");
}

// ---------------- LOOP ----------------
void loop() {
  if (millis() - lastRead >= interval) {
    lastRead = millis();

    // ---- Leer sensor ----
    int sensorValue = analogRead(SENSOR_PIN);
    Serial.print("Sensor: ");
    Serial.println(sensorValue);

    // ---- LED lectura ----
    parpadear(300); // LED encendido 300 ms para indicar lectura

    // ---- Obtener fecha y hora ----
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Error obteniendo hora");
      return;
    }

    char fechaHora[25];
    strftime(fechaHora, sizeof(fechaHora), "%Y-%m-%d_%H-%M-%S", &timeinfo);

    long timestamp = time(nullptr);

    // ---- Crear JSON ----
    FirebaseJson json;
    json.set("valor", sensorValue);
    json.set("timestamp", timestamp);

    String ruta = "/historico/" + String(fechaHora);

    // ---- Enviar a Firebase ----
    if (Firebase.RTDB.setJSON(&fbdo, ruta.c_str(), &json)) {
      Serial.print("Dato guardado: ");
      Serial.println(ruta);

      // ---- LED envío ----
      parpadear(100); // LED encendido 100 ms para indicar envío
    } else {
      Serial.println("Error Firebase:");
      Serial.println(fbdo.errorReason());
    }
  }
}
