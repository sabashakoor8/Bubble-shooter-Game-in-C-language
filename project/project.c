#include <windows.h>
#include <ansi_c.h>
#include <utility.h>
#include <cvirte.h>
#include <userint.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include "project.h"

#define WIDTH 400
#define HEIGHT 300
#define NUM_BUBBLES 112
#define MAX_RADIUS 50
#define SHOOTER_BUBBLE_SPEED 10
#define M_PI 3.14159265358979323846
#define FPS_LIMIT 120
#define FRAME_DELAY (1000 / FPS_LIMIT)

static int panelHandle, display;
CmtThreadLockHandle mlock;
CmtThreadFunctionID moveShooterThreadID;
int shooterBubbleMoving = 0;
double shooterBubbleDirectionX, shooterBubbleDirectionY;
int mouseX, mouseY;
int hardMode = 0;
double currentScore = 0.0;
int consecutiveMisses = 0;

typedef struct {
    double interval;
    int control;
    int flag;
    int color;
    int top;
    int left;
    int x;
    int y;
    int dx;
    int dy;
    int lock;
    int radius;
    int ispopped;
    CmtThreadFunctionID threadid;
} Bubble;

Bubble bubbles[NUM_BUBBLES];
Bubble shooterBubble, nextBubble;

// Function prototypes
int SaveScoreToFile(void *functionData);
int SendEmailWithScore(void *functionData);
void InitializeBubbles(void);
void DrawBubbles(int panelHandle);
void ShootShooterBubble(int x, int y);
int MoveShooterBubble(void *functionData);
void UpdateArrowDirection(void);
void GenerateNextBubble(void);
void PopBubbleCluster(int bubbleIndex);
void AddNewRowOfBubbles(void);
void ResetShooterAndNextBubble(void);
void DrawArrow(int panelHandle);
int CheckCluster(int bubbleIndex);
void AttachBubble(int bubbleIndex);

int main(int argc, char *argv[])
{
    if (InitCVIRTE(0, argv, 0) == 0)
        return -1;
    if ((panelHandle = LoadPanel(0, "project.uir", MAIN)) < 0)
        return -1;
    if ((display = LoadPanel(0, "project.uir", DISPLAY)) < 0)
        return -1;
    if (CmtNewLock(NULL, 0, &mlock) != 0)
        return -1;

    DisplayPanel(panelHandle);
    RunUserInterface();

    DiscardPanel(panelHandle);
    DiscardPanel(display);
    CmtDiscardLock(mlock);
    return 0;
}

int CVICALLBACK QuitCallback(int panel, int control, int event,
                             void *callbackData, int eventData1, int eventData2)
{
    switch (event)
    {
    case EVENT_COMMIT:
        QuitUserInterface(0);
        break;
    }
    return 0;
}

int CVICALLBACK startgamefunc(int panel, int control, int event,
                              void *callbackData, int eventData1, int eventData2)
{
    switch (event)
    {
    case EVENT_COMMIT:
        DisplayPanel(display);
        InitializeBubbles();
        GenerateNextBubble();
        DrawBubbles(display);
        break;
    }
    return 0;
}

void InitializeBubbles(void)
{
    int bubbleRadius = MAX_RADIUS / 4;
    int cols = 16;
    int rows = 7;
    int bubbleCount = 0;

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            bubbles[bubbleCount].x = j * (bubbleRadius * 2) + bubbleRadius;
            bubbles[bubbleCount].y = i * (bubbleRadius * 2) + bubbleRadius;
            bubbles[bubbleCount].radius = bubbleRadius;

            int colorChoice = rand() % 5;
            switch (colorChoice)
            {
            case 0:
                bubbles[bubbleCount].color = VAL_RED;
                break;
            case 1:
                bubbles[bubbleCount].color = VAL_GREEN;
                break;
            case 2:
                bubbles[bubbleCount].color = VAL_YELLOW;
                break;
            case 3:
                bubbles[bubbleCount].color = VAL_WHITE;
                break;
            case 4:
                bubbles[bubbleCount].color = VAL_BLUE;
                break;
            }

            bubbles[bubbleCount].dx = 0;
            bubbles[bubbleCount].dy = 0;
            bubbles[bubbleCount].ispopped = 0;
            bubbleCount++;
        }
    }

    shooterBubble.radius = bubbleRadius;
    shooterBubble.x = WIDTH / 2;
    shooterBubble.y = HEIGHT - shooterBubble.radius;
    GenerateNextBubble();
}

void GenerateNextBubble(void)
{
    int bubbleRadius = MAX_RADIUS / 4;
    nextBubble.radius = bubbleRadius;
    nextBubble.x = WIDTH / 2;
    nextBubble.y = HEIGHT - (bubbleRadius * 4);

    int colorChoice = rand() % 5;
    switch (colorChoice)
    {
    case 0:
        nextBubble.color = VAL_RED;
        break;
    case 1:
        nextBubble.color = VAL_GREEN;
        break;
    case 2:
        nextBubble.color = VAL_YELLOW;
        break;
    case 3:
        nextBubble.color = VAL_WHITE;
        break;
    case 4:
        nextBubble.color = VAL_BLUE;
        break;
    }

    shooterBubble.color = nextBubble.color;
}

void DrawBubbles(int panelHandle)
{
    CanvasClear(panelHandle, DISPLAY_CANVAS, VAL_ENTIRE_OBJECT);

    for (int i = 0; i < NUM_BUBBLES; i++)
    {
        if (!bubbles[i].ispopped)
        {
            int left = bubbles[i].x - bubbles[i].radius;
            int top = bubbles[i].y - bubbles[i].radius;
            int right = bubbles[i].x + bubbles[i].radius;
            int bottom = bubbles[i].y + bubbles[i].radius;

            SetCtrlAttribute(panelHandle, DISPLAY_CANVAS, ATTR_PEN_FILL_COLOR, bubbles[i].color);
            CanvasDrawOval(panelHandle, DISPLAY_CANVAS, MakeRect(top, left, bottom - top, right - left), VAL_DRAW_FRAME_AND_INTERIOR);
        }
    }

    // Draw shooter bubble
    int left = shooterBubble.x - shooterBubble.radius;
    int top = shooterBubble.y - shooterBubble.radius;
    int right = shooterBubble.x + shooterBubble.radius;
    int bottom = shooterBubble.y + shooterBubble.radius;

    SetCtrlAttribute(panelHandle, DISPLAY_CANVAS, ATTR_PEN_FILL_COLOR, shooterBubble.color);
    CanvasDrawOval(panelHandle, DISPLAY_CANVAS, MakeRect(top, left, bottom - top, right - left), VAL_DRAW_FRAME_AND_INTERIOR);

    // Draw next bubble
    left = nextBubble.x - nextBubble.radius;
    top = nextBubble.y - nextBubble.radius;
    right = nextBubble.x + nextBubble.radius;
    bottom = nextBubble.y + nextBubble.radius;

    SetCtrlAttribute(panelHandle, DISPLAY_CANVAS, ATTR_PEN_FILL_COLOR, nextBubble.color);
    CanvasDrawOval(panelHandle, DISPLAY_CANVAS, MakeRect(top, left, bottom - top, right - left), VAL_DRAW_FRAME_AND_INTERIOR);

    // Draw the aiming arrow
    DrawArrow(panelHandle);
}

void DrawArrow(int panelHandle) {
    if (!shooterBubbleMoving && !hardMode) {
        int arrowLength = 50;
        int arrowEndX = shooterBubble.x + (int)(shooterBubbleDirectionX * arrowLength);
        int arrowEndY = shooterBubble.y + (int)(shooterBubbleDirectionY * arrowLength);

        SetCtrlAttribute(panelHandle, DISPLAY_CANVAS, ATTR_PEN_COLOR, VAL_BLACK);
        CanvasDrawLine(panelHandle, DISPLAY_CANVAS, MakePoint(shooterBubble.x, shooterBubble.y), MakePoint(arrowEndX, arrowEndY));
    }
}

void ShootShooterBubble(int x, int y)
{
    if (shooterBubbleMoving)
        return;

    shooterBubbleMoving = 1;
    double directionX = x - shooterBubble.x;
    double directionY = y - shooterBubble.y;
    double magnitude = sqrt(directionX * directionX + directionY * directionY);
    shooterBubbleDirectionX = SHOOTER_BUBBLE_SPEED * (directionX / magnitude);
    shooterBubbleDirectionY = SHOOTER_BUBBLE_SPEED * (directionY / magnitude);

    CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, MoveShooterBubble, NULL, NULL);
}

int MoveShooterBubble(void *functionData)
{
    while (shooterBubbleMoving)
    {
        CmtGetLock(mlock);
        shooterBubble.x += (int)shooterBubbleDirectionX;
        shooterBubble.y += (int)shooterBubbleDirectionY;

        for (int i = 0; i < NUM_BUBBLES; i++)
        {
            if (!bubbles[i].ispopped)
            {
                int dx = shooterBubble.x - bubbles[i].x;
                int dy = shooterBubble.y - bubbles[i].y;
                int distanceSquared = dx * dx + dy * dy;
                int radiusSum = shooterBubble.radius + bubbles[i].radius;

                if (distanceSquared <= radiusSum * radiusSum)
                {
                    shooterBubbleMoving = 0;
                    
                    // Check if we can pop a cluster
                    if (CheckCluster(i)) {
                        PopBubbleCluster(i);
                    } else {
                        // Attach the bubble if no cluster is formed
                        AttachBubble(i);
                    }

                    CmtReleaseLock(mlock);
                    ResetShooterAndNextBubble();
                    DrawBubbles(display);
                    return 0;
                }
            }
        }

        // Handle boundaries
        if (shooterBubble.x - shooterBubble.radius < 0 || shooterBubble.x + shooterBubble.radius > WIDTH || shooterBubble.y - shooterBubble.radius < 0 || shooterBubble.y + shooterBubble.radius > HEIGHT)
        {
            shooterBubbleMoving = 0;
            consecutiveMisses++;
            if (consecutiveMisses >= 5) {
                AddNewRowOfBubbles();
                consecutiveMisses = 0;
            }
            CmtReleaseLock(mlock);
            ResetShooterAndNextBubble();
            DrawBubbles(display);
            return 0;
        }

        DrawBubbles(display);
        CmtReleaseLock(mlock);
        ProcessSystemEvents();
        Sleep(FRAME_DELAY);
    }
    return 0;
}

int CheckCluster(int bubbleIndex) {
    int color = bubbles[bubbleIndex].color;
    int queue[NUM_BUBBLES];
    int queueSize = 0;  // Initialize the queue size
    int clusterSize = 0;

    queue[queueSize++] = bubbleIndex;

    while (queueSize > 0) {
        int currentIndex = queue[--queueSize];

        if (!bubbles[currentIndex].ispopped && bubbles[currentIndex].color == color) {
            bubbles[currentIndex].ispopped = 1;
            clusterSize++;

            for (int i = 0; i < NUM_BUBBLES; i++) {
                if (!bubbles[i].ispopped && bubbles[i].color == color) {
                    int dx = bubbles[i].x - bubbles[currentIndex].x;
                    int dy = bubbles[i].y - bubbles[currentIndex].y;
                    int distanceSquared = dx * dx + dy * dy;
                    int radiusSum = bubbles[i].radius + bubbles[currentIndex].radius;

                    if (distanceSquared <= radiusSum * radiusSum) {
                        queue[queueSize++] = i;
                    }
                }
            }
        }
    }

    // Restore popped state to not affect the original bubbles
    for (int i = 0; i < NUM_BUBBLES; i++) {
        if (bubbles[i].ispopped && bubbles[i].color == color) {
            bubbles[i].ispopped = 0;
        }
    }

    // Return true if we have a cluster of 3 or more
    return clusterSize >= 3;
}

void PopBubbleCluster(int bubbleIndex) {
    int color = bubbles[bubbleIndex].color;
    int queue[NUM_BUBBLES];
    int queueSize = 0;

    queue[queueSize++] = bubbleIndex;
    int poppedBubbles = 0;

    while (queueSize > 0) {
        int currentIndex = queue[--queueSize];

        if (!bubbles[currentIndex].ispopped && bubbles[currentIndex].color == color) {
            bubbles[currentIndex].ispopped = 1;
            poppedBubbles++;

            for (int i = 0; i < NUM_BUBBLES; i++) {
                if (!bubbles[i].ispopped && bubbles[i].color == color) {
                    int dx = bubbles[i].x - bubbles[currentIndex].x;
                    int dy = bubbles[i].y - bubbles[currentIndex].y;
                    int distanceSquared = dx * dx + dy * dy;
                    int radiusSum = bubbles[i].radius + bubbles[currentIndex].radius;

                    if (distanceSquared <= radiusSum * radiusSum) {
                        queue[queueSize++] = i;
                    }
                }
            }
        }
    }

    // Add score
    currentScore += poppedBubbles * 10;
    SetCtrlVal(display, DISPLAY_SCORE, currentScore);
}

void AttachBubble(int bubbleIndex) {
    bubbles[bubbleIndex].color = shooterBubble.color;
    bubbles[bubbleIndex].ispopped = 0;  // Ensure it's not popped
}

void AddNewRowOfBubbles(void) {
    int bubbleRadius = MAX_RADIUS / 4;
    for (int j = 0; j < 16; j++) {
        for (int i = NUM_BUBBLES - 1; i >= 16; i--) {
            bubbles[i] = bubbles[i - 16];
            bubbles[i].y += bubbleRadius * 2;
        }
        int colorChoice = rand() % 5;
        switch (colorChoice)
        {
        case 0:
            bubbles[j].color = VAL_RED;
            break;
        case 1:
            bubbles[j].color = VAL_GREEN;
            break;
        case 2:
            bubbles[j].color = VAL_YELLOW;
            break;
        case 3:
            bubbles[j].color = VAL_WHITE;
            break;
        case 4:
            bubbles[j].color = VAL_BLUE;
            break;
        }
        bubbles[j].x = j * (bubbleRadius * 2) + bubbleRadius;
        bubbles[j].y = bubbleRadius;
        bubbles[j].radius = bubbleRadius;
        bubbles[j].ispopped = 0;
    }
    DrawBubbles(display);
}

void ResetShooterAndNextBubble(void) {
    shooterBubble.color = nextBubble.color;
    shooterBubble.x = WIDTH / 2;
    shooterBubble.y = HEIGHT - shooterBubble.radius;

    GenerateNextBubble();
}

int CVICALLBACK canvafunction(int panel, int control, int event,
                              void *callbackData, int eventData1, int eventData2)
{
    if (event == EVENT_LEFT_CLICK)
    {
        int x = eventData1;
        int y = eventData2;
        ShootShooterBubble(x, y);
    }
    else if (event == EVENT_MOUSE_POINTER_MOVE)
    {
        mouseX = eventData1;
        mouseY = eventData2;
        UpdateArrowDirection();
        DrawBubbles(display);
    }
    return 0;
}

int CVICALLBACK SAVEFILEFUNC(int panel, int control, int event,
                             void *callbackData, int eventData1, int eventData2)
{
    if (event == EVENT_COMMIT)
    {
        CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, SaveScoreToFile, NULL, NULL);
    }
    return 0;
}

int SaveScoreToFile(void *functionData)
{
    FILE *scoreFile = fopen("scores.txt", "a");
    if (scoreFile)
    {
        fprintf(scoreFile, "Score: %.2f\n", currentScore);
        fclose(scoreFile);
    }
    return 0;
}

int CVICALLBACK restartfunc(int panel, int control, int event,
                            void *callbackData, int eventData1, int eventData2)
{
    if (event == EVENT_COMMIT) {
        currentScore = 0;
        consecutiveMisses = 0;
        SetCtrlVal(display, DISPLAY_SCORE, currentScore);
        InitializeBubbles();
        DrawBubbles(display);
    }
    return 0;
}
int CVICALLBACK MoveBalloon(int panel, int control, int event, void *callbackData, int eventData1, int eventData2) {
    if (event == EVENT_TIMER_TICK) {
        // Example of moving the shooter bubble in some direction or updating game state
        // You can include logic to move the bubbles downwards, or manage the shooter bubble state

        // For now, we'll just clear the panel and redraw bubbles
        DrawBubbles(display);

        // You can add other logic here if the `MoveBalloon` function is responsible for more events
    }
    return 0;
}

int CVICALLBACK ToggleDifficulty(int panel, int control, int event,
                                 void *callbackData, int eventData1, int eventData2)
{
    if (event == EVENT_COMMIT)
    {
        GetCtrlVal(panel, control, &hardMode);
    }
    return 0;
}

int CVICALLBACK SendEmail(int panel, int control, int event,
                          void *callbackData, int eventData1, int eventData2)
{
    if (event == EVENT_COMMIT) {
        CmtScheduleThreadPoolFunction(DEFAULT_THREAD_POOL_HANDLE, SendEmailWithScore, NULL, NULL);
    }
    return 0;
}

int SendEmailWithScore(void *functionData)
{
    char command[256];
    sprintf(command, "echo 'Score: %.2f' | mail -s 'Your Score' user@example.com", currentScore);
    system(command);
    return 0;
}

void UpdateArrowDirection(void)
{
    if (hardMode)
    {
        shooterBubbleDirectionX = (rand() % 3 - 1) * SHOOTER_BUBBLE_SPEED;
        shooterBubbleDirectionY = (rand() % 3 - 1) * SHOOTER_BUBBLE_SPEED;
    }
    else
    {
        double directionX = mouseX - shooterBubble.x;
        double directionY = mouseY - shooterBubble.y;
        double magnitude = sqrt(directionX * directionX + directionY * directionY);
        shooterBubbleDirectionX = SHOOTER_BUBBLE_SPEED * (directionX / magnitude);
        shooterBubbleDirectionY = SHOOTER_BUBBLE_SPEED * (directionY / magnitude);
    }
}

