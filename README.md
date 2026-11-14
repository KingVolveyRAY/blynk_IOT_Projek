# Sistem Monitoring Suhu, Kelembapan Udara, dan Kelembapan Tanah Berbasis IoT (ESP32 + Blynk)

Proyek Internet of Things (IoT) untuk memantau kondisi lingkungan dan kelembapan tanah secara real-time. Sistem ini menggunakan mikrokontroler **ESP32**, sensor **DHT11**, sensor **Soil Moisture FC-28**, dan platform **Blynk IoT (Blynk 2.0)**.

---

## 1. Apa yang Dihasilkan Proyek Ini?

Sistem ini menghasilkan sebuah solusi monitoring cerdas (*smart agriculture / plant monitoring*) dengan kemampuan:
1. **Monitoring Suhu Udara (°C)** secara akurat menggunakan sensor DHT11.
2. **Monitoring Kelembapan Udara (%)** menggunakan sensor DHT11.
3. **Monitoring Kelembapan Tanah (%)** dalam bentuk persentase (0% - 100%) menggunakan sensor konduktivitas tanah FC-28.
4. **Transmisi Data Real-Time ke Cloud Blynk** melalui jaringan Wi-Fi 2.4 GHz dengan interval pembaruan data berkala.
5. **Dashboard Visual di Smartphone (Android / iOS) dan Web Browser** dengan widget visual interaktif (Gauge, Radial Gauge, Level, dan Grafik SuperChart).
6. **Mekanisme Auto-Reconnect** yang menjaga sistem tetap membaca sensor dan otomatis menyambung ulang ketika koneksi Wi-Fi terputus tanpa membuat ESP32 macet (*freeze*).

---

## 2. Skema Rangkaian dan Perkabelan

Berikut adalah visualisasi skema rangkaian lengkap sistem:

![Skema Rangkaian](skema_rangkaian.png)

*(File vektor SVG juga tersedia di [skema_rangkaian.svg](skema_rangkaian.svg) jika ingin dilihat tanpa pecah di browser).*

### Tabel Sambungan Pin (Wiring)

| Sensor / Modul | Pin Sensor | Pin ESP32 | Keterangan |
|---|---|---|---|
| **DHT11** (Suhu & Udara) | VCC (`+`) | **3V3** | Catu daya 3.3V |
| | GND (`-`) | **GND** | Ground bersama |
| | DATA (`S` / `OUT`) | **GPIO 4 (D4)** | Sinyal data digital 1-wire |
| **FC-28** (Kelembapan Tanah) | VCC | **3V3** | **Wajib 3.3V** agar tegangan analog aman untuk ADC ESP32 |
| | GND | **GND** | Ground bersama |
| | A0 (Analog Out) | **GPIO 34 (D34)** | Input ADC1 ESP32 (pembacaan kadar air) |
| | D0 (Digital Out) | *Tidak disambung* | Tidak digunakan |

### Catatan Teknis Penting

> [!IMPORTANT]
> 1. **Pemilihan Pin ADC1 (GPIO 34):** ESP32 memiliki dua kontroler ADC (ADC1 dan ADC2). Saat fitur Wi-Fi aktif (seperti pada Blynk), semua pin pada **ADC2 dinonaktifkan** oleh sistem internal ESP32. Oleh karena itu, pin analog wajib menggunakan kelompok **ADC1** (GPIO 32, 33, 34, 35, 36, 39). Pada proyek ini digunakan **GPIO 34**.
> 2. **Tegangan Catu Daya Sensor (3.3V vs 5V):** Gunakan pin **3V3** dari ESP32 untuk memberi daya ke modul FC-28 dan DHT11. Toleransi tegangan input pin GPIO ESP32 adalah maksimal 3.3V. Jika FC-28 diberi 5V, keluaran analog pin A0 dapat merusak pin GPIO ESP32.

---

## 3. Konfigurasi Blynk IoT (Blynk 2.0)

### A. Pembuatan Template di Blynk Cloud
1. Buka [blynk.cloud](https://blynk.cloud/) atau aplikasi Blynk di HP.
2. Buat Template baru dengan parameter:
   - **Name:** `Monitoring Sensor`
   - **Hardware:** `ESP32`
   - **Connection Type:** `WiFi`

### B. Konfigurasi Datastreams (Virtual Pins)
Buat 4 Datastream bertipe **Virtual Pin** pada template Anda:

| Datastream Name | Pin | Data Type | Min | Max | Satuan |
|---|---|---|---|---|---|
| **Suhu Udara** | `V0` | Double | 0 | 60 | °C |
| **Kelembapan Udara** | `V1` | Integer | 0 | 100 | % |
| **Kelembapan Tanah** | `V2` | Integer | 0 | 100 | % |
| **Raw ADC Tanah** | `V3` | Integer | 0 | 4095 | - |

### C. Konfigurasi Dashboard (Aplikasi Mobile & Web)
Tambahkan widget visual berikut di dashboard:
- **Gauge Widget** untuk Suhu Udara (`V0`).
- **Radial Gauge / Level Widget** untuk Kelembapan Udara (`V1`).
- **Radial Gauge Widget** untuk Kelembapan Tanah (`V2`).
- **SuperChart / Value Display** untuk menampilkan riwayat data grafik secara real-time.

---

## 4. Panduan Penggunaan dan Upload Program

### A. Persiapan Library Arduino IDE
Pastikan library berikut sudah terpasang melalui menu **Sketch -> Include Library -> Manage Libraries...**:
1. **Blynk** oleh Volodymyr Shymanskyy.
2. **DHT sensor library** oleh Adafruit.
3. **Adafruit Unified Sensor** oleh Adafruit.
4. Board package **esp32** oleh Espressif Systems terpasang di Boards Manager.

### B. Konfigurasi Kredensial pada Kode
Buka file `sensor_to_blynk.ino` lalu sesuaikan baris berikut:

```cpp
// Masukkan kredensial dari Template Blynk Anda
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME "Monitoring Sensor"
#define BLYNK_AUTH_TOKEN    "YourAuthToken"

// Masukkan data Wi-Fi lokasi Anda (wajib 2.4 GHz)
char ssid[] = "NAMA_WIFI_ANDA";
char pass[] = "PASSWORD_WIFI_ANDA";
```

### C. Kalibrasi Sensor FC-28
Sensor kelembapan tanah FC-28 membaca resistansi tanah secara terbalik (makin basah = nilai ADC makin kecil):
1. **Kondisi Kering (0%):** Biarkan probe di udara terbuka. Amati nilai mentah ADC pada Serial Monitor atau Datastream `V3` (rata-rata bernilai `3200` hingga `3800`).
2. **Kondisi Basah (100%):** Celupkan probe ke dalam air (hanya probe logam, jangan sampai modul komparator terkena air). Amati nilai ADC (rata-rata turun ke `1200` hingga `1600`).
3. Sesuaikan konstanta di kode jika diperlukan:
   ```cpp
   const int SOIL_DRY_VALUE = 3500;
   const int SOIL_WET_VALUE = 1300;
   ```

### D. Upload ke ESP32
1. Hubungkan ESP32 ke komputer menggunakan kabel Micro-USB.
2. Di Arduino IDE, pilih board: **DOIT ESP32 DEVKIT V1** atau **ESP32 Dev Module**.
3. Pilih port COM yang sesuai di menu **Tools -> Port**.
4. Klik tombol **Upload**.
5. Buka **Serial Monitor** pada baud rate `115200` untuk memantau proses inisialisasi dan koneksi ke server Blynk.

---

## 5. Struktur Direktori Proyek

```text
sensor_to_blynk/
├── sensor_to_blynk.ino   # Kode utama Arduino / ESP32
├── SKEMA_RANGKAIAN.md    # Dokumentasi skema dan detail teknis
├── skema_rangkaian.png   # Gambar diagram skema resolusi tinggi
├── skema_rangkaian.svg   # Gambar diagram vektor skema
└── README.md             # Dokumentasi utama proyek
```

