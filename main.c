/*
    Jumping Frog
    Kacper Neumann, 203394

    This game has been developed based on the demo game provided by Prof. Michał Małafiejski (CATCH THE BALL).
    Source: http://gut.animima.org/pp/Projekt1/demogame/win1.c

    Functions do not exceed 1024 bytes.
    Counter: https://gre-v-el.github.io/Cpp-Func-Length-Counter/
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include "cfg.h"

// --- CONSTANTS ---
// Constants indicating the reason to end the game
typedef enum {
    SUCCESS,        // reached destination
    FAILURE,        // died
    TIME_OVER,      // time is over
    INTERRUPTED     // decision to quit
} GameResult;

// Car types
typedef enum {
    ENEMY,      // normal car
    NEUTRAL,    // stops when the frog is close
    FRIENDLY    // helps the frog on demand
} CarType;

// Color identifiers
typedef enum {
    COLOR_MAIN,
    COLOR_STATUS,
    COLOR_PLAYABLE,
    COLOR_FROG,
    COLOR_ENEMY_CAR,
    COLOR_NEUTRAL_CAR,
    COLOR_FRIENDLY_CAR,
    COLOR_DEST,
    COLOR_OBSTACLE,
} Color;

// Scoring constants - should sum up to 100
const float TIME_FACTOR = 50;
const float JUMP_FACTOR = 15;
const float DIFFICULTY_FACTOR = 35;

// --- DATA STRUCTURES ---
// Window structure
typedef struct {
    WINDOW* window; // extends ncurses window
    Color color;
    int x, y;       // top-left corner coordinates
    int rows, cols;
} WIN;

// Game object structure - used for frog and destination directly, extended by CAR
typedef struct {
    WIN* win;
    Color color;
    int moveFactor;
    int nMoves;          // number of nMoves (used to track number of jumps)
    int x, y;           // top-left corner coordinates
    int xmin, xmax;     // movement boundaries
    int ymin, ymax;
    int width, height;
    char** shape;
} OBJ;

// Car structure
typedef struct {
    OBJ* obj;           // extends OBJ
    int direction;      // 0 for left, 1 for right
    int dynamicSpeed;   // 0 for constant speed, 1 for dynamic
    int disappearing;   // 0 for bouncing car, 1 for disappearing (replaced with a new car)
    int requested;      // 0 for not requested, 1 for requested (friendly car)
    CarType type;
} CAR;

// Timer structure
typedef struct {
    int frame;
    int frameTime;
    float timeLeft;
} TIMER;

// Score structure
typedef struct {
    float highest;
    float last;
} SCORE;


// --- UTILITIES ---
// Random integer (inclusive)
int RandInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}

// Calculate car's y-coordinate adjusted with frog's height so that there are safe lanes
int CarY(int carIndex, int carHeight, int frogHeight)
{
    return carIndex * (carHeight + frogHeight) + frogHeight + 1;
}

int ShapeCenter(int coord, int size)
{
    return coord + size / 2.0;
}

int EuclideanDistance(int x1, int y1, int x2, int y2)
{
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}


// --- SCORE FUNCTIONS ---
float Score(int nJumps, float timeLeft, float initialTime, CARS_CFG* cfg)
{
    float timeScore = (timeLeft / initialTime) * TIME_FACTOR;    // more time left yields higher scores
    float jumpScore = (1.0 / nJumps) * JUMP_FACTOR;              // fewer jumps yield higher scores
    int enemyCars = cfg->nCars - cfg->nNeutralCars - cfg->nFriendlyCars;
    float difficultyScore = (float)enemyCars / (cfg->nCars) * DIFFICULTY_FACTOR;   // more enemy cars increase difficulty
    float score = timeScore + jumpScore + difficultyScore;
    return score;
}

void ReadScore(SCORE* score)
{
    const char* filename = "score.txt";
    FILE* file = fopen(filename, "r");
    if (file == NULL)
    {
        file = fopen(filename, "w+");   // create the file if it doesn't exist
        if (file == NULL)
        {
            fprintf(stderr, "Error creating the score file.\n");
            return;
        }

        fprintf(file, "HIGHEST_SCORE=0.00\n");
        fprintf(file, "LAST_SCORE=0.00\n");
        rewind(file);
    }

    fscanf(file, "HIGHEST_SCORE=%f\n", &score->highest);
    fscanf(file, "LAST_SCORE=%f\n", &score->last);
    fclose(file);
}

void SaveScore(float newScore)
{
    SCORE* score = (SCORE*)malloc(sizeof(SCORE));
    ReadScore(score);

    score->last = newScore;
    if (newScore > score->highest)
    {
        score->highest = newScore;
    }

    const char* filename = "score.txt";
    FILE* file = fopen(filename, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error saving the score.\n");
        return;
    }

    fprintf(file, "HIGHEST_SCORE=%.2f\n", score->highest);
    fprintf(file, "LAST_SCORE=%.2f\n", score->last);
    free(score);
    fclose(file);
}


// --- MAIN WINDOW FUNCTIONS ---
// Main window initializer
WINDOW* InitGame()
{
    WINDOW* win;
    if ((win = initscr()) == NULL) // initialize ncurses
    {
        fprintf(stderr, "Error initializing ncurses.\n");
        exit(EXIT_FAILURE);
    }

    start_color();  // initialize colors
    init_pair(COLOR_MAIN, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_PLAYABLE, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_STATUS, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_FROG, COLOR_GREEN, COLOR_WHITE);
    init_pair(COLOR_ENEMY_CAR, COLOR_RED, COLOR_WHITE);
    init_pair(COLOR_NEUTRAL_CAR, COLOR_YELLOW, COLOR_WHITE);
    init_pair(COLOR_FRIENDLY_CAR, COLOR_BLUE, COLOR_WHITE);
    init_pair(COLOR_DEST, COLOR_GREEN, COLOR_WHITE);
    init_pair(COLOR_OBSTACLE, COLOR_MAGENTA, COLOR_WHITE);

    noecho();       // turn off displaying input and hide cursor
    curs_set(0);
    return win;
}

// Welcome screen - wait for user input before starting the game
void Welcome(WINDOW* win)
{
    SCORE* score = (SCORE*)malloc(sizeof(SCORE));
    ReadScore(score);
    mvwprintw(win, 1, 1, "Jumping Frog");
    mvwprintw(win, 3, 1, "Highest score: %.2f", score->highest);
    mvwprintw(win, 4, 1, "Last score: %.2f", score->last);
    mvwprintw(win, 6, 1, "Press any key to start the game.");
    wgetch(win);
    wclear(win);
    wrefresh(win);
    free(score);
}


// --- WIN FUNCTIONS ---
void ClearWin(WIN* win)
{
    wattron(win->window, COLOR_PAIR(win->color));
    for (int row = 0; row < win->rows; row++)
    {
        for (int col = 0; col < win->cols; col++)
        {
            mvwprintw(win->window, row, col, " ");
        }
    }
    box(win->window, 0, 0); // add border to outermost rows/cols
}

// Window initializer
WIN* InitWin(WINDOW* mainWindow, int rows, int cols, int y, int x, Color color)
{
    WIN* win = (WIN*)malloc(sizeof(WIN));
    win->x = x;
    win->y = y;
    win->rows = rows;
    win->cols = cols;
    win->color = color;
    win->window = subwin(mainWindow, rows, cols, y, x); // create the window inside of the main window
    nodelay(win->window, TRUE);                         // non-blocking input for real-time
    ClearWin(win);
    wrefresh(win->window);
    return win;
}


// --- STATUS FUNCTIONS ---
void PrintTime(WIN* status, float timeLeft)
{
    mvwprintw(status->window, 1, 2, "Time: %.2f", timeLeft);
    wrefresh(status->window);
}

void PrintJumps(WIN* status, OBJ* frog)
{
    mvwprintw(status->window, 1, status->cols / 2 - 10, "Jumps: %d", frog->nMoves);
    wrefresh(status->window);
}

// Status window initializer
void InitStatus(WIN* status, TIMER* timer, OBJ* frog)
{
    box(status->window, 0, 0);
    PrintTime(status, timer->timeLeft);
    PrintJumps(status, frog);
    const char* signature = "Kacper Neumann, 203394";
    mvwprintw(status->window, 1, status->cols - strlen(signature) - 2, "%s", signature);
}

// Display information about the result of the game and count down to quit
void EndGame(WIN* playable, WIN* status, GameResult result, CFG* cfg, int nJumps, float timeLeft)
{
    ClearWin(status);
    char message[100];
    switch (result)
    {
        case SUCCESS:
            float score = Score(nJumps, timeLeft, cfg->timing->initialTime, cfg->cars);
            SaveScore(score);
            sprintf(message, "You have reached the destination. Score: %.2f", score);
            break;
        case FAILURE:
            sprintf(message, "You died. Game over.");
            break;
        case TIME_OVER:
            sprintf(message, "Time is over. Game over.");
            break;
        case INTERRUPTED:
            sprintf(message, "You have decided to quit the game.");
    }
    for (int seconds = cfg->timing->quitTime; seconds > 0; seconds--)
    {
        mvwprintw(status->window, 1, 2, "%s Closing in %d seconds...", message, seconds);
        wrefresh(status->window);
        sleep(1);
    }
}


// --- OBJ FUNCTIONS ---
// Print game object's shape
void PrintObj(OBJ* obj)
{
    wattron(obj->win->window, COLOR_PAIR(obj->color));
    for (int i = 0; i < obj->height; i++)
    {
        mvwprintw(obj->win->window, obj->y + i, obj->x, "%s", obj->shape[i]);
    }
    wattron(obj->win->window, COLOR_PAIR(obj->win->color));
    wrefresh(obj->win->window);
}

// Move the game object along both axes by 1
void MoveObj(OBJ* obj, int dx, int dy)
{
    wattron(obj->win->window, COLOR_PAIR(obj->color));

    char* emptyRow = (char*)malloc(obj->width * sizeof(char));    // string of empty spaces to erase the old position row
    memset(emptyRow, ' ', obj->width);

    if ((dy == 1) && (obj->y + obj->height < obj->ymax))
    {
        obj->y += dy;
        mvwprintw(obj->win->window, obj->y - 1, obj->x, "%s", emptyRow);
    }
    else if ((dy == -1) && (obj->y > obj->ymin))
    {
        obj->y += dy;
        mvwprintw(obj->win->window, obj->y + obj->height, obj->x, "%s", emptyRow);
    }

    if ((dx == 1) && (obj->x + obj->width < obj->xmax))
    {
        obj->x += dx;
        for (int i = 0; i < obj->height; i++)
        {
            mvwprintw(obj->win->window, obj->y + i, obj->x - 1, " ");
        }
    }
    else if ((dx == -1) && (obj->x > obj->xmin))
    {
        obj->x += dx;
        for (int i = 0; i < obj->height; i++)
        {
            mvwprintw(obj->win->window, obj->y + i, obj->x + obj->width, " ");
        }
    }

    PrintObj(obj);
}

int Collision(OBJ* obj, OBJ* other)
{
    return ((
        (obj->y >= other->y && obj->y < other->y + other->height) ||
        (other->y >= obj->y && other->y < obj->y + obj->height)
        ) && (
            (obj->x >= other->x && obj->x < other->x + other->width) ||
            (other->x >= obj->x && other->x < obj->x + obj->width)
            )) ? 1 : 0;
}

void AllocateShape(OBJ* obj, char** shape, int height, int width)
{
    obj->shape = (char**)malloc(height * sizeof(char*));
    for (int i = 0; i < height; i++)
    {
        obj->shape[i] = (char*)malloc((width + 1) * sizeof(char));   // +1 for '\0'
        strcpy(obj->shape[i], shape[i]);
    }
}

void ClearShape(OBJ* obj)
{
    for (int i = 0; i < obj->height; i++)
    {
        for (int j = 0; j < obj->width; j++)
        {
            mvwprintw(obj->win->window, obj->y + i, obj->x + j, " ");
        }
    }
}

// Frog initializer
OBJ* InitFrog(WIN* win, FROG_CFG* cfg, Color color)
{
    OBJ* frog = (OBJ*)malloc(sizeof(OBJ));
    frog->win = win;
    frog->color = color;
    frog->moveFactor = 0;
    frog->nMoves = 0;
    frog->width = cfg->width;
    frog->height = cfg->height;
    frog->x = (win->cols - frog->width) / 2;
    frog->y = win->rows - frog->height - 1;
    frog->xmin = 1;
    frog->xmax = win->cols - 1;
    frog->ymin = 1;
    frog->ymax = win->rows - 1;
    AllocateShape(frog, cfg->shape, cfg->height, cfg->width);
    return frog;
}

// Destination initializer
OBJ* InitDest(WIN* win, Color color)
{
    OBJ* dest = (OBJ*)malloc(sizeof(OBJ));
    dest->win = win;
    dest->color = color;
    dest->x = (win->cols - dest->width) / 2;
    dest->y = 1;
    dest->width = 1;    // destination is a single point
    dest->height = 1;
    dest->shape = (char**)malloc(sizeof(char*));
    dest->shape[0] = (char*)malloc((dest->width + 1) * sizeof(char));
    strcpy(dest->shape[0], "*");
    return dest;
}

// Static obstacle initializer
OBJ* InitObstacle(WIN* win, int x, int y, int width, int height, Color color) {
    OBJ* obstacle = (OBJ*)malloc(sizeof(OBJ));
    obstacle->win = win;
    obstacle->color = color;
    obstacle->width = width;
    obstacle->height = height;
    obstacle->x = x;
    obstacle->y = y;
    obstacle->shape = (char**)malloc(sizeof(char*));
    for (int i = 0; i < height; i++)
    {
        obstacle->shape[i] = (char*)malloc((width + 1) * sizeof(char));
        memset(obstacle->shape[i], '#', width);
    }
    return obstacle;
}

OBJ** GenerateObstacles(WIN* win, CARS_CFG* cfg, int frogHeight, Color color) {
    OBJ** obstacles = (OBJ**)malloc(cfg->nCars * sizeof(OBJ*));
    for (int i = 0; i < cfg->nCars; i++) {
        int x = RandInt(1, win->cols - cfg->width - 1);
        int y = CarY(i, cfg->height, frogHeight) + cfg->height + 1;     // obstacles are below each car lane
        obstacles[i] = InitObstacle(win, x, y, cfg->width, 1, color);   // height=1 (single row)
        // TODO: randomize obstacle width/create walls with random gaps/add vertical walls on car lanes
    }
    return obstacles;
}


// --- CAR FUNCTIONS ---
// Car initializer
CAR* InitCar(WIN* win, Color color, CARS_CFG* cfg, int y, int disappearing)
{
    OBJ* obj = (OBJ*)malloc(sizeof(OBJ));
    obj->win = win;
    obj->color = color;
    obj->moveFactor = RandInt(cfg->minMoveFactor, cfg->maxMoveFactor);  // random speed
    obj->xmin = 1;
    obj->xmax = win->cols - 1;
    obj->ymin = obj->y; // cars don't move vertically
    obj->ymax = obj->y;
    obj->width = cfg->width;
    obj->height = cfg->height;
    AllocateShape(obj, cfg->shape, cfg->height, cfg->width);

    CAR* car = (CAR*)malloc(sizeof(CAR));
    car->obj = obj;
    car->direction = RandInt(0, 1);     // initial direction is random
    car->dynamicSpeed = RandInt(0, 1);  // may change speed
    car->disappearing = disappearing;
    car->type = ENEMY;                  // all cars are enemies initially
    obj->x = car->direction == 0 ? obj->xmax - obj->width : obj->xmin;  // depends on initial direction
    obj->y = y;
    return car;
}

CAR** GenerateCars(WIN* win, CARS_CFG* cfg, int frogHeight, Color enemyColor, Color neutralColor, Color friendlyColor)
{
    CAR** cars = (CAR**)malloc(cfg->nCars * sizeof(CAR*));
    for (int i = 0; i < cfg->nCars; i++)
    {
        // cars may be disappearing and are enemies initially
        cars[i] = InitCar(win, enemyColor, cfg, CarY(i, cfg->height, frogHeight), RandInt(0, 1));
    }

    for (int i = 0; i < cfg->nNeutralCars;)         // change random enemy cars to neutral if needed
    {
        int neutralIndex = RandInt(1, cfg->nCars - 1);  // the highest placed car must be an enemy
        if (cars[neutralIndex]->type == ENEMY)
        {
            cars[neutralIndex]->type = NEUTRAL;
            cars[neutralIndex]->obj->color = neutralColor;
            cars[neutralIndex]->disappearing = 0;   // neutral cars don't disappear
            i++;
        }
    }

    for (int i = 0; i < cfg->nFriendlyCars;)        // change random enemy cars to friendly if needed
    {
        int friendlyIndex = RandInt(1, cfg->nCars - 1);
        if (cars[friendlyIndex]->type == ENEMY)
        {
            cars[friendlyIndex]->type = FRIENDLY;
            cars[friendlyIndex]->obj->color = friendlyColor;
            cars[friendlyIndex]->disappearing = 0;  // friendly cars don't disappear
            cars[friendlyIndex]->requested = 0;
            i++;
        }
    }

    for (int i = 0; i < cfg->nCars; i++)
    {
        PrintObj(cars[i]->obj);     // force first render
    }
    return cars;
}

int BorderReached(CAR* car)
{
    if (car->direction == 1 && car->obj->x == car->obj->xmax - car->obj->width)
    {
        return 1;   // right border reached
    }
    else if (car->direction == 0 && car->obj->x == car->obj->xmin)
    {
        return 0;   // left
    }
    return -1;      // not reached
}

// Reverse direction when car hits the wall (bouncing)
void ReverseCarDirection(CAR* car)
{
    int reached = BorderReached(car);
    if (reached == 1)
    {
        car->direction = 0;
    }
    else if (reached == 0)
    {
        car->direction = 1;
    }
}

void PrintLanes(WIN* win, CARS_CFG* cfg, int frogHeight)
{
    for (int i = 0; i < cfg->nCars; i++)
    {
        int y = CarY(i, cfg->height, frogHeight) + cfg->height;
        mvwhline(win->window, y, win->x + 1, '-', win->cols - 2); // draw lane below the car   
    }
}

int NeutralCarThreshold(CAR* car, CARS_CFG* cfg, OBJ* frog)
{
    int frogX = ShapeCenter(frog->x, frog->width);
    int frogY = ShapeCenter(frog->y, frog->height);
    int carX = ShapeCenter(car->obj->x, car->obj->width);
    int carY = ShapeCenter(car->obj->y, car->obj->height);
    int distance = EuclideanDistance(frogX, frogY, carX, carY);
    if (distance <= cfg->neutralCarStopThreshold)
    {
        return 1;
    }
    return 0;
}


// --- TIMER FUNCTIONS ---
// Timer initializer
TIMER* InitTimer(TIMING_CFG* cfg)
{
    TIMER* timer = (TIMER*)malloc(sizeof(TIMER));
    timer->frame = 1;
    timer->frameTime = cfg->frameTime;
    timer->timeLeft = cfg->initialTime;
    return timer;
}

int UpdateTimer(TIMER* timer, WIN* win, int initialTime)
{
    timer->frame++;
    timer->timeLeft = initialTime - (timer->frame * timer->frameTime / 1000.0);
    if (timer->timeLeft < timer->frameTime / 1000.0)
    {
        timer->timeLeft = 0;
    }
    else
    {
        usleep(timer->frameTime * 1000);
    }
    PrintTime(win, timer->timeLeft);
    return timer->timeLeft == 0 ? 1 : 0;    // 1 if time has elapsed, 0 otherwise    
}


// --- MOVEMENT ---
void RequestFriendlyCar(OBJ* frog, CAR** cars, CARS_CFG* cfg)
{
    for (int i = cfg->nCars - 1; i >= 0; i--)    // find the closest friendly car above the frog
    {
        // the frog must be at least 1 row below the car lane
        if (cars[i] != NULL && cars[i]->type == FRIENDLY && cars[i]->obj->y + cars[i]->obj->height == frog->y)
        {
            if (cars[i]->obj->x < frog->x)
            {
                cars[i]->direction = 1;
            }
            else if (cars[i]->obj->x > frog->x)
            {
                cars[i]->direction = 0;
            }
            cars[i]->obj->moveFactor = cfg->minMoveFactor;
            cars[i]->requested = 1;
            return;
        }
    }
}

// Frog movement
void MoveFrog(OBJ* frog, CONTROLS_CFG* controlsCfg, char key, int moveFactor, int frame, OBJ** obstacles, CAR** cars, CARS_CFG* carsCfg)
{
    if (frame - frog->moveFactor >= moveFactor)   // move only if the move factor has passed
    {
        int dx = 0, dy = 0;
        if (key == controlsCfg->up)
        {
            dy = -1;
        }
        else if (key == controlsCfg->down)
        {
            dy = 1;
        }
        else if (key == controlsCfg->left)
        {
            dx = -1;
        }
        else if (key == controlsCfg->right)
        {
            dx = 1;
        }
        else if (key == controlsCfg->request)
        {
            RequestFriendlyCar(frog, cars, carsCfg);
        }

        MoveObj(frog, dx, dy);
        for (int i = 0; i < carsCfg->nCars; i++)
        {
            if (Collision(frog, obstacles[i]))  // undo the move if the frog hits an obstacle
            {
                MoveObj(frog, -dx, -dy);
                return;
            }
        }

        frog->moveFactor = frame;
        frog->nMoves++;
    }
}

// Car movement
void MoveCar(CAR** carPtr, int frame)   // pointer to pointer to affect the original one
{
    CAR* car = *carPtr;
    if (car == NULL)
    {
        return;
    }

    ReverseCarDirection(car);
    if (frame % car->obj->moveFactor == 0)
    {
        MoveObj(car->obj, car->direction == 0 ? -1 : 1, 0);  // depends on direction
    }

    if (car->disappearing && BorderReached(car) > -1)
    {
        ClearShape(car->obj);
        FreeShape(car->obj->shape, car->obj->height);
        free(car->obj);
        free(car);
        *carPtr = NULL; // update the pointer in the array of cars
    }
}

void UpdateCars(WIN* playable, TIMER* timer, OBJ* frog, CAR** cars, OBJ** obstacles, CFG* cfg)
{
    for (int i = 0; i < cfg->cars->nCars; i++)
    {
        PrintObj(obstacles[i]);
        if (cars[i] != NULL)
        {
            if (cars[i]->type == NEUTRAL && NeutralCarThreshold(cars[i], cfg->cars, frog))
            {
                continue;   // neutral cars will not move
            }

            if (cars[i]->type == FRIENDLY && cars[i]->requested && frog->x == cars[i]->obj->x)
            {
                // move frog above the friendly car
                ClearShape(frog);
                frog->y = cars[i]->obj->y - frog->height;
                cars[i]->obj->moveFactor = RandInt(cfg->cars->minMoveFactor, cfg->cars->maxMoveFactor); // new speed
                cars[i]->requested = 0;
                continue;
            }

            if (cars[i]->dynamicSpeed && timer->frame % cfg->timing->carSpeedChangeFactor == 0)
            {
                cars[i]->obj->moveFactor = RandInt(cfg->cars->minMoveFactor, cfg->cars->maxMoveFactor);
            }
            MoveCar(&cars[i], timer->frame);
        }
        else if (timer->frame % cfg->timing->carSpawnFactor == 0)
        {
            // the new one will be disappearing as well
            cars[i] = InitCar(playable, COLOR_ENEMY_CAR, cfg->cars, CarY(i, cfg->cars->height, cfg->frog->height), 1);
        }
    }
}


// --- MAIN LOOP ---
GameResult Play(WIN* playable, WIN* status, TIMER* timer, OBJ* frog, CAR** cars, OBJ* dest, OBJ** obstacles, CFG* cfg)
{
    PrintObj(dest);
    int key;
    while ((key = wgetch(status->window)) != cfg->controls->quit)
    {
        flushinp(); // clear input buffer
        if (key != ERR)
        {
            MoveFrog(frog, cfg->controls, key, cfg->frog->moveFactor, timer->frame, obstacles, cars, cfg->cars);
        }
        UpdateCars(playable, timer, frog, cars, obstacles, cfg);
        PrintLanes(playable, cfg->cars, cfg->frog->height);
        PrintObj(frog);  // force overlapping car lanes
        PrintJumps(status, frog);
        if (Collision(frog, dest))
        {
            return SUCCESS;
        }
        for (int i = 0; i < cfg->cars->nCars; i++)
        {
            if (cars[i] != NULL && Collision(frog, cars[i]->obj))
            {
                return FAILURE;
            }
        }
        if (UpdateTimer(timer, status, cfg->timing->initialTime))
        {
            return TIME_OVER;
        }
    }
    return INTERRUPTED;
}


// --- CLEANUP ---
void Cleanup(WIN* playable, WIN* status, WINDOW* mainWindow, OBJ* frog, CAR** cars, OBJ* dest, OBJ** obstacles, TIMER* timer, CFG* cfg)
{
    FreeShape(frog->shape, frog->height);
    free(frog);
    for (int i = 0; i < cfg->cars->nCars; i++)
    {
        if (cars[i] != NULL)    // watch out for disappearing cars
        {
            FreeShape(cars[i]->obj->shape, cars[i]->obj->height);
            free(cars[i]->obj);
            free(cars[i]);
        }
    }
    free(cars);
    FreeShape(dest->shape, dest->height);
    free(dest);
    for (int i = 0; i < cfg->cars->nCars; i++)
    {
        FreeShape(obstacles[i]->shape, obstacles[i]->height);
        free(obstacles[i]);
    }
    free(timer);
    free(cfg->timing);
    free(cfg->area);
    FreeShape(cfg->frog->shape, cfg->frog->height);
    free(cfg->frog);
    FreeShape(cfg->cars->shape, cfg->cars->height);
    free(cfg->cars);
    free(cfg->controls);
    free(cfg);
    delwin(playable->window);
    free(playable);
    delwin(status->window);
    free(status);
    delwin(mainWindow);
    refresh();
    endwin();
}


// --- MAIN PROGRAM ---
int main()
{
    srand(time(NULL));

    WINDOW* mainWindow = InitGame();
    Welcome(mainWindow);

    CFG* cfg = InitCfg();
    WIN* playable = InitWin(mainWindow, cfg->area->playableRows, cfg->area->cols, cfg->area->offy, cfg->area->offx, COLOR_PLAYABLE);
    WIN* status = InitWin(mainWindow, cfg->area->statusRows, cfg->area->cols, cfg->area->playableRows + cfg->area->offy, cfg->area->offx, COLOR_STATUS);
    TIMER* timer = InitTimer(cfg->timing);
    OBJ* frog = InitFrog(playable, cfg->frog, COLOR_FROG);
    CAR** cars = GenerateCars(playable, cfg->cars, cfg->frog->height, COLOR_ENEMY_CAR, COLOR_NEUTRAL_CAR, COLOR_FRIENDLY_CAR);
    OBJ* dest = InitDest(playable, COLOR_DEST);
    OBJ** obstacles = GenerateObstacles(playable, cfg->cars, cfg->frog->height, COLOR_OBSTACLE);

    InitStatus(status, timer, frog);

    GameResult result = Play(playable, status, timer, frog, cars, dest, obstacles, cfg);
    EndGame(playable, status, result, cfg, frog->nMoves, timer->timeLeft);
    Cleanup(playable, status, mainWindow, frog, cars, dest, obstacles, timer, cfg);
    return EXIT_SUCCESS;
}