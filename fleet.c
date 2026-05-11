/*
 * fleet.c
 * -------
 * Vehicle Management: add (standard preset or custom), view.
 * Driver Management: add, view.
 * Admin sub-menu: menuFleet().
 */

#include "../include/globals.h"
#include "../include/utils.h"
#include "../include/fleet.h"

/* ============================================================
   VEHICLE PRESET TABLE
   ============================================================ */
const VehiclePreset VEHICLE_PRESETS[3] = {
    { "Regular Truck  (Tata 407)",       REGULAR,    800.0f,  60.0f, 0.12f, 18.0f, 500.0f },
    { "Compactor Truck (Ashok)",          COMPACTOR, 2500.0f, 120.0f, 0.18f, 25.0f, 800.0f },
    { "Hazmat Van     (Force Traveller)", HAZMAT,    400.0f,  50.0f, 0.10f, 30.0f, 400.0f }
};

/* ============================================================
   VEHICLE MODULE — Standard OR Custom
   ============================================================ */
void addVehicle() {
    if (vehicleCount >= MAX_VEHICLES) { printf("[ERROR] Max vehicles reached.\n"); return; }
    printf("\n--- ADD VEHICLE ---\n");
    printf("  1. Standard preset\n");
    printf("  2. Custom vehicle\n");
    printf("  Choice: ");
    int mode; scanf("%d", &mode);

    Vehicle v; memset(&v, 0, sizeof(Vehicle));
    do {
        printf("Vehicle ID (unique number): ");
        if (scanf("%d", &v.id) != 1) { clearBuffer(); continue; }
        if (vehicleIdExists(v.id)) printf("[ERROR] ID exists.\n");
        else break;
    } while (1);

    if (mode == 1) {
        printf("Select preset:\n");
        for (int i = 0; i < 3; i++)
            printf("  %d. %s\n", i+1, VEHICLE_PRESETS[i].label);
        printf("  Choice (1-3): ");
        int choice; scanf("%d", &choice);
        if (choice < 1 || choice > 3) { printf("[ERROR] Invalid.\n"); return; }
        const VehiclePreset *p = &VEHICLE_PRESETS[choice-1];
        v.type                    = p->type;
        v.max_capacity_kg         = p->max_capacity_kg;
        v.fuel_tank_capacity      = p->fuel_tank_capacity;
        v.fuel_level              = p->fuel_tank_capacity;
        v.consumption_rate        = p->consumption_rate;
        v.cost_per_km             = p->cost_per_km;
        v.maintenance_interval_km = p->maintenance_interval_km;
        printf("[OK] Preset applied: %s\n", p->label);
    } else {
        printf("Type (0=Regular  1=Compactor  2=Hazmat): ");
        int t; scanf("%d", &t);
        v.type = (t >= 0 && t <= 2) ? (VehicleType)t : REGULAR;
        printf("Max capacity (kg): ");            scanf("%f", &v.max_capacity_kg);
        printf("Fuel tank capacity (L): ");       scanf("%f", &v.fuel_tank_capacity);
        v.fuel_level = v.fuel_tank_capacity;
        printf("Fuel consumption rate (L/km): "); scanf("%f", &v.consumption_rate);
        printf("Cost per km (INR): ");            scanf("%f", &v.cost_per_km);
        printf("Maintenance interval (km): ");    scanf("%f", &v.maintenance_interval_km);
        if (v.maintenance_interval_km <= 0) v.maintenance_interval_km = 500.0f;
    }

    v.available        = 1;
    v.under_maintenance = 0;
    v.driver_id        = -1;
    vehicles[vehicleCount++] = v;
    saveVehicles();
    printf("[OK] Vehicle %d added. Cap: %.0fkg | Tank: %.0fL | Rate: %.2fL/km | INR %.0f/km\n",
           v.id, v.max_capacity_kg, v.fuel_tank_capacity,
           v.consumption_rate, v.cost_per_km);
}

void viewVehicles() {
    if (vehicleCount == 0) { printf("\nNo vehicles.\n"); return; }
    const char *vtypes[] = {"Regular","Compactor","Hazmat"};
    printf("\n  %-3s  %-9s  %7s  %10s  %-13s\n",
           "ID","Type","Cap(kg)","Fuel(L)","Status");
    printf("  ---  ---------  -------  ----------  -------------\n");
    for (int i = 0; i < vehicleCount; i++)
        printf("  V%-2d  %-9s  %7.0f  %5.1f/%-4.0f  %s\n",
               vehicles[i].id, vtypes[vehicles[i].type],
               vehicles[i].max_capacity_kg,
               vehicles[i].fuel_level, vehicles[i].fuel_tank_capacity,
               vehicles[i].under_maintenance ? "[MAINTENANCE]" :
               vehicles[i].available ? "[Available]" : "[Busy]");
}

/* ============================================================
   DRIVER MODULE
   ============================================================ */
void addDriver() {
    if (driverCount >= MAX_DRIVERS) { printf("[ERROR] Max drivers reached.\n"); return; }
    Driver d; memset(&d, 0, sizeof(Driver));
    printf("\n--- ADD DRIVER ---\n");
    do {
        printf("Driver ID: ");
        if (scanf("%d", &d.id) != 1) { clearBuffer(); continue; }
        if (driverIdExists(d.id)) printf("[ERROR] ID exists.\n");
        else break;
    } while (1);

    d.max_hours       = MAX_WORK_HOURS;
    d.salary_per_hour = 80.0f;
    printf("Default shift: 07:00-15:00. Change it? (y/n): ");
    char ch; scanf(" %c", &ch);
    if (ch == 'y' || ch == 'Y') {
        printf("Shift start (HH:MM): "); scanf("%9s", d.shift_start);
        printf("Shift end   (HH:MM): "); scanf("%9s", d.shift_end);
    } else {
        strcpy(d.shift_start, "07:00"); strcpy(d.shift_end, "15:00");
    }
    printf("Salary per hour INR [default 80, press 0 to keep]: ");
    float sal; scanf("%f", &sal);
    if (sal > 0) d.salary_per_hour = sal;

    d.available  = 1;
    d.vehicle_id = -1;
    drivers[driverCount++] = d;
    saveDrivers();
    printf("[OK] Driver %d | Shift %s-%s | INR %.0f/hr\n",
           d.id, d.shift_start, d.shift_end, d.salary_per_hour);
}

void viewDrivers() {
    if (driverCount == 0) { printf("\nNo drivers.\n"); return; }
    printf("\n  %-3s  %-12s  %-10s  %-11s\n","ID","Hours","Salary/hr","Status");
    printf("  ---  ------------  ----------  -----------\n");
    for (int i = 0; i < driverCount; i++)
        printf("  D%-2d  %.1f/%.0fh %s  INR %-5.0f  %s\n",
               drivers[i].id,
               drivers[i].worked_hours, drivers[i].max_hours,
               drivers[i].worked_hours >= drivers[i].max_hours ? "[LIMIT]" : "       ",
               drivers[i].salary_per_hour,
               drivers[i].available ? "[Available]" : "[Assigned]");
}

/* ============================================================
   ADMIN SUB-MENU: FLEET MANAGEMENT
   ============================================================ */
void menuFleet() {
    int ch;
    do {
        printf("\n  FLEET MANAGEMENT\n");
        printf("  1. Add Vehicle  (Standard or Custom)\n");
        printf("  2. View Vehicles\n");
        printf("  3. Add Driver\n");
        printf("  4. View Drivers\n");
        printf("  0. Back\n  > ");
        if (scanf("%d", &ch) != 1) { clearBuffer(); continue; }
        switch (ch) {
            case 1: addVehicle();  break;
            case 2: viewVehicles();break;
            case 3: addDriver();   break;
            case 4: viewDrivers(); break;
            case 0: break;
            default: printf("[ERROR] Invalid.\n");
        }
    } while (ch != 0);
}
