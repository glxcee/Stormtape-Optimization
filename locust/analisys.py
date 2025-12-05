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

    #min_x = df["files_per_req"].min()
    max_y_stage = df["stage_p95"].max() * 1.2
    max_y_status = max(df["status_avg"].max(), df["status_avg2"].max()) * 1.2
    max_y_rps = df["rps"].max() * 1.2
    
    # Configurazione stile base
    plt.style.use('seaborn-v0_8-whitegrid')

    # 1. Grafico Stage Latency (POST)
    plt.figure(figsize=(10, 6))

    # plt.fill_between(
    #     df["files_per_req"],
    #     df["stage_p50"],
    #     df["stage_p95"],
    #     color='lightblue',  # Colore più chiaro
    #     alpha=0.5,
    #     label='Stage P50-P95'
    # )

    plt.errorbar(
        df["files_per_req"],
        df["stage_avg"],
        yerr=[df["stage_avg"] - df["stage_min"], df["stage_p95"] - df["stage_avg"]],
        fmt='o',
        color='blue',
        ecolor='darkblue',
        elinewidth=2,
        capsize=5,
        label='Stage Min-P95'
    )

    plt.plot(df["files_per_req"], df["stage_avg"], marker='o', linestyle='-', color='b', linewidth=2, label='Stage Avg')
    plt.title("Latenza Media Stage (POST) vs Numero di File", fontsize=14)
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Latenza Media (ms)", fontsize=12)

    plt.ylim(bottom=0, top=max_y_stage)
    #plt.xlim(left=min_x, right=max_x + (max_x * 0.05))

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    
    stage_plot_path = os.path.join(OUTDIR, "stage_latency.png")
    plt.savefig(stage_plot_path)
    plt.close()

    # 2. Grafico Poll Latency (GET)
    plt.figure(figsize=(10, 6))
    plt.plot(df["files_per_req"], df["status_avg"], marker='s', linestyle='-', color='blue', linewidth=2, label='Status (GET)')
    plt.plot(df["files_per_req"], df["status_avg2"], marker='s', linestyle='-', color='orange', linewidth=2, label='2nd Status (GET)')
    plt.title("Latenza Media Status (GET) vs Numero di File", fontsize=14)
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Latenza Media (ms)", fontsize=12)

    plt.ylim(bottom=0, top=max_y_status)

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()


    poll_plot_path = os.path.join(OUTDIR, "status_latency.png")
    plt.savefig(poll_plot_path)
    plt.close()

    # Grafico 3: RPS vs Numero di File
    plt.figure(figsize=(10, 6))
    plt.plot(df["files_per_req"], df["rps"], marker='o', linestyle='-', color='green', linewidth=2, label='Requests per Second (RPS)')
    plt.title("Requests per Second (RPS) vs Numero di File", fontsize=14)
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Requests per Second (RPS)", fontsize=12)

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
        f.write("| Files per Request | Requests/s | Failures | Stage Avg (ms) | Stage P95 (ms) | Stage Min (ms) | Status Avg (ms) | 2nd Status Avg (ms) | \n")
        f.write("\n")
        
        for entry in data:
            f.write(f"| {entry['files_per_req']} | {entry['rps']:.2f} | {entry['failures']:.2f} | Stage: {entry['stage_avg']:.2f} | {entry['stage_p95']:.2f} | {entry['stage_min']:.2f} |  {entry['status_avg']:.2f} | {entry['status_avg2']:.2f} |\n")

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
                p95_lat = row.get("95%", 0.0)
                if p95_lat == 0.0: p95_lat = row.get("95%ile", 0.0)
                rps = row.get("Requests/s", 0.0)
                fail_count = row.get("Failure Count", 0)

                # 2. Dati Specifici per Endpoint
                # Cerchiamo la riga specifica per "stage" (POST)
                stage_avg = 0.0
                stage_p95 = 0.0
                stage_row = df[df["Name"] == "stage"]
                if not stage_row.empty:
                    stage_avg = stage_row.iloc[0].get("Average Response Time", 0.0)
                    stage_p95 = stage_row.iloc[0].get("95%", 0.0)
                    stage_min = stage_row.iloc[0].get("Min Response Time", 0.0)


                # Cerchiamo la riga specifica per "get_stage" (Polling GET)
                poll_avg = 0.0
                poll_row = df[df["Name"] == "get_stage1"]
                if not poll_row.empty:
                    poll_avg = poll_row.iloc[0].get("Average Response Time", 0.0)

                poll2_avg2 = 0.0
                poll2_row = df[df["Name"] == "get_stage2"]
                if not poll2_row.empty:
                    poll2_avg = poll2_row.iloc[0].get("Average Response Time", 0.0)

                results_data.append({
                    "files_per_req": n_files,
                    "rps": rps,
                    "failures": fail_count,
                    "stage_avg": stage_avg,    # Latenza media POST /stage
                    "stage_p95": stage_p95,
                    "stage_min": stage_min,
                    "status_avg": poll_avg,       # Latenza media GET /stage/{id}
                    "status_avg2": poll2_avg
                })
                
                print(f"   -> Risultato: {rps:.2f} RPS | Stage: {stage_avg:.2f}ms | Status: {poll_avg:.2f}ms | Status2: {poll2_avg:.2f}ms | Failures: {fail_count}")

            except Exception as e:
                print(f"❌ Errore parsing CSV: {e}")
        else:
            print(f"⚠️  CSV {stats_file} non trovato.")

    print(results_data)
    generate_report(results_data)
    generate_plots(results_data)