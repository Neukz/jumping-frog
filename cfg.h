// cfg.h
#ifndef CFG_H
#define CFG_H

// --- CFG STRUCTURES ---
// Timing
typedef struct {
    unsigned int frameTime;
    int initialTime;
    int quitTime;
} TIMING_CFG;

// Area
typedef struct {
    int playableRows;
    int statusRows;
    int cols;
    int offy;
    int offx;
} AREA_CFG;

// Frog
typedef struct {
    int moveFactor;
    int width;
    int height;
    char** shape;
} FROG_CFG;

// Cars
typedef struct {
    int nCars;
    int moveFactor;
    int width;
    int height;
    char** shape;
} CARS_CFG;

// Controls
typedef struct {
    int up;
    int down;
    int left;
    int right;
    int quit;
} CONTROLS_CFG;

// Config structure - encapsulates all settings
typedef struct {
    TIMING_CFG* timing;
    AREA_CFG* area;
    FROG_CFG* frog;
    CARS_CFG* cars;
    CONTROLS_CFG* controls;
} CFG;

// --- CFG FUNCTIONS ---
void LoadDefaultTimingCfg(TIMING_CFG* timing);
void LoadDefaultAreaCfg(AREA_CFG* area);
void LoadDefaultFrogCfg(FROG_CFG* frog);
void LoadDefaultCarsCfg(CARS_CFG* cars);
void LoadDefaultControlsCfg(CONTROLS_CFG* controls);
void LoadDefaultCfg(CFG* cfg);
CFG* InitCfg();

#endif // CFG_H