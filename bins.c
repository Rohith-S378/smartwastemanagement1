/*
 * bins.c — Bin Management, Emergency Handler, menuBins()
 *
 * FIXES applied:
 *  - ANSI colour applied in BOTH viewBins() and identifyCriticalBins()
 *  - identifyCriticalBins() now shows zone total and coloured bar
 *  - updateBinStatus() braces fixed (Wmisleading-indentation warning gone)
 *  - assignResources() call properly guarded inside the else-if
 */

#include "../include/globals.h"
#include "../include/utils.h"
#include "../include/bins.h"
#include "../include/operations.h"

/* ------------------------------------------------------------------ */
static const char *barColour(float fill) {
    if (fill >= 90) return ANSI_RED;
    if (fill >= 70) return ANSI_YELLOW;
    return ANSI_GREEN;
}

static void buildBar(float fill, char *out, int maxLen) {
    int filled = (int)(fill / 5);
    int b;
    for (b = 0; b < filled && b < maxLen; b++) out[b] = '#';
    for (; b < maxLen; b++) out[b] = '-';
    out[maxLen] = '\0';
}

/* ------------------------------------------------------------------ */
void identifyCriticalBins() {
    printf("\n--- %sCRITICAL BINS%s (fill >= %d%%) ---\n",
           ANSI_BOLD, ANSI_RESET, THRESHOLD);
    int found = 0;
    const char *types[] = {"BIO","NON-BIO","HAZARDOUS"};
    for (int i = 0; i < binCount; i++) {
        if (bins[i].fill < THRESHOLD) continue;
        char barStr[21];
        buildBar(bins[i].fill, barStr, 20);
        const char *col = barColour(bins[i].fill);
        printf("  [P%d] Bin %-4d  Zone %-10s  %s[%s]%s%5.1f%%  %-9s  %s\n",
               bins[i].priority, bins[i].id, bins[i].zone,
               col, barStr, ANSI_RESET,
               bins[i].fill, types[bins[i].waste_type],
               bins[i].is_emergency ? "[EMERGENCY]" : "");
        found++;
    }
    if (!found) printf("  %sNo critical bins at this time.%s\n", ANSI_GREEN, ANSI_RESET);
}

/* ------------------------------------------------------------------ */
void addBin() {
    if (binCount >= MAX_BINS) { printf("[ERROR] Max bins reached.\n"); return; }
    Bin b; memset(&b, 0, sizeof(Bin));
    printf("\n--- ADD BIN ---\n");
    do {
        printf("Bin ID: ");
        if (scanf("%d", &b.id) != 1) { clearBuffer(); continue; }
        if (binIdExists(b.id)) printf("[ERROR] ID exists.\n");
        else break;
    } while (1);

    printf("Zone name: "); scanf("%29s", b.zone); toLowerStr(b.zone);
    int zi = getZoneIndex(b.zone);
    if (zi == -1) {
        printf("[ERROR] Zone '%s' does not exist. Add it via Zone Setup first.\n", b.zone);
        return;
    }
    if (zones[zi].bin_count >= zones[zi].max_bins) {
        printf("[ERROR] Zone '%s' is at max bin capacity (%d).\n", b.zone, zones[zi].max_bins);
        return;
    }
    printf("Waste type (0=Bio  1=NonBio  2=Hazardous): ");
    int wt; scanf("%d", &wt);
    b.waste_type = (wt >= 0 && wt <= 2) ? (WasteCategory)wt : NON_BIODEGRADABLE;
    printf("Bin capacity in kg [0 = default 120kg]: ");
    scanf("%f", &b.capacity_kg);
    if (b.capacity_kg <= 0) b.capacity_kg = 120.0f;
    printf("Current fill %% (0-100): "); scanf("%f", &b.fill);
    b.fill           = fminf(100.0f, fmaxf(0.0f, b.fill));
    b.current_kg     = b.capacity_kg * b.fill / 100.0f;
    b.priority       = calcPriority(b.fill, 0);
    float defaultRec = (b.waste_type == BIODEGRADABLE)     ? 80.0f :
                   (b.waste_type == NON_BIODEGRADABLE) ? 50.0f : 10.0f;
printf("Estimated recyclable %% for this bin [default %.0f%%]: ", defaultRec);
float userRec; scanf("%f", &userRec);
b.recyclable_pct = (userRec > 0 && userRec <= 100) ? userRec : defaultRec;
    getCurrentTime(b.last_collected, sizeof(b.last_collected));
    zones[zi].bin_count++;
    bins[binCount++] = b;
    saveBins(); saveZones();
    printf("[OK] Bin %d added  Zone: %s  Priority: %d\n", b.id, b.zone, b.priority);
    if (b.fill >= 100.0f) autoEmergency(binCount - 1);
}

/* ------------------------------------------------------------------ */
void showFillTrend(Bin *b) {
    if (b->history_count < 2) return;
    float totalGain = 0;
    for (int i = 1; i < b->history_count; i++)
        totalGain += b->fill_history[i] - b->fill_history[i-1];
    float dailyRate = totalGain / (b->history_count - 1);
    if (dailyRate <= 0) {
        printf("    %sTrend: stable / decreasing%s\n", ANSI_GREEN, ANSI_RESET);
        return;
    }
    float daysToFull = (100.0f - b->fill) / dailyRate;
    printf("    Trend: +%.1f%%/day  =>  FULL in ~%.0f day(s)\n", dailyRate, daysToFull);
    if (daysToFull <= 1)
        printf("    %s[PREDICT] CRITICAL: Will overflow tomorrow!%s\n", ANSI_RED, ANSI_RESET);
    else if (daysToFull <= 3)
        printf("    %s[PREDICT] Schedule collection within %d days.%s\n",
               ANSI_YELLOW, (int)daysToFull, ANSI_RESET);
}

/* ------------------------------------------------------------------ */
void viewBins() {
    if (binCount == 0) { printf("\nNo bins recorded.\n"); return; }
    printf("\n  %-4s  %-10s  %-7s  %-27s  %3s  %-10s\n",
           "ID","Zone","Type","Fill Bar","Pri","Status");
    printf("  %-4s  %-10s  %-7s  %-27s  %3s  %-10s\n",
           "----","----------","-------","---------------------------","---","----------");
    const char *types[] = {"Bio","NonBio","Hazard"};
    for (int i = 0; i < binCount; i++) {
        char barStr[21]; buildBar(bins[i].fill, barStr, 20);
        const char *col = barColour(bins[i].fill);
        printf("  %-4d  %-10s  %-7s  %s[%s]%s%4.0f%%  %3d  %-10s\n",
               bins[i].id, bins[i].zone, types[bins[i].waste_type],
               col, barStr, ANSI_RESET,
               bins[i].fill, bins[i].priority,
               bins[i].is_emergency  ? "EMERGENCY"  :
               bins[i].collected_today ? "Collected" :
               bins[i].assigned      ? "Assigned"   : "Pending");
        showFillTrend(&bins[i]);
    }
    pause();
}

/* ------------------------------------------------------------------ */
void updateBinStatus() {
    int id; printf("Bin ID to update: "); scanf("%d", &id);
    for (int i = 0; i < binCount; i++) {
        if (bins[i].id != id) continue;
        /* Slide fill history */
        if (bins[i].history_count < 7)
            bins[i].fill_history[bins[i].history_count++] = bins[i].fill;
        else {
            for (int j = 0; j < 6; j++)
                bins[i].fill_history[j] = bins[i].fill_history[j+1];
            bins[i].fill_history[6] = bins[i].fill;
        }
        printf("New fill %% (0-100): "); scanf("%f", &bins[i].fill);
        bins[i].fill       = fminf(100.0f, fmaxf(0.0f, bins[i].fill));
        bins[i].current_kg = bins[i].capacity_kg * bins[i].fill / 100.0f;
        bins[i].priority   = calcPriority(bins[i].fill, bins[i].is_emergency);
        saveBins();
        printf("[OK] Bin %d -> %.0f%%  Priority %d\n", id, bins[i].fill, bins[i].priority);
printf("Update recyclable %% for this bin? (0 to skip): ");
float newRec; scanf("%f", &newRec);
if (newRec > 0 && newRec <= 100) {
    bins[i].recyclable_pct = newRec;
    printf("[OK] Recyclable %% updated to %.0f%%\n", newRec);
}
        /* FIX: proper braces — assignResources() now correctly inside else-if */
        if (bins[i].fill >= 100.0f && !bins[i].is_emergency) {
            autoEmergency(i);
        } else if (bins[i].fill >= THRESHOLD && !bins[i].assigned && !bins[i].collected_today) {
            printf("  [INFO] Bin %d is critical — auto-assigning...\n", id);
            assignResources();
        }
        return;
    }
    printf("[ERROR] Bin %d not found.\n", id);
}

/* ------------------------------------------------------------------ */
void autoEmergency(int binArrayIdx) {
    Bin *b = &bins[binArrayIdx];
    b->is_emergency = 1;
    b->priority     = 0;

    printf("\n  %s*** AUTO-EMERGENCY TRIGGERED ***%s\n", ANSI_RED, ANSI_RESET);
    printf("  Bin %d in zone '%s' is OVERFLOWING (%.0f%%)!\n",
           b->id, b->zone, b->fill);
    if (b->waste_type == HAZARDOUS) {
        printf("  %s!! HAZMAT ALERT: Hazardous overflow at zone '%s' !!%s\n",
               ANSI_RED, b->zone, ANSI_RESET);
        printf("  !! Notify hazmat response team immediately.       !!\n");
    }
    char ts[30]; getCurrentTime(ts, sizeof(ts));
    printf("  Emergency registered at: %s\n", ts);

    int zIdx = getZoneIndex(b->zone);
    int bestV = -1, bestD = -1;
    float bestDist = INF;

    for (int j = 0; j < vehicleCount; j++) {
        if (!vehicles[j].available || vehicles[j].under_maintenance) continue;
        if (b->waste_type == HAZARDOUS && vehicles[j].type != HAZMAT) continue;
        int vzIdx = getZoneIndex(vehicles[j].assigned_zone);
        float d = (vzIdx >= 0 && zIdx >= 0) ? distMatrix[vzIdx][zIdx] : 5.0f;
        if (d < bestDist) { bestDist = d; bestV = j; }
    }

    if (bestV == -1) {
        printf("  No free vehicle — preempting lowest-priority assignment...\n");
        int lowestP = -1; float lowestFill = 200.0f;
        for (int i = 0; i < binCount; i++)
            if (bins[i].assigned && !bins[i].is_emergency && bins[i].fill < lowestFill)
                { lowestFill = bins[i].fill; lowestP = i; }
        if (lowestP >= 0) {
            for (int j = 0; j < vehicleCount; j++) {
                if (strcmp(vehicles[j].assigned_zone, bins[lowestP].zone)==0
                    && !vehicles[j].available) {
                    for (int k = 0; k < driverCount; k++) {
                        if (drivers[k].vehicle_id == vehicles[j].id) {
                            drivers[k].available = 1; drivers[k].vehicle_id = -1; break;
                        }
                    }
                    vehicles[j].available = 1; vehicles[j].driver_id = -1;
                    memset(vehicles[j].assigned_zone, 0, 30);
                    bestV = j; break;
                }
            }
            bins[lowestP].assigned = 0;
            printf("  Released Bin %d assignment to free V%d.\n",
                   bins[lowestP].id, vehicles[bestV].id);
        }
    }
    if (bestV >= 0) {
        for (int k = 0; k < driverCount; k++) {
            if (drivers[k].available && drivers[k].worked_hours < drivers[k].max_hours) {
                bestD = k; break;
            }
        }
    }
    if (bestV == -1 || bestD == -1) {
        printf("  [CRITICAL] No resources available! Log manually.\n");
        saveBins(); return;
    }
    b->assigned = 1;
    vehicles[bestV].available = 0;
    vehicles[bestV].driver_id = drivers[bestD].id;
    vehicles[bestV].planned_load_kg += b->current_kg;
    drivers[bestD].available  = 0;
    drivers[bestD].vehicle_id = vehicles[bestV].id;
    strcpy(vehicles[bestV].assigned_zone, b->zone);
    printf("  [EMERGENCY ASSIGNED] Bin %d -> V%d -> D%d (~%.1fkm)\n",
           b->id, vehicles[bestV].id, drivers[bestD].id, bestDist);
    printf("  Expected response: within 30 minutes of %s\n", ts);
    saveBins(); saveVehicles(); saveDrivers();
}

/* ------------------------------------------------------------------ */
void emergencyHandler() {
    printf("\n--- MANUAL EMERGENCY REGISTRATION ---\n");
    printf("  (Use this for public complaints or unreported overflows)\n");
    if (binCount >= MAX_BINS) { printf("[ERROR] Max bins.\n"); return; }
    printf("Is this an existing bin that overflowed? (1=Yes / 0=New bin): ");
    int existing; scanf("%d", &existing);
    if (existing) {
        int id; printf("Bin ID: "); scanf("%d", &id);
        for (int i = 0; i < binCount; i++) {
            if (bins[i].id == id) {
                bins[i].fill = 100.0f;
                bins[i].current_kg = bins[i].capacity_kg;
                autoEmergency(i); return;
            }
        }
        printf("[ERROR] Bin %d not found.\n", id); return;
    }
    Bin e; memset(&e, 0, sizeof(Bin));
    e.is_emergency = 1; e.fill = 100.0f; e.priority = 0;
    do {
        printf("New Bin ID: ");
        if (scanf("%d", &e.id) != 1) { clearBuffer(); continue; }
        if (binIdExists(e.id)) printf("[ERROR] ID exists.\n"); else break;
    } while (1);
    printf("Zone: "); scanf("%29s", e.zone); toLowerStr(e.zone);
    int zi = getZoneIndex(e.zone);
    if (zi == -1) {
        printf("[ERROR] Zone '%s' does not exist. Add zone first.\n", e.zone); return;
    }
    printf("Waste type (0=Bio  1=NonBio  2=Hazardous): ");
    int wt; scanf("%d", &wt);
    e.waste_type     = (wt >= 0 && wt <= 2) ? (WasteCategory)wt : NON_BIODEGRADABLE;
    e.capacity_kg    = 120.0f; e.current_kg = 120.0f;
    e.recyclable_pct = (e.waste_type == BIODEGRADABLE)     ? 80.0f :
                   (e.waste_type == NON_BIODEGRADABLE) ? 50.0f : 10.0f;
/* Emergency bins keep default — no time to prompt during crisis */
    getCurrentTime(e.last_collected, sizeof(e.last_collected));
    bins[binCount++] = e;
    saveBins();
    autoEmergency(binCount - 1);
}

/* ------------------------------------------------------------------ */
void menuBins() {
    int ch;
    do {
        printf("\n  BIN MANAGEMENT\n");
        printf("  1. Add Bin\n  2. View All Bins\n");
        printf("  3. View Critical Bins\n  4. Update Bin Fill %%\n");
        printf("  0. Back\n  > ");
        if (scanf("%d", &ch) != 1) { clearBuffer(); continue; }
        switch (ch) {
            case 1: addBin();               break;
            case 2: viewBins();             break;
            case 3: identifyCriticalBins(); break;
            case 4: updateBinStatus();      break;
            case 0: break;
            default: printf("[ERROR] Invalid.\n");
        }
    } while (ch != 0);
}
