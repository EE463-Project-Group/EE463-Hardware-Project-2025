// ESP32 WROOM-32U Pin Tanımlamaları
#define U1HIN 33  // pwm U1 HI MOSFET Q1
#define U2HIN 26  // pwm U2 HI MOSFET Q3
#define U1LIN 32  // input U1 LO MOSFET Q2
#define U2LIN 25  // input U2 LO MOSFET Q4

#define POT 14    // Analog Pot Girişi

// --- BOOTSTRAP AYARI ---
// 8-bit PWM (0-255) için %90 sınır değeri: 255 * 0.90 = 229.5 -> 230
const int MAX_PWM = 230; 

// Anlık durum bilgisini tutan değişken
char calismaModu = 'd'; // 'd' = dur, 'i' = ileri, 'g' = geri

// Serial Print zamanlaması için değişken
unsigned long sonYazdirmaZamani = 0;
const int yazdirmaAraligi = 500; 

void setup() {
  Serial.begin(115200);
  Serial.println("--- MOTOR KONTROL SISTEMI (BOOTSTRAP SAFE MODE) ---");
  Serial.println("Maksimum Duty Cycle %90 ile sinirlandirildi.");
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
    
    if (gelenVeri == 'i' || gelenVeri == 'g' || gelenVeri == 'd') {
      
      if (calismaModu != gelenVeri) {
        motoruDurdur();
        // MOSFET ve mekanik koruma için deadtime
        delay(100); 
      }
      calismaModu = gelenVeri; 
    }
  }

  // --- 2. HIZ HESAPLAMA (BOOTSTRAP KORUMALI) ---
  int potDegeri = analogRead(POT);
  
  // Potansiyometre 0-4095 arasını artık 0-230 arasına haritalıyor.
  // Böylece potu sonuna kadar açsan bile PWM asla 230'u (%90'ı) geçmez.
  int pwmHiz = map(potDegeri, 0, 4095, 0, MAX_PWM);

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

  // --- 4. VERİ YAZDIRMA ---
  if (millis() - sonYazdirmaZamani > yazdirmaAraligi) {
    
    String durumYazisi;
    if (calismaModu == 'i') durumYazisi = "ILERI  >>>";
    else if (calismaModu == 'g') durumYazisi = "<<<  GERI";
    else durumYazisi = "--- DURDU ---";

    // Yüzdelik hız hesaplama (Artık max %90 göreceksin)
    int yuzdeHiz = (pwmHiz * 100) / 255;

    Serial.printf("MOD: %s | Pot: %d | PWM: %d (Max %d) | Duty: %%%d \n", 
                  durumYazisi.c_str(), potDegeri, pwmHiz, MAX_PWM, yuzdeHiz);
    
    sonYazdirmaZamani = millis();
  }
}

// --- YARDIMCI FONKSİYONLAR ---

void ileriSur(int hiz) {
  // Önce çakışmayı önle
  digitalWrite(U1LIN, LOW);
  digitalWrite(U2HIN, LOW);
  
  // Aktif Yönü Aç
  digitalWrite(U2LIN, HIGH); // Alt MOSFET sürekli açık (Bootstrap şarjına yardım etmez ama devre tamamlanır)
  analogWrite(U1HIN, hiz);   // Üst MOSFET PWM (Burada %10 boşluk var, kondansatör şarj olacak)
}

void geriSur(int hiz) {
  // Önce çakışmayı önle
  digitalWrite(U1HIN, LOW);
  digitalWrite(U2LIN, LOW);
  
  // Aktif Yönü Aç
  digitalWrite(U1LIN, HIGH);
  analogWrite(U2HIN, hiz);
}

void motoruDurdur() {
  // PWM sinyallerini kes
  analogWrite(U1HIN, 0);
  analogWrite(U2HIN, 0);
  
  // Alt MOSFET'leri AÇIK tutuyoruz.
  // BU ÇOK ÖNEMLİ: Motor dururken alt MOSFET'lerin açık olması,
  // Bootstrap kondansatörünün VCC üzerinden şarj olmasını sağlar.
  // Bir sonraki start için driver hazır olur.
  // Aynı zamanda motora "Fren" yaptırır.
  digitalWrite(U1LIN, HIGH);
  digitalWrite(U2LIN, HIGH);
}