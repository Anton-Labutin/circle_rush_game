#include "Engine.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <list>
#include <algorithm>
#include <utility>
#include <math.h>
#include <time.h>

using namespace std;


//
//  You are free to modify this file
//

//  is_key_pressed(int button_vk_code) - check if a key is pressed,
//                                       use keycodes (VK_SPACE, VK_RIGHT, VK_LEFT, VK_UP, VK_DOWN, VK_RETURN)
//
//  get_cursor_x(), get_cursor_y() - get mouse cursor position
//  is_mouse_button_pressed(int button) - check if mouse button is pressed (0 - left button, 1 - right button)
//  schedule_quit_game() - quit game after act()


using Square = struct Square {
    double centerX;
    double centerY;
    double side;
};

enum class ObstacleType {
    kPrize, 
    kPenalty, 
    kKiller, 

    kTypesCount
};

using Obstacle = struct Obstacle {
    Square size;
 
    double vel;
    double velAngle;

    virtual ObstacleType GetType() = 0;
    virtual ~Obstacle() {}
};

struct Prize : Obstacle {
    ObstacleType GetType() 
    {
        return ObstacleType::kPrize;
    }
};

struct Penalty : Obstacle {
    ObstacleType GetType() 
    {
        return ObstacleType::kPenalty;
    }
};

struct Killer : Obstacle {
    ObstacleType GetType() 
    {
        return ObstacleType::kKiller;
    }
};

using ObstacleBaseParams = struct ObstacleBaseParams {
    double side;
    double vel;
};

struct PrizeBaseParams : ObstacleBaseParams {
};

struct PenaltyBaseParams : ObstacleBaseParams {    
};

struct KillerBaseParams : ObstacleBaseParams {
};

using BallsBaseParams = struct BallsBaseParams {
    double anglVel;
};

using Circle = struct Circle {
    double centerX;
    double centerY;
    double radius;
};

using BallsField = struct BallsField {
    Circle ring;

    struct BallsOnRing {
        double radius; 
        double angle; 
        double anglVel; // > 0 => counterclosckwise
    } balls;
};

enum class LEVEL {
    kEasy, 
    kNormal, 
    kHard, 
    kCnt
};

const int typesCnt = static_cast<int>(ObstacleType::kTypesCount);


LEVEL curLevel;
const int levelCnt = static_cast<int>(LEVEL::kCnt);

double prizeSideCoefPerLevel[levelCnt];
double penaltySideCoefPerLevel[levelCnt];
double killerSideCoefPerLevel[levelCnt];

double obstacleVelCoefPerLevel[levelCnt];

double ballsAnglVelCoefPerLevel[levelCnt];

float obstacleEmergenceTimePerLevel[levelCnt];

int scoreForNextLevel[levelCnt];

const double kPi = acos(-1.0);

BallsField ballsField;

list<Obstacle*> obstacles;

PrizeBaseParams prizeBaseParams;
PenaltyBaseParams penaltyBaseParams;
KillerBaseParams killerBaseParams; 

BallsBaseParams ballsBaseParams;

uint32_t gameFieldColor;
uint32_t ringColor;
uint32_t ballsColor;
uint32_t colorPerObstacleType[typesCnt];

int pointsPerLevelAndObstacleType[levelCnt][typesCnt];
int score;

unsigned hitPerObstacleType[typesCnt];


void 
PrintScore()
{
    printf("Score: %d\n", score);
}


void 
PrintResults()
{
    printf("Final results: \n");

    printf("   level: ");
    if (curLevel == LEVEL::kEasy) {
        printf("easy");
    } else {
        if (curLevel == LEVEL::kNormal) {
            printf("normal");
        } else {
            printf("hard");
        }
    }
    printf("\n");

    printf("   score: %d\n", score);
    printf("   hit prizes: %u\n", hitPerObstacleType[static_cast<int>(ObstacleType::kPrize)]);
    printf("   hit penalties: %u\n", hitPerObstacleType[static_cast<int>(ObstacleType::kPenalty)]);
}


double 
CalcDist2(double x, double y) 
{
    return pow(x, 2) + pow(y, 2);
}


double 
CalcDist(double x, double y) 
{
    return sqrt(CalcDist2(x, y));
}


void 
InitRandomizer() 
{
    srand((unsigned) time(0));
}


void 
InitEasyLevel()
{
    int easyLevel = static_cast<int>(LEVEL::kEasy);

    prizeSideCoefPerLevel[easyLevel] = 1;
    penaltySideCoefPerLevel[easyLevel] = 1;
    killerSideCoefPerLevel[easyLevel] = 1;

    obstacleVelCoefPerLevel[easyLevel] = 0.1;

    ballsAnglVelCoefPerLevel[easyLevel] = 0.25;

    obstacleEmergenceTimePerLevel[easyLevel] = 6;

    pointsPerLevelAndObstacleType[easyLevel][static_cast<int>(ObstacleType::kPrize)] = 1;
    pointsPerLevelAndObstacleType[easyLevel][static_cast<int>(ObstacleType::kPenalty)] = -1;
    pointsPerLevelAndObstacleType[easyLevel][static_cast<int>(ObstacleType::kKiller)] = 0;

    scoreForNextLevel[easyLevel] = 3;
}


void 
InitNormalLevel()
{
    int normalLevel = static_cast<int>(LEVEL::kNormal);

    prizeSideCoefPerLevel[normalLevel] = 1;
    penaltySideCoefPerLevel[normalLevel] = 1;
    killerSideCoefPerLevel[normalLevel] = 1;

    obstacleVelCoefPerLevel[normalLevel] = 0.15;

    ballsAnglVelCoefPerLevel[normalLevel] = 0.4;

    obstacleEmergenceTimePerLevel[normalLevel] = 4;

    pointsPerLevelAndObstacleType[normalLevel][static_cast<int>(ObstacleType::kPrize)] = 2;
    pointsPerLevelAndObstacleType[normalLevel][static_cast<int>(ObstacleType::kPenalty)] = -1;
    pointsPerLevelAndObstacleType[normalLevel][static_cast<int>(ObstacleType::kKiller)] = 0;

    scoreForNextLevel[normalLevel] = 9;
 }


void 
InitHardLevel()
{
    int hardLevel = static_cast<int>(LEVEL::kHard);

    prizeSideCoefPerLevel[hardLevel] = 0.75;
    penaltySideCoefPerLevel[hardLevel] = 1.25;
    killerSideCoefPerLevel[hardLevel] = 1.25;

    obstacleVelCoefPerLevel[hardLevel] = 0.15;

    ballsAnglVelCoefPerLevel[hardLevel] = 0.6;

    obstacleEmergenceTimePerLevel[hardLevel] = 2;

    pointsPerLevelAndObstacleType[hardLevel][static_cast<int>(ObstacleType::kPrize)] = 3;
    pointsPerLevelAndObstacleType[hardLevel][static_cast<int>(ObstacleType::kPenalty)] = -1;
    pointsPerLevelAndObstacleType[hardLevel][static_cast<int>(ObstacleType::kKiller)] = 0;
}


void 
InitLevels()
{
    curLevel = LEVEL::kEasy;
    
    InitEasyLevel();
    InitNormalLevel();
    InitHardLevel();
}


void 
InitBallsField()
{
    ballsField.ring.centerX = 0.5 * SCREEN_WIDTH;
    ballsField.ring.centerY = 0.5 * SCREEN_HEIGHT;
    ballsField.ring.radius = 0.25 * min(SCREEN_HEIGHT, SCREEN_WIDTH);
    ringColor = 0x8FBC8F;

    ballsField.balls.angle = 0;
    ballsField.balls.radius = ballsField.ring.radius / 6;

    ballsBaseParams.anglVel = kPi;
    ballsField.balls.anglVel = ballsAnglVelCoefPerLevel[static_cast<int>(curLevel)] * ballsBaseParams.anglVel;

    ballsColor = 0xFFFAFA;
}


void 
InitObstacles()
{
    double screenDiag = CalcDist(SCREEN_WIDTH, SCREEN_HEIGHT);

    prizeBaseParams.side = 2 * ballsField.balls.radius;
    prizeBaseParams.vel = screenDiag;
    colorPerObstacleType[static_cast<int>(ObstacleType::kPrize)] = ballsColor;
    hitPerObstacleType[static_cast<int>(ObstacleType::kPrize)] = 0;

    penaltyBaseParams.side = 2 * ballsField.balls.radius;
    penaltyBaseParams.vel = screenDiag;
    colorPerObstacleType[static_cast<int>(ObstacleType::kPenalty)] = 0xFFA500;
    hitPerObstacleType[static_cast<int>(ObstacleType::kPenalty)] = 0;

    killerBaseParams.side = 2 * ballsField.balls.radius;
    killerBaseParams.vel = screenDiag;
    colorPerObstacleType[static_cast<int>(ObstacleType::kKiller)] = 0xBB2222;
    hitPerObstacleType[static_cast<int>(ObstacleType::kKiller)] = 0;
}


// initialize game data in this function
void initialize()
{
    printf("Start initialization\n");

    InitRandomizer();
    InitLevels();
    InitBallsField();
    InitObstacles();

    gameFieldColor = 0x9ACD32;
    score = 0;

    printf("Finish initialization\n");

    PrintScore();
}


pair<double, double>
CalcObstacleVelAngleRange(double x, double y) 
{
    double tmpY = ballsField.ring.centerY - y;
    double tmpX = ballsField.ring.centerX - x;
    
    double angle1 = tmpY / tmpX;
    double angle2 = ballsField.ring.radius / CalcDist(tmpX, tmpY);

    double upperVelAngle = atan(angle1) - asin(angle2);
    double downVelAngle = upperVelAngle + 2 * angle2;

    return {upperVelAngle, downVelAngle};
}


double 
GenerateObstacleVelAngle(double x, double y)
{
    pair<double, double> velRange = CalcObstacleVelAngleRange(x, y);
    double tmp = 180 / kPi;
    int upperVelAngleInDegrees = static_cast<int>(velRange.first * tmp);
    int downVelAngleInDegrees = static_cast<int>(velRange.second * tmp);

    double velAngle = (rand() % (downVelAngleInDegrees - upperVelAngleInDegrees) + upperVelAngleInDegrees) / tmp;

    return velAngle;
}


Obstacle* 
GenerateNewObstacle() 
{
    printf("Start generating new obstacle\n");

    Obstacle *newObstacle = nullptr;
    int curLevelNum = static_cast<int>(curLevel);

    int random = rand();
    if (random % 3 == 0) {
        newObstacle = new Prize();
        (newObstacle -> size).side = prizeSideCoefPerLevel[curLevelNum] * prizeBaseParams.side;
        newObstacle -> vel = obstacleVelCoefPerLevel[curLevelNum] * prizeBaseParams.vel;
    } else {
        if (random % 3 == 1) {
            newObstacle = new Penalty();
            (newObstacle -> size).side = penaltySideCoefPerLevel[curLevelNum] * penaltyBaseParams.side;
            newObstacle -> vel = obstacleVelCoefPerLevel[curLevelNum] * penaltyBaseParams.vel;
        } else {
            newObstacle = new Killer();
            (newObstacle -> size).side = killerSideCoefPerLevel[curLevelNum] * killerBaseParams.side;
            newObstacle -> vel = obstacleVelCoefPerLevel[curLevelNum] * killerBaseParams.vel;
        }
    }

    (newObstacle -> size).centerX = 0;
    (newObstacle -> size).centerY = (rand() % SCREEN_HEIGHT); 

    newObstacle -> velAngle = GenerateObstacleVelAngle((newObstacle -> size).centerX, (newObstacle -> size).centerY);
    
    printf("Finish generating new obstacle\n");

    return newObstacle;
}


void 
ReverseBallsRotation()
{
    printf("Reverse balls rotation\n");

    ballsField.balls.anglVel *= -1;
}


void 
RotateBalls(float dt)
{
    ballsField.balls.angle += ballsField.balls.anglVel * dt; 
    
    if (ballsField.balls.angle > kPi) {
        ballsField.balls.angle -= 2.0 * kPi; 
    }

    if (ballsField.balls.angle <= kPi) {
        ballsField.balls.angle += 2.0 * kPi;
    }
}


void 
DeleteObstacle(const list<Obstacle*>::iterator &deletedObstacleIt) 
{
    printf("Start deleting obstacle\n");

    obstacles.erase(deletedObstacleIt);

    printf("Finish deleting obstacle\n");
}


void 
MoveObstacles(float dt)
{
    auto moveObstacle = [&](Obstacle *obstacle) {
        (obstacle -> size).centerX += (obstacle -> vel) * cos(obstacle -> velAngle) * dt;
        (obstacle -> size).centerY += (obstacle -> vel) * sin(obstacle -> velAngle) * dt;
    };

    for_each(obstacles.begin(), obstacles.end(), moveObstacle);

    if (obstacles.size() > 0) {
        const auto& firstObstacleIt = obstacles.begin();
        Obstacle *firstObstacle = *firstObstacleIt;
        double firstObstacleSide = (firstObstacle -> size).side;
        
        if ((((firstObstacle -> size).centerX) - 0.5 * firstObstacleSide >= SCREEN_WIDTH) ||
            (((firstObstacle -> size).centerY) + 0.5 * firstObstacleSide <= 0.0) || 
            (((firstObstacle -> size).centerY) - 0.5 * firstObstacleSide >= static_cast<double>(SCREEN_HEIGHT))) {
                printf("Obstacle is out of game field\n");
                DeleteObstacle(firstObstacleIt);
        }
    }
}


void 
MoveObjects(float dt) 
{
    printf("Start moving objects\n");

    RotateBalls(dt);
    MoveObstacles(dt);

    printf("Finish moving objects\n");
}


void 
AddPoints(ObstacleType obstacleType) 
{
    score += pointsPerLevelAndObstacleType[static_cast<int>(curLevel)][static_cast<int>(obstacleType)];
}


void 
AddHitObstacles(ObstacleType obstacleType) 
{
    hitPerObstacleType[static_cast<int>(obstacleType)] += 1;
}


bool 
IsScoreNegative() 
{
    return (score < 0);
}


void 
GetBalls(Circle &ball1, Circle &ball2) 
{
    ball1.radius = ballsField.balls.radius;
    ball1.centerX = ballsField.ring.centerX - ballsField.ring.radius * cos(ballsField.balls.angle);
    ball1.centerY = ballsField.ring.centerY + ballsField.ring.radius * sin(ballsField.balls.angle);

    ball2.radius = ballsField.balls.radius;
    ball2.centerX = ballsField.ring.centerX + ballsField.ring.radius * cos(ballsField.balls.angle);
    ball2.centerY = ballsField.ring.centerY - ballsField.ring.radius * sin(ballsField.balls.angle);
}


bool 
AreCircleAndSquareIntersected(Circle &circle, Square &square) 
{
    double dist2;

    double sqCX = square.centerX;
    double sqCY = square.centerY;
    double sqS = square.side;
    double sqSHalf = 0.5 * sqS;

    double cCX = circle.centerX;
    double cCY = circle.centerY;
    double cR = circle.radius;

    if ((sqCX + sqSHalf) < cCX) {
        if ((sqCY + sqSHalf) < cCY) {
            dist2 = CalcDist2(cCX - (sqCX + sqSHalf), cCY - (sqCY + sqSHalf));
        } else {
            if ((sqCY - sqSHalf) > cCY) {
                dist2 = CalcDist2(cCX - (sqCX + sqSHalf), sqCY - sqSHalf - cCY);
            } else {
                dist2 = CalcDist2(cCX - (sqCX + sqSHalf), 0);
            }
        }
    } else {
        if ((sqCX - sqSHalf) > cCX) {
            if ((sqCY + sqSHalf) < cCY) {
                dist2 = CalcDist2(sqCX - sqSHalf - cCX, cCY - (sqCY + sqSHalf));
            } else {
                if ((sqCY - sqSHalf) > cCY) {
                    dist2 = CalcDist2(sqCX - sqSHalf - cCX, sqCY - sqSHalf - cCY);
                } else {
                    dist2 = CalcDist2(sqCX - sqSHalf - cCX, 0);
                }
            }
        } else {
            if ((sqCY + sqSHalf) < (cCY - cR)) {
                return false; 
            } else {
                if ((sqCY - sqSHalf) > (cCY + cR)) {
                    return false;
                } else {
                    return true;
                }
            }
        }
    }

    double cR2 = pow(circle.radius, 2);
    return (dist2 <= cR2);
}


pair<bool, list<Obstacle*>::iterator> 
FindBallsAndObstaclesIntersection() 
{
    printf("Start finding intersection\n");

    Circle ball1, ball2;
    GetBalls(ball1, ball2);

    for (auto obstacleIt = obstacles.begin(); obstacleIt != obstacles.end(); ++obstacleIt) {
        if (AreCircleAndSquareIntersected(ball1, (*obstacleIt)->size) || 
            AreCircleAndSquareIntersected(ball2, (*obstacleIt)->size)) {
            return {true, obstacleIt};
        }
    }

    printf("Finish finding intersection\n");

    return {false, obstacles.end()};
}


bool 
AnalyzeIntersection(const list<Obstacle*>::iterator obstacleIt)
{
    printf("Start analyzing intersection\n");

    bool game_over = false;

    Obstacle *obstacle = *obstacleIt;
    auto obstacleType = obstacle -> GetType();

    if (obstacleType != ObstacleType::kKiller) {
        AddPoints(obstacleType);
        AddHitObstacles(obstacleType);

        DeleteObstacle(obstacleIt);
            
        if (IsScoreNegative()) {
            printf("GAME OVER: score < 0\n");
            game_over = true;
        }
    } else {
        printf("GAME OVER: intersection with killer\n");
        game_over = true;
    }

    printf("Finish analyzing intersection\n");

    return game_over;
}


bool 
GoToNewLevel()
{
    if (curLevel != LEVEL::kHard) {
        if (score == scoreForNextLevel[static_cast<int>(curLevel)]) {
            return true;
        } 
    }

    return false;
}


void 
ChangeLevel()
{
    if (curLevel == LEVEL::kEasy) {
        curLevel = LEVEL::kNormal;
        printf("NEW LEVEL: normal");
    } else {
        curLevel = LEVEL::kHard;
        printf("NEW LEVEL: hard");
    }

    ballsField.balls.anglVel = ballsAnglVelCoefPerLevel[static_cast<int>(curLevel)] * ballsBaseParams.anglVel;
}


// this function is called to update game data,
// dt - time elapsed since the previous update (in seconds)
void act(float dt)
{
    printf("Start act\n");

    static float timeElapsedTillObstacleEmergence = 0.0; // in sec

    if (GoToNewLevel()) {
        ChangeLevel();
    }

    if (is_key_pressed(VK_ESCAPE)) {
        PrintResults();
        schedule_quit_game();
    }
    
    if (is_key_pressed(VK_SPACE)) {
        ReverseBallsRotation();
    }
    
    timeElapsedTillObstacleEmergence += dt;
    if (timeElapsedTillObstacleEmergence >= obstacleEmergenceTimePerLevel[static_cast<int>(curLevel)]) {
        timeElapsedTillObstacleEmergence = 0.0;
        Obstacle *newObstacle = GenerateNewObstacle();
        obstacles.push_back(newObstacle);
    }

    MoveObjects(dt);
    
    pair<bool, list<Obstacle*>::iterator> intersection = FindBallsAndObstaclesIntersection();
    
    if (intersection.first) {
        bool gameOver = AnalyzeIntersection(intersection.second);

        if (!gameOver) {
            PrintScore();
        } else {
            PrintResults();
            schedule_quit_game();
        }
    }

    printf("Finish act\n");
}


void 
drawGameField()
{
    for (unsigned i = 0; i < SCREEN_HEIGHT; ++i) {
        for (unsigned j = 0; j < SCREEN_WIDTH; ++j) {
            buffer[i][j] = gameFieldColor;
        }
    }
}


void 
drawCircle(Circle &circle, uint32_t color)
{
    int cX = static_cast<int>(circle.centerX);
    int cY = static_cast<int>(circle.centerY);
    int R = static_cast<int>(circle.radius);
    int R2 = pow(R, 2);

    int minY = cY - R;
    int maxY = cY + R;

    for (int i = minY; i <= maxY; ++i) {
        int xHalfRange = sqrt(R2 - pow(cY - i, 2));
        int curMinX = cX - xHalfRange;
        int curMaxX = cX + xHalfRange;

        for (int j = curMinX; j <= curMaxX; ++j) {
            buffer[i][j] = color;
        }
    }
}


void 
drawBallsField() 
{
    Circle ball1, ball2;
    GetBalls(ball1, ball2);

    drawCircle(ballsField.ring, ringColor);
    drawCircle(ball1, ballsColor);
    drawCircle(ball2, ballsColor);
}


void 
drawSquare(Square &square, uint32_t color)
{
    int cX = static_cast<int>(square.centerX);
    int cY = static_cast<int>(square.centerY);
    int side = static_cast<int>(square.side);
    int halfSide = 0.5 * side;

    int minX = max(cX - halfSide, 0);
    int maxX = min(cX + halfSide, SCREEN_WIDTH);

    int minY = max(cY - halfSide, 0);
    int maxY = min(cY + halfSide, SCREEN_HEIGHT);

    for (int i = minY; i <= maxY; ++i) {
        for (int j = minX; j <= maxX; ++j) {
            buffer[i][j] = color;
        }
    }
}


void 
drawObstacle(Obstacle *obstacle) 
{
    drawSquare(obstacle -> size, colorPerObstacleType[static_cast<int>(obstacle -> GetType())]);
}


void 
drawObstacles()
{
    for (const auto &obstacle : obstacles) {
        drawObstacle(obstacle);
    }
}


// fill buffer in this function
// uint32_t buffer[SCREEN_HEIGHT][SCREEN_WIDTH] - is an array of 32-bit colors (8 bits per R, G, B)
void draw()
{
    printf("Start draw\n");

    // clear backbuffer
    memset(buffer, 0, SCREEN_HEIGHT * SCREEN_WIDTH * sizeof(uint32_t)); 
    
    drawGameField();
    drawBallsField();
    drawObstacles();

    printf("Finish draw\n");
}

// free game data in this function
void finalize()
{
    printf("Start finalize\n");

    for (auto& obstacle : obstacles) {
        delete obstacle;
    }

    printf("Finish finalize\n");
}