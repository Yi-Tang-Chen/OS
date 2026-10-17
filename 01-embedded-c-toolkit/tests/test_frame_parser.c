#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "frame_parser.h"

/* CRC 預期值預先獨立計算，不呼叫被測的 crc8() 產生答案。 */
static const uint8_t valid_frame[] = {
    0xA5, 0x5A, 0x01, 0x02, 0x00, 0x07, 0x10, 0x20, 0x4C
};
static size_t received_count;

static void on_frame(void)
{
    /* 每輸出一包就加一，讓測試能檢查 callback 有沒有被呼叫。 */
    received_count++;
}

static void test_check_complete_frame(void)
{
    assert(frame_parser_check(valid_frame, sizeof(valid_frame)));
    assert(!frame_parser_check(NULL, sizeof(valid_frame)));
    for (size_t size = 0; size < sizeof(valid_frame); size++) {
        assert(!frame_parser_check(valid_frame, size));
    }
    uint8_t extra[sizeof(valid_frame) + 1] = {0};
    memcpy(extra, valid_frame, sizeof(valid_frame));
    assert(!frame_parser_check(extra, sizeof(extra)));
}

static void test_feed_complete_frame(void)
{
    frame_parser_t parser;
    received_count = 0;
    assert(frame_parser_init(&parser));
    assert(parser.used == 0);
    assert(frame_parser_feed(valid_frame, sizeof(valid_frame), &parser, on_frame));
    assert(received_count == 1);
    assert(parser.used == 0);
}

static void test_every_split_point(void)
{
    /* 包括 magic 被拆開，以及 CRC 單獨在第二批到達。 */
    for (size_t split = 1; split < sizeof(valid_frame); split++) {
        frame_parser_t parser;
        received_count = 0;
        assert(frame_parser_init(&parser));
        assert(frame_parser_feed(valid_frame, split, &parser, on_frame));
        assert(received_count == 0);
        assert(parser.used == split);
        assert(memcmp(parser.buffer, valid_frame, split) == 0);
        assert(frame_parser_feed(valid_frame + split,
                                 sizeof(valid_frame) - split, &parser, on_frame));
        assert(received_count == 1);
        assert(parser.used == 0);
    }
}

static void test_one_byte_at_a_time(void)
{
    frame_parser_t parser;
    received_count = 0;
    assert(frame_parser_init(&parser));
    for (size_t i = 0; i < sizeof(valid_frame); i++) {
        assert(frame_parser_feed(&valid_frame[i], 1, &parser, on_frame));
        assert(received_count == (i + 1 == sizeof(valid_frame) ? 1u : 0u));
    }
    assert(parser.used == 0);
}

static void test_multiple_frames_in_one_feed(void)
{
    /* 45 bytes 大於 39-byte buffer；必須逐包處理並重用空間。 */
    uint8_t stream[5 * sizeof(valid_frame)];
    for (size_t i = 0; i < 5; i++) {
        memcpy(stream + i * sizeof(valid_frame), valid_frame, sizeof(valid_frame));
    }
    frame_parser_t parser;
    received_count = 0;
    assert(frame_parser_init(&parser));
    assert(frame_parser_feed(stream, sizeof(stream), &parser, on_frame));
    assert(received_count == 5);
    assert(parser.used == 0);
}

static void test_complete_then_partial_frame(void)
{
    uint8_t stream[2 * sizeof(valid_frame)];
    memcpy(stream, valid_frame, sizeof(valid_frame));
    memcpy(stream + sizeof(valid_frame), valid_frame, sizeof(valid_frame));
    frame_parser_t parser;
    received_count = 0;
    assert(frame_parser_init(&parser));
    assert(frame_parser_feed(stream, sizeof(valid_frame) + 3, &parser, on_frame));
    assert(received_count == 1);
    assert(parser.used == 3);
    assert(memcmp(parser.buffer, valid_frame, 3) == 0);
    assert(frame_parser_feed(stream + sizeof(valid_frame) + 3,
                             sizeof(valid_frame) - 3, &parser, on_frame));
    assert(received_count == 2);
    assert(parser.used == 0);
}

static void test_payload_boundaries(void)
{
    const uint8_t empty_payload[] = {
        0xA5, 0x5A, 0x01, 0x00, 0x00, 0x07, 0x03
    };
    const uint8_t max_payload[] = {
        0xA5, 0x5A, 0x01, 0x20, 0x00, 0x07,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
        0x01
    };
    const uint8_t magic_in_payload[] = {
        0xA5, 0x5A, 0x01, 0x04, 0x00, 0x07, 0xA5, 0x5A, 0x00, 0xFF, 0xAE
    };
    frame_parser_t parser;
    received_count = 0;
    assert(frame_parser_init(&parser));
    assert(frame_parser_feed(empty_payload, sizeof(empty_payload), &parser, on_frame));
    assert(received_count == 1);
    assert(frame_parser_feed(max_payload, sizeof(max_payload), &parser, on_frame));
    assert(received_count == 2);
    assert(frame_parser_feed(magic_in_payload, sizeof(magic_in_payload), &parser, on_frame));
    assert(received_count == 3);
    assert(parser.used == 0);
}

static void test_invalid_header_rejected_early(void)
{
    const uint8_t headers[][4] = {
        {0x00, 0x5A, 0x01, 0x02}, /* 第一個 magic 錯 */
        {0xA5, 0x00, 0x01, 0x02}, /* 第二個 magic 錯 */
        {0xA5, 0x5A, 0x02, 0x02}, /* version 錯 */
        {0xA5, 0x5A, 0x01, 0x21}, /* length = 33 */
        {0xA5, 0x5A, 0x01, 0xFF}  /* length = 255 */
    };
    const size_t sizes[] = {1, 2, 3, 4, 4};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        uint8_t bad[sizeof(valid_frame)];
        memcpy(bad, valid_frame, sizeof(bad));
        memcpy(bad, headers[i], sizeof(headers[i]));
        assert(!frame_parser_check(bad, sizeof(bad)));
        frame_parser_t parser;
        received_count = 0;
        assert(frame_parser_init(&parser));
        /* 標頭一到齊就拒絕，不等待 payload。 */
        assert(!frame_parser_feed(headers[i], sizes[i], &parser, on_frame));
        assert(received_count == 0);
        assert(parser.used == 0);
        assert(frame_parser_feed(valid_frame, sizeof(valid_frame), &parser, on_frame));
        assert(received_count == 1);
    }
}

static void test_corruption_rejected(void)
{
    /* 分別更改 payload 與 CRC，兩者都應拒絕。 */
    const size_t positions[] = {6, 8};
    for (size_t i = 0; i < sizeof(positions) / sizeof(positions[0]); i++) {
        uint8_t bad[sizeof(valid_frame)];
        memcpy(bad, valid_frame, sizeof(bad));
        bad[positions[i]] ^= 1;
        assert(!frame_parser_check(bad, sizeof(bad)));
        frame_parser_t parser;
        received_count = 0;
        assert(frame_parser_init(&parser));
        assert(!frame_parser_feed(bad, sizeof(bad), &parser, on_frame));
        assert(received_count == 0);
        assert(parser.used == 0);
    }
}

static void test_error_stops_current_feed(void)
{
    /* 目前遇錯就停止；不會自動接收同一批中後面的合法封包。 */
    uint8_t stream[2 * sizeof(valid_frame)];
    memcpy(stream, valid_frame, sizeof(valid_frame));
    stream[8] ^= 1;
    memcpy(stream + sizeof(valid_frame), valid_frame, sizeof(valid_frame));
    frame_parser_t parser;
    received_count = 0;
    assert(frame_parser_init(&parser));
    assert(!frame_parser_feed(stream, sizeof(stream), &parser, on_frame));
    assert(received_count == 0);
    assert(parser.used == 0);
    /* 再次明確提供一包正確資料，才能重新接收。 */
    assert(frame_parser_feed(valid_frame, sizeof(valid_frame), &parser, on_frame));
    assert(received_count == 1);
}

static void test_reset_discards_partial_frame(void)
{
    frame_parser_t parser;
    received_count = 0;
    assert(frame_parser_init(&parser));
    assert(frame_parser_feed(valid_frame, 3, &parser, on_frame));
    assert(parser.used == 3);
    assert(frame_parser_reset(&parser));
    assert(parser.used == 0);
    assert(received_count == 0);
    assert(frame_parser_feed(valid_frame, sizeof(valid_frame), &parser, on_frame));
    assert(received_count == 1);
}

static void test_null_and_empty_input(void)
{
    frame_parser_t parser;
    received_count = 0;
    assert(!frame_parser_init(NULL));
    assert(!frame_parser_reset(NULL));
    assert(frame_parser_init(&parser));
    assert(!frame_parser_feed(valid_frame, sizeof(valid_frame), NULL, on_frame));
    assert(!frame_parser_feed(NULL, 1, &parser, on_frame));
    /* 目前契約：即使長度為 0，NULL data 仍回傳 false。 */
    assert(!frame_parser_feed(NULL, 0, &parser, on_frame));
    assert(frame_parser_feed(valid_frame, 3, &parser, on_frame));
    assert(frame_parser_feed(valid_frame, 0, &parser, on_frame));
    assert(parser.used == 3);
    assert(received_count == 0);
    assert(frame_parser_reset(&parser));
    /* NULL callback 表示不通知，仍接收與驗證封包。 */
    assert(frame_parser_feed(valid_frame, sizeof(valid_frame), &parser, NULL));
    assert(parser.used == 0);
    assert(received_count == 0);
}

int main(void)
{
    test_check_complete_frame();
    test_feed_complete_frame();
    test_every_split_point();
    test_one_byte_at_a_time();
    test_multiple_frames_in_one_feed();
    test_complete_then_partial_frame();
    test_payload_boundaries();
    test_invalid_header_rejected_early();
    test_corruption_rejected();
    test_error_stops_current_feed();
    test_reset_discards_partial_frame();
    test_null_and_empty_input();
    puts("frame_parser tests passed (12 test functions)");
    return 0;
}
