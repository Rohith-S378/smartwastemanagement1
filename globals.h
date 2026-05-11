#ifndef GLOBALS_H
#define GLOBALS_H

/*
 * globals.h  — Smart Waste Management System v4
 * All shared constants, enums, structs, and extern declarations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ============================================================
   CONSTANTS
   ============================================================ */
#define MAX_BINS            100
#define MAX_VEHICLES         30
#define MAX_DRIVERS          30
#define MAX_ZONES            20
#define MAX_ITEMS            20
#define MAX_LOG             200
#define THRESHOLD            70
#define MAX_WORK_HOURS        8
#define INF               1e9f
#define ADMIN_PIN          1234
#define RECOVERY_ANSWER "sirius"   /* your team name — change this */
#define NEW_PIN_DEFAULT  9999
#define DAILY_BUDGET_INR  8000.0f

#define BIN_FILE        "bins.dat"
#define VEH_FILE        "vehicles.dat"
#define DRV_FILE        "drivers.dat"
#define ZONE_FILE       "zones.dat"
#define LOG_FILE        "collection_log.dat"
#define SCHEDULE_FILE   "daily_schedule.txt"

/* ANSI colour codes */
#define ANSI_RED        "\033[31m"
#define ANSI_YELLOW     "\033[33m"
#define ANSI_GREEN      "\033[32m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_BOLD       "\033[1m"
#define ANSI_RESET      "\033[0m"

/* ============================================================
   ENUMERATIONS
   ============================================================ */
typedef enum { BIODEGRADABLE = 0, NON_BIODEGRADABLE, HAZARDOUS } WasteCategory;
typedef enum { REGULAR = 0, COMPACTOR, HAZMAT } VehicleType;

/* ============================================================
   STRUCTS
   ============================================================ */
typedef struct {
    char name[30];
    char type[20];
    int  max_bins;
    int  bin_count;
} Zone;

typedef struct {
    char          name[40];
    WasteCategory category;
    float         fill_increase;
} WasteItem;

typedef struct {
    const char  *label;
    VehicleType  type;
    float        max_capacity_kg;
    float        fuel_tank_capacity;
    float        consumption_rate;
    float        cost_per_km;
    float        maintenance_interval_km;
} VehiclePreset;

typedef struct {
    int           id;
    char          zone[30];
    WasteCategory waste_type;
    float         fill;
    float         capacity_kg;
    float         current_kg;
    int           priority;
    int           assigned;
    int           collected_today;
    char          last_collected[20];
    float         fill_history[7];
    int           history_count;
    int           is_emergency;
    float         recyclable_pct;
    float         total_recyclable_kg;
    float         total_landfill_kg;
    float         total_hazardous_kg;
    float         last_collected_kg;   /* actual kg from last simulation run */
} Bin;

typedef struct {
    int         id;
    VehicleType type;
    float       max_capacity_kg;
    float       current_load_kg;
    float       fuel_tank_capacity;
    float       fuel_level;
    float       consumption_rate;
    float       cost_per_km;
    char        assigned_zone[30];
    int         driver_id;
    int         available;
    int         under_maintenance;
    float       total_distance_km;
    float       maintenance_interval_km;
    float       total_fuel_consumed;
    float       planned_load_kg;
} Vehicle;

typedef struct {
    int   id;
    int   vehicle_id;
    float max_hours;
    float worked_hours;
    float overtime_hours;
    float salary_per_hour;
    char  shift_start[10];
    char  shift_end[10];
    int   available;
} Driver;

typedef struct {
    int   bin_id;
    int   vehicle_id;
    int   driver_id;
    char  zone[30];
    float kg_collected;
    float fuel_used;
    float cost;
    char  timestamp[30];
    int   was_emergency;
    char  response_time[30];
} LogEntry;

/* ============================================================
   VEHICLE PRESET TABLE  (defined in fleet.c)
   ============================================================ */
extern const VehiclePreset VEHICLE_PRESETS[3];

/* ============================================================
   GLOBAL STATE  (defined in globals.c)
   ============================================================ */
extern Bin      bins[MAX_BINS];
extern Vehicle  vehicles[MAX_VEHICLES];
extern Driver   drivers[MAX_DRIVERS];
extern LogEntry daily_log[MAX_LOG];
extern Zone     zones[MAX_ZONES];

extern int binCount;
extern int vehicleCount;
extern int driverCount;
extern int logCount;
extern int zoneCount;

extern char  zoneNames[MAX_ZONES][30];
extern float distMatrix[MAX_ZONES][MAX_ZONES];

extern float g_total_recyclable_kg;
extern float g_total_landfill_kg;
extern float g_total_hazardous_kg;
extern float g_total_collected_kg;

/* Today snapshot — reset each day, used by recycling report */
extern float g_today_collected_kg;
extern float g_today_recyclable_kg;
extern float g_today_landfill_kg;
extern float g_today_hazardous_kg;

/* Daily budget tracker */
extern float g_daily_spend;

extern WasteItem ITEM_DICT[MAX_ITEMS];
extern int       itemDictSize;

#endif /* GLOBALS_H */
