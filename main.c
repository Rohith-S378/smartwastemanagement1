/*
 * main.c — Entry point, corporate portal, main()
 *
 * FIX: Zone rebuild from bin data now:
 *   1. Loads zones.dat FIRST (preserves real type/max_bins/distances)
 *   2. Only fallback-creates unknown zones (not already in zones.dat)
 *   3. Correctly counts bins into zones[zi].bin_count during rebuild
 *
 * Build (from waste_mgmt folder):
 *   gcc -std=c11 -Wall -Iinclude src/globals.c src/utils.c src/bins.c \
 *       src/fleet.c src/operations.c src/reports.c src/civilian.c \
 *       src/main.c -o waste_mgmt.exe -lm
 */

#include "../include/globals.h"
#include "../include/utils.h"
#include "../include/bins.h"
#include "../include/fleet.h"
#include "../include/operations.h"
#include "../include/reports.h"
#include "../include/civilian.h"

/* ============================================================
   CORPORATE ADMIN PORTAL
   ============================================================ */
void corporatePortal() {
    printf("[AUTO] Checking for critical bins...\n");
    assignResources();

    int ch;
    do {
        printf("\n=== CORPORATE ADMIN PORTAL ===\n");
        printf("  1. Bin Management\n");
        printf("  2. Fleet Management  (Vehicles & Drivers)\n");
        printf("  3. Zone Setup\n");
        printf("  4. Operations\n");
        printf("  5. Reports\n");
        printf("  6. Load Demo Seed Data\n");
        printf("  0. Logout\n  > ");
        if (scanf("%d", &ch) != 1) { clearBuffer(); continue; }
        switch (ch) {
            case 1: menuBins();     break;
            case 2: menuFleet();    break;
            case 3: menuZones();    break;
            case 4: menuOps();      break;
            case 5: menuReports();  break;
            case 6: loadSeedData(); break;
            case 0: printf("[Logged out]\n"); break;
            default: printf("[ERROR] Invalid.\n");
        }
    } while (ch != 0);
}

/* ============================================================
   MAIN
   ============================================================ */
int main() {
    /* Enable ANSI colours on Windows CMD if needed */
#ifdef _WIN32
    system("color 07");
#endif

    initItemDictionary();

    /* FIX: Load zones FIRST so real type/max_bins/distances are restored */
    loadZones();
    loadBins();
    loadVehicles();
    loadDrivers();

    /* FIX: Rebuild zone bin_count from loaded bin data.
            Only creates fallback zone entry if not already in zones.dat. */
    for (int i = 0; i < binCount; i++) {
        toLowerStr(bins[i].zone);
        int zi = getZoneIndex(bins[i].zone);
        if (zi == -1)
            zi = addZoneInternal(bins[i].zone, "Residential", 20);
        if (zi >= 0)
            zones[zi].bin_count++;   /* FIX: actually count the bin in */
    }

    printf("\n%s=== SMART WASTE MANAGEMENT SYSTEM  v4 ===%s\n", ANSI_BOLD, ANSI_RESET);
    printf("Loaded: %d bins | %d vehicles | %d drivers | %d zones\n\n",
           binCount, vehicleCount, driverCount, zoneCount);

    int ch;
    do {
        printf("  1. Corporate / Admin Login\n");
        printf("  2. Citizen Portal\n");
        printf("  3. Exit\n  > ");
        if (scanf("%d", &ch) != 1) { clearBuffer(); continue; }
        if (ch == 1) {
    int pin;
    printf("Admin PIN (or 0 to reset): "); scanf("%d", &pin);

    if (pin == 0) {
        /* Password reset flow */
        char answer[40];
        printf("Security question: What is your team name? ");
        scanf("%39s", answer);
        toLowerStr(answer);
        if (strcmp(answer, RECOVERY_ANSWER) == 0) {
            int newPin, confirm;
            printf("Enter new PIN: ");    scanf("%d", &newPin);
            printf("Confirm new PIN: ");  scanf("%d", &confirm);
            if (newPin == confirm && newPin > 0) {
                /* Write new PIN to a file so it persists */
                FILE *pf = fopen("admin_pin.dat", "wb");
                if (pf) { fwrite(&newPin, sizeof(int), 1, pf); fclose(pf); }
                printf("[OK] PIN updated. Please log in with new PIN.\n");
            } else {
                printf("[ERROR] PINs don't match or invalid.\n");
            }
        } else {
            printf("[ACCESS DENIED] Wrong answer.\n");
        }
        continue;
    }

    /* Load PIN from file if it exists */
    int activePin = ADMIN_PIN;
    FILE *pf = fopen("admin_pin.dat", "rb");
    if (pf) { fread(&activePin, sizeof(int), 1, pf); fclose(pf); }

    if (pin != activePin) { printf("[ACCESS DENIED]\n"); continue; }
    corporatePortal();
} else if (ch == 2) {
            civilianPortal();
        } else if (ch == 3) {
            printf("Goodbye.\n");
        } else {
            printf("[ERROR] Invalid.\n");
        }
    } while (ch != 3);
    return 0;
}
