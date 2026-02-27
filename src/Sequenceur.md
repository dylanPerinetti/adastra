# Sequenceur.md  - Séquenceur de récupération (Projet ADASTRA)

## Objectif

Mettre en place un **système de récupération** avec **ouverture de trappe** commandée électroniquement, afin de permettre le déploiement (parachute / streamer, etc.) après le décollage.

Le principe retenu est un **verrouillage mécanique** libéré par un **solénoïde**, puis une **ouverture forcée** par **ressort**.

---

## Choix technique retenu : solénoïde + ressort

### Pourquoi ce choix

* Un **solénoïde** permet une action simple : **une impulsion électrique → libération du verrou / goupille**.
* Le **ressort** garantit l’ouverture de la trappe même si elle est légèrement collée / serrée (frottements, pression, alignement).

### Architecture mécanique (concept)

* La **trappe** est maintenue fermée par un **verrou** (ex. goupille, crochet, linguet).
* Le **ressort** est **compressé** entre :

  * la **trappe**, et
  * le **corps de la fusée** (ou une butée interne).

* Au déclenchement, le solénoïde **retire la goupille** (ou actionne le verrou) et le ressort **pousse** la trappe vers l’extérieur.

> ⚠️ Le ressort est \*\*à dimensionner\*\* selon les efforts (frottements, rigidité de la trappe, pression aérodynamique, tolérances mécaniques).  
> L’objectif est d’avoir une ouverture \*\*franche\*\* et \*\*répétable\*\*.

---

## Fonctionnement du séquenceur (logique)

1. **Préparation sur rampe** : un **jack** est inséré (ou contact fermé) → l’Arduino est en mode “attente”.
2. **Décollage** : le jack est arraché (ou contact ouvert) → l’Arduino détecte le départ.
3. **Temporisation** : attente de **X secondes** (paramétrable).
4. **Déclenchement** : impulsion sur le solénoïde via MOSFET → libération du verrou.
5. **Ouverture** : le ressort ouvre la trappe.

---

## Électronique de commande

### Microcontrôleur

* **Arduino Nano**
* Pilotage du solénoïde via une sortie digitale.

### Commande de puissance

* **MOSFET N “logic-level”** (commande 5 V fiable)

  * Exemple de référence (type) : **IRLZ44N** (55 V, largement utilisé) ou équivalent *logic-level*.

* **Diode de roue libre** en parallèle sur la bobine du solénoïde (indispensable)

  * Exemple : **1N5408** (ou autre diode de puissance adaptée).

### Câblage (principe)

* Le solénoïde est alimenté par son pack batterie.
* Le MOSFET est monté en **low-side** (côté masse) :

  * +Batt → Solénoïde → Drain MOSFET
  * Source MOSFET → GND batt
  * Gate MOSFET → sortie Arduino (avec résistance série ~100 Ω)
  * Pull-down gate ~100 kΩ vers GND

* **Masse commune** : le GND Arduino doit être relié au GND du pack solénoïde (référence commune pour la commande).

---

## Alimentation

### Arduino Nano

* Alimentée par une **pile 9 V** (selon choix actuel).

> ⚠️ Note pratique : les piles 9 V “classiques” peuvent avoir une \*\*capacité / courant limité\*\*.  
> À valider par essais : stabilité 5 V de l’Arduino, vibrations, température, durée d’armement.

### Solénoïde

* Alimentation dédiée : **2× piles 12 V en série** (≈ 24 V) pour correspondre à un solénoïde annoncé “24 V” (à valider selon le modèle exact).

> ⚠️ Important : les piles 12 V de type A23 (souvent utilisées pour télécommandes) ont une \*\*capacité et un courant disponibles limités\*\*.  
> Il faut \*\*mesurer le courant\*\* du solénoïde et vérifier que ces piles peuvent fournir l’impulsion sans s’effondrer en tension.  
> Si besoin, envisager une batterie plus adaptée (pack Li‑ion/LiPo + régulation), mais ceci reste à définir après tests.

---

## Matériel (exemples / liens fournis)

### Piles 12 V (x2)

* Lien : https://www.amazon.fr/Energizer-piles-alcalines-LRV08-GP23A/dp/B002YOWPHG/ref=asc\_df\_B002YOWPHG?tag=bingshoppin0f-21\&linkCode=df0\&hvadid=80883030550846\&hvnetw=s\&hvqmt=be\&hvbmt=be\&hvdev=c\&hvlocint=\&hvlocphy=126946\&hvtargid=pla-4584482496076890\&psc=1

### Solénoïde (24 V)

* Lien : https://fr.aliexpress.com/item/32788238864.html?UTABTest=aliabtest128998\_32161\&src=bing\&albch=shopping\&acnt=135105396\&albcp=555597222\&albag=1301822999823398\&slnk=\&trgt=pla-2333163674585286\&plac=\&crea=81363998112388\&netw=o\&device=c\&mtctp=e\&utm\_source=Bing\&utm\_medium=shopping\&utm\_campaign=PA\_Bing\_FR\_PMAX\_hardware\_MAXV\_AESupply\_25.10.15\&utm\_content=HARDWARE\&utm\_term=sol%C3%A9no%C3%AFde%20aiment%2024v

### MOSFET (type)

* MOSFET N **logic-level** (ex. **IRLZ44N** ou équivalent)
* 

  * résistances : 100 Ω (gate série), 100 kΩ (pull-down)

* 

  * **diode de roue libre** (ex. 1N5408 ou équivalent)

### Mécanique

* Ressort de compression (à dimensionner selon effort)
* Verrou/goupille + guidage (éviter les coincements)
* Trappe + charnière / axe (selon conception)
* Jack de rampe (contact d’armement / détection décollage)

---

## Fichier de code du séquenceur

* Nom de fichier (Arduino IDE) : **`/dylanPerinetti/adastra/Sequenceur_Nano.ino`**

> Le code gère : détection du jack, temporisation X secondes, impulsion sur la sortie MOSFET, sécurité au démarrage.

---

## Tests \& sécurité (recommandé)

* Faire **au moins 20–50 tests au sol** (ouverture répétée) avant vol.
* Ajouter un **interrupteur d’armement** physique (sécurité) : “SAFE / ARM”.
* Protéger le circuit (fusible léger si pertinent), sécuriser les câbles (vibrations).

---

**Signature :** DylanPerineti  - *NovaCnam*  - Projet **ADASTRA**

