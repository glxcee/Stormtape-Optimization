**tape_service_par**

#include <execution>

Il loop che risolve physical_path e controlla fs::status viene trasformato in std::for_each(std::execution::par, ...) con StorageAreaResolver creato localmente nella lambda (thread-safe).

La std::transform che costruisce infos (da paths) viene resa parallela con std::transform(std::execution::par, ...).

In status() i controlli sugli status dei file rimangono sequenziali (controllo stato -> DB update), perché aggiornano stage e raccolgono files_to_update — ma l’ordinamento e l’update_timestamps restano invariati.


**file_system**

implementa utilità generiche parallel_map/parallel_for_each e usa queste utilità in tape_service.cpp su tre punti chiave:

ArchiveInfoResponse::archive_info() → usiamo parallel_map per costruire i PathInfo in parallelo (limitando il grado di concorrenza).

extend_paths_with_localities() → usiamo parallel_map per ottenere PathLocality in parallelo.

TakeOverResponse::take_over() → la parte che crea xattr viene eseguita con parallel_for_each (con bound di concorrenza) invece che con std::for_each sequenziale.

status(): parallelizza le letture di stato dei file tramite ExtendedFileStatus