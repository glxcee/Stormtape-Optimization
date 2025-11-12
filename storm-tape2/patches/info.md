**tape_service**

#include <execution> al top

Ottimizza operazioni CPU-bound

Il loop che risolve physical_path e controlla fs::status viene trasformato in std::for_each(std::execution::par, ...)

<!-- La std::transform che costruisce infos (da paths) viene resa parallela con std::for_each(std::execution::par, ...).
La funzione si occupa di restituire le location corrispondenti ai path richiesti in un vettore di PathInfo. -->

i StorageAreaResolver vengono istanziati dentro al loop in modo che ogni thread ha il suo oggetto e non vanno a referenziare lo stesso

parallelizzati le operazioni di sorting:
std::sort(std::execution::par, files.begin(), files.end(), [](File const& a, File const& b) {
    return a.logical_path < b.logical_path;
  });
e
std::sort(std::execution::par, stage.files.begin(), stage.files.end(),
              [](auto const& f1, auto const& f2) {
                return to_underlying(f1.state) < to_underlying(f2.state);
              });


In status() i controlli sugli status dei file rimangono sequenziali (controllo stato -> DB update), perché aggiornano stage e raccolgono files_to_update — ma l’ordinamento e l’update_timestamps restano invariati.

Ottimizza l'api call "/archiveinfo"


