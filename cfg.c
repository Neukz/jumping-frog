// cfg.c
#include <stdlib.h>
#include <string.h>
#include "cfg.h"

// --- CFG FUNCTIONS ---
void LoadDefaultTimingCfg(TIMING_CFG* timing)
{
    timing->frameTime = 25;
    timing->initialTime = 20;
    timing->quitTime = 3;
}

void LoadDefaultAreaCfg(AREA_CFG* area)
{
    area->playableRows = 35;
    area->statusRows = 3;
    area->cols = 100;
    area->offy = 0;
    area->offx = 0;
}

void LoadDefaultFrogCfg(FROG_CFG* frog)
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

void LoadDefaultCarsCfg(CARS_CFG* cars)
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

void LoadDefaultControlsCfg(CONTROLS_CFG* controls)
{
    controls->up = 'w';
    controls->down = 's';
    controls->left = 'a';
    controls->right = 'd';
    controls->quit = 'q';
}

// Load default configuration
void LoadDefaultCfg(CFG* cfg)
{
    cfg->timing = (TIMING_CFG*)malloc(sizeof(TIMING_CFG));
    LoadDefaultTimingCfg(cfg->timing);

    cfg->area = (AREA_CFG*)malloc(sizeof(AREA_CFG));
    LoadDefaultAreaCfg(cfg->area);

    cfg->frog = (FROG_CFG*)malloc(sizeof(FROG_CFG));
    LoadDefaultFrogCfg(cfg->frog);

    cfg->cars = (CARS_CFG*)malloc(sizeof(CARS_CFG));
    LoadDefaultCarsCfg(cfg->cars);

    cfg->controls = (CONTROLS_CFG*)malloc(sizeof(CONTROLS_CFG));
    LoadDefaultControlsCfg(cfg->controls);
}

// Config initializer
CFG* InitCfg()
{
    CFG* cfg = (CFG*)malloc(sizeof(CFG));
    LoadDefaultCfg(cfg);
    return cfg;
}