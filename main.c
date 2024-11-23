#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>


// --- SETTINGS ---

// General timing/speed settings
const int FRAME_TIME = 25;  // milliseconds interval between frames
const int INITIAL_TIME = 20;
const int QUIT_TIME = 3;    // seconds to wait after hitting KEY_QUIT
const int FROG_MOVE_FACTOR = 5;
const int CAR_MOVE_FACTOR = 2;

// Controls
const int KEY_QUIT = 'q';
const int NOKEY = ' ';

// Colors
const int COLOR_MAIN = 1;
const int COLOR_STATUS = 2;
const int COLOR_PLAYABLE = 3;
const int COLOR_FROG = 4;
const int COLOR_FROG_R = 5;
const int COLOR_CAR = 6;
const int COLOR_CAR_R = 7;

// Playable area settings
const int PLAYABLE_ROWS = 30;
const int STATUS_ROWS = 3;
const int PLAYABLE_COLS = 120;  // the same for status window
const int OFFY = 0; // optional: window offset within the terminal
const int OFFX = 0;

// Delay constants for real-time
const int DELAY_ON = 1;
const int DELAY_OFF = 0;

// --- DATA STRUCTURES ---

// Window structure
typedef struct {
    WINDOW* window; // ncurses window
    int x, y;
    int rows, cols;
    int color;
} WIN;

// Game object structure - used for frog directly, extended by CAR
typedef struct {
    WIN* win;
    int color;
    int reverseColor;
    int bgFlag;
    int moveFactor;
    int x, y;
    int xmin, xmax;
    int ymin, ymax;
    int width, height;
    char** shape;
} OBJ;

typedef enum {
    Enemy,
    Neutral,
    Friendly
} CarType;

// Car structure
typedef struct {
    OBJ* obj;           // extends game object
    int direction;      // 0 for left, 1 for right
    int dynamicSpeed;   // 1 or 0
    int disappearing;
    CarType type;
} CAR;

// TIMER structure
typedef struct {
    unsigned int frameTime;
    float timeLeft;
    int frameNo;
} TIMER;


// --- RANDOM NUMBER ---
int randInt(int min, int max)
{
    return (min + rand() % (max - min + 1));
}

// --- WINDOW FUNCTIONS ---

// Game initializer
WINDOW* InitGame()
{
    WINDOW* win;
    if ((win = initscr()) == NULL) // initialize ncurses
    {
        fprintf(stderr, "Error initializing ncurses.\n");
        exit(EXIT_FAILURE);
    }

    start_color(); // initialize colors
    init_pair(COLOR_MAIN, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_PLAYABLE, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_STATUS, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_FROG, COLOR_GREEN, COLOR_WHITE);
    init_pair(COLOR_FROG_R, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_CAR, COLOR_RED, COLOR_WHITE);
    init_pair(COLOR_CAR_R, COLOR_RED, COLOR_BLACK);

    noecho(); // turn off displaying input and hide cursor
    curs_set(0);
    return win;
}

// Welcome screen
void Welcome(WINDOW* win)
{
    mvwaddstr(win, 1, 1, "Press any key to start the game.");
    wgetch(win);    // wait for input and clear
    wclear(win);
    wrefresh(win);
}


// --- WIN FUNCTIONS ---

// Cleaning the window (prints " ")
void CleanWin(WIN* win)
{
    wattron(win->window, COLOR_PAIR(win->color));   // apply colors
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
WIN* InitWin(WINDOW* mainWindow, int rows, int cols, int y, int x, int color, int delay)
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
        nodelay(win->window, TRUE); // non-blocking input for real-time
    }
    wrefresh(win->window);
    return win;
}

// Handle quitting the game
void EndGame(WIN* win, const char* message)
{
    CleanWin(win);
    for (int seconds = QUIT_TIME; seconds > 0; seconds--)
    {
        mvwprintw(win->window, 1, 2, "%s Closing the game in %d seconds...", message, seconds);
        wrefresh(win->window);
        sleep(1);
    }
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

// Status initializer
void InitStatus(WIN* win, TIMER* timer, OBJ* frog)
{
    box(win->window, 0, 0);
    mvwprintw(win->window, 1, 3, "Time: ");
    PrintTime(win, timer->timeLeft);
    mvwprintw(win->window, 1, 35, "Position: ");
    PrintPosition(win, frog);
    mvwprintw(win->window, 1, 78, "Kacper Neumann, 203394");
}

// --- GAMEOBJECT FUNCTIONS ---

// Print game object's shape
void PrintObject(OBJ* obj)
{
    for (int i = 0; i < obj->height; i++)
    {
        mvwprintw(obj->win->window, obj->y + i, obj->x, "%s", obj->shape[i]);
    }
}

// Move the game object along both axes by 1
void MoveObject(OBJ* obj, int dx, int dy)
{
    char* emptyRow = (char*)malloc(obj->width * sizeof(char));    // string of empty spaces to erase the old position row
    memset(emptyRow, ' ', obj->width);

    if (obj->bgFlag)
    {
        wattron(obj->win->window, COLOR_PAIR(obj->color));
    }
    else
    {
        wattron(obj->win->window, COLOR_PAIR(obj->reverseColor));
    }

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

    if (obj->bgFlag)
    {
        wattron(obj->win->window, COLOR_PAIR(obj->win->color));
    }
    wrefresh(obj->win->window);
}

void SetObjectPosition(OBJ* obj, int x, int y)
{
    obj->x = x;
    obj->y = y;
}

void ReverseObjectColors(OBJ* obj)
{
    obj->bgFlag = !obj->bgFlag;
}

// Frog initializer
OBJ* InitFrog(WIN* win, int color, int reverseColor)
{
    OBJ* frog = (OBJ*)malloc(sizeof(OBJ));
    frog->win = win;
    frog->color = color;
    frog->reverseColor = reverseColor;
    frog->bgFlag = 1;
    frog->width = 6;
    frog->height = 3;
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

// // Car initializer
CAR* InitCar(WIN* win, int color, int reverseColor, int y, int dynamicSpeed, int disappearing, CarType type)
{
    OBJ* obj = (OBJ*)malloc(sizeof(OBJ));
    obj->win = win;
    obj->color = color;
    obj->reverseColor = reverseColor;
    obj->bgFlag = 1;
    obj->width = 8;
    obj->height = 3;
    obj->moveFactor = CAR_MOVE_FACTOR; // random speed

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
    obj->ymin = obj->y; // the car does not move vertically
    obj->ymax = obj->y;

    CAR* car = (CAR*)malloc(sizeof(CAR));
    car->obj = obj;
    car->direction = randInt(0, 1);
    car->dynamicSpeed = dynamicSpeed;
    car->disappearing = disappearing;
    car->type = type;
    SetObjectPosition(obj, car->direction == 0 ? win->cols - obj->width : 1, y); // based on initial direction
    return car;
}

// Frog movement
void MoveFrog(OBJ* frog, char key, unsigned int frame)
{
    if (frame - frog->moveFactor >= FROG_MOVE_FACTOR)
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

// Reverse direction when car hits the wall
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
        MoveObject(car->obj, car->direction == 0 ? -1 : 1, 0); // based on direction
    }
}

int DetectCollision(OBJ* obj, OBJ* other)
{
    return ((
        (obj->y >= other->y && obj->y < other->y + other->height) ||
        (other->y >= obj->y && other->y < obj->y + obj->height)
        ) && (
            (obj->x >= other->x && obj->x < other->x + other->width) ||
            (other->x >= obj->x && other->x < obj->x + obj->width)
            )) ? 1 : 0;
}


// --- TIMER FUNCTIONS ---

// TIMER initializer
TIMER* InitTimer()
{
    TIMER* timer = (TIMER*)malloc(sizeof(TIMER));
    timer->frameNo = 1;
    timer->frameTime = FRAME_TIME;
    timer->timeLeft = INITIAL_TIME / 1.0;
    return timer;
}

int UpdateTimer(TIMER* timer, WIN* win)
{
    timer->frameNo++;
    timer->timeLeft = INITIAL_TIME - (timer->frameNo * timer->frameTime / 1000.0);
    if (timer->timeLeft < timer->frameTime / 1000.0)
    {
        timer->timeLeft = 0;
    }
    else
    {
        usleep(timer->frameTime * 1000);
    }
    PrintTime(win, timer->timeLeft);
    return timer->timeLeft == 0 ? 1 : 0; // return 1 if time has elapsed or 0 otherwise    
}


// --- MAIN LOOP ---
int MainLoop(WIN* statusWin, OBJ* frog, CAR* car, TIMER* timer)
{
    int key;
    while ((key = wgetch(statusWin->window)) != KEY_QUIT)
    {
        if (key == ERR)
        {
            key = NOKEY;
        }
        else
        {
            MoveFrog(frog, key, timer->frameNo);
        }

        MoveCar(car, timer->frameNo);

        PrintPosition(statusWin, frog);
        flushinp(); // clear input buffer
        if (UpdateTimer(timer, statusWin))
        {
            return 1; // return points or sth
        }
    }
    return 0;
}

// --- MAIN PROGRAM ---
int main()
{
    WINDOW* mainWindow = InitGame();
    Welcome(mainWindow);

    WIN* playableWin = InitWin(mainWindow, PLAYABLE_ROWS, PLAYABLE_COLS, OFFY, OFFX, COLOR_PLAYABLE, DELAY_ON);
    WIN* statusWin = InitWin(mainWindow, STATUS_ROWS, PLAYABLE_COLS, PLAYABLE_ROWS + OFFY, OFFX, COLOR_STATUS, DELAY_OFF);

    TIMER* timer = InitTimer();

    OBJ* frog = InitFrog(playableWin, COLOR_FROG, COLOR_FROG_R);
    CAR* car = InitCar(playableWin, COLOR_CAR, COLOR_CAR_R, 1, 0, 0, Enemy);

    InitStatus(statusWin, timer, frog);
    MoveObject(frog, 0, 0);
    MoveObject(car->obj, 0, 0);

    int result;
    if ((result = MainLoop(statusWin, frog, car, timer)) == 0)
    {
        EndGame(statusWin, "You have decided to quit the game.");
    }
    else
    {
        char message[100];
        sprintf(message, "Timer is over. Points: %d", result);
        EndGame(statusWin, message);
    }

    delwin(playableWin->window);    // clean up
    delwin(statusWin->window);
    delwin(mainWindow);
    endwin();
    refresh();
    return EXIT_SUCCESS;
}