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
// Load default values for each configuration section
void LoadTimingDefaults(TIMING_CFG* timing);
void LoadAreaDefaults(AREA_CFG* area);
void LoadFrogDefaults(FROG_CFG* frog);
void LoadCarsDefaults(CARS_CFG* cars);
void LoadControlsDefaults(CONTROLS_CFG* controls);
void LoadCfgDefaults(CFG* cfg);

// Load configuration from file for each section
void LoadTimingFromFile(TIMING_CFG* timing, FILE* file);
void LoadAreaFromFile(AREA_CFG* area, FILE* file);
void LoadFrogFromFile(FROG_CFG* frog, FILE* file);
void LoadCarsFromFile(CARS_CFG* cars, FILE* file);
void LoadControlsFromFile(CONTROLS_CFG* controls, FILE* file);
void LoadCfgFromFile(CFG* cfg, const char* filename);
CFG* InitCfg();

#endif // CFG_H