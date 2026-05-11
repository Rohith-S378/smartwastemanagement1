/*
 * globals.c — defines all global arrays and counters
 */
#include "../include/globals.h"

Bin      bins[MAX_BINS];
Vehicle  vehicles[MAX_VEHICLES];
Driver   drivers[MAX_DRIVERS];
LogEntry daily_log[MAX_LOG];
Zone     zones[MAX_ZONES];

int binCount     = 0;
int vehicleCount = 0;
int driverCount  = 0;
int logCount     = 0;
int zoneCount    = 0;

char  zoneNames[MAX_ZONES][30];
float distMatrix[MAX_ZONES][MAX_ZONES];

float g_total_recyclable_kg = 0.0f;
float g_total_landfill_kg   = 0.0f;
float g_total_hazardous_kg  = 0.0f;
float g_total_collected_kg  = 0.0f;

float g_today_collected_kg  = 0.0f;
float g_today_recyclable_kg = 0.0f;
float g_today_landfill_kg   = 0.0f;
float g_today_hazardous_kg  = 0.0f;

float g_daily_spend         = 0.0f;

WasteItem ITEM_DICT[MAX_ITEMS];
int       itemDictSize = 0;
