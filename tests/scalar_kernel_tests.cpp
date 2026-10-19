#include "vector_query/kernels/scalar_kernel.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace vector_query {
namespace {

// For nonnegative inputs, n*epsilon/(1-n*epsilon) bounds sequential summation
// relative error. A factor of 4 allows reference rounding; max(1, sum) gives
// a small absolute floor near zero. The reference sums the stored floats.
void expect_result(PartialResult actual, std::uint64_t count, double sum) {
    const double scaled_epsilon = static_cast<double>(std::max(count, std::uint64_t{1})) *
                                  std::numeric_limits<double>::epsilon();
    const double tolerance =
        4.0 * scaled_epsilon / (1.0 - scaled_epsilon) * std::max(1.0, std::abs(sum));
    EXPECT_EQ(actual.match_count, count);
    EXPECT_NEAR(actual.price_sum, sum, tolerance);
}

TradeColumns hand_data() {
    return {{1.25F, 2.5F, 4.0F, 8.0F, 16.5F}, {11, 10, 12, 9, 99}, {7, 7, 8, 7, 7}};
}

TEST(ScalarKernel, FullEmptyDataset) {
    const TradeColumns columns;
    expect_result(execute_scalar(columns.view(), {0, 0}, 0, 0), 0, 0.0);
}

TEST(ScalarKernel, EmptyRangesIncludingAtEnd) {
    const auto columns = hand_data();
    for (std::size_t index = 0; index <= columns.size(); ++index) {
        expect_result(execute_scalar(columns.view(), {10, 7}, index, index), 0, 0.0);
    }
}

TEST(ScalarKernel, HandCalculatedFewerThanEightRowsFirstAndLastMatch) {
    const auto columns = hand_data();
    expect_result(execute_scalar(columns.view(), {10, 7}, 0, columns.size()), 2, 17.75);
}

TEST(ScalarKernel, NoMatchingRows) {
    const auto columns = hand_data();
    expect_result(execute_scalar(columns.view(), {100, 99}, 0, columns.size()), 0, 0.0);
}

TEST(ScalarKernel, EveryRowMatches) {
    const TradeColumns columns{{0.0F, 0.5F, 2.25F}, {11, 12, 13}, {7, 7, 7}};
    expect_result(execute_scalar(columns.view(), {10, 7}, 0, columns.size()), 3, 2.75);
}

TEST(ScalarKernel, OnlyVolumePredicateMatches) {
    const TradeColumns columns{{1.0F, 2.0F}, {11, 12}, {8, 9}};
    expect_result(execute_scalar(columns.view(), {10, 7}, 0, columns.size()), 0, 0.0);
}

TEST(ScalarKernel, OnlySymbolPredicateMatchesAndVolumeEqualityIsExcluded) {
    const TradeColumns columns{{1.0F, 2.0F}, {9, 10}, {7, 7}};
    expect_result(execute_scalar(columns.view(), {10, 7}, 0, columns.size()), 0, 0.0);
}

TEST(ScalarKernel, NonzeroSubrangeExcludesEnd) {
    const auto columns = hand_data();
    expect_result(execute_scalar(columns.view(), {9, 7}, 1, 4), 1, 2.5);
}

TEST(ScalarKernel, SubrangeEndingAtSize) {
    const auto columns = hand_data();
    expect_result(execute_scalar(columns.view(), {10, 7}, 2, columns.size()), 1, 16.5);
}

TEST(ScalarKernel, RejectsReversedRange) {
    const auto columns = hand_data();
    EXPECT_THROW((void)execute_scalar(columns.view(), {10, 7}, 3, 2), std::out_of_range);
}

TEST(ScalarKernel, RejectsEndBeyondSizeAndEmptyRangeBeyondSize) {
    const auto columns = hand_data();
    EXPECT_THROW((void)execute_scalar(columns.view(), {10, 7}, 0, columns.size() + 1),
                 std::out_of_range);
    EXPECT_THROW((void)execute_scalar(columns.view(), {10, 7}, 6, 6), std::out_of_range);
}

TEST(ScalarKernel, RejectsEachMismatchedColumnBeforeProcessingEvenEmptyRange) {
    for (int column = 0; column < 3; ++column) {
        auto columns = hand_data();
        if (column == 0)
            columns.prices.pop_back();
        if (column == 1)
            columns.volumes.pop_back();
        if (column == 2)
            columns.symbol_ids.pop_back();
        EXPECT_THROW((void)execute_scalar(columns.view(), {10, 7}, 0, 1), std::invalid_argument);
        EXPECT_THROW((void)execute_scalar(columns.view(), {10, 7}, 0, 0), std::invalid_argument);
    }
}

TEST(ScalarKernel, UnsignedVolumeAndSymbolExtremes) {
    const auto maximum = std::numeric_limits<std::uint32_t>::max();
    const TradeColumns columns{
        {1.0F, 2.0F, 4.0F}, {0, maximum - 1, maximum}, {maximum, maximum, maximum}};
    expect_result(execute_scalar(columns.view(), {maximum - 1, maximum}, 0, 3), 1, 4.0);
    expect_result(execute_scalar(columns.view(), {maximum, maximum}, 0, 3), 0, 0.0);
}

TEST(ScalarKernel, AccumulatesStoredFloatsInDouble) {
    // A float accumulator would lose the unit contributions after 2^24.
    const TradeColumns columns{
        {16'777'216.0F, 1.0F, 1.0F, 0.1F, 0.2F}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 0}};
    long double reference = 0;
    for (const float price : columns.prices)
        reference += static_cast<long double>(price);
    expect_result(execute_scalar(columns.view(), {0, 0}, 0, columns.size()), 5,
                  static_cast<double>(reference));
}

} // namespace
} // namespace vector_query
