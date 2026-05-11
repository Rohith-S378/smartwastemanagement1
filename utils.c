/*
 * utils.c — helpers, zone ops, file persistence, schedule writer
 */
#include "../include/globals.h"
#include "../include/utils.h"

/* ============================================================ HELPERS */
void clearBuffer() { int c; while ((c = getchar()) != '\n' && c != EOF); }
void pause() { printf("\nPress ENTER to continue..."); clearBuffer(); getchar(); }

void getCurrentTime(char *buf, int size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M", tm_info);
}

void toLowerStr(char *s) {
    for (int i = 0; s[i]; i++)
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] += 32;
}

/* ============================================================ ZONE HELPERS */
int getZoneIndex(const char *zone) {
    for (int i = 0; i < zoneCount; i++)
        if (strcmp(zoneNames[i], zone) == 0) return i;
    return -1;
}

int addZoneInternal(const char *normName, const char *type, int max_bins) {
    int idx = getZoneIndex(normName);
    if (idx >= 0) return idx;
    if (zoneCount >= MAX_ZONES) return -1;
    strcpy(zones[zoneCount].name,  normName);
    strcpy(zones[zoneCount].type,  type);
    zones[zoneCount].max_bins  = max_bins;
    zones[zoneCount].bin_count = 0;
    strcpy(zoneNames[zoneCount], normName);
    for (int i = 0; i < zoneCount; i++) {
        distMatrix[zoneCount][i] = 5.0f;
        distMatrix[i][zoneCount] = 5.0f;
    }
    distMatrix[zoneCount][zoneCount] = 0.0f;
    int newIdx = zoneCount++;
    saveZones();
    return newIdx;
}

int addZone(const char *name) {
    char normName[30];
    strncpy(normName, name, 29); normName[29] = '\0';
    toLowerStr(normName);
    if (getZoneIndex(normName) >= 0) {
        printf("[INFO] Zone '%s' already exists.\n", normName); return -1;
    }
    if (zoneCount >= MAX_ZONES) { printf("[ERROR] Max zones reached.\n"); return -1; }
    printf("Zone type (0=Residential  1=Commercial  2=Industrial): ");
    int t; scanf("%d", &t);
    const char *ztypes[] = {"Residential","Commercial","Industrial"};
    printf("Max bins allowed in this zone: ");
    int mb; scanf("%d", &mb);
    if (mb <= 0) mb = 10;
    return addZoneInternal(normName, ztypes[(t>=0&&t<=2)?t:0], mb);
}

void setZoneDistance() {
    printf("\nZones registered:\n");
    for (int i = 0; i < zoneCount; i++)
        printf("  [%d] %s  (%s)\n", i, zones[i].name, zones[i].type);
    int a, b; float km;
    printf("Zone A index: "); scanf("%d", &a);
    printf("Zone B index: "); scanf("%d", &b);
    printf("Distance (km): "); scanf("%f", &km);
    if (a < 0 || a >= zoneCount || b < 0 || b >= zoneCount) {
        printf("[ERROR] Invalid index.\n"); return;
    }
    distMatrix[a][b] = distMatrix[b][a] = km;
    saveZones();
    printf("[OK] %s <-> %s = %.1f km\n", zoneNames[a], zoneNames[b], km);
}

/* FIX: Distance matrix display — was completely missing from all menus */
void viewDistanceMatrix() {
    if (zoneCount == 0) { printf("  No zones registered.\n"); return; }
    printf("\n--- ZONE DISTANCE MATRIX (km) ---\n");
    /* Header */
    printf("  %-10s", "FROM\\TO");
    for (int i = 0; i < zoneCount; i++) printf("  %-9s", zoneNames[i]);
    printf("\n  ----------");
    for (int i = 0; i < zoneCount; i++) printf("  ---------");
    printf("\n");
    /* Rows */
    for (int i = 0; i < zoneCount; i++) {
        printf("  %-10s", zoneNames[i]);
        for (int j = 0; j < zoneCount; j++) {
            if (i == j) printf("     --   ");
            else        printf("  %7.1f  ", distMatrix[i][j]);
        }
        printf("\n");
    }
    printf("\n  (Set distances via Zone Setup -> Set Distance Between Zones)\n");
}

/* ============================================================ ID CHECKS */
int binIdExists(int id) {
    for (int i = 0; i < binCount; i++)
        if (bins[i].id == id) return 1;
    return 0;
}
int vehicleIdExists(int id) {
    for (int i = 0; i < vehicleCount; i++)
        if (vehicles[i].id == id) return 1;
    return 0;
}
int driverIdExists(int id) {
    for (int i = 0; i < driverCount; i++)
        if (drivers[i].id == id) return 1;
    return 0;
}

/* ============================================================ PRIORITY */
int calcPriority(float fill, int is_emergency) {
    if (is_emergency) return 0;
    if (fill >= 90)   return 1;
    if (fill >= 75)   return 2;
    if (fill >= THRESHOLD) return 3;
    return 5;
}
void sortBinsByPriority(int *indices, int count) {
    for (int i = 0; i < count - 1; i++)
        for (int j = 0; j < count - i - 1; j++)
            if (bins[indices[j]].priority > bins[indices[j+1]].priority) {
                int tmp = indices[j]; indices[j] = indices[j+1]; indices[j+1] = tmp;
            }
}

/* ============================================================ FILE PERSISTENCE */
void saveBins() {
    FILE *f = fopen(BIN_FILE, "wb");
    if (!f) { printf("[ERROR] Cannot save bins.\n"); return; }
    fwrite(&binCount, sizeof(int), 1, f);
    fwrite(bins, sizeof(Bin), binCount, f);
    fclose(f);
}
void loadBins() {
    FILE *f = fopen(BIN_FILE, "rb");
    if (!f) return;
    fread(&binCount, sizeof(int), 1, f);
    fread(bins, sizeof(Bin), binCount, f);
    fclose(f);
}
void saveVehicles() {
    FILE *f = fopen(VEH_FILE, "wb");
    if (!f) { printf("[ERROR] Cannot save vehicles.\n"); return; }
    fwrite(&vehicleCount, sizeof(int), 1, f);
    fwrite(vehicles, sizeof(Vehicle), vehicleCount, f);
    fclose(f);
}
void loadVehicles() {
    FILE *f = fopen(VEH_FILE, "rb");
    if (!f) return;
    fread(&vehicleCount, sizeof(int), 1, f);
    fread(vehicles, sizeof(Vehicle), vehicleCount, f);
    fclose(f);
}
void saveDrivers() {
    FILE *f = fopen(DRV_FILE, "wb");
    if (!f) { printf("[ERROR] Cannot save drivers.\n"); return; }
    fwrite(&driverCount, sizeof(int), 1, f);
    fwrite(drivers, sizeof(Driver), driverCount, f);
    fclose(f);
}
void loadDrivers() {
    FILE *f = fopen(DRV_FILE, "rb");
    if (!f) return;
    fread(&driverCount, sizeof(int), 1, f);
    fread(drivers, sizeof(Driver), driverCount, f);
    fclose(f);
}

/* FIX: Zone persistence — type/max_bins/distances now survive restart */
void saveZones() {
    FILE *f = fopen(ZONE_FILE, "wb");
    if (!f) return;
    fwrite(&zoneCount,  sizeof(int),             1, f);
    fwrite(zones,       sizeof(Zone),  zoneCount, f);
    fwrite(zoneNames,   sizeof(zoneNames),        1, f);
    fwrite(distMatrix,  sizeof(distMatrix),       1, f);
    fclose(f);
}
void loadZones() {
    FILE *f = fopen(ZONE_FILE, "rb");
    if (!f) return;
    fread(&zoneCount,  sizeof(int),             1, f);
    fread(zones,       sizeof(Zone),  zoneCount, f);
    fread(zoneNames,   sizeof(zoneNames),        1, f);
    fread(distMatrix,  sizeof(distMatrix),       1, f);
    fclose(f);
}
void saveLog() {
    FILE *f = fopen(LOG_FILE, "ab");
    if (!f) return;
    fwrite(daily_log, sizeof(LogEntry), logCount, f);
    fclose(f);
}

/* ============================================================ SCHEDULE FILE */
void writeScheduleFile() {
    FILE *f = fopen(SCHEDULE_FILE, "w");
    if (!f) { printf("[WARN] Could not write schedule file.\n"); return; }
    char ts[30]; getCurrentTime(ts, sizeof(ts));
    fprintf(f, "=================================================\n");
    fprintf(f, "  DAILY WASTE COLLECTION SCHEDULE\n");
    fprintf(f, "  Generated: %s\n", ts);
    fprintf(f, "=================================================\n\n");
    const char *vtypes[] = {"Regular","Compactor","Hazmat"};
    for (int j = 0; j < vehicleCount; j++) {
        if (vehicles[j].available) continue;
        fprintf(f, "Vehicle V%d  [%s]\n", vehicles[j].id, vtypes[vehicles[j].type]);
        fprintf(f, "  Driver  : D%d\n",   vehicles[j].driver_id);
        fprintf(f, "  Zone    : %s\n",    vehicles[j].assigned_zone);
        fprintf(f, "  Bins    :\n");
        float totalKg = 0;
        for (int i = 0; i < binCount; i++) {
            if (!bins[i].assigned) continue;
            if (strcmp(bins[i].zone, vehicles[j].assigned_zone) != 0) continue;
            const char *wtypes[] = {"Bio","NonBio","Hazard"};
            fprintf(f, "    - Bin %-4d  %.1f%%  %s  %.1fkg  %s\n",
                    bins[i].id, bins[i].fill, wtypes[bins[i].waste_type],
                    bins[i].current_kg, bins[i].is_emergency ? "[EMERGENCY]" : "");
            totalKg += bins[i].current_kg;
        }
        fprintf(f, "  Load    : %.1f / %.0f kg\n", totalKg, vehicles[j].max_capacity_kg);
        int zIdx = getZoneIndex(vehicles[j].assigned_zone);
        float dist = (zIdx >= 0 && zoneCount > 1) ? distMatrix[0][zIdx] * 2 : 10.0f;
        fprintf(f, "  Est Dist: %.1f km\n", dist);
        fprintf(f, "  Est Fuel: %.2f L\n\n", dist * vehicles[j].consumption_rate);
    }
    int unassigned = 0;
    for (int i = 0; i < binCount; i++)
        if (bins[i].fill >= THRESHOLD && !bins[i].assigned && !bins[i].collected_today)
            unassigned++;
    if (unassigned > 0)
        fprintf(f, "WARNING: %d critical bin(s) unassigned (insufficient resources).\n", unassigned);
    fprintf(f, "\n=================================================\n");
    fclose(f);
    printf("  [SCHEDULE] Written to '%s'\n", SCHEDULE_FILE);
}
