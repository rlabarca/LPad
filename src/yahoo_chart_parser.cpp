/**
 * @file yahoo_chart_parser.cpp
 * @brief Implementation of Yahoo Chart Data Parser
 */

#include "yahoo_chart_parser.h"
#include <ArduinoJson.h>
#include <fstream>
#include <sstream>

YahooChartParser::YahooChartParser(const std::string& filePath)
    : filePath_(filePath) {
}

bool YahooChartParser::parse() {
    // Clear any previous data
    timestamps_.clear();
    closePrices_.clear();

    // Read the file
    std::ifstream file(filePath_);
    if (!file.is_open()) {
        return false;
    }

    // Read file content into string
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string jsonContent = buffer.str();

    // Parse JSON using ArduinoJson
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonContent);

    if (error) {
        return false;
    }

    // Navigate to the data according to Yahoo Chart API structure:
    // chart.result[0].timestamp
    // chart.result[0].indicators.quote[0].close

    if (!doc["chart"].is<JsonObject>()) {
        return false;
    }

    JsonObject chart = doc["chart"];
    if (!chart["result"].is<JsonArray>()) {
        return false;
    }

    JsonArray result = chart["result"];
    if (result.size() == 0) {
        return false;
    }

    JsonObject firstResult = result[0];

    // Extract timestamps
    if (!firstResult["timestamp"].is<JsonArray>()) {
        return false;
    }

    JsonArray timestamps = firstResult["timestamp"];
    for (JsonVariant timestamp : timestamps) {
        timestamps_.push_back(timestamp.as<long>());
    }

    // Extract close prices
    if (!firstResult["indicators"].is<JsonObject>()) {
        return false;
    }

    JsonObject indicators = firstResult["indicators"];
    if (!indicators["quote"].is<JsonArray>()) {
        return false;
    }

    JsonArray quote = indicators["quote"];
    if (quote.size() == 0) {
        return false;
    }

    JsonObject firstQuote = quote[0];
    if (!firstQuote["close"].is<JsonArray>()) {
        return false;
    }

    JsonArray closePrices = firstQuote["close"];
    for (JsonVariant price : closePrices) {
        closePrices_.push_back(price.as<double>());
    }

    // Verify we got the same number of timestamps and prices
    if (timestamps_.size() != closePrices_.size()) {
        timestamps_.clear();
        closePrices_.clear();
        return false;
    }

    return true;
}

const std::vector<long>& YahooChartParser::getTimestamps() const {
    return timestamps_;
}

const std::vector<double>& YahooChartParser::getClosePrices() const {
    return closePrices_;
}
