/**
 * @file yahoo_chart_parser.h
 * @brief Yahoo Chart Data Parser
 *
 * This module parses JSON data from the Yahoo Chart API and extracts
 * time-series data (timestamps and closing prices) into simple, usable structures.
 *
 * See features/data_yahoo_chart_parser.md for complete specification.
 */

#ifndef YAHOO_CHART_PARSER_H
#define YAHOO_CHART_PARSER_H

#include <string>
#include <vector>

/**
 * @class YahooChartParser
 * @brief Parses Yahoo Chart API JSON data from file or buffer
 *
 * This class reads Yahoo Chart API responses and extracts:
 * - Timestamps (Unix epoch times)
 * - Closing prices
 *
 * The parser can handle data from files or (in future) from string buffers.
 */
class YahooChartParser {
public:
    /**
     * @brief Constructs a parser for the specified file
     * @param filePath Path to the JSON file containing Yahoo Chart data
     */
    explicit YahooChartParser(const std::string& filePath);

    /**
     * @brief Parses the JSON data from file and extracts time-series information
     * @return true if parsing succeeded, false on error (file not found, invalid JSON, etc.)
     */
    bool parse();

    /**
     * @brief Parses JSON data from a string buffer
     * @param jsonString The JSON string to parse
     * @return true if parsing succeeded, false on error (invalid JSON, etc.)
     */
    bool parseFromString(const std::string& jsonString);

    /**
     * @brief Gets the extracted timestamps
     * @return Vector of Unix epoch timestamps
     */
    const std::vector<long>& getTimestamps() const;

    /**
     * @brief Gets the extracted closing prices
     * @return Vector of closing prices (same length as timestamps)
     */
    const std::vector<double>& getClosePrices() const;

private:
    std::string filePath_;
    std::vector<long> timestamps_;
    std::vector<double> closePrices_;
};

#endif // YAHOO_CHART_PARSER_H
