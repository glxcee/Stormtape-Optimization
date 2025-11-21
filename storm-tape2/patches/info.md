**file_system**

implementa utilità generiche parallel_map/parallel_for_each e usa queste utilità in tape_service.cpp su tre punti chiave:

ArchiveInfoResponse::archive_info() → usiamo parallel_map per costruire i PathInfo in parallelo (limitando il grado di concorrenza).

extend_paths_with_localities() → usiamo parallel_map per ottenere PathLocality in parallelo.

TakeOverResponse::take_over() → la parte che crea xattr viene eseguita con parallel_for_each (con bound di concorrenza) invece che con std::for_each sequenziale.

status(): parallelizza le letture di stato dei file tramite ExtendedFileStatus