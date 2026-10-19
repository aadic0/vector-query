#include "vector_query/data/data_generator.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace vector_query {
namespace {

GeneratorConfig valid_config() { return {1'024, 42, 17, 0.25F, 900.0F, 10, 1'000}; }

TEST(TradeColumns, EmptyColumnsAndView) {
    const TradeColumns columns;
    EXPECT_EQ(columns.size(), 0U);
    EXPECT_TRUE(columns.has_consistent_sizes());
    EXPECT_EQ(columns.view().size(), 0U);
    EXPECT_TRUE(columns.view().has_consistent_sizes());
}

TEST(TradeColumns, ViewBorrowsEqualLengthColumns) {
    TradeColumns columns{{1.0F, 2.0F}, {3, 4}, {5, 6}};
    const auto view = columns.view();
    EXPECT_EQ(columns.size(), 2U);
    EXPECT_EQ(view.size(), 2U);
    EXPECT_TRUE(columns.has_consistent_sizes());
    EXPECT_TRUE(view.has_consistent_sizes());
    EXPECT_EQ(view.prices.data(), columns.prices.data());
    EXPECT_EQ(view.volumes.data(), columns.volumes.data());
    EXPECT_EQ(view.symbol_ids.data(), columns.symbol_ids.data());
    columns.prices[0] = 7.0F;
    EXPECT_FLOAT_EQ(view.prices[0], 7.0F);
}

TEST(TradeColumns, DetectsEachMismatchedColumn) {
    const TradeColumns short_volumes{{1.0F}, {}, {0}};
    const TradeColumns short_symbols{{1.0F}, {1}, {}};
    const TradeColumns short_prices{{}, {1}, {0}};
    for (const auto* columns : {&short_volumes, &short_symbols, &short_prices}) {
        EXPECT_FALSE(columns->has_consistent_sizes());
        EXPECT_FALSE(columns->view().has_consistent_sizes());
        EXPECT_EQ(columns->size(), columns->prices.size());
    }
}

TEST(Generator, ZeroRows) {
    auto config = valid_config();
    config.row_count = 0;
    const auto columns = generate_trades(config);
    EXPECT_EQ(columns.size(), 0U);
    EXPECT_TRUE(columns.has_consistent_sizes());
}

TEST(Generator, RepeatedConfigurationIsIdentical) {
    const auto first = generate_trades(valid_config());
    const auto second = generate_trades(valid_config());
    EXPECT_EQ(first.prices, second.prices);
    EXPECT_EQ(first.volumes, second.volumes);
    EXPECT_EQ(first.symbol_ids, second.symbol_ids);
}

TEST(Generator, DifferentSeedsChangeData) {
    auto config = valid_config();
    const auto first = generate_trades(config);
    ++config.seed;
    const auto second = generate_trades(config);
    EXPECT_NE(first.prices, second.prices);
    EXPECT_NE(first.volumes, second.volumes);
    EXPECT_NE(first.symbol_ids, second.symbol_ids);
}

TEST(Generator, RowCountAndAllValuesAreWithinBounds) {
    const auto config = valid_config();
    const auto columns = generate_trades(config);
    ASSERT_EQ(columns.size(), config.row_count);
    ASSERT_TRUE(columns.has_consistent_sizes());
    for (std::size_t index = 0; index < columns.size(); ++index) {
        EXPECT_TRUE(std::isfinite(columns.prices[index]));
        EXPECT_GE(columns.prices[index], config.minimum_price);
        EXPECT_LE(columns.prices[index], config.maximum_price);
        EXPECT_GE(columns.volumes[index], config.minimum_volume);
        EXPECT_LE(columns.volumes[index], config.maximum_volume);
        EXPECT_LT(columns.symbol_ids[index], config.symbol_count);
    }
}

TEST(Generator, ConstantBoundsAndSingleSymbol) {
    auto config = valid_config();
    config.minimum_price = config.maximum_price = 3.5F;
    config.minimum_volume = config.maximum_volume = std::numeric_limits<std::uint32_t>::max();
    config.symbol_count = 1;
    const auto columns = generate_trades(config);
    for (std::size_t index = 0; index < columns.size(); ++index) {
        EXPECT_FLOAT_EQ(columns.prices[index], 3.5F);
        EXPECT_EQ(columns.volumes[index], config.maximum_volume);
        EXPECT_EQ(columns.symbol_ids[index], 0U);
    }
}

TEST(Generator, ExtremeValidBoundsRemainFiniteAndInRange) {
    auto config = valid_config();
    config.minimum_price = 0.0F;
    config.maximum_price = std::numeric_limits<float>::max();
    config.minimum_volume = 0;
    config.maximum_volume = std::numeric_limits<std::uint32_t>::max();
    config.symbol_count = std::numeric_limits<std::uint32_t>::max();
    const auto columns = generate_trades(config);
    for (std::size_t index = 0; index < columns.size(); ++index) {
        EXPECT_TRUE(std::isfinite(columns.prices[index]));
        EXPECT_GE(columns.prices[index], 0.0F);
        EXPECT_LE(columns.prices[index], config.maximum_price);
        EXPECT_LT(columns.symbol_ids[index], config.symbol_count);
    }
}

TEST(Generator, RejectsZeroSymbolsEvenForZeroRows) {
    auto config = valid_config();
    config.symbol_count = 0;
    EXPECT_THROW((void)generate_trades(config), std::invalid_argument);
    config.row_count = 0;
    EXPECT_THROW((void)generate_trades(config), std::invalid_argument);
}

TEST(Generator, RejectsReversedRanges) {
    auto config = valid_config();
    config.minimum_price = config.maximum_price + 1.0F;
    EXPECT_THROW((void)generate_trades(config), std::invalid_argument);
    config = valid_config();
    config.minimum_volume = config.maximum_volume + 1;
    EXPECT_THROW((void)generate_trades(config), std::invalid_argument);
}

TEST(Generator, RejectsNegativeAndNonFinitePriceBounds) {
    for (const float value :
         {-1.0F, std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
          std::numeric_limits<float>::quiet_NaN()}) {
        auto config = valid_config();
        config.minimum_price = value;
        EXPECT_THROW((void)generate_trades(config), std::invalid_argument);
        config = valid_config();
        config.maximum_price = value;
        EXPECT_THROW((void)generate_trades(config), std::invalid_argument);
    }
}

} // namespace
} // namespace vector_query
