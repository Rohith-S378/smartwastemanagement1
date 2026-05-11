#ifndef BINS_H
#define BINS_H

/*
 * bins.h
 * ------
 * Declarations for Bin Management (admin) and Emergency handler.
 */

void identifyCriticalBins(void);
void addBin(void);
void viewBins(void);
void updateBinStatus(void);

void autoEmergency(int binArrayIdx);
void emergencyHandler(void);

void menuBins(void);

#endif /* BINS_H */
