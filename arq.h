/* This code is in the public domain. See the LICENSE file for details. */
#ifndef ARQ_H_INCLUDED
#define ARQ_H_INCLUDED

#ifndef ARQ_USE_C_STDLIB
    #error You must define ARQ_USE_C_STDLIB to 0 or 1 before including arq.h
#endif
#ifndef ARQ_COMPILE_CRC32
    #error You must define ARQ_COMPILE_CRC32 to 0 or 1 before including arq.h
#endif

#if ARQ_USE_C_STDLIB == 1
    #include <stdint.h>
    #define ARQ_UINT16_TYPE uint16_t
    #define ARQ_UINT32_TYPE uint32_t
#else
    #ifndef ARQ_UINT16_TYPE
        #error You must define ARQ_UINT16_TYPE before including arq.h
    #endif
    #ifndef ARQ_UINT32_TYPE
        #error You must define ARQ_UINT32_TYPE before including arq.h
    #endif
#endif

#ifndef ARQ_MOCKABLE
    #define ARQ_MOCKABLE(FUNC) FUNC
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef ARQ_UINT16_TYPE arq_uint16_t;
typedef ARQ_UINT32_TYPE arq_uint32_t;
typedef unsigned char arq_uchar_t;

typedef enum
{
    ARQ_OK_COMPLETED = 0,
    ARQ_OK_POLL_REQUIRED = 1,
    ARQ_ERR_INVALID_PARAM = -1,
    ARQ_ERR_NO_ASSERT_HANDLER = -2,
    ARQ_ERR_SEND_PTR_NOT_HELD = -3,
    ARQ_ERR_NEED_POLL = -4
} arq_err_t;

#define ARQ_SUCCEEDED(ARQ_RESULT) ((ARQ_RESULT) >= 0)

typedef enum
{
    ARQ_EVENT_NONE,
    ARQ_EVENT_CONN_ESTABLISHED,
    ARQ_EVENT_CONN_CLOSED,
    ARQ_EVENT_CONN_RESET_BY_PEER,
    ARQ_EVENT_CONN_LOST_PEER_TIMEOUT
} arq_event_t;

typedef arq_uint32_t arq_time_t;
typedef void (*arq_assert_cb_t)(char const *file, int line, char const *cond, char const *msg);
typedef arq_uint32_t (*arq_checksum_t)(void const *p, int len);

typedef struct arq_cfg_t
{
    int segment_length_in_bytes;
    int message_length_in_segments;
    int send_window_size_in_messages;
    int recv_window_size_in_messages;
    arq_time_t tinygram_send_delay;
    arq_time_t retransmission_timeout;
    arq_time_t keepalive_period;
    arq_time_t disconnect_timeout;
    arq_checksum_t checksum;
} arq_cfg_t;

typedef struct arq_stats_t
{
    int bytes_sent;
    int bytes_recvd;
    int frames_sent;
    int frames_recvd;
    int messages_sent;
    int messages_recvd;
    int malformed_frames_recvd;
    int checksum_failures_recvd;
    int retransmitted_frames_sent;
} arq_stats_t;

typedef enum
{
    ARQ_STATE_CLOSED,
    ARQ_STATE_RST_RECVD,
    ARQ_STATE_RST_SENT,
    ARQ_STATE_ESTABLISHED,
    ARQ_STATE_CLOSE_WAIT,
    ARQ_STATE_LAST_ACK,
    ARQ_STATE_FIN_WAIT_1,
    ARQ_STATE_FIN_WAIT_2,
    ARQ_STATE_CLOSING,
    ARQ_STATE_TIME_WAIT
} arq_state_t;

struct arq_t;

arq_err_t arq_assert_handler_set(arq_assert_cb_t assert_cb);
arq_err_t arq_assert_handler_get(arq_assert_cb_t *out_assert_cb);
arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size);
arq_err_t arq_init(arq_cfg_t const *cfg, void *arq_seat, int arq_seat_size, struct arq_t **out_arq);
arq_err_t arq_connect(struct arq_t *arq);
arq_err_t arq_reset(struct arq_t *arq);
arq_err_t arq_close(struct arq_t *arq);
arq_err_t arq_recv(struct arq_t *arq, void *recv, int recv_max, int *out_recv_size);
arq_err_t arq_send(struct arq_t *arq, void const *send, int send_max, int *out_sent_size);
arq_err_t arq_flush(struct arq_t *arq);

arq_err_t arq_backend_poll(struct arq_t *arq,
                           arq_time_t dt,
                           int *out_backend_send_size,
                           arq_event_t *out_event,
                           arq_time_t *out_next_poll);

arq_err_t arq_backend_send_ptr_get(struct arq_t *arq, void const **out_send, int *out_send_size);
arq_err_t arq_backend_send_ptr_release(struct arq_t *arq);
arq_err_t arq_backend_recv_fill(struct arq_t *arq, void const *recv, int recv_max, int *out_recv_size);

#if ARQ_COMPILE_CRC32 == 1
arq_uint32_t arq_crc32(void const *buf, int size);
#endif

/* Internal API */

typedef struct arq__lin_alloc_t
{
    arq_uchar_t *base;
    arq_uchar_t *top;
    int capacity;
} arq__lin_alloc_t;

void arq__lin_alloc_init(arq__lin_alloc_t *a, void *base, int capacity);
void *arq__lin_alloc_alloc(arq__lin_alloc_t *a, int size, int align);

enum
{
    ARQ__FRAME_HEADER_SIZE = 12,
    ARQ__FRAME_COBS_OVERHEAD = 2,
    ARQ__FRAME_MAX_SEQ_NUM = (1 << 12) - 1
};

int arq__frame_len(int seg_len);

typedef struct arq__frame_hdr_t
{
    int version;
    int seg_len;
    int win_size;
    int seq_num;
    int msg_len;
    int seg_id;
    int ack_num;
    arq_uint16_t cur_ack_vec;
    char rst;
    char fin;
    char ack;
    char seg;
} arq__frame_hdr_t;

void arq__frame_hdr_init(arq__frame_hdr_t *h);
int  arq__frame_hdr_write(arq__frame_hdr_t const *h, void *out_frame);
void arq__frame_hdr_read(void const *buf, arq__frame_hdr_t *out_frame_hdr);
int  arq__frame_seg_write(void const *seg, void *out_frame, int len);

int arq__frame_write(arq__frame_hdr_t const *h,
                     void const *seg,
                     arq_checksum_t checksum,
                     void *out_frame,
                     int frame_max);

typedef enum
{
    ARQ__FRAME_READ_RESULT_SUCCESS,
    ARQ__FRAME_READ_RESULT_ERR_CHECKSUM,
    ARQ__FRAME_READ_RESULT_ERR_MALFORMED
} arq__frame_read_result_t;

arq__frame_read_result_t arq__frame_read(void *frame,
                                         int frame_len,
                                         arq_checksum_t checksum,
                                         arq__frame_hdr_t *out_hdr,
                                         void const **out_seg);

void arq__frame_checksum_write(arq_checksum_t checksum, void *checksum_seat, void *frame, int len);
arq__frame_read_result_t arq__frame_checksum_read(void const *frame,
                                                  int frame_len,
                                                  int seg_len,
                                                  arq_checksum_t checksum);

void arq__cobs_encode(void *p, int len);
void arq__cobs_decode(void *p, int len);

typedef struct arq__msg_t
{
    arq_uint16_t len; /* in bytes */
    arq_uint16_t cur_ack_vec;
    arq_uint16_t full_ack_vec;
} arq__msg_t;

typedef struct arq__wnd_t
{
    arq__msg_t *msg;
    arq_uchar_t *buf;
    arq_uint16_t seq;
    arq_uint16_t cap; /* in messages */
    arq_uint16_t size; /* in messages */
    arq_uint16_t msg_len; /* in bytes */
    arq_uint16_t seg_len; /* in bytes */
    arq_uint16_t full_ack_vec;
} arq__wnd_t;

void arq__wnd_init(arq__wnd_t *w, int wnd_cap, int msg_len, int seg_len);
void arq__wnd_rst(arq__wnd_t *w);
void arq__wnd_seg(arq__wnd_t *w, unsigned msg, unsigned seg, void **out_seg, int *out_seg_len);

typedef struct arq__send_wnd_t
{
    arq__wnd_t w;
    arq_time_t *rtx;
} arq__send_wnd_t;

void arq__send_wnd_rst(arq__send_wnd_t *sw);
unsigned arq__send_wnd_send(arq__send_wnd_t *sw, void const *seg, unsigned len);
void arq__send_wnd_ack(arq__send_wnd_t *sw, unsigned seq, arq_uint16_t cur_ack_vec);
void arq__send_wnd_flush(arq__send_wnd_t *sw);
void arq__send_wnd_step(arq__send_wnd_t *sw, arq_time_t dt);

typedef struct arq__send_wnd_ptr_t
{
    arq_uint16_t seq;
    arq_uint16_t seg;
    arq_uchar_t valid;
} arq__send_wnd_ptr_t;

void arq__send_wnd_ptr_init(arq__send_wnd_ptr_t *p);

typedef enum
{
    ARQ__SEND_WND_PTR_NEXT_INSIDE_MSG,
    ARQ__SEND_WND_PTR_NEXT_COMPLETED_MSG
} arq__send_wnd_ptr_next_result_t;

arq__send_wnd_ptr_next_result_t arq__send_wnd_ptr_next(arq__send_wnd_ptr_t *p, arq__send_wnd_t const *sw);

typedef enum
{
    ARQ__SEND_FRAME_STATE_FREE,
    ARQ__SEND_FRAME_STATE_HELD,
    ARQ__SEND_FRAME_STATE_RELEASED
} arq__send_frame_state_t;

typedef struct arq__send_frame_t
{
    arq_uchar_t *buf;
    arq__send_frame_state_t state;
    arq_uint16_t cap;
    arq_uint16_t len;
} arq__send_frame_t;

void arq__send_frame_init(arq__send_frame_t *f, int cap);
int arq__send_poll(arq__send_wnd_t *sw,
                   arq__send_frame_t *sf,
                   arq__send_wnd_ptr_t *sp,
                   arq__frame_hdr_t *sh,
                   arq__frame_hdr_t const *rh,
                   arq_time_t dt,
                   arq_time_t rtx);

typedef struct arq__recv_wnd_t
{
    arq__wnd_t w;
    arq_uchar_t *ack;
    unsigned recv_ptr;
} arq__recv_wnd_t;

void arq__recv_wnd_rst(arq__recv_wnd_t *rw);
unsigned arq__recv_wnd_recv(arq__recv_wnd_t *rw, void *dst, unsigned dst_max);
unsigned arq__recv_wnd_frame(arq__recv_wnd_t *rw,
                             unsigned seq,
                             unsigned seg,
                             unsigned seg_cnt,
                             void const *p,
                             unsigned len);

typedef struct arq__recv_wnd_ptr_t
{
    arq_uint16_t seq;
    arq_uint16_t cur_ack_vec;
    arq_uchar_t pending;
    arq_uchar_t valid;
} arq__recv_wnd_ptr_t;

void arq__recv_wnd_ptr_init(arq__recv_wnd_ptr_t *p);
void arq__recv_wnd_ptr_set(arq__recv_wnd_ptr_t *p, unsigned seq, unsigned ack_vec);
void arq__recv_wnd_ptr_next(arq__recv_wnd_ptr_t *p, arq__recv_wnd_t const *rw);

typedef enum
{
    ARQ__RECV_FRAME_STATE_ACCUMULATING,
    ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT
} arq__recv_frame_state_t;

typedef struct arq__recv_frame_t
{
    arq_uchar_t *buf;
    arq__recv_frame_state_t state;
    arq_uint16_t cap;
    arq_uint16_t len;
} arq__recv_frame_t;

void arq__recv_frame_init(arq__recv_frame_t *f, void *buf, int len);
void arq__recv_frame_rst(arq__recv_frame_t *f);
unsigned arq__recv_frame_fill(arq__recv_frame_t *f, void const *src, unsigned len);
int arq__recv_poll(arq__recv_wnd_t *rw,
                   arq__recv_frame_t *rf,
                   arq__recv_wnd_ptr_t *rp,
                   arq_checksum_t checksum,
                   arq__frame_hdr_t *sh,
                   arq__frame_hdr_t *rh);

typedef struct arq_t
{
    arq_cfg_t cfg;
    arq_stats_t stats;
    arq_state_t state;
    arq__send_wnd_t send_wnd;
    arq__send_wnd_ptr_t send_wnd_ptr;
    arq__send_frame_t send_frame;
    arq__recv_wnd_t recv_wnd;
    arq__recv_wnd_ptr_t recv_wnd_ptr;
    arq__recv_frame_t recv_frame;
} arq_t;

unsigned arq__min(unsigned x, unsigned y);
unsigned arq__max(unsigned x, unsigned y);
arq_uint32_t arq__sub_sat(arq_uint32_t x, arq_uint32_t y);
arq_uint16_t arq__hton16(arq_uint16_t x);
arq_uint16_t arq__ntoh16(arq_uint16_t x);
arq_uint32_t arq__hton32(arq_uint32_t x);
arq_uint32_t arq__ntoh32(arq_uint32_t x);

#if defined(__cplusplus)
}
#endif
#endif /* ARQ_H_INCLUDED */

#ifdef ARQ_IMPLEMENTATION

#ifdef ARQ_IMPLEMENTATION_INCLUDED
    #error arq.h already #included with ARQ_IMPLEMENTATION_INCLUDED #defined
#endif

#define ARQ_IMPLEMENTATION_INCLUDED

#ifndef ARQ_ASSERTS_ENABLED
    #error You must define ARQ_ASSERTS_ENABLED to 0 or 1 before including arq.h with ARQ_IMPLEMENTATION
#endif
#ifndef ARQ_LITTLE_ENDIAN_CPU
    #error You must define ARQ_LITTLE_ENDIAN_CPU to 0 or 1 before including arq.h with ARQ_IMPLEMENTATION
#endif

#if ARQ_USE_C_STDLIB == 1
    #include <stdint.h>
    #include <stddef.h>
    #include <string.h>
    #define ARQ_UINTPTR_TYPE uintptr_t
    #define ARQ_NULL_PTR NULL
    #define ARQ_MEMCPY(DST, SRC, LEN) memcpy((DST), (SRC), (unsigned)(LEN))
#else
    #ifndef ARQ_UINTPTR_TYPE
        #error You must define ARQ_UINTPTR_TYPE before including arq.h with ARQ_IMPLEMENTATION
    #endif
    #ifndef ARQ_NULL_PTR
        #error You must define ARQ_NULL_PTR before including arq.h with ARQ_IMPLEMENTATION
    #endif
    #ifndef ARQ_MEMCPY
        #error You must define ARQ_MEMCPY before including arq.h with ARQ_IMPLEMENTATION
    #endif
#endif

#define ARQ_COUNT_TRAILING_ZERO_BITS(x) __builtin_ctz(x)

typedef ARQ_UINTPTR_TYPE arq_uintptr_t;

#if ARQ_ASSERTS_ENABLED == 1
    static arq_assert_cb_t s_assert_cb = ARQ_NULL_PTR;
    #define ARQ_ASSERT(COND) \
        do { if (__builtin_expect(!(COND), 0)) { s_assert_cb(__FILE__, __LINE__, #COND, ""); } } while (0)
    #define ARQ_ASSERT_FAIL() s_assert_cb(__FILE__, __LINE__, "", "explicit assert")
#else
    #define ARQ_ASSERT(COND) (void)sizeof(COND)
    #define ARQ_ASSERT_FAIL()
#endif

arq_err_t arq_assert_handler_set(arq_assert_cb_t assert_cb)
{
    (void)assert_cb;
#if ARQ_ASSERTS_ENABLED == 1
    if (!assert_cb) {
        return ARQ_ERR_INVALID_PARAM;
    }
    s_assert_cb = assert_cb;
#endif
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_assert_handler_get(arq_assert_cb_t *out_assert_cb)
{
#if ARQ_ASSERTS_ENABLED == 1
    if (!out_assert_cb) {
        return ARQ_ERR_INVALID_PARAM;
    }
    *out_assert_cb = s_assert_cb;
#else
    *out_assert_cb = ARQ_NULL_PTR;
#endif
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_required_size(arq_cfg_t const *cfg, int *out_required_size)
{
    (void)cfg;
    *out_required_size = 0;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_connect(struct arq_t *arq)
{
    (void)arq;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_recv(struct arq_t *arq, void *recv, int recv_max, int *out_recv_size)
{
    if (!arq || !recv || !out_recv_size) {
        return ARQ_ERR_INVALID_PARAM;
    }
    *out_recv_size = (int)arq__recv_wnd_recv(&arq->recv_wnd, recv, (unsigned)recv_max);
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_send(struct arq_t *arq, void const *send, int send_max, int *out_sent_size)
{
    if (!arq || !send || !out_sent_size || (send_max < 0)) {
        return ARQ_ERR_INVALID_PARAM;
    }
    *out_sent_size = (int)arq__send_wnd_send(&arq->send_wnd, send, (unsigned)send_max);
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_flush(struct arq_t *arq)
{
    if (!arq) {
        return ARQ_ERR_INVALID_PARAM;
    }
    arq__send_wnd_flush(&arq->send_wnd);
    return ARQ_OK_COMPLETED;
}

int ARQ_MOCKABLE(arq__recv_poll)(arq__recv_wnd_t *rw,
                                 arq__recv_frame_t *rf,
                                 arq__recv_wnd_ptr_t *rp,
                                 arq_checksum_t checksum,
                                 arq__frame_hdr_t *sh,
                                 arq__frame_hdr_t *rh)
{
    ARQ_ASSERT(rw && rf && rp && checksum && rh);
    if (rf->state == ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT) {
        void const *seg;
        arq__frame_read_result_t const ok = arq__frame_read(rf->buf, rf->len, checksum, rh, &seg);
        arq__recv_frame_rst(rf);
        if ((ok == ARQ__FRAME_READ_RESULT_SUCCESS) && rh->seg) {
            arq__recv_wnd_frame(rw,
                                (unsigned)rh->seq_num,
                                (unsigned)rh->seg_id,
                                (unsigned)rh->msg_len,
                                seg,
                                (unsigned)rh->seg_len);
        }
    }
    if (sh) {
        arq__recv_wnd_ptr_next(rp, rw);
        if (rp->valid) {
            sh->ack = 1;
            sh->ack_num = (int)rp->seq;
            sh->cur_ack_vec = rp->cur_ack_vec;
            if (rp->pending) {
                return 1;
            }
        }
    }
    return 0;
}

int ARQ_MOCKABLE(arq__send_poll)(arq__send_wnd_t *sw,
                                 arq__send_frame_t *sf,
                                 arq__send_wnd_ptr_t *sp,
                                 arq__frame_hdr_t *sh,
                                 arq__frame_hdr_t const *rh,
                                 arq_time_t dt,
                                 arq_time_t rtx)
{
    ARQ_ASSERT(sw && sf && sp && rh);
    if (rh->ack) {
        arq__send_wnd_ack(sw, (unsigned)rh->ack_num, rh->cur_ack_vec);
    }
    arq__send_wnd_step(sw, dt);
    if (sh) {
        unsigned const p_seq = sp->seq;
        if (arq__send_wnd_ptr_next(sp, sw) == ARQ__SEND_WND_PTR_NEXT_COMPLETED_MSG) {
            sw->rtx[p_seq % sw->w.cap] = rtx;
        }
        if (sp->valid) {
            arq__msg_t const *m = &sw->w.msg[sp->seq % sw->w.cap];
            sh->msg_len = (m->len + sw->w.seg_len - 1) / sw->w.seg_len;
            sh->seq_num = sp->seq;
            sh->seg_id = sp->seg;
            sh->seg = 1;
        }
        return sh->seg;
    }
    return 0;
}

arq_err_t arq_backend_poll(struct arq_t *arq,
                           arq_time_t dt,
                           int *out_backend_send_size,
                           arq_event_t *out_event,
                           arq_time_t *out_next_poll)
{
    arq__frame_hdr_t sh, rh, *psh = ARQ_NULL_PTR;
    int emit = 0;
    if (!arq || !out_backend_send_size || !out_event || !out_next_poll) {
        return ARQ_ERR_INVALID_PARAM;
    }
    if ((arq->send_frame.len == 0) && (arq->send_frame.state != ARQ__SEND_FRAME_STATE_HELD)) {
        arq__frame_hdr_init(&sh);
        psh = &sh;
    }
    arq__frame_hdr_init(&rh);
    emit |= arq__recv_poll(&arq->recv_wnd, &arq->recv_frame, &arq->recv_wnd_ptr, arq->cfg.checksum, psh, &rh);
    emit |= arq__send_poll(&arq->send_wnd,
                           &arq->send_frame,
                           &arq->send_wnd_ptr,
                           psh,
                           &rh,
                           dt,
                           arq->cfg.retransmission_timeout);
    if (psh && emit) {
        void *seg = ARQ_NULL_PTR;
        if (sh.seg) {
            arq__wnd_seg(&arq->send_wnd.w,
                         (unsigned)sh.seq_num,
                         (unsigned)sh.seg_id,
                         &seg,
                         &sh.seg_len);
            ARQ_ASSERT(sh.seg_len);
        }
        arq->send_frame.len =
            arq__frame_write(&sh, seg, arq->cfg.checksum, arq->send_frame.buf, arq->send_frame.cap);
        arq->send_frame.state = ARQ__SEND_FRAME_STATE_FREE;
    }
    *out_backend_send_size = arq->send_frame.len;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_backend_send_ptr_get(struct arq_t *arq, void const **out_send, int *out_send_size)
{
    if (!arq || !out_send || !out_send_size) {
        return ARQ_ERR_INVALID_PARAM;
    }
    if (arq->send_frame.len) {
        arq->send_frame.state = ARQ__SEND_FRAME_STATE_HELD;
        *out_send = arq->send_frame.buf;
        *out_send_size = arq->send_frame.len;
    } else {
        arq->send_frame.state = ARQ__SEND_FRAME_STATE_FREE;
        *out_send = ARQ_NULL_PTR;
        *out_send_size = 0;
    }
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_backend_send_ptr_release(struct arq_t *arq)
{
    if (!arq) {
        return ARQ_ERR_INVALID_PARAM;
    }
    if (arq->send_frame.state != ARQ__SEND_FRAME_STATE_HELD) {
        return ARQ_ERR_SEND_PTR_NOT_HELD;
    }
    arq->send_frame.len = 0;
    arq->send_frame.state = ARQ__SEND_FRAME_STATE_RELEASED;
    return ARQ_OK_COMPLETED;
}

arq_err_t arq_backend_recv_fill(struct arq_t *arq, void const *recv, int recv_max, int *out_recv_size)
{
    if (!arq || !recv || !out_recv_size) {
        return ARQ_ERR_INVALID_PARAM;
    }
    *out_recv_size = (int)arq__recv_frame_fill(&arq->recv_frame, recv, (unsigned)recv_max);
    return (arq->recv_frame.state == ARQ__RECV_FRAME_STATE_ACCUMULATING) ?
        ARQ_OK_COMPLETED : ARQ_OK_POLL_REQUIRED;
}

void arq__lin_alloc_init(arq__lin_alloc_t *a, void *base, int capacity)
{
    ARQ_ASSERT(a && base && (capacity > 0));
    a->base = (arq_uchar_t *)base;
    a->top = (arq_uchar_t *)base;
    a->capacity = capacity;
}

void *arq__lin_alloc_alloc(arq__lin_alloc_t *a, int size, int align)
{
    arq_uchar_t *p =
        (arq_uchar_t *)(((arq_uintptr_t)a->top + ((arq_uintptr_t)align - 1)) & ~((arq_uintptr_t)align - 1));
    arq_uchar_t *new_top = p + size;
    int new_size = new_top - a->base;
    ARQ_ASSERT(a && (size > 0) && (align <= 32) && (align > 0) && ((align & (align - 1)) == 0));
    ARQ_ASSERT(new_size <= a->capacity);
    if (new_size > a->capacity) {
        return ARQ_NULL_PTR;
    }
    a->top = new_top;
    return p;
}

arq_uint16_t arq__hton16(arq_uint16_t x)
{
#if ARQ_LITTLE_ENDIAN_CPU == 1
    return (x << 8) | (x >> 8);
#else
    return x;
#endif
}

arq_uint16_t arq__ntoh16(arq_uint16_t x)
{
    return arq__hton16(x);
}

arq_uint32_t ARQ_MOCKABLE(arq__hton32)(arq_uint32_t x)
{
#if ARQ_LITTLE_ENDIAN_CPU == 1
    return (x >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24);
#else
    return x;
#endif
}

arq_uint32_t ARQ_MOCKABLE(arq__ntoh32)(arq_uint32_t x)
{
    return arq__hton32(x);
}

void ARQ_MOCKABLE(arq__frame_hdr_read)(void const *buf, arq__frame_hdr_t *out_frame_hdr)
{
    arq_uchar_t const *src = (arq_uchar_t const *)buf;
    arq_uint16_t tmp_n;
    arq_uchar_t *dst = (arq_uchar_t *)&tmp_n;
    ARQ_ASSERT(buf && out_frame_hdr);
    out_frame_hdr->version = *src++;                    /* version */
    out_frame_hdr->seg_len = *src++;                    /* seg_len */
    out_frame_hdr->fin = !!(*src & (1 << 0));           /* flags */
    out_frame_hdr->rst = !!(*src & (1 << 1));
    out_frame_hdr->ack = !!(*src & (1 << 2));
    out_frame_hdr->seg = !!(*src++ & (1 << 3));
    out_frame_hdr->win_size = *src++;                   /* win_size */
    dst[0] = src[0] >> 4;
    dst[1] = (src[0] << 4) | (src[1] >> 4);
    out_frame_hdr->seq_num = arq__ntoh16(tmp_n);        /* seq_num */
    ++src;
    out_frame_hdr->msg_len = (arq_uchar_t)((src[0] << 4) | (src[1] >> 4)); /* msg_len */
    ++src;
    dst[0] = src[0] & 0x0F;                             /* seg_id */
    dst[1] = src[1];
    out_frame_hdr->seg_id = arq__ntoh16(tmp_n);
    src += 2;
    dst[0] = src[0] >> 4;                               /* ack_num */
    dst[1] = (src[0] << 4) | (src[1] >> 4);
    out_frame_hdr->ack_num = arq__ntoh16(tmp_n);
    src += 2;
    dst[0] = src[0] & 0x0F;                             /* cur_ack_vec */
    dst[1] = src[1];
    out_frame_hdr->cur_ack_vec = arq__ntoh16(tmp_n);
    src += 2;
    ARQ_ASSERT((src - (arq_uchar_t const *)buf) == ARQ__FRAME_HEADER_SIZE);
}

void ARQ_MOCKABLE(arq__frame_hdr_init)(arq__frame_hdr_t *h)
{
    h->version = 0;
    h->seg_len = 0;
    h->win_size = 0;
    h->seq_num = 0;
    h->msg_len = 0;
    h->seg_id = 0;
    h->ack_num = 0;
    h->cur_ack_vec = 0;
    h->rst = 0;
    h->fin = 0;
    h->seg = 0;
    h->ack = 0;
}

int ARQ_MOCKABLE(arq__frame_hdr_write)(arq__frame_hdr_t const *h, void *out_buf)
{
    arq_uchar_t *dst = (arq_uchar_t *)out_buf;
    arq_uint16_t tmp_n;
    arq_uchar_t const *src = (arq_uchar_t const *)&tmp_n;
    ARQ_ASSERT(h && out_buf && ((h->cur_ack_vec & 0xF000) == 0));
    *dst++ = (arq_uchar_t)h->version;                          /* version */
    *dst++ = (arq_uchar_t)h->seg_len;                          /* seg_len */
    *dst++ = (!!h->fin) | ((!!h->rst) << 1) | ((!!h->ack) << 2) | ((!!h->seg) << 3); /* flags */
    *dst++ = (arq_uchar_t)h->win_size;                         /* win_size */
    tmp_n = arq__hton16((arq_uint16_t)h->seq_num);             /* seq_num + msg_len */
    *dst++ = (src[0] << 4) | (src[1] >> 4);
    *dst++ = (src[1] << 4) | (arq_uchar_t)(h->msg_len >> 4);
    tmp_n = arq__hton16((arq_uint16_t)h->seg_id);              /* msg_len + seg_id */
    *dst++ = (arq_uchar_t)(h->msg_len << 4) | (src[0] & 0x0F);
    *dst++ = src[1];
    tmp_n = arq__hton16((arq_uint16_t)h->ack_num);             /* ack_num */
    *dst++ = (src[0] << 4) | (src[1] >> 4);
    *dst++ = src[1] << 4;
    tmp_n = arq__hton16(h->cur_ack_vec);                       /* cur_ack_vec */
    *dst++ = src[0] & 0x0F;
    *dst++ = src[1];
    ARQ_ASSERT((dst - (arq_uchar_t const *)out_buf) == ARQ__FRAME_HEADER_SIZE);
    return dst - (arq_uchar_t const *)out_buf;
}

int ARQ_MOCKABLE(arq__frame_seg_write)(void const *seg, void *out_buf, int len)
{
    ARQ_ASSERT((len && seg && out_buf) || (!seg && (len == 0)) || (len == 0));
    ARQ_MEMCPY(out_buf, seg, len);
    return len;
}

int ARQ_MOCKABLE(arq__frame_len)(int seg_len)
{
    ARQ_ASSERT(seg_len <= 256);
    return ARQ__FRAME_COBS_OVERHEAD + ARQ__FRAME_HEADER_SIZE + seg_len + 4;
}

int ARQ_MOCKABLE(arq__frame_write)(arq__frame_hdr_t const *h,
                                   void const *seg,
                                   arq_checksum_t checksum,
                                   void *out_frame,
                                   int frame_max)
{
    arq_uchar_t *dst = (arq_uchar_t *)out_frame + 1;
    int const frame_len = arq__frame_len(h->seg_len);
    ARQ_ASSERT(h && out_frame);
    ARQ_ASSERT((seg || (h->seg_len == 0)) && (frame_max >= frame_len));
    dst += arq__frame_hdr_write(h, dst);
    dst += arq__frame_seg_write(seg, dst, h->seg_len);
    arq__frame_checksum_write(checksum, dst, out_frame, h->seg_len);
    arq__cobs_encode(out_frame, frame_len);
    return frame_len;
}

void ARQ_MOCKABLE(arq__frame_checksum_write)(arq_checksum_t checksum,
                                             void *checksum_seat,
                                             void *frame,
                                             int seg_len)
{
    arq_uint32_t c;
    ARQ_ASSERT(checksum && checksum_seat && frame);
    c = arq__hton32(checksum((arq_uchar_t const *)frame + 1, ARQ__FRAME_HEADER_SIZE + seg_len));
    ARQ_MEMCPY(checksum_seat, &c, 4);
}

arq__frame_read_result_t ARQ_MOCKABLE(arq__frame_read)(void *frame,
                                                       int frame_len,
                                                       arq_checksum_t checksum,
                                                       arq__frame_hdr_t *out_hdr,
                                                       void const **out_seg)
{
    arq_uchar_t const *h = (arq_uchar_t const *)frame + 1;
    ARQ_ASSERT(frame && out_hdr && out_seg);
    arq__cobs_decode(frame, frame_len);
    arq__frame_hdr_read(h, out_hdr);
    *out_seg = (void const *)(h + ARQ__FRAME_HEADER_SIZE);
    return arq__frame_checksum_read(frame, frame_len, out_hdr->seg_len, checksum);
}

arq__frame_read_result_t ARQ_MOCKABLE(arq__frame_checksum_read)(void const *frame,
                                                                int frame_len,
                                                                int seg_len,
                                                                arq_checksum_t checksum)
{
    arq_uint32_t frame_checksum, computed_checksum;
    arq_uchar_t const *src;
    ARQ_ASSERT(frame && checksum);
    if (arq__frame_len(seg_len) > frame_len) {
        return ARQ__FRAME_READ_RESULT_ERR_MALFORMED;
    }
    src = (arq_uchar_t const *)frame + 1 + ARQ__FRAME_HEADER_SIZE + seg_len;
    ARQ_MEMCPY(&frame_checksum, src, 4);
    computed_checksum = checksum((arq_uchar_t const *)frame + 1, ARQ__FRAME_HEADER_SIZE + seg_len);
    frame_checksum = arq__ntoh32(frame_checksum);
    if (frame_checksum != computed_checksum) {
        return ARQ__FRAME_READ_RESULT_ERR_CHECKSUM;
    }
    return ARQ__FRAME_READ_RESULT_SUCCESS;
}

void ARQ_MOCKABLE(arq__cobs_encode)(void *p, int len)
{
    arq_uchar_t *patch = (arq_uchar_t *)p;
    arq_uchar_t *c = (arq_uchar_t *)p + 1;
    arq_uchar_t const *e = (arq_uchar_t const *)p + (len - 1);
    ARQ_ASSERT(p && (len >= 3) && (len <= 256));
    while (c < e) {
        if (*c == 0) {
            *patch = (arq_uchar_t)(c - patch);
            patch = c;
        }
        ++c;
    }
    *patch = (arq_uchar_t)(c - patch);
    *c = 0;
}

void ARQ_MOCKABLE(arq__cobs_decode)(void *p, int len)
 {
    arq_uchar_t *c = (arq_uchar_t *)p;
    arq_uchar_t const *e = c + (len - 1);
    ARQ_ASSERT(p && (len >= 3) && (len <= 256));
    while (c < e) {
        arq_uchar_t *next = c + *c;
        ARQ_ASSERT(c != next);
        *c = 0;
        c = next;
    }
}

unsigned arq__min(unsigned x, unsigned y)
{
    return (x < y) ? x : y;
}

unsigned arq__max(unsigned x, unsigned y)
{
    return (x > y) ? x : y;
}

arq_uint32_t arq__sub_sat(arq_uint32_t x, arq_uint32_t y)
{
    return (x > y) ? (x - y) : 0;
}

void ARQ_MOCKABLE(arq__wnd_init)(arq__wnd_t *w, int wnd_cap, int msg_len, int seg_len)
{
    ARQ_ASSERT(w);
    w->cap = wnd_cap;
    w->msg_len = msg_len;
    w->seg_len = seg_len;
    w->full_ack_vec = (1 << (msg_len / seg_len)) - 1;
    arq__wnd_rst(w);
}

void ARQ_MOCKABLE(arq__wnd_rst)(arq__wnd_t *w)
{
    unsigned i;
    ARQ_ASSERT(w);
    w->size = 0;
    w->seq = 0;
    for (i = 0; i < w->cap; ++i) {
        w->msg[i].len = 0;
        w->msg[i].full_ack_vec = w->full_ack_vec;
        w->msg[i].cur_ack_vec = 0;
    }
}

void ARQ_MOCKABLE(arq__wnd_seg)(arq__wnd_t *w,
                                unsigned seq,
                                unsigned seg,
                                void **out_seg,
                                int *out_seg_len)
{
    unsigned idx;
    ARQ_ASSERT(w && out_seg && out_seg_len);
    idx = seq % w->cap;
    *out_seg = &w->buf[(w->msg_len * idx) + (w->seg_len * seg)];
    *out_seg_len = (int)arq__min(w->seg_len, w->msg[idx].len - (w->seg_len * seg));
}

void ARQ_MOCKABLE(arq__send_wnd_rst)(arq__send_wnd_t *sw)
{
    unsigned i;
    ARQ_ASSERT(sw);
    arq__wnd_rst(&sw->w);
    for (i = 0; i < sw->w.cap; ++i) {
        sw->rtx[i] = 0;
    }
}

unsigned ARQ_MOCKABLE(arq__send_wnd_send)(arq__send_wnd_t *sw, void const *buf, unsigned len)
{
    unsigned last_msg_len, seq, msg_idx, cur_byte_idx, bytes_rem;
    unsigned wnd_cap_in_bytes, wnd_size_in_bytes, pre_wrap_copy_len;
    ARQ_ASSERT(sw && buf);
    seq = sw->w.seq + arq__sub_sat(sw->w.size, 1);
    msg_idx = seq % sw->w.cap;
    last_msg_len = sw->w.msg[msg_idx].len;
    wnd_size_in_bytes = (arq__sub_sat(sw->w.size, 1) * sw->w.msg_len) + last_msg_len;
    wnd_cap_in_bytes = (unsigned)sw->w.cap * sw->w.msg_len;
    len = arq__min(len, wnd_cap_in_bytes - wnd_size_in_bytes);
    cur_byte_idx = (msg_idx * sw->w.msg_len) + last_msg_len;
    pre_wrap_copy_len = arq__min(len, wnd_cap_in_bytes - cur_byte_idx);
    ARQ_MEMCPY(sw->w.buf + cur_byte_idx, buf, pre_wrap_copy_len);
    ARQ_MEMCPY(sw->w.buf, (arq_uchar_t const *)buf + pre_wrap_copy_len, len - pre_wrap_copy_len);
    bytes_rem = len;
    while (bytes_rem) {
        unsigned const cur_msg_len =
            arq__min(bytes_rem, sw->w.msg_len - (unsigned)sw->w.msg[msg_idx].len);
        sw->w.msg[msg_idx].len += cur_msg_len;
        sw->rtx[msg_idx] = 0;
        seq = (seq + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        msg_idx = seq % sw->w.cap;
        bytes_rem -= cur_msg_len;
    }
    sw->w.size = (wnd_size_in_bytes + len + (sw->w.msg_len - 1u)) / sw->w.msg_len;
    return len;
}

void ARQ_MOCKABLE(arq__send_wnd_ack)(arq__send_wnd_t *sw, unsigned seq, arq_uint16_t cur_ack_vec)
{
    unsigned ack_msg_idx, i;
    ARQ_ASSERT(sw);
    if ((sw->w.size == 0) || (sw->w.seq > seq) || ((sw->w.seq + arq__sub_sat(sw->w.size, 1)) < seq)) {
        return;
    }
    ack_msg_idx = seq % sw->w.cap;
    sw->w.msg[ack_msg_idx].cur_ack_vec = cur_ack_vec;
    for (i = 0; i < sw->w.size; ++i) {
        arq__msg_t *m = &sw->w.msg[(sw->w.seq + i) % sw->w.cap];
        if (m->cur_ack_vec != m->full_ack_vec) {
            break;
        }
        m->len = 0;
        m->cur_ack_vec = 0;
        m->full_ack_vec = sw->w.full_ack_vec;
    }
    sw->w.size -= i;
    sw->w.seq = (sw->w.seq + i) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
}

void ARQ_MOCKABLE(arq__send_wnd_flush)(arq__send_wnd_t *sw)
{
    unsigned segs, idx;
    arq__msg_t *m;
    ARQ_ASSERT(sw);
    idx = (sw->w.seq + arq__sub_sat(sw->w.size, 1)) % sw->w.cap;
    m = &sw->w.msg[idx];
    if (m->len) {
        segs = (m->len + (sw->w.seg_len - 1u)) / sw->w.seg_len;
        m->full_ack_vec = (1u << segs) - 1u;
        sw->rtx[idx] = 0;
    }
}

void ARQ_MOCKABLE(arq__send_wnd_step)(arq__send_wnd_t *sw, arq_time_t dt)
{
    unsigned i;
    ARQ_ASSERT(sw);
    for (i = 0; i < sw->w.size; ++i) {
        unsigned const idx = (sw->w.seq + i) % sw->w.cap;
        sw->rtx[idx] = arq__sub_sat(sw->rtx[idx], (arq_uint32_t)dt);
    }
}

void ARQ_MOCKABLE(arq__send_wnd_ptr_init)(arq__send_wnd_ptr_t *p)
{
    ARQ_ASSERT(p);
    p->valid = 0;
    p->seq = 0;
    p->seg = 0;
}

void arq__send_frame_init(arq__send_frame_t *f, int cap)
{
    ARQ_ASSERT(f);
    f->cap = (arq_uint16_t)cap;
    f->len = 0;
    f->state = ARQ__SEND_FRAME_STATE_FREE;
}

arq__send_wnd_ptr_next_result_t ARQ_MOCKABLE(arq__send_wnd_ptr_next)(arq__send_wnd_ptr_t *p,
                                                                     arq__send_wnd_t const *sw)
{
    unsigned i;
    arq__send_wnd_ptr_next_result_t rv = ARQ__SEND_WND_PTR_NEXT_INSIDE_MSG;
    ARQ_ASSERT(p && sw);
    if (p->valid) {
        arq__msg_t const *m = &sw->w.msg[p->seq % sw->w.cap];
        unsigned const rem = (unsigned)m->cur_ack_vec >> (p->seg + 1u);
        p->seg += (1 + ARQ_COUNT_TRAILING_ZERO_BITS(~rem));
        if (((1 << p->seg) - 1) < m->full_ack_vec) {
            return ARQ__SEND_WND_PTR_NEXT_INSIDE_MSG;
        }
        rv = ARQ__SEND_WND_PTR_NEXT_COMPLETED_MSG;
    }
    for (i = 0; i < sw->w.size; ++i) {
        unsigned const seq = (i + sw->w.seq) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        arq__msg_t const *m = &sw->w.msg[seq % sw->w.cap];
        if (p->valid && (p->seq == seq)) {
            continue;
        }
        if ((sw->rtx[seq % sw->w.cap] == 0) && (m->len > 0) && (m->cur_ack_vec < m->full_ack_vec)) {
            p->seq = seq;
            p->valid = 1;
            p->seg = ARQ_COUNT_TRAILING_ZERO_BITS(~(unsigned)m->cur_ack_vec);
            return rv;
        }
    }
    p->valid = 0;
    return rv;
}

void ARQ_MOCKABLE(arq__recv_wnd_rst)(arq__recv_wnd_t *rw)
{
    unsigned i;
    ARQ_ASSERT(rw);
    arq__wnd_rst(&rw->w);
    rw->recv_ptr = 0;
    for (i = 0; i < rw->w.cap; ++i) {
        rw->ack[i] = 0;
    }
}

unsigned ARQ_MOCKABLE(arq__recv_wnd_frame)(arq__recv_wnd_t *rw,
                                           unsigned seq,
                                           unsigned seg,
                                           unsigned seg_cnt,
                                           void const *p,
                                           unsigned len)
{
    arq__msg_t *m;
    void *seg_dst;
    unsigned const full_ack_vec = (1u << seg_cnt) - 1;
    int unused;
    ARQ_ASSERT(rw && p && (len <= rw->w.seg_len));
    if (((seq - rw->w.seq) % (ARQ__FRAME_MAX_SEQ_NUM + 1)) > rw->w.cap) {
        rw->ack[seq % rw->w.cap] = 1;
        return 0;
    }
    m = &rw->w.msg[seq % rw->w.cap];
    if (m->cur_ack_vec & (1u << seg)) {
        return 0;
    }
    arq__wnd_seg(&rw->w, seq, seg, &seg_dst, &unused);
    ARQ_MEMCPY(seg_dst, p, len);
    rw->w.size = arq__max(rw->w.size, ((seq - rw->w.seq) % (ARQ__FRAME_MAX_SEQ_NUM + 1)) + 1);
    m->full_ack_vec = full_ack_vec;
    m->cur_ack_vec |= (1u << seg);
    m->len += len;
    if (seg == seg_cnt - 1) {
        rw->ack[seq % rw->w.cap] = 1;
    }
    return len;
}

unsigned ARQ_MOCKABLE(arq__recv_wnd_recv)(arq__recv_wnd_t *rw, void *dst, unsigned dst_max)
{
    unsigned i = 0, recvd = 0, base_seq;
    ARQ_ASSERT(rw && dst);
    base_seq = rw->w.seq;
    while (dst_max && rw->w.size) {
        arq__msg_t *m = &rw->w.msg[(base_seq + i) % rw->w.cap];
        unsigned const copy_len = arq__min(dst_max, m->len - rw->recv_ptr);
        unsigned const src_idx = ((base_seq + i) % rw->w.cap) * rw->w.msg_len;
        if (m->cur_ack_vec != m->full_ack_vec) {
            break;
        }
        ARQ_MEMCPY(dst, &rw->w.buf[src_idx + rw->recv_ptr], copy_len);
        dst = (arq_uchar_t *)dst + copy_len;
        recvd += copy_len;
        dst_max -= copy_len;
        if ((rw->recv_ptr + copy_len) < m->len) {
            rw->recv_ptr += copy_len;
            break;
        }
        m->len = 0;
        m->cur_ack_vec = 0;
        m->full_ack_vec = rw->w.full_ack_vec;
        rw->recv_ptr = 0;
        --rw->w.size;
        rw->w.seq = (rw->w.seq + 1) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        ++i;
    }
    return recvd;
}

void ARQ_MOCKABLE(arq__recv_wnd_ptr_init)(arq__recv_wnd_ptr_t *p)
{
    ARQ_ASSERT(p);
    p->seq = 0;
    p->cur_ack_vec = 0;
    p->pending = 0;
    p->valid = 0;
}

void ARQ_MOCKABLE(arq__recv_wnd_ptr_next)(arq__recv_wnd_ptr_t *p, arq__recv_wnd_t const *rw)
{
    unsigned i;
    ARQ_ASSERT(rw);
    p->pending = 0;
    if (p->valid && (unsigned)(p->seq - rw->w.seq) % (ARQ__FRAME_MAX_SEQ_NUM + 1) > rw->w.cap) {
        p->seq = rw->w.seq;
    }
    for (i = 0; i < rw->w.cap; ++i) {
        unsigned const seq = (p->seq + i) % (ARQ__FRAME_MAX_SEQ_NUM + 1);
        unsigned const idx = seq % rw->w.cap;
        if (rw->ack[idx]) {
            rw->ack[idx] = 0;
            p->seq = seq;
            p->pending = 1;
            p->valid = 1;
            break;
        }
    }
    p->cur_ack_vec = rw->w.msg[p->seq % rw->w.cap].cur_ack_vec;
}

void ARQ_MOCKABLE(arq__recv_wnd_ptr_set)(arq__recv_wnd_ptr_t *p, unsigned seq, unsigned ack_vec)
{
    ARQ_ASSERT(p);
    p->seq = (arq_uint16_t)seq;
    p->cur_ack_vec = (arq_uint16_t)ack_vec;
    p->pending = 1;
}

void arq__recv_frame_init(arq__recv_frame_t *f, void *buf, int len)
{
    ARQ_ASSERT(f && buf);
    f->cap = len;
    f->buf = (arq_uchar_t *)buf;
    arq__recv_frame_rst(f);
}

void ARQ_MOCKABLE(arq__recv_frame_rst)(arq__recv_frame_t *f)
{
    ARQ_ASSERT(f);
    f->len = 0;
    f->state = ARQ__RECV_FRAME_STATE_ACCUMULATING;
}

unsigned ARQ_MOCKABLE(arq__recv_frame_fill)(arq__recv_frame_t *f, void const *src, unsigned len)
{
    arq_uchar_t const *src_bytes = (arq_uchar_t const *)src;
    arq_uchar_t *dst;
    unsigned ret;
    ARQ_ASSERT(f && src);
    if (f->state == ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT) {
        return 0;
    }
    ret = len = arq__min(len, (unsigned)f->cap - (unsigned)f->len);
    dst = f->buf + f->len;
    while (len) {
        *dst = *src_bytes;
        --len;
        if (*src_bytes == 0) {
            f->state = ARQ__RECV_FRAME_STATE_FULL_FRAME_PRESENT;
            break;
        }
        ++src_bytes;
        ++dst;
    }
    f->len += ret - len;
    return ret - len;
}

#if ARQ_COMPILE_CRC32 == 1
arq_uint32_t arq_crc32(void const *buf, int size)
{
    static arq_uint32_t const crc32_tab[] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    arq_uchar_t const *p = (arq_uchar_t const *)buf;
    arq_uint32_t crc = 0xFFFFFFFF;
    while (size--) {
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}
#endif

#endif

