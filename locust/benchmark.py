#!/usr/bin/env python3
"""
Automated StoRM Tape load testing tool with Locust.
Author: [Il tuo nome]
Date: 2025-10-14

Funzionalità:
- esegue serie di test Locust con utenti crescenti
- raccoglie CSV e log di Locust
- misura la CPU usage media del processo 'storm-tape'
- genera grafici PNG (latency, RPS, CPU)
- produce un file Markdown con i risultati

Dipendenze:
    pip install matplotlib pandas psutil
"""

import os
import time
import csv
import subprocess
import psutil
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
from threading import Thread

# === CONFIGURAZIONE ===
HOST = "http://localhost:8080"
LOCUSTFILE = "client.py"
USERS_LIST = [10, 20, 30]
SPAWN_RATE = 2
DURATION = "10s"

TIMESTAMP = datetime.now().strftime("%Y%m%d_%H%M")
OUTDIR = f"results/{TIMESTAMP}"
SUMMARY_MD = os.path.join(OUTDIR, "summary.md")

os.makedirs(OUTDIR, exist_ok=True)


# === FUNZIONE: monitora CPU del processo storm-tape ===
def monitor_cpu(stop_event, cpu_log_file):
    with open(cpu_log_file, "w") as f:
        f.write("timestamp,cpu_percent,open_fds,connections\n")
        while not stop_event.is_set():
            cpu_percent = 0.0
            open_fds = 0
            conns = 0
            for proc in psutil.process_iter(["pid", "name", "cpu_percent"]):
                if "storm-tape" in proc.info["name"]:
                    cpu_percent += proc.info["cpu_percent"]
                    try:
                        open_fds += proc.num_fds()
                        conns += len(proc.net_connections())
                    except Exception:
                        pass
            f.write(f"{time.time()},{cpu_percent},{open_fds},{conns}\n")
            f.flush()
            time.sleep(1)


# === FUNZIONE: esegui test con Locust ===
def run_locust_test(users):
    prefix = f"{OUTDIR}/test_{users}users"
    cpu_log = f"{prefix}_cpu.csv"

    print(f"\n🚀 Avvio test con {users} utenti...")
    stop_event = psutil._common.threading.Event()
    monitor_thread = Thread(target=monitor_cpu, args=(stop_event, cpu_log))
    monitor_thread.start()

    try:
        result = subprocess.run(
            [
                "locust",
                "-f", LOCUSTFILE,
                "--host", HOST,
                "--users", str(users),
                "--spawn-rate", str(SPAWN_RATE),
                "--run-time", DURATION,
                "--headless",
                "--csv", prefix,
                "--only-summary",
            ],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            print(f"⚠️ Locust exited with code {result.returncode} for {users} users.")
            print("   Possible overload or timeout — continuing test sequence.")
            with open(f"{prefix}.log", "w") as f:
                f.write(result.stdout + "\n" + result.stderr)
        else:
            print(f"✅ Test {users} utenti completato.")
    except Exception as e:
        print(f"❌ Errore durante test {users} utenti: {e}")
    finally:
        stop_event.set()
        monitor_thread.join()

    print(f"✅ Test {users} utenti completato.")
    return prefix, cpu_log


# === FUNZIONE: estrai metriche principali da CSV Locust ===
def extract_metrics(stats_csv):
    df = pd.read_csv(stats_csv)
    if df.empty:
        return None

    # Mappatura nomi possibili delle colonne (Locust cambia spesso naming)
    col_avg = next((c for c in df.columns if "Average" in c and "Response" in c), None)
    col_p95 = next((c for c in df.columns if "95" in c), None)
    col_rps = next((c for c in df.columns if "Requests/s" in c or "Requests per Second" in c), None)
    col_fail = next((c for c in df.columns if "Failure" in c), None)
    col_count = next((c for c in df.columns if "Request Count" in c), None)

    if not all([col_avg, col_p95, col_rps]):
        print(f"⚠️  Impossibile trovare alcune colonne nel CSV {stats_csv}")
        print("   Colonne trovate:", df.columns.tolist())
        return None

    avg_response = df[col_avg].mean()
    p95 = df[col_p95].mean()
    rps = df[col_rps].mean()

    # Calcola percentuale errori se i campi ci sono
    if col_fail and col_count and df[col_count].sum() > 0:
        errors = df[col_fail].sum() / df[col_count].sum() * 100
    else:
        errors = 0.0

    return avg_response, p95, rps, errors



# === FUNZIONE: CPU media durante test ===
def average_cpu(cpu_log_file):
    df = pd.read_csv(cpu_log_file)
    return df["cpu_percent"].mean() if not df.empty else 0


# === MAIN LOOP ===
results = []

for users in USERS_LIST:
    prefix, cpu_log = run_locust_test(users)
    stats_file = f"{prefix}_stats.csv"

    if os.path.exists(stats_file):
        metrics = extract_metrics(stats_file)
        if metrics:
            avg, p95, rps, err = metrics
        else:
            avg, p95, rps, err = (None, None, None, None)
    else:
        avg, p95, rps, err = (None, None, None, None)

    cpu = average_cpu(cpu_log)
    results.append({
        "users": users,
        "avg_ms": avg,
        "p95_ms": p95,
        "rps": rps,
        "errors": err,
        "cpu": cpu
    })

# === SALVA SUMMARY .MD ===
with open(SUMMARY_MD, "w") as f:
    f.write(f"# StoRM Tape Load Test Summary\n\n")
    f.write(f"**Host:** {HOST}\n\n")
    f.write(f"**Durata test singolo:** {DURATION}\n\n")
    f.write(f"**Data esecuzione:** {datetime.now()}\n\n")
    f.write("| Utenti | Avg (ms) | P95 (ms) | RPS | Errori (%) | CPU (%) |\n")
    f.write("|--------|-----------|----------|------|-------------|---------|\n")
    for r in results:
        f.write(f"| {r['users']} | {r['avg_ms']:.1f} | {r['p95_ms']:.1f} | {r['rps']:.1f} | {r['errors']:.2f} | {r['cpu']:.1f} |\n")

print(f"\n📊 Risultati salvati in {SUMMARY_MD}")

# === GENERA GRAFICI ===
df = pd.DataFrame(results)

def make_plot(x, y, ylabel, title, filename):
    plt.figure()
    plt.plot(df[x], df[y], marker="o", linewidth=2)
    plt.xlabel("Numero utenti")
    plt.ylabel(ylabel)
    plt.title(title)
    plt.grid(True)
    plt.savefig(os.path.join(OUTDIR, filename))
    plt.close()

make_plot("users", "avg_ms", "Tempo medio (ms)", "Tempo medio vs Utenti", "avg_response.png")
make_plot("users", "p95_ms", "Tempo P95 (ms)", "Tempo P95 vs Utenti", "p95_response.png")
make_plot("users", "rps", "Richieste/s", "Throughput vs Utenti", "rps.png")
make_plot("users", "cpu", "CPU usage (%)", "Uso CPU vs Utenti", "cpu_usage.png")

print("📈 Grafici generati in:", OUTDIR)
