import pandas as pd
import os
import matplotlib.pyplot as plt

data = [
    {
        "files_per_req": 10,
        
        "og_rps": 15.68,
        "mt1_1_rps": 19.47,
        "mt4_1_rps": 20.12,
        "mt1_2_rps": 45.47,
        "mt1_4_rps": 78.89,
        "mt1_8_rps": 129.85,
        "mt1_16_rps": 149.39,
        "og_wal_rps": 6.48,

        "og_stage_avg": 93.87,
        "mt1_1_stage_avg": 84.18,
        "mt4_1_stage_avg": 82.14
    },
    {
        "files_per_req": 50,

        "og_rps": 11.89,
        "mt1_1_rps": 20.1,
        "mt4_1_rps": 20.84 ,
        "mt1_2_rps": 32.85,
        "mt1_4_rps": 49.59,
        "mt1_8_rps": 57.16,
        "mt1_16_rps": 59.43,
        "og_wal_rps": 3.98,

        "og_stage_avg": 101.05,
        "mt1_1_stage_avg": 82.63,
        "mt4_1_stage_avg": 79.89,
    },
    {
        "files_per_req": 100,
        
        "og_rps": 7.74,
        "mt1_1_rps": 11.22,
        "mt4_1_rps": 10.70,
        "mt1_2_rps": 20.21,
        "mt1_4_rps": 30.98,
        "mt1_8_rps": 34.11,
        "mt1_16_rps": 31.66,
        "og_wal_rps": 6.45,

        "og_stage_avg": 179.75,
        "mt1_1_stage_avg": 187.38,
        "mt4_1_stage_avg": 205.48,
    },
    {
        "files_per_req": 200,

        "og_rps": 5.66,
        "mt1_1_rps": 6.1,
        "mt4_1_rps": 6.91,
        "mt1_2_rps": 11.21,
        "mt1_4_rps": 14.89,
        "mt1_8_rps": 17.01,
        "mt1_16_rps": 15.18,
        "og_wal_rps": 4.80,

        "og_stage_avg": 357.90,
        "mt1_1_stage_avg": 373.00,
        "mt4_1_stage_avg": 341.22,
    }
]

if __name__ == "__main__":
    df = pd.DataFrame(data)

    max_y = df["mt4_1_rps"].max() * 1.2

    plt.figure(figsize=(10, 6))
    plt.plot(df["files_per_req"], df["og_rps"], marker='o', linestyle='-', color='gray', linewidth=2, label='Sequenziale - 1 Server Thread')
    plt.plot(df["files_per_req"], df["og_wal_rps"], marker='o', linestyle='-', color='darkgray', linewidth=2, label='Sequenziale (WAL attiva) - 1 Server Thread')
    plt.plot(df["files_per_req"], df["mt1_1_rps"], marker='o', linestyle='-', color='greenyellow', linewidth=2, label='Parallelo - 1 Server Thread')
    plt.plot(df["files_per_req"], df["mt4_1_rps"], marker='o', linestyle='-', color='red', linewidth=2, label='Parallelo - 3 Server Thread')


    #plt.title("Requests per Second (RPS) vs Numero di File (1 USER)", fontsize=14)
    plt.title("Solo 1 Utente Attivo", fontsize=14)
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Richieste al secondo", fontsize=12)

    plt.ylim(bottom=0, top=max_y)

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    rps_plot_path = os.path.join(os.getcwd(), "rps_og_1_1mt4.png")
    plt.savefig(rps_plot_path)
    plt.close()

    #2

    max_y = df["mt1_16_rps"].max() * 1.2

    plt.figure(figsize=(10, 6))
    plt.plot(df["files_per_req"], df["og_rps"], marker='o', linestyle='-', color='gray', linewidth=2, label='Sequenziale - 1 Utente')
    plt.plot(df["files_per_req"], df["mt1_1_rps"], marker='o', linestyle='-', color='greenyellow', linewidth=2, label='Parallelo - 1 Utente')
    plt.plot(df["files_per_req"], df["mt1_2_rps"], marker='o', linestyle='-', color='lime', linewidth=2, label='Parallelo - 2 Utenti')
    plt.plot(df["files_per_req"], df["mt1_4_rps"], marker='o', linestyle='-', color='limegreen', linewidth=2, label='Parallelo - 4 Utenti')
    plt.plot(df["files_per_req"], df["mt1_8_rps"], marker='o', linestyle='-', color='green', linewidth=2, label='Parallelo - 8 Utenti')
    plt.plot(df["files_per_req"], df["mt1_16_rps"], marker='o', linestyle='-', color='darkgreen', linewidth=2, label='Parallelo - 16 Utenti')


    plt.title("Solo 1 Server Thread Attivo", fontsize=14)
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Richieste al secondo", fontsize=12)

    plt.ylim(bottom=0, top=max_y)

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    rps_plot_path = os.path.join(os.getcwd(), "rps_og_1__16.png")
    plt.savefig(rps_plot_path)
    plt.close()



    max_y = df["og_stage_avg"].max() * 1.2

    plt.figure(figsize=(10, 6))
    plt.plot(df["files_per_req"], df["og_stage_avg"], marker='o', linestyle='-', color='gray', linewidth=2, label='Sequenziale - 1 Server Thread')
    plt.plot(df["files_per_req"], df["mt1_1_stage_avg"], marker='o', linestyle='-', color='blue', linewidth=2, label='Parallelo - 1 Server Thread')
    plt.plot(df["files_per_req"], df["mt4_1_stage_avg"], marker='o', linestyle='-', color='darkblue', linewidth=2, label='Parallelo - 3 Server Thread')


    plt.title("Solo 1 Utente Attivo", fontsize=14)
    plt.xlabel("Numero di File per Richiesta", fontsize=12)
    plt.ylabel("Latenza Media (ms)", fontsize=12)

    plt.ylim(bottom=0, top=max_y)

    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend()
    rps_plot_path = os.path.join(os.getcwd(), "stage_og_1_1mt4.png")
    plt.savefig(rps_plot_path)
    plt.close()

