#pragma once
struct GSEStatistics{
    int totalPayloadLength = 0;
    int gseAmount = 0;
    int bbfNum = 0;
    int totalPaddingLength = 0;
    int totalLength = 0;
    int totalHeaderLength = 0;
    double overheadRatio = 0.0;
    double efficiency = 0.0;
    double paddingRatio = 0.0;
};