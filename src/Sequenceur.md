# Sequenceur de recuperation - Projet ADASTRA

## Objectif

Systeme de **recuperation par ouverture de trappe** commandee electroniquement, permettant le deploiement du parachute / streamer apres le decollage.

Principe : **verrouillage mecanique** libere par un **solenoide**, puis **ouverture forcee** par **ressort de compression**.

---

## Architecture mecanique

```
     Corps fusee
    ┌───────────────────┐
    │  ┌─────────────┐  │
    │  │   Ressort    │  │  <- comprime entre trappe et butee
    │  │   ~~~~~~     │  │
    │  └──────┬──────┘  │
    │         │         │
    │  ┌──────▼──────┐  │
    │  │   Trappe    │◄─┼── verrou / goupille
    │  └─────────────┘  │         │
    └───────────────────┘         │
                           ┌──────▼──────┐
                           │  Solenoide  │  <- retire la goupille
                           └─────────────┘
```

- La **trappe** est maintenue fermee par un **verrou** (goupille, crochet ou linguet).
- Le **ressort** est comprime entre la trappe et une butee interne.
- Au declenchement, le solenoide **retire la goupille** et le ressort **pousse** la trappe vers l'exterieur.

> Le ressort est a dimensionner selon : frottements, rigidite de la trappe, pression aerodynamique, tolerances mecaniques.
> L'objectif : une ouverture **franche** et **repetable**.

---

## Sequence de fonctionnement

```
  [Sur rampe]          [Decollage]          [Tempo]           [Ouverture]
       │                    │                  │                   │
  Jack insere          Jack arrache       X secondes         Solenoide ON
  Arduino en           Arduino detecte    (parametrable)     Verrou libere
  mode attente         le depart          compte a rebours   Ressort pousse
       │                    │                  │                   │
   WAIT_ARM  ──────▶  WAIT_LAUNCH ──────▶  WAIT_DELAY ──────▶  FIRE ──▶ DONE
```

1. **Preparation sur rampe** : jack insere (D2 = LOW) -> Arduino en mode "attente"
2. **Decollage** : jack arrache (D2 = HIGH) -> detection du depart
3. **Temporisation** : attente de **X secondes** (configurable, defaut 4 s)
4. **Declenchement** : impulsion de 250 ms sur le solenoide via MOSFET
5. **Ouverture** : le ressort ouvre la trappe, etat DONE

---

## Electronique de commande

### Microcontroleur

- **Arduino Nano** (dedie au sequenceur, independant du data logger)

### Schema de commande (MOSFET low-side)

```
  +Batt (24 V) ─────┐
                     │
              ┌──────▼──────┐
              │  Solenoide  │
              └──────┬──────┘
                     │
              ┌──────┤ Diode roue libre (1N5408)
              │      │ (cathode vers +Batt)
              │      │
              │  Drain MOSFET (IRLZ44N)
              │      │
              │  Source ──── GND batt
              │
              Gate ──── 100 Ohm ──── D9 Arduino
              │
              100 kOhm (pull-down vers GND)
```

### Cablage Arduino

| Pin Arduino Nano | Fonction |
|---|---|
| D2 | Entree jack de rampe (INPUT_PULLUP) |
| D7 | LED "ARME" (optionnelle) |
| D8 | LED "TEMPO" (optionnelle) |
| D9 | Sortie MOSFET gate (commande solenoide) |
| D10 | LED "DONE" (optionnelle) |
| LED_BUILTIN (D13) | LED statut / FIRE |

> **Masse commune obligatoire** : le GND Arduino doit etre relie au GND du pack solenoide.

### LEDs d'etat

| LED | Etat | Comportement |
|---|---|---|
| LED integree (D13) | WAIT_ARM | Clignotement lent (800 ms) |
| LED ARME (D7) | WAIT_LAUNCH | Clignotement moyen (600 ms) |
| LED TEMPO (D8) | WAIT_DELAY | Clignotement rapide (250 ms) |
| LED integree (D13) | FIRE | Fixe ON pendant impulsion |
| LED DONE (D10) | DONE | Fixe ON |

---

## Alimentation

### Arduino Nano

- **Pile 9 V** (via Vin ou jack barrel)

> Note : les piles 9 V classiques ont une capacite limitee. A valider par essais (stabilite 5 V, vibrations, temperature, duree d'armement sur rampe).

### Solenoide

- **2x piles 12 V en serie** (~24 V) pour un solenoide annonce 24 V.

> Les piles 12 V type A23 (telecommandes) ont un courant disponible limite.
> Mesurer le courant du solenoide et verifier que les piles peuvent fournir l'impulsion sans s'effondrer en tension.
> Alternative si necessaire : pack Li-ion / LiPo + regulation.

---

## Parametres logiciels

Definis dans `Sequenceur_Nano.ino`, facilement modifiables :

| Parametre | Valeur par defaut | Description |
|---|---|---|
| `DELAY_MS` | 4000 ms | Temporisation post-decollage |
| `PULSE_MS` | 250 ms | Duree impulsion solenoide |
| `DEBOUNCE_MS` | 150 ms | Anti-rebond lecture jack |
| `USE_EXTERNAL_LEDS` | 1 | Activer LEDs externes (0 = LED integree seule) |

---

## Materiel et liens

### Solenoide 24 V

- Type : solenoide poussoir / tirette, course ~10 mm
- Lien : https://amzn.eu/d/0choRAVG

### MOSFET N logic-level

- Reference : **IRLZ44N** (Vgs(th) < 5 V, 55 V, 47 A)
- Lien : https://amzn.eu/d/5bYtHvN

### Diode de roue libre

- Reference : **1N5408** (3 A, 1000 V) ou equivalent
- Lien : https://amzn.eu/d/gT8MMEM

### Piles 12 V (x2 en serie)

- Type : A23 / GP23A / LRV08 (12 V alcaline)
- Lien : https://amzn.eu/d/5VWfVjR

### LEDs

- Kit LEDs 5 mm assorties (rouge, vert, jaune)
- Lien : https://amzn.eu/d/dN9rPSL

### Resistances

- 100 Ohm (gate serie MOSFET) + 100 kOhm (pull-down gate)
- Kit resistances : https://amzn.eu/d/2kxkV0U

### Mecanique

- Ressort de compression (a dimensionner selon effort requis)
- Verrou / goupille + guidage (eviter les coincements)
- Trappe + charniere / axe (selon conception fusee)
- Jack de rampe (contact d'armement / detection decollage)

---

## Fichier source

- **`src/Sequenceur_Nano.ino`**

Le code gere :
- Detection d'insertion / arrachement du jack (anti-rebond)
- Machine a etats (WAIT_ARM -> WAIT_LAUNCH -> WAIT_DELAY -> FIRE -> DONE)
- Impulsion solenoide via MOSFET
- Clignotement LEDs non-bloquant selon l'etat
- Securite : MOSFET OFF au demarrage, declenchement unique (re-armer = couper l'alim)

---

## Tests et securite

- Faire **minimum 20-50 tests au sol** (ouverture repetee) avant vol
- Ajouter un **interrupteur d'armement** physique (SAFE / ARM) en serie sur l'alimentation
- Proteger le circuit et securiser les cables (vibrations en vol)
- Verifier la tenue mecanique du verrou sous acceleration (phase propulsee)
- Chronometrer la temporisation reelle vs parametree

---

**Signature :** Dylan Perinetti - *NovaCnam* - Projet **ADASTRA**

