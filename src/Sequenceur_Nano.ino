/*
  Sequenceur_Nano.ino - Séquenceur de trappe (solénoïde + ressort)
  DylanPerineti - NovaCnam - Projet ADASTRA

  Câblage :
  - Jack de rampe sur D2 : jack inséré => D2 = LOW (vers GND)
                           jack arraché => D2 = HIGH (pull-up)
  - MOSFET (gate) sur D9 : D9 HIGH => solénoïde alimenté (impulsion)
  - Masse commune obligatoire entre Arduino et alim solénoïde

  Gestion LEDs d'états :
  - Par défaut, le code gère des LEDs externes (facultatif) + la LED intégrée.
  - Si tu n'as qu'une seule LED (celle de la Nano), mets USE_EXTERNAL_LEDS à 0.

  Etats :
  1) WAIT_ARM      : attente insertion jack (pas armé)
  2) WAIT_LAUNCH   : armé, attente décollage (jack arraché)
  3) WAIT_DELAY    : temporisation X secondes
  4) FIRE          : impulsion solénoïde
  5) DONE          : déclenchement terminé
*/

#include <Arduino.h>

// ===================== CONFIG =====================
const uint8_t PIN_JACK = 2;   // entrée jack (D2)
const uint8_t PIN_FIRE = 9;   // sortie vers MOSFET gate (D9)

// Paramètres à ajuster
const unsigned long DELAY_MS    = 4000;  // X secondes après décollage
const unsigned long PULSE_MS    = 250;   // durée d'impulsion solénoïde
const unsigned long DEBOUNCE_MS = 150;   // anti-rebond jack

// ----- LEDs d'états -----
#define USE_EXTERNAL_LEDS 1

#if USE_EXTERNAL_LEDS
// Exemple de pins dispo sur un Nano (adaptables à ton câblage)
// (Sur ton schéma, un header semble sortir D7/D8/D9/D10 : on évite D9 car c'est FIRE)
const uint8_t PIN_LED_ARMED  = 7;   // LED "ARMÉ"
const uint8_t PIN_LED_DELAY  = 8;   // LED "TEMPO"
const uint8_t PIN_LED_DONE   = 10;  // LED "DONE"
#endif

const uint8_t PIN_LED_FIRE = LED_BUILTIN; // LED intégrée = "FIRE/STATUS"
// ==================================================


// ===================== ETATS ======================
enum State : uint8_t { WAIT_ARM, WAIT_LAUNCH, WAIT_DELAY, FIRE, DONE };
State state = WAIT_ARM;

unsigned long t0 = 0;            // timer général (départ selon état)
unsigned long tBlink = 0;        // timer clignotement
bool blinkLevel = false;         // niveau de clignotement

// Lecture stable (anti-rebond simple) : "expectedHigh=true" attend HIGH, sinon attend LOW
static bool readJackStable(bool expectedHigh, unsigned long stableMs) {
  unsigned long start = millis();
  while (millis() - start < stableMs) {
    if (digitalRead(PIN_JACK) != (expectedHigh ? HIGH : LOW)) return false;
  }
  return true;
}

// Mise à jour LEDs en fonction de l'état + clignotements non-bloquants
static void updateLeds() {
  const unsigned long now = millis();

  // Périodes de blink selon état (ms). 0 = pas de blink (LED fixe)
  unsigned long period = 0;
  bool armedOn = false;
  bool delayOn = false;
  bool fireOn  = false;
  bool doneOn  = false;

  switch (state) {
    case WAIT_ARM:
      // Pas armé : tout OFF, sauf éventuellement blink lent sur LED intégrée
      period = 800; // blink lent intégré
      fireOn = blinkLevel;
      break;

    case WAIT_LAUNCH:
      // Armé, attente décollage : LED ARMÉ clignote lentement
      period = 600; // ~1,6 Hz
      armedOn = blinkLevel;
      break;

    case WAIT_DELAY:
      // Temporisation : LED TEMPO clignote plus vite
      period = 250; // 4 Hz
      delayOn = blinkLevel;
      break;

    case FIRE:
      // Déclenchement : LED FIRE fixe ON pendant l'impulsion
      fireOn = true;
      break;

    case DONE:
      // Terminé : LED DONE fixe ON
      doneOn = true;
      break;
  }

  // Gestion du blinkLevel (non-bloquant)
  if (period > 0) {
    if (now - tBlink >= period) {
      tBlink = now;
      blinkLevel = !blinkLevel;
    }
  } else {
    // pas de blink => niveau cohérent (OFF par défaut)
    blinkLevel = false;
  }

  // Appliquer sorties LED
#if USE_EXTERNAL_LEDS
  digitalWrite(PIN_LED_ARMED, armedOn ? HIGH : LOW);
  digitalWrite(PIN_LED_DELAY, delayOn ? HIGH : LOW);
  digitalWrite(PIN_LED_DONE,  doneOn  ? HIGH : LOW);
#endif
  digitalWrite(PIN_LED_FIRE,  fireOn  ? HIGH : LOW);
}

// Changement d'état (centralisé)
static void setState(State s) {
  state = s;
  // Reset blink à chaque changement d'état pour un feedback clair
  tBlink = millis();
  blinkLevel = false;
  updateLeds();
}

// ===================== SETUP/LOOP =================
void setup() {
  pinMode(PIN_JACK, INPUT_PULLUP);

  pinMode(PIN_FIRE, OUTPUT);
  digitalWrite(PIN_FIRE, LOW); // sécurité

  pinMode(PIN_LED_FIRE, OUTPUT);
  digitalWrite(PIN_LED_FIRE, LOW);

#if USE_EXTERNAL_LEDS
  pinMode(PIN_LED_ARMED, OUTPUT);
  pinMode(PIN_LED_DELAY, OUTPUT);
  pinMode(PIN_LED_DONE,  OUTPUT);
  digitalWrite(PIN_LED_ARMED, LOW);
  digitalWrite(PIN_LED_DELAY, LOW);
  digitalWrite(PIN_LED_DONE,  LOW);
#endif

  setState(WAIT_ARM);
}

void loop() {
  updateLeds();

  const int jack = digitalRead(PIN_JACK); // LOW = jack présent, HIGH = jack arraché

  switch (state) {
    case WAIT_ARM:
      // On passe "armé" uniquement si le jack est présent de façon stable
      if (jack == LOW && readJackStable(false, DEBOUNCE_MS)) {
        setState(WAIT_LAUNCH);
      }
      break;

    case WAIT_LAUNCH:
      // Décollage : jack arraché (passe à HIGH de façon stable)
      if (jack == HIGH && readJackStable(true, DEBOUNCE_MS)) {
        t0 = millis();
        setState(WAIT_DELAY);
      }
      break;

    case WAIT_DELAY:
      if (millis() - t0 >= DELAY_MS) {
        // Impulsion sur solénoïde via MOSFET
        digitalWrite(PIN_FIRE, HIGH);
        t0 = millis();
        setState(FIRE);
      }
      break;

    case FIRE:
      if (millis() - t0 >= PULSE_MS) {
        digitalWrite(PIN_FIRE, LOW);
        setState(DONE);
      }
      break;

    case DONE:
      // Déclenchement unique. Pour re-armer : couper/remettre l'alimentation.
      break;
  }
}
