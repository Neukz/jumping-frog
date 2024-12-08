// cfg.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cfg.h"

#define CONFIG_FILE "settings.txt"

// --- DEFAULT SETTINGS ---
// Timing
const int FRAME_TIME = 25;      // milliseconds interval between frames
const float INITIAL_TIME = 20.0;
const int CAR_SPAWN_FACTOR = 100;
const int CAR_SPEED_CHANGE_FACTOR = 200;
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
const int N_NEUTRAL_CARS = 1;
const int N_FRIENDLY_CARS = 1;
const int NEUTRAL_CAR_STOP_THRESHOLD = 8;
const int CAR_MIN_MOVE_FACTOR = 2;
const int CAR_MAX_MOVE_FACTOR = 4;
const int CAR_WIDTH = 8;
const int CAR_HEIGHT = 3;

// Stork
const int STORK_MOVE_FACTOR = 25;
const int STORK_WIDTH = 5;
const int STORK_HEIGHT = 2;

// Controls
const int UP = 'w';
const int DOWN = 's';
const int LEFT = 'a';
const int RIGHT = 'd';
const int REQUEST = 'e';
const int QUIT = 'q';


// --- LOADING DEFAULT CONFIGURATION ---
void LoadTimingDefaults(CFG* cfg)
{
    cfg->frameTime = FRAME_TIME;
    cfg->initialTime = INITIAL_TIME;
    cfg->carSpawnFactor = CAR_SPAWN_FACTOR;
    cfg->carSpeedChangeFactor = CAR_SPEED_CHANGE_FACTOR;
    cfg->quitTime = QUIT_TIME;
}

void LoadAreaDefaults(CFG* cfg)
{
    cfg->playableRows = PLAYABLE_ROWS;
    cfg->statusRows = STATUS_ROWS;
    cfg->cols = PLAYABLE_COLS;
    cfg->offy = OFFY;
    cfg->offx = OFFX;
}

void LoadFrogDefaults(CFG* cfg)
{
    cfg->frogMoveFactor = FROG_MOVE_FACTOR;
    cfg->frogWidth = FROG_WIDTH;
    cfg->frogHeight = FROG_HEIGHT;
    cfg->frogShape = (char**)malloc(cfg->frogHeight * sizeof(char*));
    for (int i = 0; i < cfg->frogHeight; i++)
    {
        cfg->frogShape[i] = (char*)malloc((cfg->frogWidth + 1) * sizeof(char));
    }
    strcpy(cfg->frogShape[0], " @..@ ");
    strcpy(cfg->frogShape[1], "(----)");
    strcpy(cfg->frogShape[2], " ^  ^ ");
}

void LoadCarsDefaults(CFG* cfg)
{
    cfg->nCars = N_CARS;
    cfg->nNeutralCars = N_NEUTRAL_CARS;
    cfg->neutralCarStopThreshold = NEUTRAL_CAR_STOP_THRESHOLD;
    cfg->nFriendlyCars = N_FRIENDLY_CARS;
    cfg->carMinMoveFactor = CAR_MIN_MOVE_FACTOR;
    cfg->carMaxMoveFactor = CAR_MAX_MOVE_FACTOR;
    cfg->carWidth = CAR_WIDTH;
    cfg->carHeight = CAR_HEIGHT;
    cfg->carShape = (char**)malloc(cfg->carHeight * sizeof(char*));
    for (int i = 0; i < cfg->carHeight; i++)
    {
        cfg->carShape[i] = (char*)malloc((cfg->carWidth + 1) * sizeof(char));
    }
    strcpy(cfg->carShape[0], "  ____  ");
    strcpy(cfg->carShape[1], "_/____\\_");
    strcpy(cfg->carShape[2], " O    O ");
}

void LoadStorkDefaults(CFG* cfg)
{
    cfg->storkMoveFactor = STORK_MOVE_FACTOR;
    cfg->storkWidth = STORK_WIDTH;
    cfg->storkHeight = STORK_HEIGHT;
    cfg->storkShape = (char**)malloc(cfg->storkHeight * sizeof(char*));
    for (int i = 0; i < cfg->storkHeight; i++)
    {
        cfg->storkShape[i] = (char*)malloc((cfg->storkWidth + 1) * sizeof(char));
    }
    strcpy(cfg->storkShape[0], " <o\\ ");
    strcpy(cfg->storkShape[1], ">-O-<");
}

void LoadControlsDefaults(CFG* cfg)
{
    cfg->up = UP;
    cfg->down = DOWN;
    cfg->left = LEFT;
    cfg->right = RIGHT;
    cfg->request = REQUEST;
    cfg->quit = QUIT;
}

void LoadCfgDefaults(CFG* cfg)
{
    LoadTimingDefaults(cfg);
    LoadAreaDefaults(cfg);
    LoadFrogDefaults(cfg);
    LoadCarsDefaults(cfg);
    LoadStorkDefaults(cfg);
    LoadControlsDefaults(cfg);
}


// --- SHAPE UTILITIES ---
void FreeShape(char** shape, int height) {
    for (int i = 0; i < height; i++)
    {
        free(shape[i]);
    }
    free(shape);
}

void ReadShapeFromFile(FILE* file, char*** shape, int width, int height) {
    *shape = (char**)malloc(height * sizeof(char*)); // allocate with new height and width
    for (int i = 0; i < height; i++)
    {
        (*shape)[i] = (char*)malloc((width + 1) * sizeof(char));
    }

    int ch;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            do
            {
                ch = fgetc(file);
            } while (ch == '\n');   // skip newline characters

            if (ch == EOF)
            {
                return;
            }

            (*shape)[i][j] = (char)ch;
        }
    }
}


// --- LOADING CONFIGURATION FROM FILE ---
void LoadTimingFromFile(CFG* cfg, FILE* file)
{
    fscanf(file, "FRAME_TIME=%d\n", &cfg->frameTime);
    fscanf(file, "INITIAL_TIME=%f\n", &cfg->initialTime);
    fscanf(file, "CAR_SPAWN_FACTOR=%d\n", &cfg->carSpawnFactor);
    fscanf(file, "CAR_SPEED_CHANGE_FACTOR=%d\n", &cfg->carSpeedChangeFactor);
    fscanf(file, "QUIT_TIME=%d\n", &cfg->quitTime);
}

void LoadAreaFromFile(CFG* cfg, FILE* file)
{
    fscanf(file, "PLAYABLE_ROWS=%d\n", &cfg->playableRows);
    fscanf(file, "STATUS_ROWS=%d\n", &cfg->statusRows);
    fscanf(file, "COLS=%d\n", &cfg->cols);
    fscanf(file, "OFFY=%d\n", &cfg->offy);
    fscanf(file, "OFFX=%d\n", &cfg->offx);
}

void LoadFrogFromFile(CFG* cfg, FILE* file)
{
    FreeShape(cfg->frogShape, cfg->frogHeight);   // free default shape
    fscanf(file, "FROG_MOVE_FACTOR=%d\n", &cfg->frogMoveFactor);
    fscanf(file, "FROG_WIDTH=%d\n", &cfg->frogWidth);
    fscanf(file, "FROG_HEIGHT=%d\n", &cfg->frogHeight);
    fscanf(file, "FROG_SHAPE:");
    ReadShapeFromFile(file, &cfg->frogShape, cfg->frogWidth, cfg->frogHeight);
}

void LoadCarsFromFile(CFG* cfg, FILE* file)
{
    FreeShape(cfg->carShape, cfg->carHeight);
    fscanf(file, "N_CARS=%d\n", &cfg->nCars);
    fscanf(file, "N_NEUTRAL_CARS=%d\n", &cfg->nNeutralCars);
    fscanf(file, "N_FRIENDLY_CARS=%d\n", &cfg->nFriendlyCars);
    fscanf(file, "NEUTRAL_CAR_STOP_THRESHOLD=%d\n", &cfg->neutralCarStopThreshold);
    fscanf(file, "CAR_MIN_MOVE_FACTOR=%d\n", &cfg->carMinMoveFactor);
    fscanf(file, "CAR_MAX_MOVE_FACTOR=%d\n", &cfg->carMaxMoveFactor);
    fscanf(file, "CAR_WIDTH=%d\n", &cfg->carWidth);
    fscanf(file, "CAR_HEIGHT=%d\n", &cfg->carHeight);
    fscanf(file, "CAR_SHAPE:");
    ReadShapeFromFile(file, &cfg->carShape, cfg->carWidth, cfg->carHeight);
}

void LoadControlsFromFile(CFG* cfg, FILE* file)
{
    fscanf(file, "UP=%c\n", &cfg->up);
    fscanf(file, "DOWN=%c\n", &cfg->down);
    fscanf(file, "LEFT=%c\n", &cfg->left);
    fscanf(file, "RIGHT=%c\n", &cfg->right);
    fscanf(file, "REQUEST=%c\n", &cfg->request);
    fscanf(file, "QUIT=%c\n", &cfg->quit);
}

// Load config from file and override default values
void LoadCfgFromFile(CFG* cfg) {
    FILE* file = fopen(CONFIG_FILE, "r");
    if (!file)
    {
        return; // file not found
    }

    while (!feof(file))
    {
        char section[20];
        fscanf(file, "%s\n", section);
        if (strcmp(section, "---TIMING---") == 0)
        {
            LoadTimingFromFile(cfg, file);
        }
        else if (strcmp(section, "---AREA---") == 0)
        {
            LoadAreaFromFile(cfg, file);
        }
        else if (strcmp(section, "---FROG---") == 0)
        {
            LoadFrogFromFile(cfg, file);
        }
        else if (strcmp(section, "---CARS---") == 0)
        {
            LoadCarsFromFile(cfg, file);
        }
        else if (strcmp(section, "---CONTROLS---") == 0)
        {
            LoadControlsFromFile(cfg, file);
        }
    }

    fclose(file);
}

CFG* InitCfg()
{
    CFG* cfg = (CFG*)malloc(sizeof(CFG));
    LoadCfgDefaults(cfg);
    LoadCfgFromFile(cfg);
    return cfg;
}