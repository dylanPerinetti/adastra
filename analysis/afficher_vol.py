#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
======================================================================
 ADASTRA V2 - Analyse et visualisation des données de vol
 Club NOVA CNAM
----------------------------------------------------------------------
 Lit un fichier CSV produit par le data logger (VOLxx.CSV), affiche un
 résumé du vol (apogée, accélération max, vitesse max, durée...) et
 trace les courbes principales.

 Utilisation :
     python afficher_vol.py                 # lit VOL00_test.csv
     python afficher_vol.py VOL00.CSV       # lit le fichier indiqué

 Dépendances : pandas, numpy, matplotlib
     pip install pandas numpy matplotlib
======================================================================
"""

import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

G = 9.81  # m/s²

# Colonnes attendues, dans l'ordre du firmware
COLONNES = [
    "time_ms", "ax", "ay", "az", "gx", "gy", "gz",
    "pressure_Pa", "altitude_m", "temp_C", "sound_speed_ms",
    "mag_x_uT", "mag_y_uT", "mag_z_uT",
]


def charger(fichier):
    """Charge le CSV, nettoie les lignes corrompues et prépare les données."""
    # on_bad_lines='skip' : ignore une éventuelle dernière ligne tronquée
    # (cas d'une coupure d'alim pendant l'écriture)
    df = pd.read_csv(fichier, on_bad_lines="skip")

    # Garde uniquement les colonnes connues si le fichier en a d'autres
    manquantes = [c for c in COLONNES if c not in df.columns]
    if manquantes:
        raise ValueError("Colonnes manquantes dans le CSV : %s" % manquantes)

    # Force le type numérique et jette les lignes non convertibles
    df = df.apply(pd.to_numeric, errors="coerce").dropna().reset_index(drop=True)

    # Temps relatif en secondes (le logger démarre rarement à 0)
    df["t"] = (df["time_ms"] - df["time_ms"].iloc[0]) / 1000.0

    # Norme de l'accélération (indépendante de l'orientation de la carte)
    df["a_norm"] = np.sqrt(df.ax**2 + df.ay**2 + df.az**2)

    # Estimation de la vitesse verticale par dérivée de l'altitude baro,
    # lissée pour atténuer le bruit du capteur.
    v = np.gradient(df["altitude_m"].to_numpy(), df["t"].to_numpy())
    df["v_est"] = pd.Series(v).rolling(15, center=True, min_periods=1).mean()

    return df


def detecter_evenements(df):
    """Repère le décollage, l'apogée et l'atterrissage."""
    ev = {}

    # Décollage : première fois que |a| dépasse ~2 g de façon franche
    seuil = 2.0 * G
    au_dessus = df.index[df["a_norm"] > seuil]
    ev["liftoff"] = int(au_dessus[0]) if len(au_dessus) else None

    # Apogée : altitude maximale
    ev["apogee"] = int(df["altitude_m"].idxmax())

    # Atterrissage : après l'apogée, retour durable près du sol
    apres = df.iloc[ev["apogee"]:]
    bas = apres.index[apres["altitude_m"] < 2.0]
    ev["landing"] = int(bas[0]) if len(bas) else None

    return ev


def resume(df, ev):
    """Affiche un résumé texte du vol."""
    def t_de(i):
        return df["t"].iloc[i] if i is not None else float("nan")

    apo_i = ev["apogee"]
    echos = df[df["sound_speed_ms"] > 0]["sound_speed_ms"]

    print("=" * 56)
    print("  RÉSUMÉ DU VOL")
    print("=" * 56)
    print("  Durée d'enregistrement : %6.1f s   (%d échantillons)"
          % (df["t"].iloc[-1], len(df)))
    fe = len(df) / df["t"].iloc[-1] if df["t"].iloc[-1] > 0 else 0
    print("  Fréquence moyenne      : %6.1f Hz" % fe)
    if ev["liftoff"] is not None:
        print("  Décollage détecté à    : %6.2f s" % t_de(ev["liftoff"]))
    print("  Apogée                 : %6.1f m  à %.2f s"
          % (df["altitude_m"].iloc[apo_i], t_de(apo_i)))
    if ev["landing"] is not None:
        print("  Atterrissage           : %6.2f s" % t_de(ev["landing"]))
    print("  Accélération max       : %6.1f m/s²  (%.1f g)"
          % (df["a_norm"].max(), df["a_norm"].max() / G))
    print("  Vitesse verticale max  : %6.1f m/s  (estimée)" % df["v_est"].max())
    print("  Vitesse de descente    : %6.1f m/s  (estimée)" % df["v_est"].min())
    print("  Température min / max   : %5.1f / %5.1f °C"
          % (df["temp_C"].min(), df["temp_C"].max()))
    if len(echos):
        print("  Vitesse du son (sol)   : %6.1f m/s  (%d échos valides)"
              % (echos.mean(), len(echos)))
    else:
        print("  Vitesse du son         : aucun écho HC-SR04 valide")
    print("=" * 56)


def tracer(df, ev, fichier):
    """Trace les 6 graphes principaux."""
    t = df["t"]
    t_lo = df["t"].iloc[ev["liftoff"]] if ev["liftoff"] is not None else None
    t_apo = df["t"].iloc[ev["apogee"]]
    h_apo = df["altitude_m"].iloc[ev["apogee"]]

    fig, axes = plt.subplots(3, 2, figsize=(13, 9), sharex=True)
    fig.suptitle("ADASTRA V2 - Vol : %s   (apogée %.0f m)" % (fichier, h_apo),
                 fontsize=14, fontweight="bold")

    def marquer(ax):
        if t_lo is not None:
            ax.axvline(t_lo, color="green", ls="--", lw=1, alpha=0.7)
        ax.axvline(t_apo, color="red", ls="--", lw=1, alpha=0.7)
        ax.grid(True, alpha=0.3)

    # 1) Altitude
    ax = axes[0, 0]
    ax.plot(t, df["altitude_m"], color="#1f77b4")
    ax.scatter([t_apo], [h_apo], color="red", zorder=5)
    ax.annotate("apogée %.0f m" % h_apo, (t_apo, h_apo),
                textcoords="offset points", xytext=(8, -4), color="red")
    if t_lo is not None:
        ax.annotate("décollage", (t_lo, 0), textcoords="offset points",
                    xytext=(5, 10), color="green")
    ax.set_ylabel("Altitude (m)")
    ax.set_title("Altitude")
    marquer(ax)

    # 2) Accélération
    ax = axes[0, 1]
    ax.plot(t, df["a_norm"], color="#d62728", label="|a|")
    ax.plot(t, df["az"], color="#7f7f7f", lw=0.8, alpha=0.8, label="az")
    ax.axhline(G, color="black", ls=":", lw=0.8, alpha=0.5)
    ax.set_ylabel("Accélération (m/s²)")
    ax.set_title("Accélération")
    ax.legend(loc="upper right", fontsize=8)
    marquer(ax)

    # 3) Vitesse verticale estimée
    ax = axes[1, 0]
    ax.plot(t, df["v_est"], color="#2ca02c")
    ax.axhline(0, color="black", lw=0.8, alpha=0.5)
    ax.set_ylabel("Vitesse (m/s)")
    ax.set_title("Vitesse verticale (estimée par dérivée de l'altitude)")
    marquer(ax)

    # 4) Vitesses angulaires (gyro)
    ax = axes[1, 1]
    ax.plot(t, df["gx"], lw=0.8, label="gx")
    ax.plot(t, df["gy"], lw=0.8, label="gy")
    ax.plot(t, df["gz"], lw=0.8, label="gz")
    ax.set_ylabel("Rotation (rad/s)")
    ax.set_title("Gyroscope")
    ax.legend(loc="upper right", fontsize=8)
    marquer(ax)

    # 5) Pression + Température
    ax = axes[2, 0]
    ax.plot(t, df["pressure_Pa"] / 100.0, color="#9467bd")
    ax.set_ylabel("Pression (hPa)", color="#9467bd")
    ax.tick_params(axis="y", labelcolor="#9467bd")
    ax2 = ax.twinx()
    ax2.plot(t, df["temp_C"], color="#ff7f0e", lw=0.9)
    ax2.set_ylabel("Température (°C)", color="#ff7f0e")
    ax2.tick_params(axis="y", labelcolor="#ff7f0e")
    ax.set_title("Pression & Température")
    ax.set_xlabel("Temps (s)")
    marquer(ax)

    # 6) Magnétomètre
    ax = axes[2, 1]
    ax.plot(t, df["mag_x_uT"], lw=0.8, label="mx")
    ax.plot(t, df["mag_y_uT"], lw=0.8, label="my")
    ax.plot(t, df["mag_z_uT"], lw=0.8, label="mz")
    ax.set_ylabel("Champ magnétique (µT)")
    ax.set_title("Magnétomètre")
    ax.set_xlabel("Temps (s)")
    ax.legend(loc="upper right", fontsize=8)
    marquer(ax)

    fig.tight_layout(rect=[0, 0, 1, 0.97])

    sortie = fichier.rsplit(".", 1)[0] + "_analyse.png"
    fig.savefig(sortie, dpi=130)
    print("Figure enregistrée : %s" % sortie)
    plt.show()


def main():
    fichier = sys.argv[1] if len(sys.argv) > 1 else "VOL00_test.csv"
    try:
        df = charger(fichier)
    except FileNotFoundError:
        print("Fichier introuvable : %s" % fichier)
        sys.exit(1)

    if len(df) < 10:
        print("Trop peu de données exploitables dans %s" % fichier)
        sys.exit(1)

    ev = detecter_evenements(df)
    resume(df, ev)
    tracer(df, ev, fichier)


if __name__ == "__main__":
    main()
