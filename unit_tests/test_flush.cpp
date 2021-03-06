#include "arq_in_unit_tests.h"
#include "arq_runtime_mock_plugin.h"
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <cstring>

TEST_GROUP(flush) {};

namespace {

TEST(flush, invalid_params)
{
    CHECK_EQUAL(ARQ_ERR_INVALID_PARAM, arq_flush(nullptr));
}

void MockSendWndFlush(arq__send_wnd_t *sw)
{
    mock().actualCall("arq__send_wnd_flush").withParameter("sw", sw);
}

TEST(flush, forwards_send_window_to_send_wnd_flush)
{
    arq_t arq;
    ARQ_MOCK_HOOK(arq__send_wnd_flush, MockSendWndFlush);
    mock().expectOneCall("arq__send_wnd_flush").withParameter("sw", &arq.send_wnd);
    arq_flush(&arq);
}

TEST(flush, returns_success)
{
    arq_t arq;
    ARQ_MOCK_HOOK(arq__send_wnd_flush, MockSendWndFlush);
    mock().ignoreOtherCalls();
    arq_err_t const e = arq_flush(&arq);
    CHECK_EQUAL(ARQ_OK_COMPLETED, e);
}

}

