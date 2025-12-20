import pandas as pd
import matplotlib.pyplot as plt

# Charger le fichier CSV
df = pd.read_csv("vol_adastra.csv")

# Temps en secondes
df["time_s"] = df["time_ms"] / 1000.0

# ===================== GRAPHIQUES =====================

plt.figure()
plt.plot(df["time_s"], df["altitude_m"])
plt.xlabel("Temps (s)")
plt.ylabel("Altitude (m)")
plt.title("Profil d'altitude - ADASTRA")
plt.grid()
plt.show()

plt.figure()
plt.plot(df["time_s"], df["az"])
plt.xlabel("Temps (s)")
plt.ylabel("Accélération Z (m/s²)")
plt.title("Accélération verticale")
plt.grid()
plt.show()

plt.figure()
plt.plot(df["time_s"], df["gx"], label="gx")
plt.plot(df["time_s"], df["gy"], label="gy")
plt.plot(df["time_s"], df["gz"], label="gz")
plt.xlabel("Temps (s)")
plt.ylabel("Vitesse angulaire (rad/s)")
plt.title("Gyroscope")
plt.legend()
plt.grid()
plt.show()

# ===================== ÉVÉNEMENTS =====================

apogee_index = df["altitude_m"].idxmax()
apogee_time = df.loc[apogee_index, "time_s"]
apogee_alt = df.loc[apogee_index, "altitude_m"]

print("===== ANALYSE VOL ADASTRA =====")
print(f"Apogée : {apogee_alt:.2f} m")
print(f"Temps apogée : {apogee_time:.2f} s")
