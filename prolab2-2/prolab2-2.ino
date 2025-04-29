#include <LiquidCrystal.h>

// Pin tanımları
const int motorBtnPin   = 22;
const int beltBtnPin    = 23;
const int doorSwitchPin = 24;
const int redLedPin     = 30;
const int blueLedPin    = 31;
const int yellowLedPin  = 32;
const int pinkRedPin    = 33;
const int pinkGreenPin  = 34;
const int pinkBluePin   = 35;
const int buzzerPin     = 36;
const int dcMotorPin    = 40;
const int dcFanPin      = 41;
const int tempPin       = A0;
const int ldrPin        = A1;
const int fuelPotPin    = A2;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

// Durum değişkenleri
bool beltOn = false, lastBeltBtnState = HIGH, engineOn = false, lastMotorBtnState = HIGH;
bool yellowBlinkState = false;
unsigned long lastBlinkMillis = 0;
unsigned long lastScreenUpdate = 0;
bool fanOn = false;
bool farOn = false;
bool lowFuelWarned = false;
bool criticalFuelWarned = false;
bool lastBuzzerState = false;
unsigned long lastTempFanMillis = 0;
unsigned long lastMainScreenUpdate = 0;
bool showTempFanScreen = false;
bool showFarScreen = false;
unsigned long lastFarScreenMillis = 0;

bool doorWarningShown = false;

// Klima için statik değişkenler
static bool klimaAcik = false;
static float sonSicaklik = -1000;

void setPinkLed(bool state) {
  if (state) {
    digitalWrite(pinkRedPin, HIGH);   
    digitalWrite(pinkBluePin, HIGH);   
    digitalWrite(pinkGreenPin, LOW);   
  } else {
    digitalWrite(pinkRedPin, LOW);
    digitalWrite(pinkBluePin, LOW);
    digitalWrite(pinkGreenPin, LOW);
  }
}

// Tüm LED'leri ve buzzer'ı kapatır
void turnOffAllLEDs() {
  digitalWrite(redLedPin, LOW);
  digitalWrite(blueLedPin, LOW);
  digitalWrite(yellowLedPin, LOW);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(pinkRedPin, LOW);
  digitalWrite(pinkBluePin, LOW);
  digitalWrite(pinkGreenPin, LOW);
}

void setup() {
  pinMode(motorBtnPin, INPUT_PULLUP);
  pinMode(beltBtnPin, INPUT_PULLUP);
  pinMode(doorSwitchPin, INPUT_PULLUP);
  pinMode(redLedPin, OUTPUT);
  pinMode(blueLedPin, OUTPUT);
  pinMode(yellowLedPin, OUTPUT);
  pinMode(pinkRedPin, OUTPUT);
  pinMode(pinkGreenPin, OUTPUT);
  pinMode(pinkBluePin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(dcMotorPin, OUTPUT);
  pinMode(dcFanPin, OUTPUT);

  // Başlangıçta tüm çıkışlar kapalı
  turnOffAllLEDs();
  digitalWrite(dcMotorPin, LOW);
  digitalWrite(dcFanPin, LOW);
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Arac Hazir.");
  lcd.setCursor(0, 1);
  lcd.print("Hosgeldiniz!");
  delay(1200);
  lcd.clear();
}

void loop() {
  // Sensör değerlerini oku
  int tempRaw = analogRead(tempPin);
  float tempC = (tempRaw * 5.0 / 1024.0) * 100.0;
  int ldrValue = analogRead(ldrPin);
  int fuelRaw = analogRead(fuelPotPin);
  int fuelPercent = map(fuelRaw, 0, 1023, 0, 100);
  bool doorClosed = digitalRead(doorSwitchPin);
  bool motorBtnState = digitalRead(motorBtnPin);
  bool beltBtnState = digitalRead(beltBtnPin);

  // --- Yakıt Durumu: Yakıt 0 olduğunda tüm çıkışları kapat
  if (fuelPercent <= 0) {
    engineOn = false;
    digitalWrite(dcMotorPin, LOW);
    digitalWrite(dcFanPin, LOW);
    turnOffAllLEDs();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Motor Durdu");
    lcd.setCursor(0, 1);
    lcd.print("Yakit Bitti");
    delay(2000);
    lcd.clear();
    return;
  }

  // --- Kapı Durumu: Eğer kapı açıksa motor çalışmayacak
  if (!doorClosed) {
    // Motor ve klima kapalı olsun
    digitalWrite(dcMotorPin, LOW);
    digitalWrite(dcFanPin, LOW);
    // Kapı göstergesi (pembe LED) yanmalı
    setPinkLed(true);
    // Göstergeler arasında sarı LED de açılabilir
    digitalWrite(yellowLedPin, HIGH);
  } else {
    // Kapı kapalıysa, kapı göstergesi kapatılır.
    setPinkLed(false);
    digitalWrite(yellowLedPin, LOW);
    doorWarningShown = false; // Uyarı tekrar gösterilebilsin
  }

  // --- Kemer Düğmesi: Kemer durumunu toggle et
  if (lastBeltBtnState == HIGH && beltBtnState == LOW) {
    beltOn = !beltOn;
    delay(200);
  }
  lastBeltBtnState = beltBtnState;

  // --- Motor Düğmesi İşlemleri ---
  if (lastMotorBtnState == HIGH && motorBtnState == LOW) {
    if (!doorClosed) { // Kapı açıkken
      if (!beltOn) { // Kemeri takmadıysa
         digitalWrite(redLedPin, HIGH);
         digitalWrite(buzzerPin, HIGH);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Emniyet Kemeri");
         lcd.setCursor(0, 1);
         lcd.print("Takili Degil!");
         delay(1000);
         lcd.clear();
         // Not: Motor çalıştırılmaz.
      } else {
         // Kapı açık, kemer takılı olsa dahi motor çalışmaz
         if (!doorWarningShown) {
           lcd.clear();
           lcd.setCursor(0, 0);
           lcd.print("Uyari: Kapi Acik");
           lcd.setCursor(0, 1);
           lcd.print("Motor Calismaz");
           delay(1500);
           lcd.clear();
           doorWarningShown = true;
         }
      }
    } else { // Kapı kapalı ise
      if (!beltOn) { // Kemeri takmadıysa
         engineOn = false;
         if (!lastBuzzerState) {
            digitalWrite(redLedPin, HIGH);
            digitalWrite(buzzerPin, HIGH);
            lastBuzzerState = true;
         }
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Emniyet Kemeri");
         lcd.setCursor(0, 1);
         lcd.print("Takili Degil!");
         delay(1000);
         lcd.clear();
      } else {
         // Hem kapı kapalı hem kemer takılıysa motor çalışır
         engineOn = true;
         if (lastBuzzerState) {
           digitalWrite(redLedPin, LOW);
           digitalWrite(buzzerPin, LOW);
           lastBuzzerState = false;
         }
         digitalWrite(dcMotorPin, HIGH);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("Motor Calisti!");
         lcd.setCursor(0, 1);
         lcd.print("Iyi Surusler!");
         delay(1000);
         lcd.clear();
      }
    }
  }
  lastMotorBtnState = motorBtnState;

  //  Motor çalışırken kemer çıkarılırsa
  if (engineOn && !beltOn) {
    engineOn = false;
    digitalWrite(dcMotorPin, LOW);
    digitalWrite(redLedPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kemer Cikartildi!");
    lcd.setCursor(0, 1);
    lcd.print("Motor Durdu");
    delay(1500);
    lcd.clear();
  }
  if (beltOn && lastBuzzerState) {
    digitalWrite(redLedPin, LOW);
    digitalWrite(buzzerPin, LOW);
    lastBuzzerState = false;
  }

  // --- Far Kontrolü ---
  if (ldrValue <= 250) {
    if (!farOn) { // Farlar kapalıydı, şimdi açılıyor
      digitalWrite(blueLedPin, HIGH);
      farOn = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Farlar Acik");
      delay(1500);
      lcd.clear();
    }
  } else {
    if (farOn) { // Farlar açıktı, şimdi kapanıyor
      digitalWrite(blueLedPin, LOW);
      farOn = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Farlar Kapandi");
      delay(1500);
      lcd.clear();
    }
  }

  // --- Yakıt Uyarıları ---
  if (fuelPercent < 5) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastBlinkMillis >= 200) {
      lastBlinkMillis = currentMillis;
      yellowBlinkState = !yellowBlinkState;
      digitalWrite(yellowLedPin, yellowBlinkState ? HIGH : LOW);
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kritik: Yakit");
    lcd.setCursor(0, 1);
    lcd.print("Cok Az - ");
    lcd.print(fuelPercent);
    lcd.print("%");
    delay(2000);
    lcd.clear();
  } else if (fuelPercent < 10) {
    digitalWrite(yellowLedPin, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Uyari: Yakit Az");
    lcd.setCursor(0, 1);
    lcd.print("Seviye: ");
    lcd.print(fuelPercent);
    lcd.print("%");
    delay(2000);
    lcd.clear();
  } else {
    digitalWrite(yellowLedPin, LOW);
  }

  // --- Sıcaklık Kontrolü ve Otomatik Klima Sistemi ---
  if (abs(tempC - sonSicaklik) >= 0.5) {
    sonSicaklik = tempC;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sicaklik: ");
    lcd.print(tempC, 1);
    lcd.print((char)223);
    lcd.print("C");
    if (tempC > 25.0) {
      lcd.setCursor(0, 1);
      lcd.print("Klima Acildi");
      digitalWrite(dcFanPin, HIGH);
      klimaAcik = true;
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Klima Kapandi");
      digitalWrite(dcFanPin, LOW);
      klimaAcik = false;
    }
    delay(2000);
    lcd.clear();
  }

  // Özel ekranlar  otomatik kapanır
  if (showTempFanScreen && millis() - lastTempFanMillis > 2000) {
    showTempFanScreen = false;
    lcd.clear();
  }
  if (showFarScreen && millis() - lastFarScreenMillis > 2000) {
    showFarScreen = false;
    lcd.clear();
  }
}
