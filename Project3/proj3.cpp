/* David LaMartina
 * lamartid@oregonstate.edu
 * Project 3: Functional Decomposition
 * CS475 Spr2019
 * Due May 6, 2019
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <omp.h>

#include "helpers.h"

#define NUMTHREADS          4
#define FIRST_YEAR          2019
#define LAST_YEAR           2024

// System 'state' globals
int     NowYear;            // 2019 - 2024
int     NowMonth;           // 0 - 11

float   NowAngle;           // Angle used for temp & precip calcs
float   NowPrecip;          // inches of rain per month
float   NowTemp;            // temperature this month
float   NowHeight;          // grain height in inches
int     NowNumDeer;         // number of deer in current population
int     NowNumBears;        // number of bears in current population
float   DeerToBears;        // Ratio of deer to bears

// Params defaults
// Grain growth in inches
// Temperature in degrees F
// Precipitation in inches
const float GRAIN_GROWS_PER_MONTH   =       8.0;
const float ONE_DEER_EATS_PER_MONTH =       0.5;

const float AVG_PRECIP_PER_MONTH    =       6.0;    // average
const float AMP_PRECIP_PER_MONTH    =       6.0;    // plus or minus
const float RANDOM_PRECIP           =       2.0;    // plus or minus noise

const float AVG_TEMP                =       50.0;   // average
const float AMP_TEMP                =       20.0;   // plus or minus
const float RANDOM_TEMP             =       10.0;   // plus or minus noise

const float MIDTEMP                 =       40.0;
const float MIDPRECRIP              =       10.0;

const float DB_EQUILIBRIUM          =       2.0;    // Equilibrium b/w deer and bears

// Prototypes
void GrainDeer(unsigned int*);
void Grain(unsigned int*);
void Watcher(unsigned int*);
void Bears(unsigned int*);
void UpdateState(unsigned int*);
int  AbsoluteMonth(int year, int month);

int
main(int argc, char* argv[]){ 
    // Starting state
    NowMonth    =   0;
    NowYear     =   FIRST_YEAR;

    unsigned int seedI = time(0);   // Separate seed for main & each thread
    NowNumDeer  =   1;
    NowNumBears =   1;
    NowHeight   =   1.;
    UpdateState(&seedI);
 
    printf("Month\tPrecip\tTemp\tHeight\tDeer\tBears\n");

    omp_set_num_threads(NUMTHREADS);    // same as # decomposed functions
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            unsigned int seedGD = time(0);
            GrainDeer(&seedGD);
        }
        #pragma omp section
        {
            unsigned int seedG = time(0);
            Grain(&seedG);
        }
        #pragma omp section
        {
            unsigned int seedW = time(0);
            Watcher(&seedW);
        }
        #pragma omp section
        {
            unsigned int seedB = time(0);
            Bears(&seedB);
        }
    }
    return 0;
}

void GrainDeer(unsigned int* seedp){
    // Compute number of deer based on amount of grain
    // Increment or decrement based on available grain
    int deltaDeer;
    while(NowYear <= LAST_YEAR){
        // Account for grain height
        if (NowNumDeer > NowHeight){
            deltaDeer = -1;
        }
        else if (NowNumDeer < NowHeight){
            deltaDeer = 1;
        }
        else{
            deltaDeer = 0;
        }
        // Account for bears
        if (DeerToBears > 2){
            deltaDeer -= 1;
        }
        #pragma omp barrier // compute barrier

        NowNumDeer += deltaDeer;
        if (NowNumDeer < 0)
            NowNumDeer = 0;
        #pragma omp barrier // assign barrier

        #pragma omp barrier // output & update barrier
    }
}

// Bears eat deer! But they'll die off if there aren't enough deer around.
void Bears(unsigned int* seedp){
    int deltaBears;
    while(NowYear <= LAST_YEAR){
        if (NowNumBears > NowNumDeer){
            deltaBears = -1;
        }
        else if (NowNumBears < NowNumDeer){
            deltaBears = 1;
        }
        else{
            deltaBears = 0;
        }
        #pragma omp barrier // compute barrier

        NowNumBears += deltaBears;
        if (NowNumBears <= 0)
            NowNumBears = 0;
        #pragma omp barrier // assign barrier

        #pragma omp barrier // output & update barrier
    }
}

void Grain(unsigned int* seedp){
    // Compute height of grain based on temp, precip, numDeer
    float deltaHeight;
    while(NowYear <= LAST_YEAR){
        deltaHeight = TempFactor(NowTemp, MIDTEMP) *
            PrecipFactor(NowPrecip, MIDPRECRIP) *
            GRAIN_GROWS_PER_MONTH;
       
        deltaHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        #pragma omp barrier // compute barrier

        NowHeight += deltaHeight;
        if (NowHeight < 0){
            NowHeight = 0;
        }
        #pragma omp barrier // assign barrier

        #pragma omp barrier // ouput & update barrier
    }    
}

void Watcher(unsigned int* seedp){
    while(NowYear <= LAST_YEAR){
        #pragma omp barrier // compute barrier
        #pragma omp barrier // assign barrier

        // Output all globals
        printf("%d\t%.3lf\t%.3lf\t%.3lf\t%d\t%d\n",
                AbsoluteMonth(NowYear, NowMonth),
                NowPrecip,
                FtoC(NowTemp),
                NowHeight,
                NowNumDeer,
                NowNumBears);

        // Update globals
        NowMonth++;
        if (NowMonth > 11){
            NowYear++;
            NowMonth = 0;
        }
        UpdateState(seedp);
        #pragma omp barrier // output & update barrier
    }
}


// Update state 
void UpdateDeerToBears(){
    if (NowNumBears <= 0){
        DeerToBears = FLT_MAX;
    }
    else{
        DeerToBears = (float)NowNumDeer / (float)NowNumBears;
    }
}

void UpdateState(unsigned int* seedp){
    NowAngle    = Angle(NowMonth);
    NowPrecip   = Precip(AVG_PRECIP_PER_MONTH,
                         AMP_PRECIP_PER_MONTH,
                         RANDOM_PRECIP,
                         NowAngle,
                         seedp);
    NowTemp     = Temp  (AVG_TEMP,
                         AMP_TEMP,
                         RANDOM_TEMP,
                         NowAngle,
                         seedp);
    UpdateDeerToBears();
}


// Calculate exact month for graph-friendly output
int AbsoluteMonth(int year, int month){
    return (year - FIRST_YEAR) * 12 + month + 1;
}
