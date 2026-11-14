/**********************************************************************************
 * SISTEM MONITORING SUHU, KELEMBAPAN UDARA, DAN KELEMBAPAN TANAH BERBASIS IOT
 * Microcontroller : ESP32 Dev Module
 * Sensor          : DHT11 (Suhu & Kelembapan Udara) + FC-28 (Kelembapan Tanah)
 * Platform IoT    : Blynk IoT (Blynk 2.0)
 * 
 * Sesuai Diagram Alir:
 * Mulai -> Inisialisasi -> Konek WiFi & Blynk -> Pembacaan Sensor -> 
 * Pengolahan Data -> Cek WiFi (Ya: Kirim ke Blynk, Tidak: Reconnect) -> Selesai
 **********************************************************************************/

/* 1. KREDENSIAL BLYNK (Wajib di baris paling atas sebelum include library) */
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxx"       // Ganti dengan Template ID Blynk Anda
#define BLYNK_TEMPLATE_NAME "Monitoring Sensor" // Ganti dengan nama template Anda
#define BLYNK_AUTH_TOKEN    "YourAuthToken"    // Ganti dengan Auth Token dari Device Blynk

#define BLYNK_PRINT Serial

/* 2. LIBRARY YANG DIGUNAKAN */
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

/* 3. KONFIGURASI WIFI */
char ssid[] = "NAMA_WIFI_ANDA";     // Ganti dengan nama WiFi (SSID)
char pass[] = "PASSWORD_WIFI_ANDA"; // Ganti dengan kata sandi WiFi

/* 4. PINOUT DAN DEFINISI SENSOR */
// Sensor DHT11
#define DHTPIN 4           // GPIO 4 terhubung ke Pin Data DHT11
#define DHTTYPE DHT11      // Tipe sensor DHT: DHT11
DHT dht(DHTPIN, DHTTYPE);

// Sensor Soil Moisture FC-28
// PENTING: Gunakan ADC1 (GPIO 32, 33, 34, 35, 36, 39). ADC2 tidak bisa digunakan saat WiFi aktif.
#define SOIL_PIN 34        // GPIO 34 (Analog Input) terhubung ke Pin A0 FC-28

/* 5. KALIBRASI SENSOR SOIL MOISTURE FC-28 */
// Nilai ADC ESP32 berkisar antara 0 - 4095 (12-bit ADC)
// Pada kondisi kering (di udara bebas): nilai ADC tinggi (~3200 - 4095)
// Pada kondisi basah (dalam air): nilai ADC rendah (~1200 - 1800)
// Silakan sesuaikan nilai kalibrasi ini dengan hasil pengujian Anda
const int SOIL_DRY_VALUE = 3500;  // Nilai saat tanah/probe kering (0%)
const int SOIL_WET_VALUE = 1300;  // Nilai saat tanah sangat basah/air (100%)

/* 6. VIRTUAL PIN BLYNK */
#define VPIN_SUHU          V0  // Virtual Pin untuk Suhu Udara (°C)
#define VPIN_HUMIDITY      V1  // Virtual Pin untuk Kelembapan Udara (%)
#define VPIN_SOIL_PERCENT  V2  // Virtual Pin untuk Kelembapan Tanah (%)
#define VPIN_SOIL_RAW      V3  // Virtual Pin untuk Nilai ADC Mentah Tanah

/* 7. TIMER */
BlynkTimer timer;

// Interval pembacaan sensor dan pengiriman data (misal tiap 2 detik)
const unsigned long SEND_INTERVAL_MS = 2000L;

/* FUNGSI: Pembacaan Sensor dan Pengiriman Data ke Blynk */
void sendSensorData() {
  Serial.println("\n--- [PEMBACAAN SENSOR] ---");

  // 1. Baca Sensor DHT11 (Suhu dan Kelembapan Udara)
  float humidityAir = dht.readHumidity();
  float tempAir = dht.readTemperature();

  // Validasi pembacaan DHT11
  bool dhtValid = true;
  if (isnan(humidityAir) || isnan(tempAir)) {
    Serial.println("[ERROR] Gagal membaca data dari sensor DHT11!");
    dhtValid = false;
  } else {
    Serial.print("Suhu Udara       : ");
    Serial.print(tempAir);
    Serial.println(" °C");

    Serial.print("Kelembapan Udara : ");
    Serial.print(humidityAir);
    Serial.println(" %");
  }

  // 2. Baca Sensor Soil Moisture FC-28 (Kelembapan Tanah)
  // Ambil beberapa sampel untuk mengurangi noise pembacaan analog
  long soilRawSum = 0;
  const int samples = 10;
  for (int i = 0; i < samples; i++) {
    soilRawSum += analogRead(SOIL_PIN);
    delay(5);
  }
  int soilRaw = soilRawSum / samples;

  // 3. ESP32 Mengolah Data: Konversi nilai mentah ADC ke persentase (0% - 100%)
  // Karakteristik FC-28: Nilai ADC makin tinggi = tanah makin kering (terbalik)
  int soilMoisturePercent = map(soilRaw, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
  // Batasi rentang nilai agar tetap antara 0 hingga 100%
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  Serial.print("ADC Mentah Tanah : ");
  Serial.println(soilRaw);
  Serial.print("Kelembapan Tanah : ");
  Serial.print(soilMoisturePercent);
  Serial.println(" %");

  // 4. Pengambilan Keputusan: Apakah Koneksi WiFi Aktif?
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[STATUS] WiFi Terhubung. Mengirim data ke Server Blynk...");

    // Pastikan koneksi ke cloud Blynk aktif
    if (Blynk.connected()) {
      // Kirim data ke Virtual Pins Blynk
      if (dhtValid) {
        Blynk.virtualWrite(VPIN_SUHU, tempAir);
        Blynk.virtualWrite(VPIN_HUMIDITY, humidityAir);
      }
      Blynk.virtualWrite(VPIN_SOIL_PERCENT, soilMoisturePercent);
      Blynk.virtualWrite(VPIN_SOIL_RAW, soilRaw);
      
      Serial.println("[SUKSES] Data berhasil terkirim ke Server Blynk!");
    } else {
      Serial.println("[PERINGATAN] Terhubung ke WiFi namun terputus dari Blynk. Mencoba reconnect Blynk...");
      Blynk.connect();
    }
  } else {
    // Jalur TIDAK: Coba koneksi ulang WiFi
    Serial.println("[PERINGATAN] Koneksi WiFi terputus! Mencoba koneksi ulang ke WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
}

void setup() {
  // Inisialisasi Komunikasi Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("==================================================");
  Serial.println("  SISTEM MONITORING IOT ESP32 - BLYNK DIMULAI     ");
  Serial.println("==================================================");

  // Inisialisasi Sensor DHT11
  Serial.println("[1/3] Inisialisasi Sensor DHT11...");
  dht.begin();

  // Inisialisasi Pin Sensor FC-28
  Serial.println("[2/3] Inisialisasi Pin Sensor Soil Moisture FC-28...");
  pinMode(SOIL_PIN, INPUT);
  // Set resolusi ADC ESP32 ke 12-bit (0 - 4095)
  analogReadResolution(12);

  // Inisialisasi Koneksi WiFi dan Blynk
  Serial.println("[3/3] Menghubungkan ke WiFi dan Server Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Atur interval pembacaan sensor dengan BlynkTimer
  timer.setInterval(SEND_INTERVAL_MS, sendSensorData);

  Serial.println("[INFO] Inisialisasi Sistem Selesai! Memulai loop monitoring.");
}

void loop() {
  // Jalankan handler Blynk
  if (Blynk.connected()) {
    Blynk.run();
  }

  // Jalankan timer untuk pembacaan berkala
  timer.run();
}
