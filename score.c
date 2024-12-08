// score.c
#include <stdio.h>
#include <stdlib.h>
#include "score.h"

// Scoring weights - should sum up to 100
const float TIME_FACTOR = 50;
const float JUMP_FACTOR = 15;
const float DIFFICULTY_FACTOR = 35;


// --- SCORE FUNCTIONS ---
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

float CalculateScore(int nJumps, float timeLeft, CFG* cfg)
{
    float timeScore = (timeLeft / cfg->initialTime) * TIME_FACTOR;      // more time left yields higher scores
    float jumpScore = (1.0 / nJumps) * JUMP_FACTOR;                     // fewer jumps yield higher scores
    int enemyCars = cfg->nCars - cfg->nNeutralCars - cfg->nFriendlyCars;
    float difficultyScore = (float)enemyCars / (cfg->nCars) * DIFFICULTY_FACTOR;   // more enemy cars increase difficulty
    float score = timeScore + jumpScore + difficultyScore;
    return score;
}