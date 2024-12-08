// score.h
#ifndef SCORE_H
#define SCORE_H

#include "cfg.h"

// Score structure
typedef struct {
    float highest;
    float last;
} SCORE;

// --- SCORE FUNCTIONS ---
void ReadScore(SCORE* score);
void SaveScore(float newScore);
float CalculateScore(int nJumps, float timeLeft, CFG* cfg);

#endif // SCORE_H