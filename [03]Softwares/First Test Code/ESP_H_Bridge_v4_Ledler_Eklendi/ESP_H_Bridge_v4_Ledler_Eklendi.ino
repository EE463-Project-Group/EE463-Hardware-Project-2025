// ESP32 WROOM-32E Hibrit Motor Kontrol (Serial + Buton)
// -----------------------------------------------------
// ÖNCELİK SIRASI:
// 1. Fiziksel Fren Butonu (Basılı Tutulduğu Sürece)
// 2. Serial Komutlar (Anlık Tepki)
// 3. Diğer Butonlar (Seç -> Başlat Mantığı)

// --- PIN TANIMLAMALARI ---
// Motor Pinleri
#define U1HIN 33  // PWM Q1
#define U2HIN 26  // PWM Q3
#define U1LIN 32  // LOW Q2
#define U2LIN 25  // LOW Q4

#define POT 14    // Hız Ayarı

// Buton Pinleri
#define BTN_ILERI 15
#define BTN_GERI  16
#define BTN_START 17
#define BTN_STOP  18
#define BTN_FREN  19

// Gösterge LED'leri
#define LED_CW      4   // İleri Yön Seçim LEDi (Sarı/Yeşil vb.)
#define LED_CCW     5   // Geri Yön Seçim LEDi (Sarı/Yeşil vb.)

// Yeni Şematik Düzeni:
#define LED_ALIVE   22  // "Sistem Alive" - Motor İleri/Geri dönerken yanar
#define LED_BRAKE   23  // Fren LEDi - Sadece Fren anında yanar
#define LED_DEBUG   21   // "Disable/Boşta" LEDi - Motor Boşta iken yanar (Varsayılan: Dahili LED)

// --- AYARLAR ---
const int MAX_PWM = 230; // %90 Duty Cycle (Bootstrap Koruması)

// --- DEĞİŞKENLER ---
char calismaModu = 'b'; // 'b'=Boşta, 'i'=İleri, 'g'=Geri, 'f'=Fren
char secilenYon = 'i';  // 'i'=İleri, 'g'=Geri (Varsayılan İleri)
bool frenBasiliMi = false;

// Serial Zamanlayıcı
unsigned long sonYazdirmaZamani = 0;
const int yazdirmaAraligi = 200; 

void setup() {
  Serial.begin(115200);
  Serial.println("--- HIBRIT MOTOR KONTROL SISTEMI ---");
  Serial.println("Kontrol: Butonlar VEYA Serial Monitor");
  Serial.println("Serial Komutlar: 'i'(Ileri), 'g'(Geri), 'b'/'d'(Dur/Bosta), 'f'(Fren)");
  
  // Güvenli Başlangıç (Pinleri önce LOW yap)
  digitalWrite(U1HIN, LOW); digitalWrite(U2HIN, LOW);
  digitalWrite(U1LIN, LOW); digitalWrite(U2LIN, LOW);
  
  pinMode(U1HIN, OUTPUT); pinMode(U2HIN, OUTPUT);
  pinMode(U1LIN, OUTPUT); pinMode(U2LIN, OUTPUT);
  
  pinMode(LED_CW, OUTPUT);
  pinMode(LED_CCW, OUTPUT);
  pinMode(LED_BRAKE, OUTPUT);
  pinMode(LED_ALIVE, OUTPUT); // Yeni
  pinMode(LED_DEBUG, OUTPUT); // Yeni

  // Dahili Pull-Up Dirençleri
  pinMode(BTN_ILERI, INPUT_PULLUP);
  pinMode(BTN_GERI,  INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP,  INPUT_PULLUP);
  pinMode(BTN_FREN,  INPUT_PULLUP);

  bostaBirak();
  ledleriGuncelle();
}

void loop() {
  // ---------------------------------------------------------
  // 1. EN YÜKSEK ÖNCELİK: FİZİKSEL FREN BUTONU
  // ---------------------------------------------------------
  if (digitalRead(BTN_FREN) == LOW) { 
    if (calismaModu != 'f') {
      frenYap();
      calismaModu = 'f';
      frenBasiliMi = true;
    }
    motorSurucusuGuncelle(); // Freni uygula
    delay(10); 
    return; // Fren basılıyken başka komut dinleme!
  } 
  else {
    // Fren butonu bırakıldı mı?
    if (frenBasiliMi == true) {
      bostaBirak();
      calismaModu = 'b';
      frenBasiliMi = false;
    }
  }

  // ---------------------------------------------------------
  // 2. SERIAL HABERLEŞME KONTROLÜ
  // ---------------------------------------------------------
  if (Serial.available() > 0) {
    char gelenVeri = Serial.read();
    
    // Geçerli komut kontrolü
    if (gelenVeri == 'i' || gelenVeri == 'g' || gelenVeri == 'f' || 
        gelenVeri == 'b' || gelenVeri == 'd') { // d veya b durdurur
      
      // 'd' gelirse 'b' (boşta) olarak işle
      if (gelenVeri == 'd') gelenVeri = 'b';

      // Yön değişimi varsa ölü zaman (Deadtime) uygula
      if (calismaModu != gelenVeri && calismaModu != 'b') {
        bostaBirak();
        delay(100); 
      }

      // Serial'den gelen komuta göre hem modu hem de seçili yönü güncelle
      calismaModu = gelenVeri;
      
      if (gelenVeri == 'i') secilenYon = 'i';
      if (gelenVeri == 'g') secilenYon = 'g';
      
      ledleriGuncelle(); // Serial'den gelen yönü LED'e yansıt
      
      // Geri bildirim
      Serial.print("SERIAL KOMUT: "); 
      Serial.println(gelenVeri);
    }
  }

  // ---------------------------------------------------------
  // 3. BUTON KONTROLLERİ
  // ---------------------------------------------------------
  
  // STOP Butonu
  if (digitalRead(BTN_STOP) == LOW) {
    if (calismaModu != 'b') {
      bostaBirak();
      calismaModu = 'b';
      Serial.println("BUTON: STOP");
      delay(200); 
    }
  }

  // Yön Seçim Butonları (Sadece Hafızayı Değiştirir)
  if (digitalRead(BTN_ILERI) == LOW) {
    secilenYon = 'i';
    ledleriGuncelle();
  }
  if (digitalRead(BTN_GERI) == LOW) {
    secilenYon = 'g';
    ledleriGuncelle();
  }

  // START Butonu
  if (digitalRead(BTN_START) == LOW) {
    if (calismaModu != secilenYon) {
      // Yön değişiyorsa Deadtime
      if (calismaModu != 'b') {
        bostaBirak();
        delay(100);
      }
      calismaModu = secilenYon;
      Serial.println("BUTON: START");
      delay(200); 
    }
  }

  // ---------------------------------------------------------
  // 4. MOTOR SÜRÜŞ VE EKRAN
  // ---------------------------------------------------------
  motorSurucusuGuncelle();

  if (millis() - sonYazdirmaZamani > yazdirmaAraligi) {
    bilgiYazdir();
    sonYazdirmaZamani = millis();
  }
}

// --- YARDIMCI FONKSİYONLAR ---

void motorSurucusuGuncelle() {
  int potDegeri = analogRead(POT);
  int pwmHiz = map(potDegeri, 0, 4095, 0, MAX_PWM);
  
  switch (calismaModu) {
    case 'i': ileriSur(pwmHiz); break;
    case 'g': geriSur(pwmHiz); break;
    case 'f': frenYap(); break;     // PWM'den bağımsız tam fren
    case 'b': default: bostaBirak(); break;
  }
  
  // LED durumlarını her döngüde kontrol et (Yanıp sönme vs. eklenirse burası önemli olur)
  ledleriGuncelle();
}

void ledleriGuncelle() {
  // 1. YÖN SEÇİM LEDLERİ (CW/CCW) - Kullanıcı niyeti
  if (secilenYon == 'i') {
    digitalWrite(LED_CW, HIGH);
    digitalWrite(LED_CCW, LOW);
  } else {
    digitalWrite(LED_CW, LOW);
    digitalWrite(LED_CCW, HIGH);
  }
  
  // 2. FREN LEDİ
  digitalWrite(LED_BRAKE, (calismaModu == 'f') ? HIGH : LOW);

  // 3. ALIVE LED (IO22) - Motor dönüyorsa (İleri veya Geri)
  if (calismaModu == 'i' || calismaModu == 'g') {
    digitalWrite(LED_ALIVE, HIGH);
  } else {
    digitalWrite(LED_ALIVE, LOW);
  }

  // 4. DEBUG LED (IO2) - Motor "Boşta/Disable" ise
  if (calismaModu == 'b') {
    digitalWrite(LED_DEBUG, HIGH);
  } else {
    digitalWrite(LED_DEBUG, LOW);
  }
}

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

void frenYap() {
  // Alt MOSFET'ler açık, Üstler kapalı (Short Brake & Bootstrap Charge)
  analogWrite(U1HIN, 0); 
  analogWrite(U2HIN, 0);
  digitalWrite(U1LIN, HIGH); 
  digitalWrite(U2LIN, HIGH);
}

void bostaBirak() {
  // Her şey kapalı (Coasting)
  analogWrite(U1HIN, 0);
  analogWrite(U2HIN, 0);
  digitalWrite(U1LIN, LOW);
  digitalWrite(U2LIN, LOW);
}

void bilgiYazdir() {
  String durumStr;
  if (calismaModu == 'i') durumStr = "ILERI SURUS";
  else if (calismaModu == 'g') durumStr = "GERI SURUS";
  else if (calismaModu == 'f') durumStr = "!!! FREN !!!";
  else durumStr = "BOSTA (STOP)";

  String secimStr = (secilenYon == 'i') ? "ILERI" : "GERI";
  int pot = analogRead(POT);
  int hizYuzde = map(pot, 0, 4095, 0, 90);

  Serial.printf("MOD: %-13s | SECIM: %-5s | HIZ: %%%d \n", 
                durumStr.c_str(), secimStr.c_str(), hizYuzde);
}