/*
 * operations.c — Route optimisation, assignment, simulation, menus
 *
 * FIXES applied:
 *  - simulateCollection() now populates g_today_* globals (fix recycling report)
 *  - simulateCollection() stores last_collected_kg on each bin before reset
 *  - simulateCollection() tracks g_daily_spend for budget cap
 *  - resetDayState() clears g_today_* so next day starts fresh
 *  - assignResources() has basic workload balancing (skip over-loaded vehicle)
 *  - menuZones() adds option 4: View Distance Matrix
 *  - (void)zoneKg dead expression removed from routePlan()
 *  - loadSeedData() calls saveZones() so zone data persists
 */

#include "../include/globals.h"
#include "../include/utils.h"
#include "../include/bins.h"
#include "../include/operations.h"

/* ============================================================
   ROUTE OPTIMISATION — Greedy Nearest-Neighbour
   ============================================================ */
void optimizeRoute(int *zoneIdxList, int n, int *routeOut, float *totalDist) {
    int visited[MAX_ZONES] = {0};
    *totalDist = 0.0f;
    int depotIdx    = getZoneIndex("depot");
int dumpyardIdx = getZoneIndex("dumpyard");
int current = (depotIdx >= 0) ? depotIdx : 0;
    if (n > 0) routeOut[0] = current;
    for (int step = 1; step < n; step++) {
        float best = INF; int next = -1;
        for (int i = 0; i < n; i++) {
            int zi = zoneIdxList[i];
            if (!visited[zi] && zi != current) {
                float d = (current >= 0) ? distMatrix[current][zi] : 5.0f;
                if (d < best) { best = d; next = zi; }
            }
        }
        if (next == -1) break;
        visited[next] = 1; routeOut[step] = next;
        *totalDist += best; current = next;
    }
    /* Route ends at dumpyard, then returns to depot */
if (n > 0 && current >= 0) {
    if (dumpyardIdx >= 0) {
        *totalDist += distMatrix[current][dumpyardIdx];
        *totalDist += distMatrix[dumpyardIdx][depotIdx >= 0 ? depotIdx : 0];
    } else {
        *totalDist += distMatrix[current][depotIdx >= 0 ? depotIdx : 0];
    }
}
}

/* ============================================================
   ASSIGNMENT — AUTO, DOUBLE-ASSIGN PROOF, WORKLOAD BALANCED
   ============================================================ */
void assignResources() {
    printf("\n--- AUTO ASSIGNMENT PROCESS ---\n");

    for (int j = 0; j < vehicleCount; j++) vehicles[j].planned_load_kg = 0.0f;

    int critIdx[MAX_BINS], critCount = 0;
    for (int i = 0; i < binCount; i++)
        if (bins[i].fill >= THRESHOLD && !bins[i].assigned && !bins[i].collected_today)
            critIdx[critCount++] = i;

    if (critCount == 0) { printf("  No bins require collection right now.\n"); return; }
    sortBinsByPriority(critIdx, critCount);

    /* FIX: compute average vehicle load for workload balancing */
    float avgLoad = 0; int busyVeh = 0;
    for (int j = 0; j < vehicleCount; j++)
        if (!vehicles[j].available) { avgLoad += vehicles[j].planned_load_kg; busyVeh++; }
    if (busyVeh > 0) avgLoad /= busyVeh;

    int assigned = 0;
    for (int ci = 0; ci < critCount; ci++) {
        int bi = critIdx[ci];
        Bin *bin = &bins[bi];
        int bestV = -1, bestD = -1;
        float bestRemain = -1.0f;

        for (int j = 0; j < vehicleCount; j++) {
            Vehicle *v = &vehicles[j];
            if (v->under_maintenance) continue;
            if (bin->waste_type == HAZARDOUS && v->type != HAZMAT) continue;
            float fuelNeeded = 10.0f * v->consumption_rate;
            if (v->fuel_level < fuelNeeded) continue;
            float remainCap = v->max_capacity_kg - v->planned_load_kg;
            if (bin->current_kg > remainCap) continue;
            /* Double-assign guard */
            if (!v->available && strcmp(v->assigned_zone, bin->zone) != 0) continue;
            /* FIX: workload balance — skip if vehicle is >1.5x the average load */
            if (busyVeh > 1 && !v->available &&
                v->planned_load_kg > avgLoad * 1.5f) continue;
            if (remainCap > bestRemain) { bestRemain = remainCap; bestV = j; }
        }

        if (bestV == -1) {
            printf("  [SKIP] Bin %d — no suitable vehicle.\n", bin->id); continue;
        }

        /* Driver: prefer existing driver of busy vehicle, else find free one */
        if (!vehicles[bestV].available && vehicles[bestV].driver_id >= 0) {
            for (int k = 0; k < driverCount; k++) {
                if (drivers[k].id == vehicles[bestV].driver_id &&
                    drivers[k].worked_hours < drivers[k].max_hours) {
                    bestD = k; break;
                }
            }
        }
        if (bestD == -1) {
            for (int k = 0; k < driverCount; k++) {
                if (drivers[k].available &&
                    drivers[k].worked_hours < drivers[k].max_hours) {
                    bestD = k; break;
                }
            }
        }

        if (bestD == -1) {
            printf("  [SKIP] Bin %d — no driver within hour limit.\n", bin->id); continue;
        }

        bin->assigned = 1;
        vehicles[bestV].planned_load_kg += bin->current_kg;
        /* update average for next iteration */
        if (!vehicles[bestV].available) {
            avgLoad = (avgLoad * busyVeh + bin->current_kg) / busyVeh;
        }

        if (vehicles[bestV].available) {
            vehicles[bestV].available = 0;
            vehicles[bestV].driver_id = drivers[bestD].id;
            drivers[bestD].available  = 0;
            drivers[bestD].vehicle_id = vehicles[bestV].id;
            strcpy(vehicles[bestV].assigned_zone, bin->zone);
            busyVeh++;
        }

        printf("  [ASSIGNED] Bin %-4d  Zone %-10s  P%d  %.1fkg  -> V%d -> D%d\n",
               bin->id, bin->zone, bin->priority,
               bin->current_kg, vehicles[bestV].id, drivers[bestD].id);
        assigned++;
    }

    if (assigned == 0) printf("  No assignments made.\n");
    else {
        saveVehicles(); saveDrivers(); saveBins();
        printf("  %d bin(s) assigned.\n", assigned);
        writeScheduleFile();
    }
}

/* ============================================================
   COLLECTION SIMULATION
   ============================================================ */
void simulateCollection() {
    printf("\n--- SIMULATING WASTE COLLECTION ---\n");
    int simCount = 0; float totalCost = 0.0f;

    for (int i = 0; i < binCount; i++) {
        if (!bins[i].assigned || bins[i].collected_today) continue;

        int vIdx = -1;
        for (int j = 0; j < vehicleCount; j++) {
            if (!vehicles[j].available &&
                strcmp(vehicles[j].assigned_zone, bins[i].zone) == 0) {
                int dOk = 0;
                for (int k = 0; k < driverCount; k++)
                    if (drivers[k].vehicle_id == vehicles[j].id && !drivers[k].available)
                        { dOk = 1; break; }
                if (dOk) { vIdx = j; break; }
            }
        }
        if (vIdx == -1) continue;

        if (vehicles[vIdx].current_load_kg + bins[i].current_kg
            > vehicles[vIdx].max_capacity_kg) {
            printf("  [OVERFLOW SKIP] Bin %d — V%d would exceed capacity.\n",
                   bins[i].id, vehicles[vIdx].id);
            bins[i].assigned = 0; continue;
        }

        int zIdx = getZoneIndex(bins[i].zone);
        float dist     = (zIdx >= 0 && zoneCount > 1) ? distMatrix[0][zIdx] * 2 : 10.0f;
        float fuelUsed = dist * vehicles[vIdx].consumption_rate;
        float tripCost = dist * vehicles[vIdx].cost_per_km;

        vehicles[vIdx].fuel_level          -= fuelUsed;
        if (vehicles[vIdx].fuel_level < 0) vehicles[vIdx].fuel_level = 0;
        vehicles[vIdx].total_distance_km   += dist;
        vehicles[vIdx].total_fuel_consumed += fuelUsed;
        vehicles[vIdx].current_load_kg     += bins[i].current_kg;

        if (vehicles[vIdx].total_distance_km >= vehicles[vIdx].maintenance_interval_km) {
            vehicles[vIdx].under_maintenance = 1;
            printf("  %s[MAINTENANCE DUE] Vehicle %d sent for service.%s\n",
                   ANSI_YELLOW, vehicles[vIdx].id, ANSI_RESET);
        }

        float collected_kg = bins[i].current_kg;

        /* FIX: store actual collected kg on the bin BEFORE resetting */
        bins[i].last_collected_kg = collected_kg;

        /* Recycling accounting — cumulative globals */
        float rec = collected_kg * bins[i].recyclable_pct / 100.0f;
        float lf  = 0.0f;
        if (bins[i].waste_type == HAZARDOUS) {
            g_total_hazardous_kg += collected_kg;
            g_today_hazardous_kg += collected_kg;   /* FIX: today snapshot */
        } else {
            lf = collected_kg * (1.0f - bins[i].recyclable_pct / 100.0f);
            g_total_landfill_kg += lf;
            g_today_landfill_kg += lf;              /* FIX: today snapshot */
        }
        g_total_recyclable_kg += rec;
        g_total_collected_kg  += collected_kg;
        g_today_recyclable_kg += rec;               /* FIX: today snapshot */
        g_today_collected_kg  += collected_kg;      /* FIX: today snapshot */

        /* Also update per-bin cumulative totals */
        bins[i].total_recyclable_kg += rec;
        bins[i].total_landfill_kg   += lf;
        if (bins[i].waste_type == HAZARDOUS)
            bins[i].total_hazardous_kg += collected_kg;

        /* Reset bin */
        bins[i].fill = 0.0f; bins[i].current_kg = 0.0f;
        bins[i].assigned = 0; bins[i].collected_today = 1;
        bins[i].priority = 5; bins[i].is_emergency = 0;
        getCurrentTime(bins[i].last_collected, sizeof(bins[i].last_collected));

        /* Driver hours */
        float hours = dist / 30.0f;
        float driverCost = 0;
        for (int k = 0; k < driverCount; k++) {
            if (drivers[k].vehicle_id == vehicles[vIdx].id) {
                drivers[k].worked_hours += hours;
                if (drivers[k].worked_hours > drivers[k].max_hours) {
                    drivers[k].overtime_hours +=
                        (drivers[k].worked_hours - drivers[k].max_hours);
                    printf("  [OVERTIME] Driver %d exceeded limit by %.1fh\n",
                           drivers[k].id, drivers[k].overtime_hours);
                }
                driverCost = drivers[k].salary_per_hour * hours;
                totalCost += driverCost;
                break;
            }
        }

        /* Log entry */
        if (logCount < MAX_LOG) {
            LogEntry *lg = &daily_log[logCount++];
            lg->bin_id = bins[i].id; lg->vehicle_id = vehicles[vIdx].id;
            lg->kg_collected = collected_kg; lg->fuel_used = fuelUsed;
            lg->cost = tripCost + driverCost;
            lg->was_emergency = 0;
            strcpy(lg->zone, bins[i].zone);
            getCurrentTime(lg->timestamp, sizeof(lg->timestamp));
        }

        printf("  [COLLECTED] Bin %-4d  Zone %-10s  %.1fkg  Fuel %.2fL  INR %.0f\n",
               bins[i].id, bins[i].zone, collected_kg, fuelUsed, tripCost);
        totalCost += tripCost;
        simCount++;
    }

    /* Free completed vehicles/drivers */
    for (int j = 0; j < vehicleCount; j++) {
        if (vehicles[j].available) continue;
        int stillBusy = 0;
        for (int i = 0; i < binCount; i++)
            if (bins[i].assigned &&
                strcmp(bins[i].zone, vehicles[j].assigned_zone) == 0)
                { stillBusy = 1; break; }
        if (!stillBusy) {
            for (int k = 0; k < driverCount; k++) {
                if (drivers[k].vehicle_id == vehicles[j].id) {
                    drivers[k].available = 1; drivers[k].vehicle_id = -1; break;
                }
            }
            vehicles[j].available = 1; vehicles[j].driver_id = -1;
            vehicles[j].planned_load_kg = 0.0f;
            memset(vehicles[j].assigned_zone, 0, sizeof(vehicles[j].assigned_zone));
        }
    }

    if (simCount == 0) {
        printf("  Nothing to simulate. Run Assignment first.\n");
    } else {
        g_daily_spend += totalCost;   /* FIX: accumulate daily budget */
        printf("  ----\n  %d collections | Total cost: INR %.0f\n", simCount, totalCost);
        /* FIX: inline budget warning right after simulation */
        if (g_daily_spend > DAILY_BUDGET_INR)
            printf("  %s[BUDGET ALERT] Daily spend INR %.0f exceeds cap INR %.0f!%s\n",
                   ANSI_RED, g_daily_spend, DAILY_BUDGET_INR, ANSI_RESET);
        saveBins(); saveVehicles(); saveDrivers(); saveLog(); logCount = 0;
    }
}

/* ============================================================
   ROUTE PLAN DISPLAY
   ============================================================ */
void routePlan() {
    printf("\n--- OPTIMISED ROUTE PLAN ---\n");
    int zoneSet[MAX_ZONES], zoneSetCount = 0;
    for (int i = 0; i < binCount; i++) {
        if (!bins[i].assigned && !bins[i].collected_today) continue;
        int zi = getZoneIndex(bins[i].zone); if (zi < 0) continue;
        int dup = 0;
        for (int x = 0; x < zoneSetCount; x++) if (zoneSet[x]==zi) { dup=1; break; }
        if (!dup) zoneSet[zoneSetCount++] = zi;
    }
    if (zoneSetCount == 0) { printf("  No active routes. Assign resources first.\n"); return; }

    int routeOut[MAX_ZONES]; float totalDist;
    optimizeRoute(zoneSet, zoneSetCount, routeOut, &totalDist);

    printf("  DEPOT");
    for (int i = 0; i < zoneSetCount; i++) printf(" -> %s", zoneNames[routeOut[i]]);
    printf(" -> DUMPYARD -> DEPOT\n  Est. total distance: %.1f km\n", totalDist);

    for (int i = 0; i < zoneSetCount; i++) {
        float zoneKg = 0;   /* FIX: actually printed now, not suppressed */
        printf("\n  [%s]\n", zoneNames[routeOut[i]]);
        for (int b = 0; b < binCount; b++) {
            if (strcmp(bins[b].zone, zoneNames[routeOut[i]])==0 &&
                (bins[b].assigned || bins[b].collected_today)) {
                printf("    Bin %-4d  %.1f%%  %.1fkg  %s\n",
                       bins[b].id, bins[b].fill, bins[b].current_kg,
                       bins[b].is_emergency ? "[EMERGENCY]" : "");
                zoneKg += bins[b].current_kg;
            }
        }
        for (int j = 0; j < vehicleCount; j++) {
            if (strcmp(vehicles[j].assigned_zone, zoneNames[routeOut[i]])==0
                && !vehicles[j].available) {
                printf("    -> V%d carrying %.1f/%.0fkg  (Driver D%d)\n",
                       vehicles[j].id, vehicles[j].planned_load_kg,
                       vehicles[j].max_capacity_kg, vehicles[j].driver_id);
            }
        }
        /* FIX: show zone total kg instead of suppressing it */
        printf("    Zone total: %.1f kg\n", zoneKg);
    }
}

/* ============================================================
   DAY RESET
   ============================================================ */
void resetDayState() {
    for (int i = 0; i < binCount; i++) bins[i].collected_today = 0;
    for (int i = 0; i < vehicleCount; i++) {
        vehicles[i].current_load_kg = 0;
        vehicles[i].planned_load_kg = 0;
    }
    for (int i = 0; i < driverCount; i++) {
        drivers[i].worked_hours   = 0;
        drivers[i].overtime_hours = 0;
    }
    /* FIX: reset today-snapshot globals for fresh day */
    g_today_collected_kg  = 0.0f;
    g_today_recyclable_kg = 0.0f;
    g_today_landfill_kg   = 0.0f;
    g_today_hazardous_kg  = 0.0f;
    g_daily_spend         = 0.0f;

    saveBins(); saveVehicles(); saveDrivers();
    printf("[OK] Day reset. All bins collectable. Driver hours and budget cleared.\n");
}

/* ============================================================
   SEED DATA
   ============================================================ */
void loadSeedData() {
    printf("\n[SEED] Loading demo data...\n");
    typedef struct { const char *name; const char *type; int mb; } ZS;
    ZS zseeds[] = {
    {"depot",    "Depot",       1},   /* index 0 — always the start */
    {"north",    "Residential", 8},
    {"south",    "Commercial",  6},
    {"east",     "Residential", 6},
    {"west",     "Industrial",  5},
    {"central",  "Commercial",  4},
    {"dumpyard", "Dumpyard",    1}    /* index 6 — waste destination */
};
float dists[7][7] = {
    /* depot  north  south  east  west  central  dumpyard */
    {  0,     3,     5,     5,    7,    3,       8  },  /* depot    */
    {  3,     0,     8,     5,    7,    3,       10 },  /* north    */
    {  5,     8,     0,     6,    4,    5,       7  },  /* south    */
    {  5,     5,     6,     0,    9,    4,       9  },  /* east     */
    {  7,     7,     4,     9,    0,    6,       6  },  /* west     */
    {  3,     3,     5,     4,    6,    0,       8  },  /* central  */
    {  8,     10,    7,     9,    6,    8,       0  }   /* dumpyard */
};
for (int i = 0; i < 7; i++) {
    if (getZoneIndex(zseeds[i].name) != -1) continue;
    addZoneInternal(zseeds[i].name, zseeds[i].type, zseeds[i].mb);
}
for (int i = 0; i < 7; i++){
    for (int j = 0; j < 7; j++)
        distMatrix[i][j] = dists[i][j];
}
    typedef struct { int id; int zi; WasteCategory wt; float fill; float cap; } BS;
    BS bseeds[] = {
        {101,0,NON_BIODEGRADABLE,92.0f,120.0f},
        {102,0,BIODEGRADABLE,    55.0f,120.0f},
        {103,1,BIODEGRADABLE,    85.0f,120.0f},
        {104,1,HAZARDOUS,        75.0f,100.0f},
        {105,2,NON_BIODEGRADABLE,71.0f,120.0f},
        {106,2,BIODEGRADABLE,    40.0f,120.0f},
        {107,3,NON_BIODEGRADABLE,88.0f,120.0f},
        {108,3,BIODEGRADABLE,    60.0f,120.0f},
        {109,4,HAZARDOUS,        95.0f,100.0f},
        {110,4,NON_BIODEGRADABLE,30.0f,120.0f}
    };
    int bCount = (int)(sizeof(bseeds)/sizeof(bseeds[0]));
    for (int i = 0; i < bCount; i++) {
        if (binIdExists(bseeds[i].id)) continue;
        Bin b; memset(&b, 0, sizeof(Bin));
        b.id=bseeds[i].id; b.waste_type=bseeds[i].wt;
        b.fill=bseeds[i].fill; b.capacity_kg=bseeds[i].cap;
        b.current_kg=b.capacity_kg*b.fill/100.0f;
        b.priority=calcPriority(b.fill,0);
        b.recyclable_pct=(b.waste_type==BIODEGRADABLE)?80.0f:
                         (b.waste_type==NON_BIODEGRADABLE)?50.0f:10.0f;
        strcpy(b.zone, zseeds[bseeds[i].zi].name);
        getCurrentTime(b.last_collected, sizeof(b.last_collected));
        bins[binCount++]=b; zones[bseeds[i].zi].bin_count++;
    }
    saveBins();

    typedef struct { int id; int preset; } VS;
    VS vseeds[] = {{201,0},{202,1},{203,2}};
    for (int i = 0; i < 3; i++) {
        if (vehicleIdExists(vseeds[i].id)) continue;
        const VehiclePreset *p = &VEHICLE_PRESETS[vseeds[i].preset];
        Vehicle v; memset(&v, 0, sizeof(Vehicle));
        v.id=vseeds[i].id; v.type=p->type;
        v.max_capacity_kg=p->max_capacity_kg;
        v.fuel_tank_capacity=p->fuel_tank_capacity;
        v.fuel_level=p->fuel_tank_capacity;
        v.consumption_rate=p->consumption_rate;
        v.cost_per_km=p->cost_per_km;
        v.maintenance_interval_km=p->maintenance_interval_km;
        v.available=1; v.driver_id=-1;
        vehicles[vehicleCount++]=v;
    }
    saveVehicles();

    typedef struct { int id; float sal; } DS;
    DS dseeds[] = {{301,85},{302,80},{303,90},{304,75}};
    for (int i = 0; i < 4; i++) {
        if (driverIdExists(dseeds[i].id)) continue;
        Driver d; memset(&d, 0, sizeof(Driver));
        d.id=dseeds[i].id; d.max_hours=MAX_WORK_HOURS;
        d.salary_per_hour=dseeds[i].sal;
        d.available=1; d.vehicle_id=-1;
        strcpy(d.shift_start,"07:00"); strcpy(d.shift_end,"15:00");
        drivers[driverCount++]=d;
    }
    saveDrivers();
    saveZones();   /* FIX: persist real zone types and distance matrix */

    printf("[SEED] %d zones | %d bins | %d vehicles | %d drivers\n",
           zoneCount, binCount, vehicleCount, driverCount);
    printf("  Critical bins: 101(north) 103(south) 107(west) 109(central)\n");
    printf("  Bin 104 and 109 are HAZARDOUS — only V203 (Hazmat) handles them.\n");
}

/* ============================================================
   ADMIN SUB-MENU: ZONE MANAGEMENT  (FIX: option 4 = distance matrix)
   ============================================================ */
void menuZones() {
    int ch;
    do {
        printf("\n  ZONE MANAGEMENT\n");
        printf("  1. Add New Zone\n");
        printf("  2. View All Zones\n");
        printf("  3. Set Distance Between Zones\n");
        printf("  4. View Distance Matrix\n");       /* FIX: new option */
        printf("  0. Back\n  > ");
        if (scanf("%d", &ch) != 1) { clearBuffer(); continue; }
        switch (ch) {
            case 1: {
                char zname[30];
                printf("Zone name: "); scanf("%29s", zname);
                int idx = addZone(zname);
                if (idx >= 0)
                    printf("[OK] Zone '%s' registered.\n", zones[idx].name);
                break;
            }
            case 2:
                printf("\n  %-12s  %-13s  %s\n","Name","Type","Bins (used/max)");
                for (int i = 0; i < zoneCount; i++)
                    printf("  %-12s  %-13s  %d/%d\n",
                           zones[i].name, zones[i].type,
                           zones[i].bin_count, zones[i].max_bins);
                break;
            case 3: setZoneDistance();    break;
            case 4: viewDistanceMatrix(); break;   /* FIX */
            case 0: break;
            default: printf("[ERROR] Invalid.\n");
        }
    } while (ch != 0);
}

/* ============================================================
   ADMIN SUB-MENU: OPERATIONS
   ============================================================ */
void menuOps() {
    int ch;
    do {
        printf("\n  OPERATIONS\n");
        printf("  1. Auto-Assign Resources\n");
        printf("  2. Show Optimised Route Plan\n");
        printf("  3. Simulate Collection Run\n");
        printf("  4. Emergency Override\n");
        printf("  5. Reset Day State\n");
        printf("  0. Back\n  > ");
        if (scanf("%d", &ch) != 1) { clearBuffer(); continue; }
        switch (ch) {
            case 1: assignResources();    break;
            case 2: routePlan();          break;
            case 3: simulateCollection(); break;
            case 4: emergencyHandler();   break;
            case 5: resetDayState();      break;
            case 0: break;
            default: printf("[ERROR] Invalid.\n");
        }
    } while (ch != 0);
}
