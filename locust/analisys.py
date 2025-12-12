from datetime import datetime
import os
import pandas as pd
import matplotlib.pyplot as plt
import sys

TIMESTAMP = datetime.now().strftime(f"%Y%m%d")

if sys.argv.__len__() < 2:
    print("Usage: python analisys.py <output_directory>")
    sys.exit(1)

OUTDIR = f"{sys.argv[1]}"

SUMMARY_MD = os.path.join(OUTDIR, "summary.md")

HOST = "http://localhost:8080"
USERS = 1
DURATION = "10s"


def generate_plots(data):
    if not data:
        return

    df = pd.DataFrame(data)

    # Calcolo i limiti Y includendo anche i nuovi P95 per status
    max_y_stage = df["stage_p95"].max() * 1.2
    
    # Per il max Y dello status, prendiamo il massimo tra i due P95
    max_status_p95 = max(df["status_p95"].max(), df["status2_p95"].max())
    max_y_status = max_status_p95 * 1.2
    
    max_y_rps = df["rps"].max() * 1.2
    
    # Configurazione stile base
    plt.style.use('seaborn-v0_8-whitegrid')

    # ---------------------------------------------------------
    # 1. Grafico Stage Latency (POST)
    # ---------------------------------------------------------
    plt.figure(figsize=(10, 6))

    # Errorbar per Stage
    plt.errorbar(
        df["files_per_req"],
        df["stage_avg"],
        yerr=[df["stage_avg"] - df["stage_min"], df["stage_p95"] - df["stage_avg"]],
        fmt='o',
        color='blue',
        ecolor='darkblue',
        elinewidth=2,
        capsize=5,
        linestyle='-', # Aggiunto linea di collegamento
        label='Stage Min-Avg-P95'
    )

    # plt.plot rimossa perché inclusa in errorbar tramite linestyle='-'
    
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Latenza Media (ms)", fontsize=12)
    plt.ylim(bottom=0, top=max_y_stage)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    
    stage_plot_path = os.path.join(OUTDIR, "stage_latency.png")
    plt.savefig(stage_plot_path)
    plt.close()

    # ---------------------------------------------------------
    # 2. Grafico Poll Latency (GET) - CON ERRORBAR
    # ---------------------------------------------------------
    plt.figure(figsize=(10, 6))

    # Errorbar per Status 1 (get_stage1)
    plt.errorbar(
        df["files_per_req"],
        df["status_avg"],
        yerr=[df["status_avg"] - df["status_min"], df["status_p95"] - df["status_avg"]],
        fmt='s', # Marker quadrato
        color='blue',
        ecolor='darkblue',
        elinewidth=2,
        capsize=5,
        linestyle='-',
        label='Status Min-Avg-P95'
    )

    # Errorbar per Status 2 (get_stage2)
    plt.errorbar(
        df["files_per_req"],
        df["status_avg2"],
        yerr=[df["status_avg2"] - df["status2_min"], df["status2_p95"] - df["status_avg2"]],
        fmt='^', # Marker triangolo
        color='orange',
        ecolor='darkorange',
        elinewidth=2,
        capsize=5,
        linestyle='-',
        label='2nd Status Min-Avg-P95'
    )

    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Latenza Media (ms)", fontsize=12)
    plt.ylim(bottom=0, top=max_y_status)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()

    poll_plot_path = os.path.join(OUTDIR, "status_latency.png")
    plt.savefig(poll_plot_path)
    plt.close()

    # ---------------------------------------------------------
    # 3. Grafico RPS
    # ---------------------------------------------------------
    plt.figure(figsize=(10, 6))
    plt.plot(df["files_per_req"], df["rps"], marker='o', linestyle='-', color='green', linewidth=2, label='Richieste al secondo')
    
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Richieste al secondo", fontsize=12)
    plt.ylim(bottom=0, top=max_y_rps)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    
    rps_plot_path = os.path.join(OUTDIR, "rps.png")
    plt.savefig(rps_plot_path)
    plt.close()

    print(f"📈 Grafici generati:\n   - {stage_plot_path}\n   - {poll_plot_path}\n    - {rps_plot_path}")


def generate_report(data):
    if not data:
        return

    with open(SUMMARY_MD, "w") as f:
        f.write(f"# Summary Benchmark - {TIMESTAMP}\n\n")
        f.write(f"Host: {HOST}\n\n")
        f.write(f"Users: {USERS}\n\n")
        f.write(f"Duration: {DURATION}\n\n")
        f.write("## Risultati per Numero di File per Richiesta\n\n")
        # Aggiorno header tabella per includere info extra se necessario, mantengo compatta
        f.write("| Files | RPS | Fail | Stage Avg | Stage P95 | Stage Min | Status Avg | Status P95 | Status Min | Status2 Avg | Status2 P95 | Status2 Min |\n")
        f.write("|---|---|---|---|---|---|---|---|---|---|---|---|\n")
        
        for entry in data:
            f.write(f"| {entry['files_per_req']} | {entry['rps']:.2f} | {entry['failures']} | "
                    f"{entry['stage_avg']:.2f} | {entry['stage_p95']:.2f} | {entry['stage_min']:.2f} | "
                    f"{entry['status_avg']:.2f} | {entry['status_p95']:.2f} | {entry['status_min']:.2f} | "
                    f"{entry['status_avg2']:.2f} | {entry['status2_p95']:.2f} | {entry['status2_min']:.2f} |\n")

    print(f"📝 Report generato: {SUMMARY_MD}")

FILES_PER_REQUEST_LIST = [10, 50, 100, 200]

if __name__ == "__main__":
    results_data = []
    for n_files in FILES_PER_REQUEST_LIST:
        prefix = f"{OUTDIR}/test_{n_files}_files"
        stats_file = f"{prefix}_stats.csv"
        if os.path.exists(stats_file):
            try:
                df = pd.read_csv(stats_file)
                
                # 1. Dati Aggregati (Totali)
                if "Name" in df.columns:
                    agg_row = df[df["Name"] == "Aggregated"]
                    row = agg_row.iloc[0] if not agg_row.empty else df.iloc[-1]
                else:
                    row = df.iloc[-1]

                avg_lat = row.get("Average Response Time", 0.0)
                rps = row.get("Requests/s", 0.0)
                fail_count = row.get("Failure Count", 0)

                # 2. Dati Specifici per Endpoint
                
                # --- STAGE (POST) ---
                stage_avg, stage_p95, stage_min = 0.0, 0.0, 0.0
                stage_row = df[df["Name"] == "stage"]
                if not stage_row.empty:
                    stage_avg = stage_row.iloc[0].get("Average Response Time", 0.0)
                    stage_p95 = stage_row.iloc[0].get("95%", 0.0)
                    if stage_p95 == 0.0: stage_p95 = stage_row.iloc[0].get("95%ile", 0.0)
                    stage_min = stage_row.iloc[0].get("Min Response Time", 0.0)

                # --- POLL 1 (get_stage1) ---
                poll_avg, poll_p95, poll_min = 0.0, 0.0, 0.0
                poll_row = df[df["Name"] == "get_stage1"]
                if not poll_row.empty:
                    poll_avg = poll_row.iloc[0].get("Average Response Time", 0.0)
                    poll_p95 = poll_row.iloc[0].get("95%", 0.0)
                    if poll_p95 == 0.0: poll_p95 = poll_row.iloc[0].get("95%ile", 0.0)
                    poll_min = poll_row.iloc[0].get("Min Response Time", 0.0)

                # --- POLL 2 (get_stage2) ---
                poll2_avg, poll2_p95, poll2_min = 0.0, 0.0, 0.0
                poll2_row = df[df["Name"] == "get_stage2"]
                if not poll2_row.empty:
                    poll2_avg = poll2_row.iloc[0].get("Average Response Time", 0.0)
                    poll2_p95 = poll2_row.iloc[0].get("95%", 0.0)
                    if poll2_p95 == 0.0: poll2_p95 = poll2_row.iloc[0].get("95%ile", 0.0)
                    poll2_min = poll2_row.iloc[0].get("Min Response Time", 0.0)

                results_data.append({
                    "files_per_req": n_files,
                    "rps": rps,
                    "failures": fail_count,
                    
                    # Stage Stats
                    "stage_avg": stage_avg,
                    "stage_p95": stage_p95,
                    "stage_min": stage_min,
                    
                    # Status 1 Stats
                    "status_avg": poll_avg,
                    "status_p95": poll_p95,
                    "status_min": poll_min,
                    
                    # Status 2 Stats
                    "status_avg2": poll2_avg,
                    "status2_p95": poll2_p95,
                    "status2_min": poll2_min
                })
                
                print(f"   -> {n_files} files: {rps:.2f} RPS | Stage Avg: {stage_avg:.1f} | Status Avg: {poll_avg:.1f}")

            except Exception as e:
                print(f"❌ Errore parsing CSV {stats_file}: {e}")
        else:
            print(f"⚠️  CSV {stats_file} non trovato.")

    # print(results_data)
    generate_report(results_data)
    generate_plots(results_data)