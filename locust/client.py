from locust import HttpUser, task, between
import uuid
import json
import time
import random

create_amount_max = 10  # numero massimo di file da richiedere in una singola operazione di stage
check_amount_max = 5  # numero massimo di file da richiedere in una singola operazione di archiveinfo

class StormTapeUser(HttpUser):
    """
    Utente simulato per Storm-Tape che:
    1. invia una richiesta di stage (POST /api/v1/stage),
    2. salva il requestId restituito,
    3. fa polling (GET /api/v1/stage/{id}) fino al completamento,
    4. opzionalmente cancella o chiede archiveinfo.
    """
    wait_time = between(1, 3)  # tempo di attesa fra operazioni
    files = [] # tiene traccia dei file richiesti

    def on_start(self):
        """
        Chiamato quando l’utente virtuale “parte”. Puoi predisporre qui eventuali setup.
        """
        self.request_id = None

    @task(3) # pesantezza maggiore -> + frequente
    def do_stage_and_poll(self):
        """
        Task principale:
        - manda POST /stage
        - poi effettua polling periodico GET /stage/{requestId}
        """
        # 1. Prepara payload per la richiesta di stage
        new_files = []
        for i in range(random.randrange(1,create_amount_max)):  # puoi variare il numero di file se vuoi
            new_files.append({"path": f"/tmp/testfile-{uuid.uuid4()}.txt"})

        payload = {
            "files": new_files
        }

        self.files += new_files

        # 2. Invia la richiesta POST /api/v1/stage
        with self.client.post("/api/v1/stage", json=payload, catch_response=True, name="stage") as resp:
            if resp.status_code == 200 or resp.status_code == 201:
                try:
                    j = resp.json()
                    self.request_id = j.get("requestId")
                    if not self.request_id:
                        resp.failure("No requestId in response")
                except Exception as e:
                    resp.failure(f"JSON parse error: {e}")
            else:
                resp.failure(f"Stage failed with status {resp.status_code}")

        # 3. Se abbiamo un request_id valido, facciamo polling
        if self.request_id:
            max_poll_rounds = 5
            poll_interval = 1  # secondi fra polling
            for _ in range(max_poll_rounds):
                time.sleep(poll_interval)
                # GET /api/v1/stage/{id}
                r = self.client.get(f"/api/v1/stage/{self.request_id}", name="get_stage")
                # opzionale: puoi controllare il corpo per vedere se è “COMPLETED” o “FAILED”
                if r.status_code == 200:
                    try:
                        j2 = r.json()
                        files = j2.get("files", [])
                        # se vuoi interrompere il polling quando è completato
                        all_done = True
                        for f in files:
                            state = f.get("state")
                            if state not in ("COMPLETED", "FAILED"):
                                all_done = False
                                break
                        if all_done:
                            break
                    except:
                        pass
                # se status non 200 non facciamo nulla di speciale qui, continua polling

    @task(2)
    def do_archive_info(self):
        """
        Task secondario: chiedi informazioni di archivio per un file (POST /api/v1/archiveinfo)
        """
        #print(len(self.files))

        checking_files = []
        if len(self.files) == 0:
            checking_files.append({"path": "/tmp/example2.txt"})
        else:
            for i in range(random.randrange(1,min(len(self.files),check_amount_max)+1)):
                checking_files.append(self.files[random.randrange(0, len(self.files))])

        payload = {
            "files": checking_files
        }
        self.client.post("/api/v1/archiveinfo", json=payload, name="archiveinfo")

    @task(1)
    def do_cancel(self):
        """
        Task secondario: cancella una richiesta di stage (DELETE) se c’è un requestId
        """
        if self.request_id:
            self.client.delete(f"/api/v1/stage/{self.request_id}", name="cancel")

            self.request_id = None
