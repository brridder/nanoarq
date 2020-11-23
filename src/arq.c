/* This code is in the public domain. See the LICENSE file for details. */

#include "arq.h"

arq_err_t arq_init(arq_cfg_t const *cfg, void *arq_seat, unsigned arq_seat_size, struct arq_t **out_arq)
{
    arq__lin_alloc_t la;
    arq_err_t check_err;
    arq_t *arq;
    if (!cfg || !arq_seat || !out_arq) {
        return ARQ_ERR_INVALID_PARAM;
    }
    check_err = arq__check_cfg(cfg);
    if (!ARQ_SUCCEEDED(check_err)) {
        return check_err;
    }
    arq__lin_alloc_init(&la, arq_seat, arq_seat_size);
    arq = arq__alloc(cfg, &la);
    if (!arq) {
        return ARQ_ERR_INVALID_PARAM;
    }
    arq->cfg = *cfg;
    arq__init(arq);
    arq__rst(arq);
    *out_arq = arq;
    return ARQ_OK_COMPLETED;
}

