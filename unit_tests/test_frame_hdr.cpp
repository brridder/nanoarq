#include "nanoarq_in_test_project.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(frame_hdr) {};

namespace
{

struct Fixture
{
    enum
    {
        Version = 123,
        SegmentLen = 44,
        WindowSize = 19,
        SequenceNumber = 0x5AF, // 0101 1010 1111
        MessageLen = 0xC3,      // 1100 0011
        SegmentID = 0xA71,      // 1010 0111 0001
    };

    Fixture()
    {
        uint8_t *src = buf;
        *src++ = (uint8_t)Version;
        *src++ = (uint8_t)SegmentLen;
        src++; // flags
        *src++ = (uint8_t)WindowSize;
        *src++ = 0x5A;
        *src++ = 0xFC;
        *src++ = 0x3A;
        *src++ = 0x71;

         arq__frame_hdr_read(&buf, &h);
    }

    arq__frame_hdr_t h;
    uint8_t buf[32];
};

TEST(frame_hdr, read_version)
{
    Fixture f;
    CHECK_EQUAL(Fixture::Version, f.h.version);
}

TEST(frame_hdr, read_segment_length)
{
    Fixture f;
    CHECK_EQUAL(Fixture::SegmentLen, f.h.seg_len);
}

TEST(frame_hdr, read_rst_flag)
{
    Fixture f;
    f.buf[2] = 1 << 1;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.rst);

    f.buf[2] = 0;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.rst);
}

TEST(frame_hdr, read_fin_flag)
{
    Fixture f;
    f.buf[2] = 1;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(1, (int)f.h.fin);

    f.buf[2] = 0;
    arq__frame_hdr_read(&f.buf, &f.h);
    CHECK_EQUAL(0, (int)f.h.fin);
}

TEST(frame_hdr, read_window_size)
{
    Fixture f;
    CHECK_EQUAL(Fixture::WindowSize, f.h.win_size);
}

TEST(frame_hdr, read_sequence_number)
{
    Fixture f;
    CHECK_EQUAL(Fixture::SequenceNumber, f.h.seq_num);
}

TEST(frame_hdr, read_message_length)
{
    Fixture f;
    CHECK_EQUAL(Fixture::MessageLen, f.h.msg_len);
}

TEST(frame_hdr, read_segment_id)
{
    Fixture f;
    CHECK_EQUAL(Fixture::SegmentID, f.h.seg_id);
}

}

