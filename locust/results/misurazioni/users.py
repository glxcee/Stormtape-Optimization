import pandas as pd
import os
import matplotlib.pyplot as plt

data = [
    {
        "users": 1,
        "mt1_rps": 6.1,
        "mt4_rps": 6.91,
        "stage_avg": 373,
        "og_rps": 5.66,
        "og_stage_avg": 367,
        "og_wal_rps": 4.8,
        "og_wal_stage_avg": 445,
        "mt4_stage_avg": 341,
    },
    {
        "users": 2,
        "mt1_rps": 11.21,
        "mt4_rps": 12.22,
        "stage_avg": 408,
        "og_rps": 8.77,
        "og_stage_avg": 439,
        "og_wal_rps": 9.58,
        "og_wal_stage_avg": 441,
        "mt4_stage_avg": 376,
    },
    {
        "users": 4,
        "mt1_rps": 14.89,
        "mt4_rps": 16.3,
        "stage_avg": 502,
        "og_rps": 12.33,
        "og_stage_avg": 547,
        "og_wal_rps": 13.15,
        "og_wal_stage_avg": 548,
        "mt4_stage_avg": 504,
    },
    {
        "users": 8,
        "mt1_rps": 17.01,
        "mt4_rps": 17.46,
        "stage_avg": 862,
        "og_rps": 14.18,
        "og_stage_avg": 961,
        "og_wal_rps": 13.69,
        "og_wal_stage_avg": 1014,
        "mt4_stage_avg": 868,
    },
    {
        "users": 16,
        "mt1_rps": 15.18,
        "mt4_rps": 15.88,
        "stage_avg": 1678,
        "og_rps": 11.37,
        "og_stage_avg": 2369,
        "og_wal_rps": 12.59,
        "og_wal_stage_avg": 2213,
        "mt4_stage_avg": 1733,
    }
]

if __name__ == "__main__":
    df = pd.DataFrame(data)

    max_y = df["mt4_rps"].max() * 1.2

    plt.figure(figsize=(10, 6))
    #plt.plot(df["files_per_req"], df["og_rps"], marker='o', linestyle='-', color='gray', linewidth=2, label='Seriale')
    plt.plot(df["users"], df["og_rps"], marker='o', linestyle='-', color='gray', linewidth=2, label='Sequenziale - 1 Server Thread')
    plt.plot(df["users"], df["og_wal_rps"], marker='o', linestyle='-', color='darkgray', linewidth=2, label='Sequenziale (WAL attiva) - 1 Server Thread')
    plt.plot(df["users"], df["mt1_rps"], marker='o', linestyle='-', color='green', linewidth=2, label='Parallelo - 1 Server Thread')
    plt.plot(df["users"], df["mt4_rps"], marker='o', linestyle='-', color='red', linewidth=2, label='Parallelo - 3 Server Thread')


    plt.title("Fissati 200 Files per richiesta", fontsize=14)
    plt.xlabel("Utenti", fontsize=12)
    plt.ylabel("Richieste al secondo", fontsize=12)

    plt.ylim(bottom=0, top=max_y)

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    rps_plot_path = os.path.join(os.getcwd(), "rps_total.png")
    plt.savefig(rps_plot_path)
    plt.close()



    max_y = df["og_wal_stage_avg"].max() * 1.2

    plt.figure(figsize=(10, 6))
    plt.plot(df["users"], df["og_stage_avg"], marker='o', linestyle='-', color='gray', linewidth=2, label='Sequenziale - 1 Server Thread')
    plt.plot(df["users"], df["og_wal_stage_avg"], marker='o', linestyle='-', color='darkgray', linewidth=2, label='Sequenziale (WAL attiva) - 1 Server Thread')
    plt.plot(df["users"], df["stage_avg"], marker='o', linestyle='-', color='blue', linewidth=2, label='Parallelo - 1 Server Thread')
    plt.plot(df["users"], df["mt4_stage_avg"], marker='o', linestyle='-', color='red', linewidth=2, label='Parallelo - 3 Server Thread')

    plt.title("Fissati 200 Files per richiesta", fontsize=14)
    plt.xlabel("Utenti", fontsize=12)
    plt.ylabel("Latenza Media (ms)", fontsize=12)

    plt.ylim(bottom=0, top=max_y)

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    rps_plot_path = os.path.join(os.getcwd(), "stage_total.png")
    plt.savefig(rps_plot_path)
    plt.close()

