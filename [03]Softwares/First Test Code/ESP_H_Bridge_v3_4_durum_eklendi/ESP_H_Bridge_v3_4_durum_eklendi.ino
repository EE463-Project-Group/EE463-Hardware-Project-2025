// ESP32 WROOM-32U Pin Tanımlamaları
#define U1HIN 33  // pwm U1 HI MOSFET Q1
#define U2HIN 26  // pwm U2 HI MOSFET Q3
#define U1LIN 32  // input U1 LO MOSFET Q2
#define U2LIN 25  // input U2 LO MOSFET Q4

#define POT 14    // Analog Pot Girişi

// --- BOOTSTRAP AYARI ---
// %90 Sınır (255 * 0.90 = ~230)
const int MAX_PWM = 230; 

// Anlık durum bilgisini tutan değişken
// Başlangıç durumu: 'b' (Boşta/Güvenli)
char calismaModu = 'b'; 

// Serial Print zamanlaması
unsigned long sonYazdirmaZamani = 0;
const int yazdirmaAraligi = 500; 

void setup() {
  Serial.begin(115200);
  Serial.println("--- 4 MODLU MOTOR KONTROL SISTEMI ---");
  Serial.println("Komutlar:");
  Serial.println(" 'i' -> Ileri Surus");
  Serial.println(" 'g' -> Geri Surus");
  Serial.println(" 'f' -> Frenleme (Motoru Kilitler)");
  Serial.println(" 'b' -> Bosta (Serbest Dusus)");

  // GÜVENLİK: Önce LOW yaz, sonra OUTPUT yap
  digitalWrite(U1HIN, LOW);
  digitalWrite(U2HIN, LOW);
  digitalWrite(U1LIN, LOW);
  digitalWrite(U2LIN, LOW);

  pinMode(U1HIN, OUTPUT);
  pinMode(U2HIN, OUTPUT);
  pinMode(U1LIN, OUTPUT);
  pinMode(U2LIN, OUTPUT);

  bostaBirak(); // Sistemi boşta başlat
}

void loop() {
  // --- 1. KOMUT KONTROLÜ ---
  if (Serial.available() > 0) {
    char gelenVeri = Serial.read();
    
    // Geçerli komutlardan biri mi?
    if (gelenVeri == 'i' || gelenVeri == 'g' || gelenVeri == 'f' || gelenVeri == 'b') {
      
      // Durum değişiyorsa araya güvenlik beklemesi (Deadtime) koy
      if (calismaModu != gelenVeri) {
        bostaBirak(); // Geçiş anında her şeyi kes
        delay(100);   // MOSFET'lerin tamamen kapanmasını bekle
      }
      calismaModu = gelenVeri; 
    }
  }

  // --- 2. HIZ HESAPLAMA ---
  int potDegeri = analogRead(POT);
  int pwmHiz = map(potDegeri, 0, 4095, 0, MAX_PWM);

  // --- 3. DURUM MAKİNESİ (STATE MACHINE) ---
  switch (calismaModu) {
    case 'i':
      ileriSur(pwmHiz);
      break;
    case 'g':
      geriSur(pwmHiz);
      break;
    case 'f':
      frenYap();
      break;
    case 'b':
      bostaBirak();
      break;
  }

  // --- 4. BİLGİ EKRANI ---
  if (millis() - sonYazdirmaZamani > yazdirmaAraligi) {
    
    String durumYazisi;
    if (calismaModu == 'i') durumYazisi = "ILERI GIDIYOR >>>";
    else if (calismaModu == 'g') durumYazisi = "<<< GERI GIDIYOR";
    else if (calismaModu == 'f') durumYazisi = "[!] FRENLEME";
    else durumYazisi = "(Zzz) BOSTA";

    int yuzdeHiz = (pwmHiz * 100) / 255;

    Serial.printf("MOD: %s | Pot: %d | PWM: %d | Hiz: %%%d \n", 
                  durumYazisi.c_str(), potDegeri, pwmHiz, yuzdeHiz);
    
    sonYazdirmaZamani = millis();
  }
}

// ---------------------------------------------------------
// -------------- MOTOR KONTROL FONKSİYONLARI --------------
// ---------------------------------------------------------

// MOD 1: İLERİ
void ileriSur(int hiz) {
  // Güvenlik: Çaprazı kapat
  digitalWrite(U1LIN, LOW);
  digitalWrite(U2HIN, LOW);
  
  // Sürüş: Q4(U2LIN) Açık, Q1(U1HIN) PWM
  digitalWrite(U2LIN, HIGH); 
  analogWrite(U1HIN, hiz);   
}

// MOD 2: GERİ
void geriSur(int hiz) {
  // Güvenlik: Çaprazı kapat
  digitalWrite(U1HIN, LOW);
  digitalWrite(U2LIN, LOW);
  
  // Sürüş: Q2(U1LIN) Açık, Q3(U2HIN) PWM
  digitalWrite(U1LIN, HIGH);
  analogWrite(U2HIN, hiz);
}

// MOD 3: FRENLEME (BRAKING)
// Alt MOSFET'leri açarak motoru kısa devre eder.
// Ayrıca Bootstrap kondansatörlerinin şarj olmasını sağlar.
void frenYap() {
  analogWrite(U1HIN, 0); // Üstleri Kapat
  analogWrite(U2HIN, 0);
  
  digitalWrite(U1LIN, HIGH); // Altları Aç
  digitalWrite(U2LIN, HIGH);
}

// MOD 4: BOŞTA (COASTING)
// Tüm MOSFET'ler kapalı. Motor akışına bırakılır.
void bostaBirak() {
  analogWrite(U1HIN, 0);
  analogWrite(U2HIN, 0);
  digitalWrite(U1LIN, LOW);
  digitalWrite(U2LIN, LOW);
}