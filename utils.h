#ifndef UTILS_H
#define UTILS_H

void clearBuffer(void);
void pause(void);
void getCurrentTime(char *buf, int size);
void toLowerStr(char *s);

int  getZoneIndex(const char *zone);
int  addZoneInternal(const char *normName, const char *type, int max_bins);
int  addZone(const char *name);
void setZoneDistance(void);
void viewDistanceMatrix(void);

int binIdExists(int id);
int vehicleIdExists(int id);
int driverIdExists(int id);

int  calcPriority(float fill, int is_emergency);
void sortBinsByPriority(int *indices, int count);

void saveBins(void);
void loadBins(void);
void saveVehicles(void);
void loadVehicles(void);
void saveDrivers(void);
void loadDrivers(void);
void saveZones(void);
void loadZones(void);
void saveLog(void);

void writeScheduleFile(void);

#endif /* UTILS_H */
