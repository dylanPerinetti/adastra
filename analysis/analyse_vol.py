#!/usr/bin/env python3
"""
ADASTRA – Analyse post-vol  (Club NOVA CNAM · CSPACE 2026)

Charge le fichier CSV produit par le data-logger micro-SD,
génère des graphiques et un rapport synthétique du vol.

Utilisation :
    python analyse_vol.py                          # cherche ADASTRA.CSV dans le dossier courant
    python analyse_vol.py --file chemin/DATA.CSV   # fichier explicite
    python analyse_vol.py --save                   # sauvegarde PNG au lieu de plt.show()
"""

import argparse
import sys
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy.signal import butter, filtfilt

# ─────────────────────────── Configuration ───────────────────────────

DEFAULT_CSV = "ADASTRA.CSV"
G = 9.80665  # m/s²

# Butterworth low-pass (lissage capteurs)
FILTER_ORDER = 4
FILTER_CUTOFF_HZ = 5.0      # fréquence de coupure

# Seuils de détection d'événements
LIFTOFF_ACCEL_THRESHOLD = 2.0 * G   # norme accéléro > 2 g  → décollage
LANDING_DIST_THRESHOLD  = 30        # distance HC-SR04 < 30 cm → sol


# ─────────────────────────── Utilitaires ─────────────────────────────

def lowpass(signal: np.ndarray, fs: float, cutoff: float = FILTER_CUTOFF_HZ,
            order: int = FILTER_ORDER) -> np.ndarray:
    """Filtre Butterworth passe-bas (forward-backward pour zéro déphasage)."""
    nyq = 0.5 * fs
    b, a = butter(order, cutoff / nyq, btype="low")
    return filtfilt(b, a, signal)


def compute_velocity(accel_ms2: np.ndarray, dt: float) -> np.ndarray:
    """Intégration trapézoïdale simple de l'accélération → vitesse."""
    return np.cumsum(accel_ms2) * dt


def detect_liftoff(accel_norm: np.ndarray, threshold: float = LIFTOFF_ACCEL_THRESHOLD) -> int:
    """Renvoie l'indice du premier échantillon ≥ seuil."""
    indices = np.where(accel_norm >= threshold)[0]
    return int(indices[0]) if len(indices) > 0 else 0


def detect_landing(distance_cm: pd.Series, threshold: float = LANDING_DIST_THRESHOLD) -> int:
    """Renvoie l'indice du dernier passage sous le seuil de distance."""
    indices = np.where((distance_cm > 0) & (distance_cm < threshold))[0]
    return int(indices[-1]) if len(indices) > 0 else len(distance_cm) - 1


# ─────────────────────────── Chargement ──────────────────────────────

def load_csv(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)

    # Noms attendus du CSV produit par le data-logger V2
    required = {"time_ms", "ax", "ay", "az", "gx", "gy", "gz",
                "pressure_Pa", "altitude_m", "temp_C", "distance_cm"}
    missing = required - set(df.columns)
    if missing:
        sys.exit(f"❌ Colonnes manquantes dans le CSV : {missing}")

    # Colonnes dérivées
    df["time_s"] = df["time_ms"] / 1000.0
    df["accel_norm"] = np.sqrt(df["ax"]**2 + df["ay"]**2 + df["az"]**2)

    return df


# ─────────────────────────── Graphiques ──────────────────────────────

def plot_altitude(df: pd.DataFrame, events: dict, save: bool, out_dir: Path):
    fig, ax = plt.subplots(figsize=(12, 5))
    ax.plot(df["time_s"], df["altitude_m"], linewidth=0.8, label="Altitude brute")

    # Altitude filtrée
    fs = 1.0 / np.median(np.diff(df["time_s"]))
    alt_filt = lowpass(df["altitude_m"].values, fs)
    ax.plot(df["time_s"], alt_filt, linewidth=1.5, label="Altitude filtrée")

    # Marqueurs événements
    for name, ev in events.items():
        ax.axvline(ev["time_s"], linestyle="--", color=ev.get("color", "grey"),
                   label=f"{name} ({ev['time_s']:.2f} s)")

    ax.set_xlabel("Temps (s)")
    ax.set_ylabel("Altitude (m)")
    ax.set_title("Profil d'altitude – ADASTRA")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    _save_or_show(fig, "altitude", save, out_dir)


def plot_acceleration(df: pd.DataFrame, events: dict, save: bool, out_dir: Path):
    fig, axes = plt.subplots(2, 1, figsize=(12, 8), sharex=True)

    # Axes individuels
    for col, label in [("ax", "X"), ("ay", "Y"), ("az", "Z")]:
        axes[0].plot(df["time_s"], df[col], linewidth=0.6, label=f"a{label}")
    axes[0].set_ylabel("Accélération (m/s²)")
    axes[0].set_title("Accélérations XYZ – ADASTRA")
    axes[0].legend()
    axes[0].grid(True, alpha=0.3)

    # Norme
    axes[1].plot(df["time_s"], df["accel_norm"], linewidth=0.6, color="tab:red",
                 label="‖a‖")
    axes[1].axhline(G, color="grey", linestyle=":", label=f"1 g ({G:.2f} m/s²)")
    for name, ev in events.items():
        axes[1].axvline(ev["time_s"], linestyle="--", color=ev.get("color", "grey"),
                        label=f"{name}")
    axes[1].set_xlabel("Temps (s)")
    axes[1].set_ylabel("Norme accélération (m/s²)")
    axes[1].legend()
    axes[1].grid(True, alpha=0.3)

    fig.tight_layout()
    _save_or_show(fig, "acceleration", save, out_dir)


def plot_gyroscope(df: pd.DataFrame, save: bool, out_dir: Path):
    fig, ax = plt.subplots(figsize=(12, 5))
    for col, label in [("gx", "X"), ("gy", "Y"), ("gz", "Z")]:
        ax.plot(df["time_s"], df[col], linewidth=0.6, label=f"ω{label}")
    ax.set_xlabel("Temps (s)")
    ax.set_ylabel("Vitesse angulaire (rad/s)")
    ax.set_title("Gyroscope – ADASTRA")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    _save_or_show(fig, "gyroscope", save, out_dir)


def plot_pressure_temp(df: pd.DataFrame, save: bool, out_dir: Path):
    fig, ax1 = plt.subplots(figsize=(12, 5))
    color_p = "tab:blue"
    ax1.plot(df["time_s"], df["pressure_Pa"] / 100.0, color=color_p, linewidth=0.8)
    ax1.set_xlabel("Temps (s)")
    ax1.set_ylabel("Pression (hPa)", color=color_p)
    ax1.tick_params(axis="y", labelcolor=color_p)
    ax1.grid(True, alpha=0.3)

    ax2 = ax1.twinx()
    color_t = "tab:orange"
    ax2.plot(df["time_s"], df["temp_C"], color=color_t, linewidth=0.8)
    ax2.set_ylabel("Température (°C)", color=color_t)
    ax2.tick_params(axis="y", labelcolor=color_t)

    fig.suptitle("Pression & Température – ADASTRA")
    fig.tight_layout()
    _save_or_show(fig, "pression_temp", save, out_dir)


def plot_distance(df: pd.DataFrame, save: bool, out_dir: Path):
    fig, ax = plt.subplots(figsize=(12, 4))
    ax.plot(df["time_s"], df["distance_cm"], linewidth=0.7, color="tab:green")
    ax.set_xlabel("Temps (s)")
    ax.set_ylabel("Distance sol (cm)")
    ax.set_title("Capteur ultrason HC-SR04 – ADASTRA")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    _save_or_show(fig, "distance_sol", save, out_dir)


def plot_velocity(df: pd.DataFrame, events: dict, save: bool, out_dir: Path):
    """Vitesse verticale estimée par intégration de az - g."""
    dt = np.median(np.diff(df["time_s"].values))
    fs = 1.0 / dt
    az_filt = lowpass(df["az"].values, fs)
    vz = compute_velocity(az_filt - G, dt)

    fig, ax = plt.subplots(figsize=(12, 5))
    ax.plot(df["time_s"], vz, linewidth=0.8, color="tab:purple")
    for name, ev in events.items():
        ax.axvline(ev["time_s"], linestyle="--", color=ev.get("color", "grey"),
                   label=f"{name}")
    ax.axhline(0, color="grey", linestyle=":")
    ax.set_xlabel("Temps (s)")
    ax.set_ylabel("Vitesse verticale estimée (m/s)")
    ax.set_title("Vitesse verticale (intégration az) – ADASTRA")
    ax.legend()
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    _save_or_show(fig, "vitesse_verticale", save, out_dir)


def _save_or_show(fig, name: str, save: bool, out_dir: Path):
    if save:
        out_dir.mkdir(parents=True, exist_ok=True)
        path = out_dir / f"{name}.png"
        fig.savefig(path, dpi=150)
        print(f"  📄 {path}")
        plt.close(fig)
    else:
        plt.show()


# ─────────────────────────── Rapport texte ───────────────────────────

def print_report(df: pd.DataFrame, events: dict):
    duration = df["time_s"].iloc[-1] - df["time_s"].iloc[0]
    fs = len(df) / duration if duration > 0 else 0

    apogee_idx = df["altitude_m"].idxmax()
    apogee_alt = df.loc[apogee_idx, "altitude_m"]
    apogee_t   = df.loc[apogee_idx, "time_s"]

    max_accel = df["accel_norm"].max()
    max_accel_g = max_accel / G

    print()
    print("=" * 52)
    print("   RAPPORT DE VOL – ADASTRA · Club NOVA CNAM")
    print("=" * 52)
    print(f"  Durée totale enregistrement : {duration:.2f} s")
    print(f"  Nombre d'échantillons       : {len(df)}")
    print(f"  Fréquence moyenne           : {fs:.1f} Hz")
    print()
    print(f"  Apogée                      : {apogee_alt:.2f} m")
    print(f"  Temps apogée                : {apogee_t:.2f} s")
    print(f"  Accélération max            : {max_accel:.2f} m/s² ({max_accel_g:.2f} g)")
    print(f"  Pression min                : {df['pressure_Pa'].min() / 100:.2f} hPa")
    print(f"  Pression max                : {df['pressure_Pa'].max() / 100:.2f} hPa")
    print(f"  Température min / max       : {df['temp_C'].min():.1f} / {df['temp_C'].max():.1f} °C")
    print()

    for name, ev in events.items():
        print(f"  [{name:^12}]  t = {ev['time_s']:.2f} s")

    print("=" * 52)
    print()


# ─────────────────────────── Main ────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Analyse post-vol ADASTRA")
    parser.add_argument("--file", "-f", default=DEFAULT_CSV,
                        help="Chemin vers le fichier CSV (défaut : ADASTRA.CSV)")
    parser.add_argument("--save", "-s", action="store_true",
                        help="Sauvegarder les graphiques en PNG au lieu de les afficher")
    parser.add_argument("--outdir", "-o", default="plots",
                        help="Dossier de sortie pour les PNG (défaut : plots/)")
    args = parser.parse_args()

    csv_path = Path(args.file)
    if not csv_path.exists():
        sys.exit(f"❌ Fichier introuvable : {csv_path}")

    out_dir = Path(args.outdir)

    # ── Chargement ──
    print(f"📂 Chargement de {csv_path} …")
    df = load_csv(str(csv_path))
    print(f"   {len(df)} lignes chargées.")

    # ── Détection d'événements ──
    events = {}

    liftoff_idx = detect_liftoff(df["accel_norm"].values)
    events["Décollage"] = {"time_s": df.loc[liftoff_idx, "time_s"], "color": "green"}

    apogee_idx = df["altitude_m"].idxmax()
    events["Apogée"] = {"time_s": df.loc[apogee_idx, "time_s"], "color": "red"}

    landing_idx = detect_landing(df["distance_cm"])
    events["Atterrissage"] = {"time_s": df.loc[landing_idx, "time_s"], "color": "orange"}

    # ── Rapport ──
    print_report(df, events)

    # ── Graphiques ──
    plot_altitude(df, events, args.save, out_dir)
    plot_acceleration(df, events, args.save, out_dir)
    plot_gyroscope(df, args.save, out_dir)
    plot_pressure_temp(df, args.save, out_dir)
    plot_distance(df, args.save, out_dir)
    plot_velocity(df, events, args.save, out_dir)

    if args.save:
        print(f"\n✅ Tous les graphiques sauvegardés dans {out_dir}/")


if __name__ == "__main__":
    main()
