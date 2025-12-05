from locust import HttpUser, task, constant
#import uuid
import json
import time
import random
import os

token = ""

auth = {"Authorization": f"Bearer {token}"} if token != "" else {}

create_amount = 10
try:
    create_amount = int(os.getenv("STORM_FILES_PER_REQ", "10"))  
    print(f"⚙️  STORM_FILES_PER_REQ = {create_amount}")
except:
    print("⚠️  STORM_FILES_PER_REQ non valido, uso default 10")
# numero massimo di file da richiedere in una singola operazione di stage

check_amount_max = 5  # numero massimo di file da richiedere in una singola operazione di archiveinfo

class StormTapeUser(HttpUser):
    """
    Utente simulato per Storm-Tape che:
    1. invia una richiesta di stage (POST /api/v1/stage),
    2. salva il requestId restituito,
    3. fa polling (GET /api/v1/stage/{id}) fino al completamento,
    4. opzionalmente cancella o chiede archiveinfo.
    """
    wait_time = constant(0)  # tempo di attesa fra operazioni
    

    def on_start(self):
        """
        Chiamato quando l’utente virtuale “parte”. Puoi predisporre qui eventuali setup.
        """
        self.request_id = None

    @task(3) # pesantezza maggiore -> + frequente
    def do_stage_and_status(self):
        """
        Task principale:
        - manda POST /stage
        - poi effettua polling periodico GET /stage/{requestId}
        """
        # 1. Prepara payload per la richiesta di stage
        new_files = []
        for i in range(create_amount): 
            dirtext = f"{random.randrange(1, 101):03d}"
            filetext = f"{random.randrange(1, 101):03d}"

            new_files.append({"path": f"/tape/dir{dirtext}/file{filetext}"})

        payload = {
            "files": new_files
        }
    

        # 2. Invia la richiesta POST /api/v1/stage
        with self.client.post("/api/v1/stage", 
                headers=auth, 
                verify=False,
                json=payload, 
                catch_response=True, 
                name="stage") as resp:

            print(resp)
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

        # 3. Se abbiamo un request_id valido, facciamo status
        if self.request_id:
            #poll_interval = 1  # secondi fra status
            for i in range(2):
                #time.sleep(poll_interval)
                # GET /api/v1/stage/{id}
                r = self.client.get(f"/api/v1/stage/{self.request_id}",
                    headers=auth, verify=False,
                    name=f"get_stage{i+1}")
                
                
    @task(0)
    def do_archive_info(self):
        """
        Task secondario: chiedi informazioni di archivio per un file (POST /api/v1/archiveinfo)
        """
        #print(len(self.files))

        checking_files = []
        for i in range(create_amount): 
            dirtext = f"{random.randrange(1, 101):03d}"
            filetext = f"{random.randrange(1, 101):03d}"

            checking_files.append({"path": f"/tape/dir{dirtext}/file{filetext}"})

        payload = { "files": checking_files}
        self.client.post("/api/v1/archiveinfo", headers={"Authorization": f"Bearer {token}"}, verify=False, json=payload, name="archiveinfo")

    @task(0)
    def do_cancel(self):
        """
        Task secondario: cancella una richiesta di stage (DELETE) se c’è un requestId
        """
        if self.request_id:
            self.client.delete(f"/api/v1/stage/{self.request_id}", headers={"Authorization": f"Bearer {token}"}, verify=False, name="cancel")

            self.request_id = None
