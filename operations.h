#ifndef OPERATIONS_H
#define OPERATIONS_H

/*
 * operations.h
 * ------------
 * Declarations for scheduling, route optimisation, simulation,
 * and zone management menu.
 */

void optimizeRoute(int *zoneIdxList, int n, int *routeOut, float *totalDist);
void assignResources(void);
void simulateCollection(void);
void routePlan(void);
void resetDayState(void);
void loadSeedData(void);

void menuZones(void);
void menuOps(void);

#endif /* OPERATIONS_H */
