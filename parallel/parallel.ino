// ============================================
// NV10 Parallel Mode - Arduino Controller
// VERSIUNE AJUSTATĂ pentru pulsuri lungi
// ============================================

// Pini Vend (ieșiri de la NV10)
const int VEND1_PIN = 2;  // Canal 1
const int VEND2_PIN = 3;  // Canal 2
const int VEND3_PIN = 4;  // Canal 3
const int VEND4_PIN = 5;  // Canal 4

// Pin Busy (opțional)
const int BUSY_PIN = 6;

// Valori bancnote pentru fiecare canal (RON)
int CHANNEL_VALUES[4] = {
  1,    // Canal 1 = 1 RON
  5,    // Canal 2 = 5 RON
  10,   // Canal 3 = 10 RON
  50    // Canal 4 = 50 RON
};

// PARAMETRI AJUSTAȚI pentru pulsuri mai lungi
const unsigned long DEBOUNCE_TIME = 100;      // Debounce mai mare
const unsigned long PULSE_MIN_TIME = 50;      // Acceptă pulsuri de la 50ms
const unsigned long PULSE_MAX_TIME = 500;     // Până la 500ms (mai flexibil!)
const unsigned long PULSE_TIMEOUT = 600;      // Timeout maxim pentru citire

// Variabile
unsigned long lastPulseTime[4] = {0, 0, 0, 0};
bool pulseDetected[4] = {false, false, false, false};

// Statistici
int totalBills = 0;
int totalAmount = 0;
int channelCount[4] = {0, 0, 0, 0};

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  printHeader();
  
  // Configurează pini Vend ca INPUT cu PULLUP
  pinMode(VEND1_PIN, INPUT_PULLUP);
  pinMode(VEND2_PIN, INPUT_PULLUP);
  pinMode(VEND3_PIN, INPUT_PULLUP);
  pinMode(VEND4_PIN, INPUT_PULLUP);
  
  // Pin Busy (opțional)
  pinMode(BUSY_PIN, INPUT_PULLUP);
  
  Serial.println("✓ Arduino configurat");
  Serial.println("✓ Pini INPUT_PULLUP setați");
  Serial.println();
  
  // Afișează setări
  Serial.println("Setări detectare puls:");
  Serial.print("  Minim: ");
  Serial.print(PULSE_MIN_TIME);
  Serial.println(" ms");
  Serial.print("  Maxim: ");
  Serial.print(PULSE_MAX_TIME);
  Serial.println(" ms");
  Serial.print("  Timeout: ");
  Serial.print(PULSE_TIMEOUT);
  Serial.println(" ms");
  Serial.println();
  
  // Verificare inițială
  printConnectionStatus();
  
  Serial.println("════════════════════════════════════════");
  Serial.println("  GATA DE FUNCȚIONARE!");
  Serial.println("════════════════════════════════════════");
  Serial.println();
  Serial.println("Valori canale:");
  for (int i = 0; i < 4; i++) {
    Serial.print("  Canal ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(CHANNEL_VALUES[i]);
    Serial.println(" RON");
  }
  Serial.println();
  Serial.println("Introdu o bancnotă...");
  Serial.println();
}

void loop() {
  // Verifică fiecare canal
  checkChannel(1, VEND1_PIN);
  checkChannel(2, VEND2_PIN);
  checkChannel(3, VEND3_PIN);
  checkChannel(4, VEND4_PIN);
  
  // Comenzi Serial
  checkSerialCommands();
  
  // Status periodic (la fiecare 30 secunde)
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 30000) {
    printStats();
    lastStatus = millis();
  }
  
  delay(1);
}

void checkChannel(int channel, int pin) {
  int channelIndex = channel - 1;
  
  // Citește starea pinului (LOW = puls activ)
  int state = digitalRead(pin);
  
  // Detectează falling edge (start puls)
  static int lastState[4] = {HIGH, HIGH, HIGH, HIGH};
  
  if (state == LOW && lastState[channelIndex] == HIGH) {
    // Start puls detectat
    unsigned long now = millis();
    
    // Verifică debounce
    if (now - lastPulseTime[channelIndex] > DEBOUNCE_TIME) {
      
      // Măsoară durata pulsului
      unsigned long pulseStart = now;
      
      // Așteaptă sfârșitul pulsului SAU timeout
      while (digitalRead(pin) == LOW && (millis() - pulseStart) < PULSE_TIMEOUT) {
        delayMicroseconds(100); // Delay foarte mic pentru precizie
      }
      
      unsigned long pulseWidth = millis() - pulseStart;
      
      // Debug - afișează orice puls detectat
      Serial.print("[Debug] Canal ");
      Serial.print(channel);
      Serial.print(" - Puls detectat: ");
      Serial.print(pulseWidth);
      Serial.print(" ms");
      
      // Verifică dacă pulsul e în range-ul valid
      if (pulseWidth >= PULSE_MIN_TIME && pulseWidth <= PULSE_MAX_TIME) {
        Serial.println(" → VALID ✓");
        
        // Puls valid!
        int value = CHANNEL_VALUES[channelIndex];
        processBill(channel, value, pulseWidth);
        lastPulseTime[channelIndex] = millis();
        
      } else if (pulseWidth < PULSE_MIN_TIME) {
        Serial.println(" → Prea scurt ✗");
        
      } else if (pulseWidth >= PULSE_TIMEOUT) {
        Serial.println(" → Timeout ✗");
        
      } else {
        Serial.println(" → Prea lung ✗");
      }
    }
  }
  
  lastState[channelIndex] = state;
}

void processBill(int channel, int value, unsigned long pulseWidth) {
  // Actualizează statistici
  totalBills++;
  totalAmount += value;
  channelCount[channel - 1]++;
  
  // Afișează mesaj mare
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  ✓✓✓ BANCNOTĂ ACCEPTATĂ! ✓✓✓          ║");
  Serial.print("║  Canal: ");
  Serial.print(channel);
  Serial.println("                                 ║");
  Serial.print("║  Valoare: ");
  Serial.print(value);
  Serial.print(" RON");
  String spaces = "                        ";
  if (value >= 100) spaces = "                     ";
  else if (value >= 10) spaces = "                      ";
  Serial.print(spaces);
  Serial.println("║");
  Serial.print("║  Durata puls: ");
  Serial.print(pulseWidth);
  Serial.print(" ms");
  if (pulseWidth < 100) Serial.print(" ");
  Serial.println("                    ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // Total curent
  Serial.print("Total sesiune: ");
  Serial.print(totalAmount);
  Serial.println(" RON");
  Serial.println();
  
  // Beep opțional (dacă ai buzzer pe pin 10)
  // tone(10, 1000, 100);
}

void printHeader() {
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  NV10 Controller - Parallel Mode      ║");
  Serial.println("║  Version 1.1 (Extended Pulse Support) ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
}

void printConnectionStatus() {
  Serial.println("Status conexiuni:");
  
  // Verifică fiecare pin Vend
  int pins[] = {VEND1_PIN, VEND2_PIN, VEND3_PIN, VEND4_PIN};
  
  for (int i = 0; i < 4; i++) {
    int state = digitalRead(pins[i]);
    
    Serial.print("  Canal ");
    Serial.print(i + 1);
    Serial.print(" (Pin ");
    Serial.print(pins[i]);
    Serial.print("): ");
    Serial.println(state == HIGH ? "HIGH ✓" : "LOW (activ?)");
  }
  
  // Verifică Busy
  int busyState = digitalRead(BUSY_PIN);
  Serial.print("  Busy (Pin ");
  Serial.print(BUSY_PIN);
  Serial.print("): ");
  Serial.println(busyState == HIGH ? "HIGH ✓" : "LOW");
  
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
  Serial.println();
  Serial.println("Detalii pe canal:");
  
  for (int i = 0; i < 4; i++) {
    if (channelCount[i] > 0) {
      Serial.print("  • Canal ");
      Serial.print(i + 1);
      Serial.print(" (");
      Serial.print(CHANNEL_VALUES[i]);
      Serial.print(" RON): ");
      Serial.print(channelCount[i]);
      Serial.print(" buc = ");
      Serial.print(channelCount[i] * CHANNEL_VALUES[i]);
      Serial.println(" RON");
    }
  }
  
  Serial.println("════════════════════════════════════════");
  Serial.println();
}

void resetStats() {
  totalBills = 0;
  totalAmount = 0;
  for (int i = 0; i < 4; i++) {
    channelCount[i] = 0;
  }
  Serial.println();
  Serial.println("✓ Statistici resetate");
  Serial.println();
}

void setChannelValue(int channel, int value) {
  if (channel >= 1 && channel <= 4) {
    CHANNEL_VALUES[channel - 1] = value;
    Serial.print("✓ Canal ");
    Serial.print(channel);
    Serial.print(" setat la ");
    Serial.print(value);
    Serial.println(" RON");
  }
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "R" || cmd == "r") {
      resetStats();
    }
    else if (cmd == "S" || cmd == "s") {
      printStats();
    }
    else if (cmd == "C" || cmd == "c") {
      printConnectionStatus();
    }
    else if (cmd == "H" || cmd == "h") {
      printHelp();
    }
    else if (cmd.startsWith("V")) {
      // Comandă setare valoare: V1=5 (setează canal 1 la 5 RON)
      int equalPos = cmd.indexOf('=');
      if (equalPos > 0) {
        int channel = cmd.substring(1, equalPos).toInt();
        int value = cmd.substring(equalPos + 1).toInt();
        setChannelValue(channel, value);
      }
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
  Serial.println("  S - Afișează statistici");
  Serial.println("  C - Verifică conexiuni");
  Serial.println("  H - Help (această listă)");
  Serial.println("  V1=10 - Setează valoarea canal 1 la 10 RON");
  Serial.println("  V2=50 - Setează valoarea canal 2 la 50 RON");
  Serial.println("  (etc pentru V3, V4)");
  Serial.println("════════════════════════════════════════");
  Serial.println();
}