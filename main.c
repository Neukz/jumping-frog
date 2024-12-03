/*
    Jumping Frog
    Kacper Neumann, 203394

    This game has been developed based on the demo game provided by Prof. Michał Małafiejski (CATCH THE BALL).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include "cfg.h"


// --- CONSTANTS ---
// Main loop (Play function) constants indicating the reason to end the game
typedef enum {
    SUCCESS,        // reached destination
    FAILURE,        // died
    TIME_OVER,      // time is over
    INTERRUPTED     // decision to quit
} GameResult;

// Delay constants for real-time
const int DELAY_ON = 1;
const int DELAY_OFF = 0;


// --- DATA STRUCTURES ---
typedef enum {
    COLOR_MAIN,
    COLOR_STATUS,
    COLOR_PLAYABLE,
    COLOR_FROG,
    COLOR_CAR,
    COLOR_DEST
} Color;

// Window structure
typedef struct {
    WINDOW* window; // extends ncurses window
    Color color;
    int x, y;       // top-left corner coordinates
    int rows, cols;
} WIN;

// Game object structure - used for frog directly, extended by CAR
typedef struct {
    WIN* win;
    Color color;
    int moveFactor;
    int x, y;           // top-left corner coordinates
    int xmin, xmax;     // movement boundaries
    int ymin, ymax;
    int width, height;
    char** shape;
} OBJ;

typedef enum {
    Enemy,      // normal car
    Neutral,    // stops when the frog is close
    Friendly    // helps the frog on demand
} CarType;

// Car structure
typedef struct {
    OBJ* obj;           // extends OBJ
    int direction;      // 0 for left, 1 for right
    int dynamicSpeed;   // 0 for constant speed, 1 for dynamic
    int disappearing;   // 0 for perpetually bouncing car, 1 for disappearing (replaced with a new car)
    CarType type;
} CAR;

// Destination structure
typedef struct {
    WIN* win;
    Color color;
    int x, y;           // top-left corner coordinates
    int width, height;
} DEST;

// Timer structure
typedef struct {
    int frameTime;
    float timeLeft;
    int frameNo;
} TIMER;


// --- RANDOM NUMBER (inclusive) ---
int RandInt(int min, int max)
{
    return min + rand() % (max - min + 1);
}


// --- WINDOW FUNCTIONS ---
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
    init_pair(COLOR_CAR, COLOR_RED, COLOR_WHITE);
    init_pair(COLOR_DEST, COLOR_GREEN, COLOR_BLUE);

    noecho();       // turn off displaying input and hide cursor
    curs_set(0);
    return win;
}

// Welcome screen - wait for user input before starting the game
void Welcome(WINDOW* win)
{
    mvwaddstr(win, 1, 1, "Press any key to start the game.");
    wgetch(win);
    wclear(win);
    wrefresh(win);
}


// --- WIN FUNCTIONS ---
// Cleaning the window (prints " ")
void CleanWin(WIN* win)
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
WIN* InitWin(WINDOW* mainWindow, int rows, int cols, int y, int x, Color color, int delay)
{
    WIN* win = (WIN*)malloc(sizeof(WIN));
    win->x = x;
    win->y = y;
    win->rows = rows;
    win->cols = cols;
    win->color = color;
    win->window = subwin(mainWindow, rows, cols, y, x); // create the window inside of the main window
    CleanWin(win);
    if (delay == DELAY_OFF)
    {
        nodelay(win->window, TRUE);                     // non-blocking input for real-time
    }
    wrefresh(win->window);
    return win;
}


// --- STATUS FUNCTIONS ---
void PrintTime(WIN* win, float timeLeft)
{
    mvwprintw(win->window, 1, 2, "Time: %.2f", timeLeft);
    wrefresh(win->window);
}

void PrintPosition(WIN* win, OBJ* frog)
{
    mvwprintw(win->window, 1, win->cols / 2 - 10, "Position: x: %d y: %d", frog->x, frog->y);
    wrefresh(win->window);
}

// Status window initializer
void InitStatus(WIN* win, TIMER* timer, OBJ* frog)
{
    box(win->window, 0, 0);
    PrintTime(win, timer->timeLeft);
    PrintPosition(win, frog);
    char* signature = "Kacper Neumann, 203394";
    mvwprintw(win->window, 1, win->cols - strlen(signature) - 2, "%s", signature);
}

// Display information about the result of the game and count down to quit
void EndGame(WIN* win, GameResult result, int quitTime)
{
    CleanWin(win);
    char message[100];
    switch (result)
    {
        case SUCCESS:
            sprintf(message, "Congratulations! You have reached the destination.");
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
    for (int seconds = quitTime; seconds > 0; seconds--)
    {
        mvwprintw(win->window, 1, 2, "%s Closing the game in %d seconds...", message, seconds);
        wrefresh(win->window);
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

void SetObjPosition(OBJ* obj, int x, int y)
{
    obj->x = x;
    obj->y = y;
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

// Frog initializer
OBJ* InitFrog(WIN* win, Color color, FROG_CFG* cfg)
{
    OBJ* frog = (OBJ*)malloc(sizeof(OBJ));
    frog->win = win;
    frog->color = color;
    frog->width = cfg->width;
    frog->height = cfg->height;
    frog->moveFactor = 0;
    frog->xmin = 1;
    frog->xmax = win->cols - 1;
    frog->ymin = 1;
    frog->ymax = win->rows - 1;

    AllocateShape(frog, cfg->shape, cfg->height, cfg->width);
    SetObjPosition(frog, (win->cols - frog->width) / 2, win->rows - frog->height - 1);
    return frog;
}

// Frog movement
void MoveFrog(OBJ* frog, CONTROLS_CFG* cfg, char key, int moveFactor, int frame)
{
    if (frame - frog->moveFactor >= moveFactor)   // movement cooldown condition
    {
        if (key == cfg->up)
        {
            MoveObj(frog, 0, -1);
        }
        else if (key == cfg->down)
        {
            MoveObj(frog, 0, 1);
        }
        else if (key == cfg->left)
        {
            MoveObj(frog, -1, 0);
        }
        else if (key == cfg->right)
        {
            MoveObj(frog, 1, 0);
        }
        frog->moveFactor = frame;
    }
}


// --- CAR FUNCTIONS ---
// Car initializer
CAR* InitCar(WIN* win, Color color, CARS_CFG* cfg, int y, int dynamicSpeed, CarType type)
{
    OBJ* obj = (OBJ*)malloc(sizeof(OBJ));
    obj->win = win;
    obj->color = color;
    obj->width = cfg->width;
    obj->height = cfg->height;
    obj->moveFactor = cfg->moveFactor;
    obj->xmin = 1;
    obj->xmax = win->cols - 1;
    obj->ymin = obj->y; // cars don't move vertically
    obj->ymax = obj->y;

    AllocateShape(obj, cfg->shape, cfg->height, cfg->width);

    CAR* car = (CAR*)malloc(sizeof(CAR));
    car->obj = obj;
    car->direction = RandInt(0, 1);     // initial direction is random
    car->dynamicSpeed = dynamicSpeed;
    car->disappearing = RandInt(0, 1);  // may disappear
    car->type = type;
    SetObjPosition(obj, car->direction == 0 ? obj->xmax - obj->width : obj->xmin, y); // depends on initial direction
    return car;
}

CAR** GenerateCars(WIN* win, Color color, CARS_CFG* cfg, int frogHeight)
{
    CAR** cars = (CAR**)malloc(cfg->nCars * sizeof(CAR*));
    for (int i = 0; i < cfg->nCars; i++)
    {
        cars[i] = InitCar(win, color, cfg, i * (cfg->height + frogHeight) + frogHeight, 0, Enemy);
        MoveObj(cars[i]->obj, 0, 0); // force first render
    }
    return cars;
}

// Reverse direction when car hits the wall (bouncing)
void ReverseCarDirection(CAR* car)
{
    if (car->direction == 1 && car->obj->x == car->obj->xmax - car->obj->width)
    {
        car->direction = 0;
    }
    else if (car->direction == 0 && car->obj->x == car->obj->xmin)
    {
        car->direction = 1;
    }
}

// Car movement
void MoveCar(CAR* car, int frame)
{
    ReverseCarDirection(car);
    if (frame % car->obj->moveFactor == 0)
    {
        MoveObj(car->obj, car->direction == 0 ? -1 : 1, 0);  // depends on direction
    }

    mvwhline(car->obj->win->window, car->obj->y + car->obj->height, car->obj->win->x + 1, '-', car->obj->win->cols - 2); // draw lane
}


// --- DESTINATION (DEST) FUNCTIONS ---
// Destination initializer
DEST* InitDest(WIN* win, Color color, int width)
{
    DEST* dest = (DEST*)malloc(sizeof(DEST));
    dest->win = win;
    dest->color = color;
    dest->width = width;
    dest->height = 1;   // single row
    dest->x = (win->cols - dest->width) / 2;
    dest->y = 1;
    return dest;
}

void PrintDest(DEST* dest)
{
    wattron(dest->win->window, COLOR_PAIR(dest->color));
    for (int y = 0; y < dest->height; y++)
    {
        for (int x = 0; x < dest->width; x++)
        {
            mvwprintw(dest->win->window, dest->y + y, dest->x + x, " ");
        }
    }
    wattron(dest->win->window, COLOR_PAIR(dest->win->color));
    wrefresh(dest->win->window);
}

// Returns 1 if the frog has reached the destination, 0 otherwise
int DestReached(OBJ* frog, DEST* dest)
{
    return (frog->y == dest->y && frog->x == dest->x) ? 1 : 0;
}

// --- TIMER FUNCTIONS ---
// TIMER initializer
TIMER* InitTimer(TIMING_CFG* cfg)
{
    TIMER* timer = (TIMER*)malloc(sizeof(TIMER));
    timer->frameNo = 1;
    timer->frameTime = cfg->frameTime;
    timer->timeLeft = cfg->initialTime / 1.0;
    return timer;
}

int UpdateTimer(TIMER* timer, WIN* win, int initialTime)
{
    timer->frameNo++;
    timer->timeLeft = initialTime - (timer->frameNo * timer->frameTime / 1000.0);
    if (timer->timeLeft < timer->frameTime / 1000.0)
    {
        timer->timeLeft = 0;
    }
    else
    {
        usleep(timer->frameTime * 1000);
    }
    PrintTime(win, timer->timeLeft);
    return timer->timeLeft == 0 ? 1 : 0; // 1 if time has elapsed, 0 otherwise    
}


// --- MAIN LOOP ---
GameResult Play(WIN* statusWin, OBJ* frog, CAR** cars, DEST* dest, TIMER* timer, CFG* cfg)
{
    int key;
    while ((key = wgetch(statusWin->window)) != cfg->controls->quit)
    {
        flushinp(); // clear input buffer
        if (key != ERR)
        {
            MoveFrog(frog, cfg->controls, key, cfg->frog->moveFactor, timer->frameNo);
        }
        for (int i = 0; i < cfg->cars->nCars; i++)
        {
            MoveCar(cars[i], timer->frameNo);
        }
        PrintDest(dest);
        PrintObj(frog);  // force overlapping car lanes
        PrintPosition(statusWin, frog);
        if (DestReached(frog, dest))
        {
            return SUCCESS;
        }
        for (int i = 0; i < cfg->cars->nCars; i++)
        {
            if (Collision(frog, cars[i]->obj))
            {
                return FAILURE;
            }
        }
        if (UpdateTimer(timer, statusWin, cfg->timing->initialTime))
        {
            return TIME_OVER;
        }
    }
    return INTERRUPTED;
}


// --- CLEANUP ---
void Cleanup(WIN* playableWin, WIN* statusWin, WINDOW* mainWindow, OBJ* frog, CAR** cars, DEST* dest, TIMER* timer)
{
    delwin(playableWin->window);
    free(playableWin);
    delwin(statusWin->window);
    free(statusWin);
    delwin(mainWindow);
    free(frog);
    for (int i = 0; i < sizeof(cars) / sizeof(CAR*); i++)
    {
        free(cars[i]->obj);
        free(cars[i]);
    }
    free(cars);
    free(dest);
    free(timer); // TODO: add the rest of pointers to the cleanup function
    endwin();
    refresh();
}


// --- MAIN PROGRAM ---
int main()
{
    srand(time(NULL));

    WINDOW* mainWindow = InitGame();
    Welcome(mainWindow);

    CFG* cfg = InitCfg();
    WIN* playableWin = InitWin(mainWindow, cfg->area->playableRows, cfg->area->cols, cfg->area->offy, cfg->area->offx, COLOR_PLAYABLE, DELAY_ON);
    WIN* statusWin = InitWin(mainWindow, cfg->area->statusRows, cfg->area->cols, cfg->area->playableRows + cfg->area->offy, cfg->area->offx, COLOR_STATUS, DELAY_OFF);
    TIMER* timer = InitTimer(cfg->timing);
    OBJ* frog = InitFrog(playableWin, COLOR_FROG, cfg->frog);
    CAR** cars = GenerateCars(playableWin, COLOR_CAR, cfg->cars, cfg->frog->height);
    DEST* destination = InitDest(playableWin, COLOR_DEST, cfg->frog->width); // destination is a single row of the frog's width

    InitStatus(statusWin, timer, frog);

    GameResult result = Play(statusWin, frog, cars, destination, timer, cfg);
    EndGame(statusWin, result, cfg->timing->quitTime);
    Cleanup(playableWin, statusWin, mainWindow, frog, cars, destination, timer);
    return EXIT_SUCCESS;
}