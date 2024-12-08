// cfg.h
#ifndef CFG_H
#define CFG_H

// Config structure - encapsulates all settings
typedef struct {
    // Timing
    int frameTime;
    float initialTime;
    int carSpawnFactor;
    int carSpeedChangeFactor;
    int quitTime;

    // Area
    int playableRows;
    int statusRows;
    int cols;
    int offy;
    int offx;

    // Frog
    int frogMoveFactor;
    int frogWidth;
    int frogHeight;
    char** frogShape;

    // Cars
    int nCars;
    int nNeutralCars;
    int nFriendlyCars;
    int neutralCarStopThreshold;
    int carMinMoveFactor;
    int carMaxMoveFactor;
    int carWidth;
    int carHeight;
    char** carShape;

    // Stork
    int storkMoveFactor;
    int storkWidth;
    int storkHeight;
    char** storkShape;

    // Controls
    char up;
    char down;
    char left;
    char right;
    char request;
    char quit;
} CFG;

// --- CFG FUNCTIONS ---
// Load default values for each section
void LoadTimingDefaults(CFG* cfg);
void LoadAreaDefaults(CFG* cfg);
void LoadFrogDefaults(CFG* cfg);
void LoadCarsDefaults(CFG* cfg);
void LoadStorkDefaults(CFG* cfg);
void LoadControlsDefaults(CFG* cfg);
void LoadCfgDefaults(CFG* cfg);

// Utilities for handling shapes
void FreeShape(char** shape, int height);
void ReadShapeFromFile(FILE* file, char*** shape, int width, int height);

// Load configuration from file for each section
void LoadTimingFromFile(CFG* cfg, FILE* file);
void LoadAreaFromFile(CFG* cfg, FILE* file);
void LoadFrogFromFile(CFG* cfg, FILE* file);
void LoadCarsFromFile(CFG* cfg, FILE* file);
void LoadControlsFromFile(CFG* cfg, FILE* file);
void LoadCfgFromFile(CFG* cfg);
CFG* InitCfg();

#endif // CFG_H