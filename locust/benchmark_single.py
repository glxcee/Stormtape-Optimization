#!/usr/bin/env python3
import os
import time
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime
import sys

# === CONFIGURAZIONE TEST ===
HOST = "http://localhost:8080"#"https://storm-tape.cr.cnaf.infn.it:8443"
LOCUSTFILE = "locustfile.py" 
USERS = 1
SPAWN_RATE = 1
DURATION = "10s"

# LISTA di test da eseguire: numero di file per singola richiesta
FILES_PER_REQUEST_LIST = [10, 50, 100, 200] 

TIMESTAMP = datetime.now().strftime("%Y%m%dT%H%M%S")
OUTDIR = f"results/{TIMESTAMP}_{USERS}"
SUMMARY_MD = os.path.join(OUTDIR, "summary.md")

os.makedirs(OUTDIR, exist_ok=True)

def run_benchmark_suite():
    results_data = []
    
    print(f"🚀 Avvio Suite di Benchmark: {USERS} utente per {DURATION} a test.")
    print(f"   Target: {HOST}")
    print(f"   Scenari (files/req): {FILES_PER_REQUEST_LIST}")

    # Iteriamo sui diversi carichi di file
    for n_files in FILES_PER_REQUEST_LIST:4
        print(f"\n--- Inizio Test: {n_files} file per richiesta ---")
        
        prefix = f"{OUTDIR}/test_{n_files}_files"
        
        # Comando Locust
        cmd = [
            "locust",
            "-f", LOCUSTFILE,
            "--host", HOST,
            "--users", str(USERS),
            "--spawn-rate", str(SPAWN_RATE),
            "--run-time", DURATION,
            "--headless",
            "--csv", prefix,
            "--only-summary"
        ]

        #print(cmd)
        #continue
        # Setup ambiente
        env = os.environ.copy()
        env["PYTHONWARNINGS"] = "ignore:Unverified HTTPS request"
        env["STORM_FILES_PER_REQ"] = str(n_files)

        try:
            subprocess.run(cmd, check=True, env=env)
        except subprocess.CalledProcessError as e:
            print(f"❌ Errore Locust nel test {n_files}: {e}")




if __name__ == "__main__":
    run_benchmark_suite()