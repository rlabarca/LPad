/**
 * @file test_data_tnx_5m.h
 * @brief Embedded test data for TNX 5-minute chart (1 day)
 *
 * Source: Yahoo Finance chart data for ^TNX (10-Year Treasury Note)
 * This data is embedded directly in the binary to avoid filesystem dependencies.
 */

#pragma once

#include <cstddef>

namespace TestData {

// TNX 5-minute chart data (15 data points)
constexpr size_t TNX_5M_COUNT = 15;

constexpr long TNX_5M_TIMESTAMPS[TNX_5M_COUNT] = {
    1770057900, 1770058200, 1770058500, 1770058800, 1770059100,
    1770059400, 1770059700, 1770060000, 1770060300, 1770060600,
    1770060900, 1770061200, 1770061500, 1770061800, 1770062100
};

constexpr double TNX_5M_CLOSE_PRICES[TNX_5M_COUNT] = {
    4.270999908447266,   4.2729997634887695, 4.275000095367432,
    4.275000095367432,   4.2769999504089355, 4.275000095367432,
    4.2769999504089355,  4.279000282287598,  4.279000282287598,
    4.2769999504089355,  4.2769999504089355, 4.275000095367432,
    4.2729997634887695,  4.2729997634887695, 4.275000095367432
};

} // namespace TestData
