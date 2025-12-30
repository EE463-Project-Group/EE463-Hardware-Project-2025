// ESP32 WROOM-32U Pin Tanımlamaları
#define U1HIN 33  // pwm U1 HI MOSFET Q1
#define U2HIN 26  // pwm U2 HI MOSFET Q3
#define U1LIN 32  // input U1 LO MOSFET Q2
#define U2LIN 25  // input U2 LO MOSFET Q4

#define POT 14    // Analog Pot Girişi

// Anlık durum bilgisini tutan değişken
char calismaModu = 'd'; // 'd' = dur, 'i' = ileri, 'g' = geri

// Serial Print zamanlaması için değişken
unsigned long sonYazdirmaZamani = 0;
const int yazdirmaAraligi = 500; // 500 ms (Yarım saniyede bir yazdır)

void setup() {
  Serial.begin(115200);
  Serial.println("--- MOTOR KONTROL SISTEMI BASLATILDI ---");
  Serial.println("Komutlar: 'i' (Ileri), 'g' (Geri), 'd' (Dur)");

  pinMode(U1HIN, OUTPUT);
  pinMode(U2HIN, OUTPUT);
  pinMode(U1LIN, OUTPUT);
  pinMode(U2LIN, OUTPUT);

  motoruDurdur();
}

void loop() {
  // --- 1. KOMUT KONTROLÜ ---
  if (Serial.available() > 0) {
    char gelenVeri = Serial.read();
    
    // Satır sonu karakterlerini temizle, sadece komutları al
    if (gelenVeri == 'i' || gelenVeri == 'g' || gelenVeri == 'd') {
      
      // Yön değişimi varsa önce güvenli duruş yap
      if (calismaModu != gelenVeri) {
        motoruDurdur();
        // Ani yön değişiminde MOSFET koruması için minik bekleme
        // (Buradaki delay print'i etkilemez, sadece geçişte 1 kere olur)
        delay(100); 
      }
      calismaModu = gelenVeri; 
    }
  }

  // --- 2. HIZ HESAPLAMA ---
  int potDegeri = analogRead(POT);
  int pwmHiz = map(potDegeri, 0, 4095, 0, 255);

  // --- 3. MOTOR SÜRÜŞ ---
  if (calismaModu == 'i') {
    ileriSur(pwmHiz);
  } 
  else if (calismaModu == 'g') {
    geriSur(pwmHiz);
  } 
  else {
    motoruDurdur();
  }

  // --- 4. VERİ YAZDIRMA (Non-blocking / Kod duraksamadan) ---
  if (millis() - sonYazdirmaZamani > yazdirmaAraligi) {
    
    // Okunabilir mod isimlendirmesi
    String durumYazisi;
    if (calismaModu == 'i') durumYazisi = "ILERI  >>>";
    else if (calismaModu == 'g') durumYazisi = "<<<  GERI";
    else durumYazisi = "--- DURDU ---";

    // Yüzdelik hız hesaplama (Görsel olarak daha anlaşılır)
    int yuzdeHiz = (pwmHiz * 100) / 255;

    // Ekrana tek satırda yazdır
    Serial.printf("MOD: %s | Pot: %d | PWM: %d | Hiz: %%%d \n", 
                  durumYazisi.c_str(), potDegeri, pwmHiz, yuzdeHiz);
    
    sonYazdirmaZamani = millis(); // Zamanlayıcıyı sıfırla
  }
}

// --- YARDIMCI FONKSİYONLAR ---

void ileriSur(int hiz) {
  digitalWrite(U1LIN, LOW);
  digitalWrite(U2HIN, LOW);
  digitalWrite(U2LIN, HIGH);
  analogWrite(U1HIN, hiz);
}

void geriSur(int hiz) {
  digitalWrite(U1HIN, LOW);
  digitalWrite(U2LIN, LOW);
  digitalWrite(U1LIN, HIGH);
  analogWrite(U2HIN, hiz);
}

void motoruDurdur() {
  analogWrite(U1HIN, 0);
  analogWrite(U2HIN, 0);
  digitalWrite(U1LIN, HIGH);
  digitalWrite(U2LIN, HIGH);
}