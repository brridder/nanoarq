#include "functional_tests.h"

namespace {

TEST(functional, frame_serialization)
{
    arq_uchar_t seg[256];
    arq_uchar_t frame[256];

    for (auto i = 0u; i < sizeof(seg); ++i) {
        seg[i] = (arq_uchar_t)i;
    }

    for (auto i = 0; i < 239; ++i) {
        arq__frame_hdr_t write_hdr;
        arq__frame_hdr_init(&write_hdr);
        write_hdr.seg_len = i;
        auto const frame_len = arq__frame_write(&write_hdr, seg, arq_crc32, frame, sizeof(frame));

        arq__frame_hdr_t read_hdr;
        void const *read_seg;
        auto const r = arq__frame_read(frame, frame_len, arq_crc32, &read_hdr, &read_seg);
        CHECK_EQUAL(ARQ__FRAME_READ_RESULT_SUCCESS, r);
        CHECK_EQUAL(write_hdr.seg_len, read_hdr.seg_len);
        MEMCMP_EQUAL(seg, read_seg, read_hdr.seg_len);
    }
}

}

