#include <stdio.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include "..\Include\SDL2\SDL.h"
#include "..\Include\SDL2\SDL_ttf.h"

//COLORS
#define WHITE   255,255,255,255
#define BLACK   0,0,0,255
#define GREY    80,80,80,255

#define PADDLE_STATE_NONE 0
#define PADDLE_STATE_UP 1
#define PADDLE_STATE_DOWN 2

#define TRUE 1
#define FALSE 0

typedef struct Speed{
    int x;
    int y;
} Speed;

typedef struct Pos{
    int x;
    int y;
} Pos;

uint8_t CheckCollision(SDL_Rect a, SDL_Rect b)
{
    int aP1 = a.x;
    int aP2 = a.x + a.w;
    int aP3 = a.y; 
    int aP4 = a.y + a.h;

    int bP1 = b.x;
    int bP2 = b.x + b.w;
    int bP3 = b.y;
    int bP4 = b.y + b.h ;

    if(aP1 <= bP2 && aP2 >= bP1 && aP4 >= bP3 && aP3 <= bP4)
    {
        return 1;
    }
    return 0;
}

int ResetData(SDL_Rect *ball, Speed *currentBallSpeed, float *ballBoost);

void IncreaseScore(int *score, char * scoreText);

int RenderText(TTF_Font *font,char* text, SDL_Color textColor, SDL_Renderer *renderer,
        SDL_Texture *fontTexture, int x, int y);

int main(int argc, char **argv)
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    SDL_Rect paddle01;
    SDL_Rect paddle02;
    SDL_Rect ball;

    Speed ballSpeed = {5, 5};

    Speed currentBallSpeed = {
        ballSpeed.x, 
        0};

    float paddleSpeed = 10;
    float currentPaddleSpeed01 = paddleSpeed;
    float currentPaddleSpeed02 = paddleSpeed;
    
    int scoreP1 = 0;
    char scoreP1Text[2] = "0";

    int scoreP2= 0;
    char scoreP2Text[2] = "0";

    uint8_t paddle01State = PADDLE_STATE_NONE;
    uint8_t paddle02State = PADDLE_STATE_NONE;

    SDL_Event event;                        

    uint8_t isRunning = 1;      

    int startFrameTime = 0;
    int endFrameTime = 0;
    int deltaTime = 0;
    float ballBoost = 1;

    printf("Pong Initialized!\n");
    printf("With TTF...");

    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("Unable to initialize SDL!\n");
        return 1;
    }

    //Creates window...
    window = SDL_CreateWindow("PONG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);

    if(window == NULL)
    {
        printf("Unable to initialize window!\n");
        return 1;
    }

    //Creates renderer...
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if(renderer == NULL)
    {
        printf("Unable to initialize renderer!\n");
        return 1;
    }

     //------------------------------------------------------------------
     //                             FONT
     //------------------------------------------------------------------

     if(TTF_Init() < 0)
    {
        printf("Unable to initialize TTF!\n");
        return 1;
    }

    TTF_Font *font =  TTF_OpenFont("../Resources/ARCADECLASSIC.TTF", 59);

    if(font == NULL)
    {
        printf("Unable to initialize the font!\n");
        return 1;
    }

    SDL_Color fontColor = {WHITE};
    SDL_Texture *fontTexture = NULL;

    //------------------------------------------------------------------

    paddle01.w = 16; 
    paddle01.h = 128;
    paddle01.x = 100;
    paddle01.y = (600 * 0.5) - (128 * 0.5);

    paddle02.w = 16; 
    paddle02.h = 128;
    paddle02.x = 800 - 100;
    paddle02.y = (600 * 0.5) - (128 * 0.5);

    ball.w = 16;
    ball.h = 16;
    ball.x = 800 * 0.5;
    ball.y = (600 * 0.5) - (128 * 0.5);
   
    while(isRunning)
    {
        startFrameTime = SDL_GetTicks();

        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                isRunning = 0; 
                SDL_Quit();
            }

            if(event.type == SDL_KEYDOWN)
            {
                if(event.key.keysym.sym == SDLK_w)
                {
                    paddle01State = PADDLE_STATE_UP;
                } 

                if(event.key.keysym.sym == SDLK_s)
                {
                    paddle01State = PADDLE_STATE_DOWN;
                }

                if(event.key.keysym.sym == SDLK_UP)
                {
                    paddle02State = PADDLE_STATE_UP;
                }

                if(event.key.keysym.sym == SDLK_DOWN)
                {
                    paddle02State = PADDLE_STATE_DOWN;
                }
            } 
            else if(event.type ==SDL_KEYUP)
            {
                if(event.key.keysym.sym == SDLK_w || event.key.keysym.sym == SDLK_s)
                {
                    paddle01State = PADDLE_STATE_NONE;
                }

                if(event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN)
                {
                    paddle02State = PADDLE_STATE_NONE;
                }
            }
        }

        if(paddle01State == PADDLE_STATE_UP) 
        {
            if(paddle01.y > 0)
                paddle01.y -= paddleSpeed; 
        }
        else if (paddle01State == PADDLE_STATE_DOWN)
        {
            if(paddle01.y < 600 - paddle01.h)
                paddle01.y += paddleSpeed; 
        }

        if(paddle02State == PADDLE_STATE_UP)
        {
            if(paddle02.y > 0)
                paddle02.y -= paddleSpeed;
        }
        else if (paddle02State == PADDLE_STATE_DOWN)
        {
            if(paddle02.y < 600 - paddle02.h)
                paddle02.y += paddleSpeed; 
        }

        //Calculate upper bound and lower bounds of the screen...
        if(ball.y < 0) 
        {
            currentBallSpeed.y = ballSpeed.y;
        } 
        else if(ball.y + ball.h > 600)
        {
            currentBallSpeed.y = -ballSpeed.y;
        }

        if(ball.x < 0)
        {
            if(ResetData(&ball, &currentBallSpeed, &ballBoost) == FALSE)
            {
                printf("Unable to reset data...\n");
            }
            IncreaseScore(&scoreP2, scoreP2Text);
        } 
        if ( ball.x + ball.w > 800)
        {
            if(ResetData(&ball, &currentBallSpeed, &ballBoost) == FALSE)
            {
                printf("Unable to reset data...\n");
            }
            IncreaseScore(&scoreP1, scoreP1Text);
        }

        if(CheckCollision(ball,paddle02)) 
        {
            ballBoost +=0.1 ;
            currentBallSpeed.x = -ballSpeed.x - ballBoost;

            if(ball.y > paddle02.y && ball.y < paddle02.y + (paddle02.h * 0.5))
            {
                currentBallSpeed.y = -ballSpeed.y - ballBoost; 
            }else 
            {
                currentBallSpeed.y = ballSpeed.y + ballBoost;
            }
        }

        if(CheckCollision(ball, paddle01) == 1)
        {

            ballBoost += 0.1;
            currentBallSpeed.x = ballSpeed.x + ballBoost;

            if(ball.y > paddle01.y && ball.y < paddle01.y + (paddle01.h * 0.5))
            {
                currentBallSpeed.y = -ballSpeed.y - ballBoost; 
            }else 
            {
                currentBallSpeed.y = ballSpeed.y + ballBoost;
            }
        }

        ball.x += currentBallSpeed.x;
        ball.y += currentBallSpeed.y;

        //Render...
        SDL_SetRenderDrawColor(renderer, GREY);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, WHITE);

        RenderText(font, scoreP1Text, fontColor, renderer, fontTexture, 200, 50);
        RenderText(font, scoreP2Text, fontColor, renderer, fontTexture, 800-200, 50);

        SDL_RenderFillRect(renderer, &paddle01);
        SDL_RenderFillRect(renderer, &paddle02);

        SDL_SetRenderDrawColor(renderer, 255,0,0,1);

        SDL_RenderFillRect(renderer, &ball);

        SDL_RenderPresent(renderer);

        endFrameTime = SDL_GetTicks();

        deltaTime = endFrameTime - startFrameTime;

        if(deltaTime < 16.66)
        {
            SDL_Delay(16.66 - deltaTime); 
        }
    }

    //Clean up...
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    SDL_DestroyTexture(fontTexture);
    return 0;
}

void IncreaseScore(int *score, char * scoreText)
{
    *score += 1;

    if(*score > 5)
    {
        printf("Somebody wins!\n");
        return;
    }

    scoreText[0] = 48 + (*score);
}


int RenderText(TTF_Font *font,char* text, SDL_Color textColor, SDL_Renderer *renderer, 
        SDL_Texture *fontTexture, int x, int y)
{
    SDL_Color fontColor = {WHITE};

    SDL_Surface *fontSurface = TTF_RenderText_Blended(font, text, fontColor);
    if(fontSurface == NULL)
    {
        printf("Font surface initialization error!\n");
        return FALSE;
    }

    fontTexture = SDL_CreateTextureFromSurface(renderer, fontSurface);
    if(fontTexture == NULL)
    {
        printf("Font texture initialization error!\n");
        return FALSE;
    }

    SDL_Rect fontRect = { x, y, fontSurface->w, fontSurface->h};

    SDL_FreeSurface(fontSurface);

    SDL_RenderCopy(renderer, fontTexture, NULL, &fontRect);

    return TRUE;
}

int ResetData(SDL_Rect *ball, Speed *currentBallSpeed, float *ballBoost)
{
    ball->x = 800 * 0.5 - ball->w * 0.5;
    ball->y = 600 * 0.5 - ball->h * 0.5;

    currentBallSpeed->y = 0;
    currentBallSpeed->x = 5;

    (*ballBoost) = 1;
}
