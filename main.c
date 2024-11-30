/*
    Jumping Frog
    Kacper Neumann, 203394

    This game has been developed based on the demo game provided by Prof. Michał Małafiejski (CATCH THE BALL)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>


// --- SETTINGS ---
// General timing/speed settings
const int FRAME_TIME = 25;      // milliseconds interval between frames
const int INITIAL_TIME = 20;
const int QUIT_TIME = 3;        // seconds to wait after hitting KEY_QUIT

// Frog
const int FROG_WIDTH = 6;
const int FROG_HEIGHT = 3;
const int FROG_MOVE_FACTOR = 5;

// Car
const int CAR_WIDTH = 8;
const int CAR_HEIGHT = 3;
const int CAR_MOVE_FACTOR = 2;

// Controls
const int KEY_QUIT = 'q';

// Colors
typedef enum {
    COLOR_MAIN,
    COLOR_STATUS,
    COLOR_PLAYABLE,
    COLOR_FROG,
    COLOR_CAR,
    COLOR_DEST
} Color;

// Area settings
const int PLAYABLE_ROWS = 30;
const int PLAYABLE_COLS = 120;  // the same for status window
const int STATUS_ROWS = 3;
const int OFFY = 0;             // optional: window offset within the main window
const int OFFX = 0;

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
    unsigned int frameTime;
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
void PrintPosition(WIN* win, OBJ* frog)
{
    mvwprintw(win->window, 1, 45, "x: %d y: %d", frog->x, frog->y);
    wrefresh(win->window);
}

void PrintTime(WIN* win, float timeLeft)
{
    mvwprintw(win->window, 1, 9, "%.2f", timeLeft);
    wrefresh(win->window);
}

// Status window initializer
void InitStatus(WIN* win, TIMER* timer, OBJ* frog)
{
    box(win->window, 0, 0);
    mvwprintw(win->window, 1, 3, "Time: ");
    PrintTime(win, timer->timeLeft);
    mvwprintw(win->window, 1, 35, "Position: ");
    PrintPosition(win, frog);
    mvwprintw(win->window, 1, 78, "Kacper Neumann, 203394");
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
void PrintObject(OBJ* obj)
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
void MoveObject(OBJ* obj, int dx, int dy)
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

    PrintObject(obj);
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

void SetObjectPosition(OBJ* obj, int x, int y)
{
    obj->x = x;
    obj->y = y;
}

// Frog initializer
OBJ* InitFrog(WIN* win, Color color, int width, int height)
{
    OBJ* frog = (OBJ*)malloc(sizeof(OBJ));
    frog->win = win;
    frog->color = color;
    frog->width = width;
    frog->height = height;
    frog->moveFactor = 0;
    frog->shape = (char**)malloc(frog->height * sizeof(char*));
    for (int i = 0; i < frog->height; i++)
    {
        frog->shape[i] = (char*)malloc((frog->width + 1) * sizeof(char));   // +1 for '\0'
    }

    strcpy(frog->shape[0], " @..@ ");
    strcpy(frog->shape[1], "(----)");
    strcpy(frog->shape[2], " ^  ^ ");

    SetObjectPosition(frog, (win->cols - frog->width) / 2, win->rows - frog->height - 1);

    frog->xmin = 1;
    frog->xmax = win->cols - 1;
    frog->ymin = 1;
    frog->ymax = win->rows - 1;
    return frog;
}

// Frog movement
void MoveFrog(OBJ* frog, char key, unsigned int frame)
{
    if (frame - frog->moveFactor >= FROG_MOVE_FACTOR)   // movement cooldown condition // TODO: move elsewhere once settings structure is implemented
    {
        switch (key)
        {
            case 'w':
                MoveObject(frog, 0, -1);
                break;
            case 's':
                MoveObject(frog, 0, 1);
                break;
            case 'a':
                MoveObject(frog, -1, 0);
                break;
            case 'd':
                MoveObject(frog, 1, 0);
        }
        frog->moveFactor = frame;
    }
}


// --- CAR FUNCTIONS ---
// Car initializer
CAR* InitCar(WIN* win, Color color, int width, int height, int moveFactor, int y, int dynamicSpeed, int disappearing, CarType type)
{
    OBJ* obj = (OBJ*)malloc(sizeof(OBJ));
    obj->win = win;
    obj->color = color;
    obj->width = width;
    obj->height = height;
    obj->moveFactor = moveFactor;

    obj->shape = (char**)malloc(obj->height * sizeof(char*));
    for (int i = 0; i < obj->height; i++)
    {
        obj->shape[i] = (char*)malloc((obj->width + 1) * sizeof(char)); // +1 for '\0'
    }

    strcpy(obj->shape[0], "  ____  ");
    strcpy(obj->shape[1], "_/____\\_");
    strcpy(obj->shape[2], " O    O ");

    obj->xmin = 1;
    obj->xmax = win->cols - 1;
    obj->ymin = obj->y; // cars don't move vertically
    obj->ymax = obj->y;

    CAR* car = (CAR*)malloc(sizeof(CAR));
    car->obj = obj;
    car->direction = RandInt(0, 1);     // initial direction is random
    car->dynamicSpeed = dynamicSpeed;
    car->disappearing = disappearing;
    car->type = type;
    SetObjectPosition(obj, car->direction == 0 ? obj->xmax - obj->width : obj->xmin, y); // depends on initial direction
    return car;
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
void MoveCar(CAR* car, unsigned int frame)
{
    ReverseCarDirection(car);
    if (frame % car->obj->moveFactor == 0)
    {
        MoveObject(car->obj, car->direction == 0 ? -1 : 1, 0);  // depends on direction
    }

    mvwhline(car->obj->win->window, car->obj->y - 1, car->obj->win->x + 1, '-', car->obj->win->cols - 2);  // draw car lane
    mvwhline(car->obj->win->window, car->obj->y + car->obj->height, car->obj->win->x + 1, '-', car->obj->win->cols - 2);
}


// --- DESTINATION (DEST) FUNCTIONS ---
// Destination initializer
DEST* InitDest(WIN* win, Color color, int width, int height)
{
    DEST* dest = (DEST*)malloc(sizeof(DEST));
    dest->win = win;
    dest->color = color;
    dest->width = width;
    dest->height = height;
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
TIMER* InitTimer(unsigned int frameTime, int initialTime)
{
    TIMER* timer = (TIMER*)malloc(sizeof(TIMER));
    timer->frameNo = 1;
    timer->frameTime = frameTime;
    timer->timeLeft = initialTime / 1.0;
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
GameResult Play(WIN* statusWin, OBJ* frog, CAR* car, DEST* dest, TIMER* timer, int quit, int initialTime)
{
    int key;
    while ((key = wgetch(statusWin->window)) != quit)
    {
        flushinp(); // clear input buffer
        if (key != ERR)
        {
            MoveFrog(frog, key, timer->frameNo);
        }
        MoveCar(car, timer->frameNo);   // TODO: move all cars
        PrintDest(dest);
        PrintObject(frog);  // force overlapping car lanes
        PrintPosition(statusWin, frog);
        if (DestReached(frog, dest))
        {
            return SUCCESS;
        }
        if (Collision(frog, car->obj))
        {
            return FAILURE;
        }
        if (UpdateTimer(timer, statusWin, initialTime))
        {
            return TIME_OVER;
        }
    }
    return INTERRUPTED;
}


// --- MAIN PROGRAM ---
int main()
{
    srand(time(NULL));

    WINDOW* mainWindow = InitGame();
    Welcome(mainWindow);

    WIN* playableWin = InitWin(mainWindow, PLAYABLE_ROWS, PLAYABLE_COLS, OFFY, OFFX, COLOR_PLAYABLE, DELAY_ON);
    WIN* statusWin = InitWin(mainWindow, STATUS_ROWS, PLAYABLE_COLS, PLAYABLE_ROWS + OFFY, OFFX, COLOR_STATUS, DELAY_OFF);

    TIMER* timer = InitTimer(FRAME_TIME, INITIAL_TIME);

    OBJ* frog = InitFrog(playableWin, COLOR_FROG, FROG_WIDTH, FROG_HEIGHT);
    CAR* car = InitCar(playableWin, COLOR_CAR, CAR_WIDTH, CAR_HEIGHT, CAR_MOVE_FACTOR, 20, 0, 0, Enemy); // TODO: create a car on each lane
    DEST* destination = InitDest(playableWin, COLOR_DEST, FROG_WIDTH, 1);

    MoveObject(car->obj, 0, 0);     // force first render
    InitStatus(statusWin, timer, frog);

    GameResult result = Play(statusWin, frog, car, destination, timer, KEY_QUIT, INITIAL_TIME);
    EndGame(statusWin, result, QUIT_TIME);

    delwin(playableWin->window);    // clean-up
    delwin(statusWin->window);
    delwin(mainWindow);
    endwin();
    refresh();      // TODO: free memory - Cleanup()
    return EXIT_SUCCESS;
}