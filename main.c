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
    Enemy,      // normal car
    Neutral,    // stops when the frog is close
    Friendly    // helps the frog on demand
} CarType;

// Color identifiers
typedef enum {
    COLOR_MAIN,
    COLOR_STATUS,
    COLOR_PLAYABLE,
    COLOR_FROG,
    COLOR_CAR,
    COLOR_DEST
} Color;


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
    int disappearing;   // 0 for perpetually bouncing car, 1 for disappearing (replaced with a new car)
    CarType type;
} CAR;

// Timer structure
typedef struct {
    int frameNo;
    int frameTime;
    float timeLeft;
} TIMER;


// --- RANDOM NUMBER (inclusive) ---
int RandInt(int min, int max)
{
    return min + rand() % (max - min + 1);
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
    init_pair(COLOR_CAR, COLOR_RED, COLOR_WHITE);
    init_pair(COLOR_DEST, COLOR_GREEN, COLOR_WHITE);

    noecho();       // turn off displaying input and hide cursor
    curs_set(0);
    return win;
}

// Welcome screen - wait for user input before starting the game
void Welcome(WINDOW* win)
{
    mvwprintw(win, 1, 1, "Press any key to start the game.");
    wgetch(win);
    wclear(win);
    wrefresh(win);
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
    nodelay(win->window, TRUE);                     // non-blocking input for real-time
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

void PrintPosition(WIN* status, OBJ* frog)
{
    mvwprintw(status->window, 1, status->cols / 2 - 10, "Position: x: %d y: %d", frog->x, frog->y);
    wrefresh(status->window);
}

// Status window initializer
void InitStatus(WIN* status, TIMER* timer, OBJ* frog)
{
    box(status->window, 0, 0);
    PrintTime(status, timer->timeLeft);
    PrintPosition(status, frog);
    const char* signature = "Kacper Neumann, 203394";
    mvwprintw(status->window, 1, status->cols - strlen(signature) - 2, "%s", signature);
}

// Display information about the result of the game and count down to quit
void EndGame(WIN* status, GameResult result, int quitTime)
{
    ClearWin(status);
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
        mvwprintw(status->window, 1, 2, "%s Closing the game in %d seconds...", message, seconds);
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
OBJ* InitFrog(WIN* win, Color color, FROG_CFG* cfg)
{
    OBJ* frog = (OBJ*)malloc(sizeof(OBJ));
    frog->win = win;
    frog->color = color;
    frog->moveFactor = 0;
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


// --- CAR FUNCTIONS ---
// Car initializer
CAR* InitCar(WIN* win, Color color, CARS_CFG* cfg, int y, int disappearing, CarType type)
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
    car->type = type;
    obj->x = car->direction == 0 ? obj->xmax - obj->width : obj->xmin;  // depends on initial direction
    obj->y = y;
    return car;
}

// Calculate car's y-coordinate adjusted with frog's height so that there are safe lanes
int CalculateCarY(int carIndex, int carHeight, int frogHeight)
{
    return carIndex * (carHeight + frogHeight) + frogHeight + 1;
}

CAR** GenerateCars(WIN* win, Color color, CARS_CFG* cfg, int frogHeight)
{
    CAR** cars = (CAR**)malloc(cfg->nCars * sizeof(CAR*));
    for (int i = 0; i < cfg->nCars; i++)
    {
        // cars may be disappearing and are enemies initially
        cars[i] = InitCar(win, color, cfg, CalculateCarY(i, cfg->height, frogHeight), RandInt(0, 1), Enemy);
        MoveObj(cars[i]->obj, 0, 0); // force first render
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

void PrintLanes(WIN* win, CARS_CFG* cfg, int frogHeight)
{
    for (int i = 0; i < cfg->nCars; i++)
    {
        int y = CalculateCarY(i, cfg->height, frogHeight);
        mvwhline(win->window, y + cfg->height, win->x + 1, '-', win->cols - 2); // draw lane below the car   
    }
}


// --- TIMER FUNCTIONS ---
// Timer initializer
TIMER* InitTimer(TIMING_CFG* cfg)
{
    TIMER* timer = (TIMER*)malloc(sizeof(TIMER));
    timer->frameNo = 1;
    timer->frameTime = cfg->frameTime;
    timer->timeLeft = cfg->initialTime;
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
    return timer->timeLeft == 0 ? 1 : 0;    // 1 if time has elapsed, 0 otherwise    
}


// --- MAIN LOOP ---
GameResult Play(WIN* playable, WIN* status, OBJ* frog, CAR** cars, OBJ* dest, TIMER* timer, CFG* cfg)
{
    PrintObj(dest);
    int key;
    while ((key = wgetch(status->window)) != cfg->controls->quit)
    {
        flushinp(); // clear input buffer
        if (key != ERR)
        {
            MoveFrog(frog, cfg->controls, key, cfg->frog->moveFactor, timer->frameNo);
        }
        // TODO: move to independent function
        for (int i = 0; i < cfg->cars->nCars; i++)
        {
            if (cars[i] != NULL)
            {
                if (cars[i]->dynamicSpeed && timer->frameNo % cfg->timing->carSpeedChangeFactor == 0)
                {
                    cars[i]->obj->moveFactor = RandInt(cfg->cars->minMoveFactor, cfg->cars->maxMoveFactor);
                }
                MoveCar(&cars[i], timer->frameNo);
            }
            else if (timer->frameNo % cfg->timing->carSpawnFactor == 0)
            {
                // the new one will be disappearing as well
                cars[i] = InitCar(playable, COLOR_CAR, cfg->cars, CalculateCarY(i, cfg->cars->height, cfg->frog->height), 1, Enemy);
            }
        }
        PrintLanes(playable, cfg->cars, cfg->frog->height);
        PrintObj(frog);  // force overlapping car lanes
        PrintPosition(status, frog);
        if (Collision(frog, dest))
        {
            return SUCCESS;
        }
        // TODO: move to independent function
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
void Cleanup(WIN* playable, WIN* status, WINDOW* mainWindow, OBJ* frog, CAR** cars, OBJ* dest, TIMER* timer)
{
    delwin(playable->window);   // TODO: include more cleanup
    free(playable);
    delwin(status->window);
    free(status);
    delwin(mainWindow);
    free(frog->shape);
    free(frog);
    for (int i = 0; i < sizeof(cars) / sizeof(CAR*); i++)
    {
        if (cars[i] != NULL)    // watch out disappearing cars
        {
            free(cars[i]->obj->shape);
            free(cars[i]->obj);
            free(cars[i]);
        }
    }
    free(cars);
    free(dest);
    free(timer);
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
    WIN* playable = InitWin(mainWindow, cfg->area->playableRows, cfg->area->cols, cfg->area->offy, cfg->area->offx, COLOR_PLAYABLE);
    WIN* status = InitWin(mainWindow, cfg->area->statusRows, cfg->area->cols, cfg->area->playableRows + cfg->area->offy, cfg->area->offx, COLOR_STATUS);
    TIMER* timer = InitTimer(cfg->timing);
    OBJ* frog = InitFrog(playable, COLOR_FROG, cfg->frog);
    CAR** cars = GenerateCars(playable, COLOR_CAR, cfg->cars, cfg->frog->height);
    OBJ* dest = InitDest(playable, COLOR_DEST);

    InitStatus(status, timer, frog);

    GameResult result = Play(playable, status, frog, cars, dest, timer, cfg);
    EndGame(status, result, cfg->timing->quitTime);
    Cleanup(playable, status, mainWindow, frog, cars, dest, timer);
    return EXIT_SUCCESS;
}