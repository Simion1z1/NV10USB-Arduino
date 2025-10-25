// ============================================
// NV10 PULSE Mode - Arduino Controller
// ============================================
// Un singur pin (Vend 1) primește MULTIPLE pulsuri
// Număr de pulsuri = Valoare / Multiplier
// ============================================

// CONFIGURAȚIE PULSURI (din Validator Manager)
const int PULSE_PIN = 2;              // Pin pentru pulsuri (NV10 Pin 1)
const int PULSE_HIGH_TIME = 200;      // ms (din setări)
const int PULSE_LOW_TIME = 100;       // ms (din setări)
const int PULSE_MULTIPLIER = 1;       // din setări

// Toleranță pentru detectare (±20%)
const int PULSE_HIGH_MIN = PULSE_HIGH_TIME * 0.8;
const int PULSE_HIGH_MAX = PULSE_HIGH_TIME * 1.2;
const int PULSE_LOW_MIN = PULSE_LOW_TIME * 0.8;
const int PULSE_LOW_MAX = PULSE_LOW_TIME * 1.2;

// Timeout pentru pauza între pulsuri (dacă depășește, s-a terminat seria)
const unsigned long PULSE_SERIES_TIMEOUT = 500; // ms

// Pin Busy (opțional)
const int BUSY_PIN = 6;

// Variabile pentru numărare pulsuri
int pulseCount = 0;
unsigned long lastPulseTime = 0;
bool countingPulses = false;
unsigned long seriesStartTime = 0;

// Statistici
int totalBills = 0;
int totalAmount = 0;
int billHistory[50];  // Istoric ultimele 50 bancnote
int historyIndex = 0;

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  printHeader();
  
  // Configurare pin
  pinMode(PULSE_PIN, INPUT_PULLUP);
  pinMode(BUSY_PIN, INPUT_PULLUP);
  
  Serial.println("✓ Arduino configurat pentru modul PULSE");
  Serial.println();
  
  // Afișează setări
  Serial.println("Setări PULSE (din Validator Manager):");
  Serial.print("  • Pulse High Time: ");
  Serial.print(PULSE_HIGH_TIME);
  Serial.println(" ms");
  Serial.print("  • Pulse Low Time: ");
  Serial.print(PULSE_LOW_TIME);
  Serial.println(" ms");
  Serial.print("  • Pulse Multiplier: ");
  Serial.println(PULSE_MULTIPLIER);
  Serial.print("  • Series Timeout: ");
  Serial.print(PULSE_SERIES_TIMEOUT);
  Serial.println(" ms");
  Serial.println();
  
  Serial.println("Toleranță detectare:");
  Serial.print("  • High: ");
  Serial.print(PULSE_HIGH_MIN);
  Serial.print("-");
  Serial.print(PULSE_HIGH_MAX);
  Serial.println(" ms");
  Serial.print("  • Low: ");
  Serial.print(PULSE_LOW_MIN);
  Serial.print("-");
  Serial.print(PULSE_LOW_MAX);
  Serial.println(" ms");
  Serial.println();
  
  // Status conexiune
  printConnectionStatus();
  
  Serial.println("════════════════════════════════════════");
  Serial.println("  GATA DE FUNCȚIONARE!");
  Serial.println("════════════════════════════════════════");
  Serial.println();
  Serial.println("Modul PULSE activ:");
  Serial.println("  • 1 puls = 1 RON");
  Serial.println("  • 5 pulsuri = 5 RON");
  Serial.println("  • 10 pulsuri = 10 RON");
  Serial.println("  • etc.");
  Serial.println();
  Serial.println("LED ar trebui APRINS!");
  Serial.println("Introdu o bancnotă...");
  Serial.println();
}

void loop() {
  checkPulses();
  
  // Verifică dacă seria de pulsuri s-a terminat
  if (countingPulses && (millis() - lastPulseTime > PULSE_SERIES_TIMEOUT)) {
    // Serie completă!
    finalizePulseSeries();
  }
  
  // Comenzi Serial
  checkSerialCommands();
  
  // Status periodic
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 60000) {
    printStats();
    lastStatus = millis();
  }
  
  delay(1);
}

void checkPulses() {
  static int lastState = HIGH;
  static unsigned long pulseStartTime = 0;
  
  int currentState = digitalRead(PULSE_PIN);
  
  // Detectează FALLING EDGE (start puls HIGH)
  if (currentState == LOW && lastState == HIGH) {
    pulseStartTime = millis();
    
    // Dacă e primul puls dintr-o serie
    if (!countingPulses) {
      countingPulses = true;
      pulseCount = 0;
      seriesStartTime = millis();
      Serial.println("─────────────────────────────────────");
      Serial.println("Detectare serie pulsuri...");
    }
  }
  
  // Detectează RISING EDGE (sfârșit puls HIGH)
  else if (currentState == HIGH && lastState == LOW) {
    unsigned long pulseHighDuration = millis() - pulseStartTime;
    
    // Verifică dacă durata HIGH e validă
    if (pulseHighDuration >= PULSE_HIGH_MIN && pulseHighDuration <= PULSE_HIGH_MAX) {
      
      // Așteaptă partea LOW
      unsigned long lowStartTime = millis();
      while (digitalRead(PULSE_PIN) == HIGH && (millis() - lowStartTime) < 300) {
        delayMicroseconds(100);
      }
      
      unsigned long pulseLowDuration = 0;
      
      // Dacă a revenit la LOW, măsoară durata
      if (digitalRead(PULSE_PIN) == LOW) {
        unsigned long lowEnd = millis();
        pulseLowDuration = lowEnd - lowStartTime;
        
        // Verifică dacă LOW e valid
        if (pulseLowDuration >= PULSE_LOW_MIN && pulseLowDuration <= PULSE_LOW_MAX) {
          // PULS VALID!
          pulseCount++;
          lastPulseTime = millis();
          
          Serial.print("  Puls #");
          Serial.print(pulseCount);
          Serial.print(" - High: ");
          Serial.print(pulseHighDuration);
          Serial.print("ms, Low: ");
          Serial.print(pulseLowDuration);
          Serial.println("ms ✓");
          
        } else {
          // LOW invalid
          Serial.print("  [Skip] Low invalid: ");
          Serial.print(pulseLowDuration);
          Serial.println("ms");
        }
      } else {
        // Ultimul puls din serie (nu mai are LOW după)
        pulseCount++;
        lastPulseTime = millis();
        
        Serial.print("  Puls #");
        Serial.print(pulseCount);
        Serial.print(" (final) - High: ");
        Serial.print(pulseHighDuration);
        Serial.println("ms ✓");
      }
      
    } else {
      // HIGH invalid
      Serial.print("  [Skip] High invalid: ");
      Serial.print(pulseHighDuration);
      Serial.println("ms");
    }
  }
  
  lastState = currentState;
}

void finalizePulseSeries() {
  countingPulses = false;
  
  if (pulseCount > 0) {
    unsigned long seriesDuration = millis() - seriesStartTime;
    int value = pulseCount * PULSE_MULTIPLIER;
    
    Serial.println("─────────────────────────────────────");
    Serial.print("Serie completă: ");
    Serial.print(pulseCount);
    Serial.print(" pulsuri în ");
    Serial.print(seriesDuration);
    Serial.println("ms");
    Serial.println();
    
    processBill(value, pulseCount);
  }
}

void processBill(int value, int pulses) {
  // Actualizează statistici
  totalBills++;
  totalAmount += value;
  
  // Salvează în istoric
  billHistory[historyIndex] = value;
  historyIndex = (historyIndex + 1) % 50;
  
  // Afișează mesaj mare
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  ✓✓✓ BANCNOTĂ ACCEPTATĂ! ✓✓✓          ║");
  Serial.print("║  Pulsuri: ");
  Serial.print(pulses);
  if (pulses < 10) Serial.print(" ");
  Serial.println("                             ║");
  Serial.print("║  Valoare: ");
  Serial.print(value);
  Serial.print(" RON");
  if (value < 10) Serial.print("  ");
  else if (value < 100) Serial.print(" ");
  Serial.println("                       ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // Total
  Serial.print("Total sesiune: ");
  Serial.print(totalAmount);
  Serial.print(" RON (");
  Serial.print(totalBills);
  Serial.println(" bancnote)");
  Serial.println();
  
  // Beep opțional
  // tone(10, 1000, 100);
}

void printHeader() {
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  NV10 Controller - PULSE Mode         ║");
  Serial.println("║  Version 1.0                           ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
}

void printConnectionStatus() {
  Serial.println("Status conexiuni:");
  
  int pulseState = digitalRead(PULSE_PIN);
  Serial.print("  Pulse Pin (Pin ");
  Serial.print(PULSE_PIN);
  Serial.print("): ");
  Serial.println(pulseState == HIGH ? "HIGH (idle) ✓" : "LOW (activ?)");
  
  int busyState = digitalRead(BUSY_PIN);
  Serial.print("  Busy Pin (Pin ");
  Serial.print(BUSY_PIN);
  Serial.print("): ");
  Serial.println(busyState == HIGH ? "HIGH (idle) ✓" : "LOW");
  
  Serial.println();
}

void printStats() {
  Serial.println();
  Serial.println("════════════════════════════════════════");
  Serial.println("  STATISTICI SESIUNE");
  Serial.println("════════════════════════════════════════");
  Serial.print("Total bancnote: ");
  Serial.print(totalBills);
  Serial.println(" buc");
  Serial.print("Total valoare: ");
  Serial.print(totalAmount);
  Serial.println(" RON");
  
  if (totalBills > 0) {
    Serial.print("Medie/bancnotă: ");
    Serial.print(totalAmount / totalBills);
    Serial.println(" RON");
  }
  
  // Afișează ultimele 10 bancnote
  Serial.println();
  Serial.println("Ultimele bancnote:");
  int displayCount = min(totalBills, 10);
  for (int i = 0; i < displayCount; i++) {
    int index = (historyIndex - 1 - i + 50) % 50;
    Serial.print("  ");
    Serial.print(displayCount - i);
    Serial.print(". ");
    Serial.print(billHistory[index]);
    Serial.println(" RON");
  }
  
  Serial.println("════════════════════════════════════════");
  Serial.println();
}

void resetStats() {
  totalBills = 0;
  totalAmount = 0;
  historyIndex = 0;
  for (int i = 0; i < 50; i++) {
    billHistory[i] = 0;
  }
  Serial.println();
  Serial.println("✓ Statistici resetate");
  Serial.println();
}

void testPulse() {
  Serial.println();
  Serial.println("TEST MODE: Simulare pulsuri...");
  Serial.println("Apasă orice tastă pentru a opri");
  Serial.println();
  
  // Simulează 5 pulsuri (5 RON)
  for (int i = 0; i < 5; i++) {
    Serial.print("Test puls #");
    Serial.println(i + 1);
    
    if (Serial.available() > 0) {
      Serial.read();
      Serial.println("Test oprit");
      return;
    }
    
    delay(PULSE_HIGH_TIME + PULSE_LOW_TIME);
  }
  
  Serial.println("Test complet!");
  Serial.println();
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    
    if (cmd == "R") {
      resetStats();
    }
    else if (cmd == "S") {
      printStats();
    }
    else if (cmd == "C") {
      printConnectionStatus();
    }
    else if (cmd == "H") {
      printHelp();
    }
    else if (cmd == "T") {
      testPulse();
    }
    else if (cmd.length() > 0) {
      Serial.println("✗ Comandă necunoscută. Tastează H pentru help.");
    }
  }
}

void printHelp() {
  Serial.println();
  Serial.println("════════════════════════════════════════");
  Serial.println("  COMENZI DISPONIBILE");
  Serial.println("════════════════════════════════════════");
  Serial.println("  R - Reset statistici");
  Serial.println("  S - Afișează statistici detaliate");
  Serial.println("  C - Verifică status conexiuni");
  Serial.println("  T - Test mode (simulare)");
  Serial.println("  H - Help (această listă)");
  Serial.println("════════════════════════════════════════");
  Serial.println();
}