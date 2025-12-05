from locust import HttpUser, task, constant
#import uuid
import json
import time
import random
import os

token = "eyJraWQiOiJyc2ExIiwiYWxnIjoiUlMyNTYifQ.eyJlbnRpdGxlbWVudHMiOlsidXJuOmdlYW50OmNuYWZzZDpncm91cDpkZXY6eGZlcnMiLCJ1cm46Z2VhbnQ6Y25hZnNkOmdyb3VwOmRldiJdLCJzdWIiOiI0MGQ5ODU0Ny0yNzc0LWJiNmMtYTc0ZC00YWJlMzBjMDBkOTkiLCJpc3MiOiJodHRwczovL2lhbS1kZXYuY2xvdWQuY25hZi5pbmZuLml0LyIsInByZWZlcnJlZF91c2VybmFtZSI6ImFydWdnZXJpIiwiY2xpZW50X2lkIjoiNDI5OTlhNjMtNzQ0OS00M2ZiLTk1MmUtNDJmMmQ3NWI4NjViIiwidm9wZXJzb25faWQiOiI0MGQ5ODU0Ny0yNzc0LWJiNmMtYTc0ZC00YWJlMzBjMDBkOTlAaWFtLWRldiIsIm5iZiI6MTc2NDkzNzU3OCwic2NvcGUiOiJhZGRyZXNzIHBob25lIG9wZW5pZCBvZmZsaW5lX2FjY2VzcyBwcm9maWxlIGFhcmMgZW1haWwiLCJlZHVwZXJzb25fc2NvcGVkX2FmZmlsaWF0aW9uIjoibWVtYmVyQGlhbS1kZXYiLCJlZHVwZXJzb25fYXNzdXJhbmNlIjpbImh0dHBzOi8vcmVmZWRzLm9yZy9hc3N1cmFuY2UiLCJodHRwczovL3JlZmVkcy5vcmcvYXNzdXJhbmNlL0lBUC9sb3ciXSwiZXhwIjoxNzY0OTQxMjM4LCJpYXQiOjE3NjQ5Mzc2MzgsImp0aSI6IjdjOTc3ODNmLWVlMDYtNDU5NS05YzZlLTc1MDY2NmRkMWY1ZiIsImVtYWlsIjoiYW5nZWxvLnJ1Z2dpZXJpMkBzdHVkaW8udW5pYm8uaXQifQ.jFKsInRJZJ_9ZuVAPtzm8NaEJ123xF6phazlQNDh0DYEvJr9_NF18bYzdMfEPtLh1S__6ESf4CsDRj9zSm3YCFxtdTvJcjeEIXP7ypKMIcCYw9MHSTm-Pyo5vIdM7jtTqj7OhCHpvw93AYs9Ks-KJ1EM7-g3uZZR3h-MRec2xPfcs6JpRf-M1NhaZWm4e1sfXgoxZB1ArjAJg1gq2AtsBb61w5rqWks_wWjdjDXieTOq5lmdbA8A1MTL-0H7YXmDKAVYQwQ94OExNg3psbwT6q6zGDHkmr2vTiA1eYKMowygnBYecMuJ2zQx_pyf2ANuAXpyRzpypyKq8KySJbrrBg"

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
