// cfg.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg.h"

// --- LOADING DEFAULT CONFIGURATION ---
void LoadTimingDefaults(TIMING_CFG* timing)
{
    timing->frameTime = 25;
    timing->initialTime = 20;
    timing->quitTime = 3;
}

void LoadAreaDefaults(AREA_CFG* area)
{
    area->playableRows = 35;
    area->statusRows = 3;
    area->cols = 100;
    area->offy = 0;
    area->offx = 0;
}

void LoadFrogDefaults(FROG_CFG* frog)
{
    frog->moveFactor = 5;
    frog->width = 6;
    frog->height = 3;
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
    cars->nCars = 5;
    cars->moveFactor = 2;
    cars->width = 8;
    cars->height = 3;
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
    controls->up = 'w';
    controls->down = 's';
    controls->left = 'a';
    controls->right = 'd';
    controls->quit = 'q';
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