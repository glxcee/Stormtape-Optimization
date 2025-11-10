**tape_service**

#include <execution> al top

Il loop che risolve physical_path e controlla fs::status viene trasformato in std::for_each(std::execution::par, ...)

La std::transform che costruisce infos (da paths) viene resa parallela con std::for_each(std::execution::par, ...).
La funzione si occupa di restituire le location corrispondenti ai path richiesti in un vettore di PathInfo.

i StorageAreaResolver vengono istanziati dentro al loop in modo che ogni thread ha il suo oggetto e non vanno a referenziare lo stesso

In status() i controlli sugli status dei file rimangono sequenziali (controllo stato -> DB update), perché aggiornano stage e raccolgono files_to_update — ma l’ordinamento e l’update_timestamps restano invariati.

Ottimizza l'api call "/archiveinfo" controllando *parallelamente* lo stato e location di tutti i file richiesti

