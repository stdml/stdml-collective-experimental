#include <memory>

#include <stdml/mpi.h>

#include <stdml/collective>

std::unique_ptr<stdml::collective::peer> _default_peer;
std::unique_ptr<stdml::collective::session> _default_session;

int MPI_Init(int *argc, char ***argv)
{
    using P = stdml::collective::peer;
    using S = stdml::collective::session;
    _default_peer.reset(new P(P::from_ompi_env()));
    _default_session.reset(new S(_default_peer->join()));
    return 0;
}

int MPI_Finalize()
{
    _default_session.reset();
    _default_peer.reset();
    return 0;
}

int MPI_Comm_rank(mpi_comm_t, int *p)
{
    *p = _default_session->rank();
    return 0;
}

int MPI_Comm_size(mpi_comm_t, int *p)
{
    *p = _default_session->size();
    return 0;
}
