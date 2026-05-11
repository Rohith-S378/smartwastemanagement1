/*
 * civilian.c — Waste item dictionary + Citizen Portal
 *
 * FIX: Complaint escalation (zLocal==0 path) now creates an emergency
 *      bin and calls autoEmergency() instead of just printing [LOGGED].
 */

#include "../include/globals.h"
#include "../include/utils.h"
#include "../include/bins.h"
#include "../include/civilian.h"

void initItemDictionary() {
    strcpy(ITEM_DICT[itemDictSize].name,      "Food_Scraps_Small");
    ITEM_DICT[itemDictSize].category        = BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 1.5f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Food_Scraps_Large");
    ITEM_DICT[itemDictSize].category        = BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 4.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Fruit_Vegetable_Waste");
    ITEM_DICT[itemDictSize].category        = BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 5.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Garden_Leaves");
    ITEM_DICT[itemDictSize].category        = BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 6.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Cooked_Food_Bag");
    ITEM_DICT[itemDictSize].category        = BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 2.5f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Cardboard_Box_Small");
    ITEM_DICT[itemDictSize].category        = BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 8.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Newspaper_Bundle");
    ITEM_DICT[itemDictSize].category        = BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 3.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Plastic_Bottle");
    ITEM_DICT[itemDictSize].category        = NON_BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 2.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Plastic_Bag");
    ITEM_DICT[itemDictSize].category        = NON_BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 1.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Glass_Bottle");
    ITEM_DICT[itemDictSize].category        = NON_BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 3.5f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Aluminium_Can");
    ITEM_DICT[itemDictSize].category        = NON_BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 1.5f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Styrofoam_Box");
    ITEM_DICT[itemDictSize].category        = NON_BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 7.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Clothing_Item");
    ITEM_DICT[itemDictSize].category        = NON_BIODEGRADABLE;
    ITEM_DICT[itemDictSize++].fill_increase = 5.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Paint_Can");
    ITEM_DICT[itemDictSize].category        = HAZARDOUS;
    ITEM_DICT[itemDictSize++].fill_increase = 6.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Battery");
    ITEM_DICT[itemDictSize].category        = HAZARDOUS;
    ITEM_DICT[itemDictSize++].fill_increase = 2.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Medical_Waste_Bag");
    ITEM_DICT[itemDictSize].category        = HAZARDOUS;
    ITEM_DICT[itemDictSize++].fill_increase = 5.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Chemical_Container");
    ITEM_DICT[itemDictSize].category        = HAZARDOUS;
    ITEM_DICT[itemDictSize++].fill_increase = 8.0f;
    strcpy(ITEM_DICT[itemDictSize].name,      "Pesticide_Bottle");
    ITEM_DICT[itemDictSize].category        = HAZARDOUS;
    ITEM_DICT[itemDictSize++].fill_increase = 3.5f;
}

void showItemDictionary(int filterCat) {
    const char *catHeaders[] = {
        "--- BIODEGRADABLE ITEMS ---",
        "--- NON-BIODEGRADABLE ITEMS ---",
        "--- HAZARDOUS ITEMS ---"
    };
    printf("\n%s\n", catHeaders[filterCat]);
    printf("  #   Item                       Fill%%/unit\n");
    printf("  --  -------------------------  ----------\n");
    int shown = 0;
    for (int i = 0; i < itemDictSize; i++) {
        if (ITEM_DICT[i].category == (WasteCategory)filterCat) {
            printf("  %-2d  %-25s  %5.1f%%\n",
                   i+1, ITEM_DICT[i].name, ITEM_DICT[i].fill_increase);
            shown++;
        }
    }
    if (!shown) printf("  (none)\n");
}

void showFullDictionary() {
    printf("\n=== FULL WASTE ITEM DICTIONARY ===\n");
    printf("  #   %-25s  %-16s  Fill%%\n", "Item", "Category");
    printf("  --  %-25s  %-16s  -----\n","-------------------------","----------------");
    const char *catNames[] = {"Biodegradable","Non-Biodegradable","Hazardous"};
    for (int i = 0; i < itemDictSize; i++)
        printf("  %-2d  %-25s  %-16s  %5.1f%%\n",
               i+1, ITEM_DICT[i].name,
               catNames[ITEM_DICT[i].category],
               ITEM_DICT[i].fill_increase);
}

/* ================================================================
   CITIZEN PORTAL
   FIX: When no bin exists in the zone (zLocal==0), take the
        citizen's complaint, auto-create an emergency bin in that
        zone (if zone is registered), and call autoEmergency().
        This replaces the previous dead-code [LOGGED] path.
   ================================================================ */
void civilianPortal() {
    printf("\n=== CITIZEN WASTE REPORTING PORTAL ===\n");
    char zone[30];
    printf("Your zone/area name: "); scanf("%29s", zone);
    toLowerStr(zone);

    int zoneBins[MAX_BINS], zLocal = 0;
    for (int i = 0; i < binCount; i++)
        if (strcmp(bins[i].zone, zone) == 0 && !bins[i].collected_today)
            zoneBins[zLocal++] = i;

    /* ---- FIX: complaint escalation pipeline ---- */
    if (zLocal == 0) {
        printf("\n%s[ALERT]%s No active bin found in zone '%s'.\n",
               ANSI_YELLOW, ANSI_RESET, zone);
        printf("Please describe the overflow / issue:\n> ");
        char complaint[200]; clearBuffer(); fgets(complaint, 200, stdin);

        int zi = getZoneIndex(zone);
        if (zi == -1 || binCount >= MAX_BINS) {
            /* Zone unknown or bins full — log only */
            printf("[LOGGED] Complaint recorded. Supervisor notified.\n");
            printf("         Zone '%s' not registered — admin must add it.\n", zone);
            return;
        }

        /* Auto-create emergency bin from complaint */
        Bin e; memset(&e, 0, sizeof(Bin));
/* Generate a safe unique ID starting at 900 */
e.id = 900 + binCount;
while (binIdExists(e.id)) e.id++;
        e.is_emergency = 1;
        e.fill         = 100.0f;
        e.capacity_kg  = 120.0f;
        e.current_kg   = 120.0f;
        e.priority     = 0;
        e.waste_type   = NON_BIODEGRADABLE;
        e.recyclable_pct = 50.0f;
        strcpy(e.zone, zone);
        getCurrentTime(e.last_collected, sizeof(e.last_collected));
        bins[binCount++] = e;
        zones[zi].bin_count++;
        saveBins(); saveZones();

        printf("[ESCALATED] Emergency bin #%d auto-registered in zone '%s'.\n",
               e.id, zone);
        printf("            Dispatching nearest available vehicle...\n");
        autoEmergency(binCount - 1);
        return;
    }
    /* ---- end fix ---- */

    const char *catNames[] = {"Biodegradable","Non-Biodegradable","Hazardous"};
    printf("\nBins in zone '%s':\n", zone);
    for (int i = 0; i < zLocal; i++) {
        Bin *b = &bins[zoneBins[i]];
        char barStr[21]; int bk;
        int filled = (int)(b->fill/5);
        for (bk=0; bk<filled&&bk<20; bk++) barStr[bk]='#';
        for (;bk<20;bk++) barStr[bk]='-';
        barStr[20]='\0';
        const char *col = (b->fill>=90)?ANSI_RED:(b->fill>=70)?ANSI_YELLOW:ANSI_GREEN;
        printf("  %d. Bin %-4d  %s[%s]%s%4.0f%%  %s\n",
               i+1, b->id, col, barStr, ANSI_RESET, b->fill, catNames[b->waste_type]);
    }

    int pick = 1;
    if (zLocal > 1) {
        printf("Which bin are you at? (1-%d): ", zLocal);
        scanf("%d", &pick);
        if (pick < 1 || pick > zLocal) pick = 1;
    }

    int binIdx = zoneBins[pick-1];
    WasteCategory binCat = bins[binIdx].waste_type;
    printf("\nBin %d selected — %.0f%% full. Accepts: %s ONLY.\n\n",
           bins[binIdx].id, bins[binIdx].fill, catNames[binCat]);
    showItemDictionary((int)binCat);

    int more = 1;
    while (more && bins[binIdx].fill < 100.0f) {
        printf("\nSelect item (1-%d): ", itemDictSize);
        int choice;
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > itemDictSize) {
            printf("[ERROR] Invalid.\n"); clearBuffer(); continue;
        }
        WasteItem *item = &ITEM_DICT[choice-1];
        if (item->category != binCat) {
            printf("[REJECTED] '%s' is %s waste — wrong bin.\n",
                   item->name, catNames[item->category]);
            printf("           Please use the %s bin.\n", catNames[item->category]);
            printf("Try another? (1=Yes / 0=No): "); scanf("%d", &more); continue;
        }
        int qty; printf("Quantity: "); scanf("%d", &qty);
        if (qty < 1) qty = 1;
        float increase = item->fill_increase * qty;
        bins[binIdx].fill = fminf(100.0f, bins[binIdx].fill + increase);
        bins[binIdx].current_kg = bins[binIdx].capacity_kg * bins[binIdx].fill / 100.0f;
        bins[binIdx].priority   = calcPriority(bins[binIdx].fill, 0);
        printf("[OK] +%dx %s (+%.1f%%). Bin now %.1f%%.\n",
               qty, item->name, increase, bins[binIdx].fill);
        if (bins[binIdx].fill >= 90.0f)
            printf("%s[ALERT] Bin nearly full — municipal team notified.%s\n",
                   ANSI_YELLOW, ANSI_RESET);
        if (bins[binIdx].fill >= 100.0f) {
            saveBins(); autoEmergency(binIdx); return;
        }
        printf("Add more? (1=Yes / 0=No): "); scanf("%d", &more);
    }
    saveBins();
    printf("\nThank you! Authorities have been notified.\n");
}
