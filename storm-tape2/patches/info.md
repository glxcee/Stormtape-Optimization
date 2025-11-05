**tape_service_par**

#include <execution>

Il loop che risolve physical_path e controlla fs::status viene trasformato in std::for_each(std::execution::par, ...) con StorageAreaResolver creato localmente nella lambda (thread-safe).

La std::transform che costruisce infos (da paths) viene resa parallela con std::transform(std::execution::par, ...).

In status() i controlli sugli status dei file rimangono sequenziali (controllo stato -> DB update), perché aggiornano stage e raccolgono files_to_update — ma l’ordinamento e l’update_timestamps restano invariati.

