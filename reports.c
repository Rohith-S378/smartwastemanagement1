/*
 * reports.c — Cost, Recycling, Zone-wise reports + menuReports()
 *
 * FIXES applied:
 *  - generateRecyclingReport() today-section now uses g_today_* globals
 *    (populated during simulateCollection) instead of capacity_kg proxy.
 *  - Environmental grade is 4-tier with ANSI colour.
 *  - Budget cap warning using DAILY_BUDGET_INR.
 *  - Workload balance summary added to zoneWiseReport().
 */

#include "../include/globals.h"
#include "../include/utils.h"
#include "../include/civilian.h"
#include "../include/reports.h"

/* ------------------------------------------------------------------ */
void generateCostReport() {
    printf("\n--- %sFUEL & COST REPORT%s ---\n", ANSI_BOLD, ANSI_RESET);
    float totalFuel = 0, totalVehCost = 0;
    const char *vtypes[] = {"Regular","Compactor","Hazmat"};
    for (int i = 0; i < vehicleCount; i++) {
        float fu = vehicles[i].total_fuel_consumed;
        float co = vehicles[i].total_distance_km * vehicles[i].cost_per_km;
        printf("  V%-2d [%-9s]  Dist %6.1fkm  Fuel %5.1fL  Cost INR %6.0f  %s\n",
               vehicles[i].id, vtypes[vehicles[i].type],
               vehicles[i].total_distance_km, fu, co,
               vehicles[i].under_maintenance ?
               "\033[31m[MAINT DUE]\033[0m" : "");
        totalFuel += fu; totalVehCost += co;
    }
    float staffCost = 0;
    for (int i = 0; i < driverCount; i++) {
        float base = fminf(drivers[i].worked_hours, drivers[i].max_hours)
                     * drivers[i].salary_per_hour;
        float ot   = drivers[i].overtime_hours * drivers[i].salary_per_hour * 1.5f;
        staffCost += base + ot;
    }
    float grandTotal = totalVehCost + staffCost;
    printf("  ----\n");
    printf("  Staff cost   : INR %.0f\n", staffCost);
    printf("  Fuel total   : %.1f L\n",   totalFuel);
    printf("  TOTAL COST   : %sINR %.0f%s\n",
           grandTotal > DAILY_BUDGET_INR ? ANSI_RED : ANSI_GREEN,
           grandTotal, ANSI_RESET);

    /* FIX: Budget cap warning */
    if (grandTotal > DAILY_BUDGET_INR)
        printf("  %s[BUDGET ALERT] Daily spend INR %.0f exceeds cap INR %.0f!%s\n",
               ANSI_RED, grandTotal, DAILY_BUDGET_INR, ANSI_RESET);
    else
        printf("  %s[BUDGET OK] INR %.0f remaining.%s\n",
               ANSI_GREEN, DAILY_BUDGET_INR - grandTotal, ANSI_RESET);
}

/* ------------------------------------------------------------------ */
void generateRecyclingReport() {
    printf("\n--- %sRECYCLING & ENVIRONMENTAL REPORT%s ---\n", ANSI_BOLD, ANSI_RESET);

    /* FIX: Use g_today_* globals populated during simulateCollection()
            instead of the old proxy: bins[i].capacity_kg             */
    printf("  --- TODAY ---\n");
    printf("  Collected   : %.1f kg\n", g_today_collected_kg);
    if (g_today_collected_kg > 0) {
        printf("  Recyclable  : %.1f kg  (%.0f%%)\n",
               g_today_recyclable_kg,
               g_today_recyclable_kg / g_today_collected_kg * 100.0f);
        printf("  Landfill    : %.1f kg\n", g_today_landfill_kg);
        printf("  Hazardous   : %.1f kg\n", g_today_hazardous_kg);
    } else {
        printf("  (No collections run today yet)\n");
    }

    printf("\n  --- CUMULATIVE (all time) ---\n");
    printf("  Total collected : %.1f kg\n", g_total_collected_kg);
    float cRate = g_total_collected_kg > 0
                  ? g_total_recyclable_kg / g_total_collected_kg * 100.0f : 0.0f;
    printf("  Recyclable      : %.1f kg  (%.0f%%)\n", g_total_recyclable_kg, cRate);
    printf("  Landfill        : %.1f kg\n", g_total_landfill_kg);
    printf("  Hazardous       : %.1f kg\n", g_total_hazardous_kg);

    /* FIX: 4-tier graded environmental score with colour */
    const char *grade, *col;
    if      (cRate >= 75) { grade = "EXCELLENT  (Target achieved)";          col = ANSI_GREEN;  }
    else if (cRate >= 60) { grade = "GOOD       (Above threshold)";           col = ANSI_CYAN;   }
    else if (cRate >= 40) { grade = "POOR       (Below target)";              col = ANSI_YELLOW; }
    else                  { grade = "CRITICAL   (Immediate action required)"; col = ANSI_RED;    }
    printf("\n  Recycling rate  : %.1f%%\n", cRate);
    printf("  Env. Grade      : %s%s%s\n", col, grade, ANSI_RESET);
}

/* ------------------------------------------------------------------ */
void zoneWiseReport() {
    printf("\n--- %sZONE-WISE REPORT%s ---\n", ANSI_BOLD, ANSI_RESET);
    printf("  %-12s  %-13s  %5s  %8s  %8s  %8s  %8s\n",
           "Zone","Type","Bins","Avg Fill","Critical","Assigned","Collected");
    printf("  %-12s  %-13s  %5s  %8s  %8s  %8s  %8s\n",
           "------------","-------------","-----","--------","--------","--------","--------");
    for (int z = 0; z < zoneCount; z++) {
        int total=0, critical=0, assigned=0, collected=0;
        float avgFill=0, totalKg=0;
        for (int i = 0; i < binCount; i++) {
            if (strcmp(bins[i].zone, zoneNames[z]) != 0) continue;
            total++; avgFill += bins[i].fill; totalKg += bins[i].current_kg;
            if (bins[i].fill >= THRESHOLD) critical++;
            if (bins[i].assigned)          assigned++;
            if (bins[i].collected_today)   collected++;
        }
        if (total == 0) continue;
        const char *col = critical > 0 ? ANSI_YELLOW : ANSI_GREEN;
        printf("  %s%-12s%s  %-13s  %5d  %7.0f%%  %8d  %8d  %8d\n",
               col, zones[z].name, ANSI_RESET,
               zones[z].type, total, avgFill/total,
               critical, assigned, collected);
    }

    /* Workload balance summary across vehicles */
    printf("\n  --- VEHICLE WORKLOAD BALANCE ---\n");
    float avgLoad = 0; int busy = 0;
    for (int j = 0; j < vehicleCount; j++)
        if (!vehicles[j].available) { avgLoad += vehicles[j].planned_load_kg; busy++; }
    if (busy > 0) {
        avgLoad /= busy;
        for (int j = 0; j < vehicleCount; j++) {
            if (vehicles[j].available) continue;
            float pct = vehicles[j].max_capacity_kg > 0
                        ? vehicles[j].planned_load_kg / vehicles[j].max_capacity_kg * 100.0f : 0;
            const char *wCol = pct > 85 ? ANSI_RED : pct > 50 ? ANSI_YELLOW : ANSI_GREEN;
            printf("  V%-2d  Zone %-10s  Load %6.1fkg / %.0fkg  %s(%3.0f%%)%s\n",
                   vehicles[j].id, vehicles[j].assigned_zone,
                   vehicles[j].planned_load_kg, vehicles[j].max_capacity_kg,
                   wCol, pct, ANSI_RESET);
        }
    } else {
        printf("  No vehicles currently assigned.\n");
    }
}
void generatePerBinRecyclingReport() {
    printf("\n--- %sPER-BIN RECYCLING HISTORY%s ---\n", ANSI_BOLD, ANSI_RESET);

    int anyData = 0;
    const char *wtypes[] = {"Bio", "NonBio", "Hazard"};
    printf("  %-4s  %-10s  %-7s  %10s  %10s  %10s  %8s\n",
           "ID", "Zone", "Type",
           "Recycled(kg)", "Landfill(kg)", "Hazard(kg)", "Rec%%");
    printf("  %-4s  %-10s  %-7s  %10s  %10s  %10s  %8s\n",
           "----","----------","-------",
           "------------","------------","----------","--------");

    for (int i = 0; i < binCount; i++) {
        float total = bins[i].total_recyclable_kg
                    + bins[i].total_landfill_kg
                    + bins[i].total_hazardous_kg;
        if (total <= 0) continue;   /* never collected — skip */

        float recPct = total > 0
                       ? bins[i].total_recyclable_kg / total * 100.0f : 0.0f;
        const char *col = recPct >= 60 ? ANSI_GREEN :
                          recPct >= 40 ? ANSI_YELLOW : ANSI_RED;

        printf("  %-4d  %-10s  %-7s  %10.1f  %10.1f  %10.1f  %s%6.0f%%%s\n",
               bins[i].id, bins[i].zone, wtypes[bins[i].waste_type],
               bins[i].total_recyclable_kg,
               bins[i].total_landfill_kg,
               bins[i].total_hazardous_kg,
               col, recPct, ANSI_RESET);
        anyData = 1;
    }

    if (!anyData)
        printf("  No collection history yet. Run a simulation first.\n");
}

/* ------------------------------------------------------------------ */
void menuReports() {
    int ch;
    do {
        printf("\n  REPORTS\n");
        printf("  1. Fuel & Cost Report\n");
        printf("  2. Recycling & Environmental Report\n");
        printf("  3. Zone-wise Report\n");
        printf("  4. View Item Dictionary\n");
printf("  5. Per-Bin Recycling History\n");
printf("  0. Back\n  > ");
        if (scanf("%d", &ch) != 1) { clearBuffer(); continue; }
        switch (ch) {
            case 1: generateCostReport();      break;
            case 2: generateRecyclingReport(); break;
            case 3: zoneWiseReport();          break;
            case 4: showFullDictionary();      break;
            case 5: generatePerBinRecyclingReport(); break;
            case 0: break;
            default: printf("[ERROR] Invalid.\n");
        }
    } while (ch != 0);
}
