// cfg.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg.h"

// --- DEFAULT SETTINGS ---
// Timing
const int FRAME_TIME = 25;      // milliseconds interval between frames
const int INITIAL_TIME = 20;
const int QUIT_TIME = 3;        // seconds to wait after hitting QUIT

// Area
const int PLAYABLE_ROWS = 35;
const int STATUS_ROWS = 3;
const int PLAYABLE_COLS = 100;  // the same for both windows
const int OFFY = 0;             // optional: window offset within the main window
const int OFFX = 0;

// Frog
const int FROG_MOVE_FACTOR = 5;
const int FROG_WIDTH = 6;
const int FROG_HEIGHT = 3;

// Cars
const int N_CARS = 5;
const int CAR_MOVE_FACTOR = 2;
const int CAR_WIDTH = 8;
const int CAR_HEIGHT = 3;

// Controls
const int UP = 'w';
const int DOWN = 's';
const int LEFT = 'a';
const int RIGHT = 'd';
const int QUIT = 'q';


// --- LOADING DEFAULT CONFIGURATION ---
void LoadTimingDefaults(TIMING_CFG* timing)
{
    timing->frameTime = FRAME_TIME;
    timing->initialTime = INITIAL_TIME;
    timing->quitTime = QUIT_TIME;
}

void LoadAreaDefaults(AREA_CFG* area)
{
    area->playableRows = PLAYABLE_ROWS;
    area->statusRows = STATUS_ROWS;
    area->cols = PLAYABLE_COLS;
    area->offy = OFFY;
    area->offx = OFFX;
}

void LoadFrogDefaults(FROG_CFG* frog)
{
    frog->moveFactor = FROG_MOVE_FACTOR;
    frog->width = FROG_WIDTH;
    frog->height = FROG_HEIGHT;
    frog->shape = malloc(frog->height * sizeof(char*));
    for (int i = 0; i < frog->height; i++) {
        frog->shape[i] = malloc((frog->width + 1) * sizeof(char));
    }
    strcpy(frog->shape[0], " @..@ ");
    strcpy(frog->shape[1], "(----)");
    strcpy(frog->shape[2], " ^  ^ ");
}

void LoadCarsDefaults(CARS_CFG* cars)
{
    cars->nCars = N_CARS;
    cars->moveFactor = CAR_MOVE_FACTOR;
    cars->width = CAR_WIDTH;
    cars->height = CAR_HEIGHT;
    cars->shape = malloc(cars->height * sizeof(char*));
    for (int i = 0; i < cars->height; i++) {
        cars->shape[i] = malloc((cars->width + 1) * sizeof(char));
    }
    strcpy(cars->shape[0], "  ____  ");
    strcpy(cars->shape[1], "_/____\\_");
    strcpy(cars->shape[2], " O    O ");
}

void LoadControlsDefaults(CONTROLS_CFG* controls)
{
    controls->up = UP;
    controls->down = DOWN;
    controls->left = LEFT;
    controls->right = RIGHT;
    controls->quit = QUIT;
}

// Load default configuration
void LoadCfgDefaults(CFG* cfg)
{
    cfg->timing = (TIMING_CFG*)malloc(sizeof(TIMING_CFG));
    LoadTimingDefaults(cfg->timing);

    cfg->area = (AREA_CFG*)malloc(sizeof(AREA_CFG));
    LoadAreaDefaults(cfg->area);

    cfg->frog = (FROG_CFG*)malloc(sizeof(FROG_CFG));
    LoadFrogDefaults(cfg->frog);

    cfg->cars = (CARS_CFG*)malloc(sizeof(CARS_CFG));
    LoadCarsDefaults(cfg->cars);

    cfg->controls = (CONTROLS_CFG*)malloc(sizeof(CONTROLS_CFG));
    LoadControlsDefaults(cfg->controls);
}


// --- LOADING CONFIGURATION FROM FILE ---
void LoadTimingFromFile(TIMING_CFG* timing, FILE* file)
{
    fscanf(file, "FRAME_TIME=%d\n", &timing->frameTime);
    fscanf(file, "INITIAL_TIME=%d\n", &timing->initialTime);
    fscanf(file, "QUIT_TIME=%d\n", &timing->quitTime);
}

void LoadAreaFromFile(AREA_CFG* area, FILE* file)
{
    fscanf(file, "PLAYABLE_ROWS=%d\n", &area->playableRows);
    fscanf(file, "STATUS_ROWS=%d\n", &area->statusRows);
    fscanf(file, "COLS=%d\n", &area->cols);
    fscanf(file, "OFFY=%d\n", &area->offy);
    fscanf(file, "OFFX=%d\n", &area->offx);
}

void LoadFrogFromFile(FROG_CFG* frog, FILE* file)
{
    fscanf(file, "FROG_MOVE_FACTOR=%d\n", &frog->moveFactor);
    fscanf(file, "FROG_WIDTH=%d\n", &frog->width);
    fscanf(file, "FROG_HEIGHT=%d\n", &frog->height);
    // TODO: handle shape assignment
}

void LoadCarsFromFile(CARS_CFG* cars, FILE* file)
{
    fscanf(file, "N_CARS=%d\n", &cars->nCars);
    fscanf(file, "CAR_MOVE_FACTOR=%d\n", &cars->moveFactor);
    fscanf(file, "CAR_WIDTH=%d\n", &cars->width);
    fscanf(file, "CAR_HEIGHT=%d\n", &cars->height);
    // TODO: handle shape assignment
}

void LoadControlsFromFile(CONTROLS_CFG* controls, FILE* file)
{
    fscanf(file, "UP=%d\n", &controls->up);
    fscanf(file, "DOWN=%d\n", &controls->down);
    fscanf(file, "LEFT=%d\n", &controls->left);
    fscanf(file, "RIGHT=%d\n", &controls->right);
    fscanf(file, "QUIT=%d\n", &controls->quit);
}

// Load config from file and override default values
void LoadCfgFromFile(CFG* cfg, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return; // file not found
    }

    while (!feof(file)) {   // read config file and ovrride defaults
        char section[20];
        fscanf(file, "%s\n", section);
        if (strcmp(section, "---TIMING---") == 0) {
            LoadTimingFromFile(cfg->timing, file);
        }
        else if (strcmp(section, "---AREA---") == 0) {
            LoadAreaFromFile(cfg->area, file);
        }
        else if (strcmp(section, "---FROG---") == 0) {
            LoadFrogFromFile(cfg->frog, file);
        }
        else if (strcmp(section, "---CARS---") == 0) {
            LoadCarsFromFile(cfg->cars, file);
        }
        else if (strcmp(section, "---CONTROLS---") == 0) {
            LoadControlsFromFile(cfg->controls, file);
        }
    }

    fclose(file);
}

// Config initializer
CFG* InitCfg()
{
    const char* filename = "settings.txt";  // default config file
    CFG* cfg = (CFG*)malloc(sizeof(CFG));
    LoadCfgDefaults(cfg);
    LoadCfgFromFile(cfg, filename);
    return cfg;
}