#include <stdint.h>
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

//GAME STATES...
#define MENU         0b1 
#define GET_READY   0b10 
#define IN_GAME    0b100 
#define RESULT    0b1000 

//PLAYER STATES...
#define PLAYERS_NOT_READY   0b0
#define PLAYER_1_READY      0b1
#define PLAYER_2_READY      0b10

//SCREEN RESOLUTION...
#define SCREEN_RES_HEIGHT       600
#define SCREEN_RES_WIDTH        800

//PADDLE DIMENSIONS...
#define PADDLE_WIDTH            16 
#define PADDLE_HEIGHT           128 

//PADDLE POSITION...
#define PADDLE01_DEFAULT_X_POS  100 - (PADDLE_WIDTH * 0.5) 
#define PADDLE01_DEFAULT_Y_POS  (SCREEN_RES_HEIGHT * 0.5) - (PADDLE_HEIGHT * 0.5)

#define PADDLE02_DEFAULT_X_POS  SCREEN_RES_WIDTH - 100 - (PADDLE_WIDTH * 0.5)
#define PADDLE02_DEFAULT_Y_POS  (SCREEN_RES_HEIGHT * 0.5) - (PADDLE_HEIGHT * 0.5)

typedef struct Speed{
    float x;
    float y;
} Speed;

typedef struct Pos{
    int x;
    int y;
} Pos;

typedef struct TextObject{
    SDL_Texture * texture;
    char * text;
    SDL_Rect rect;
}TextObject;

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

int ResetData(SDL_Rect *ball, Speed *currentBallSpeed, float *ballBoost, char *playerState, 
        SDL_Rect *paddle01, SDL_Rect *paddle02);

void IncreaseScore(int *score, char * scoreText);

int InitializeText(TTF_Font *font,char* text, SDL_Color textColor, SDL_Renderer *renderer,
        SDL_Texture **fontTexture, int x, int y, SDL_Rect *fontRect);

void HandleGameStateMenu(char * gameState);

void HandleReadyState(char * gameState, char playerState);

int CreateFont(TTF_Font **font, unsigned int fontSize);

int main(int argc, char **argv)
{
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    SDL_Rect paddle01;
    SDL_Rect paddle02;
    SDL_Rect ball;

    Speed ballSpeed = {350, 150};

    Speed currentBallSpeed = {
        ballSpeed.x, 
        0};

    char gameState = MENU; //initialize to MENU game state...
     //initialize to MENU game state...

    char playerState = 0; 

    float paddleSpeed = 350;
    float ballBoost = 1;

    uint8_t paddle01State = PADDLE_STATE_NONE;
    uint8_t paddle02State = PADDLE_STATE_NONE;

    SDL_Event event;                        

    uint8_t isRunning = 1;      

    const double targetFPS = 16.66;
    int frameCounter = 0;
    double fpsCollector = 0;
    uint32_t startFrameTime = 0;
    uint32_t currentFrameTime = 0;
    uint32_t frameTime = 0;
    float deltaTime = 0;

    printf("Pong Initialized!\n");

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
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

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

    TTF_Font *fontMedium =  NULL;
    TTF_Font *fontSmall =  NULL;

    if(CreateFont(&fontMedium, 30) == FALSE)
    {
        return 1;
    }

    if(CreateFont(&fontSmall, 15) == FALSE)
    {
        return 1;
    }

    SDL_Color fontColor = {WHITE};

    TextObject p1ReadyText = {NULL, "Press 'W'", {0}};

    InitializeText(fontSmall, p1ReadyText.text, fontColor, renderer, &p1ReadyText.texture, 800 * 0.25, 500, &p1ReadyText.rect);

    TextObject p2ReadyText = {NULL, "Press 'UP'", {0}};

    InitializeText(fontSmall, p2ReadyText.text, fontColor, renderer, &p2ReadyText.texture, 800 * 0.75, 500, &p2ReadyText.rect);

    TextObject p1ReadyPromptText = {NULL, "Ready?", {0}};

    InitializeText(fontSmall, p1ReadyPromptText.text, fontColor, renderer, &p1ReadyPromptText.texture, 800 * 0.25, 450, &p1ReadyPromptText.rect);

    TextObject p2ReadyPromptText = {NULL, "Ready?", {0}};

    InitializeText(fontSmall, p2ReadyPromptText.text, fontColor, renderer, &p2ReadyPromptText.texture, 800 * 0.75, 450, &p2ReadyPromptText.rect);

    SDL_Texture *scoreP1FontTexture = NULL;
    SDL_Rect scoreP1FontRect = {0};
    int scoreP1 = 0;
    char scoreP1Text[2] = "0";

    InitializeText(fontMedium, scoreP1Text, fontColor, renderer, &scoreP1FontTexture, 200, 50, &scoreP1FontRect);

    SDL_Texture *scoreP2FontTexture = NULL;
    SDL_Rect scoreP2FontRect = {0};
    int scoreP2= 0;
    char scoreP2Text[2] = "0";

    InitializeText(fontMedium, scoreP2Text, fontColor, renderer, &scoreP2FontTexture, 800-200, 50, &scoreP2FontRect);

    SDL_Texture *fpsFontTexture = NULL;
    SDL_Rect fpsFontRect = {0};
    double fps = 0;
    char fpsText[1000] = "0";

    //InitializeText(font, fpsText, fontColor, renderer, &fpsFontTexture, 500, 100, &fpsFontRect);

    //------------------------------------------------------------------

    paddle01.w = PADDLE_WIDTH; 
    paddle01.h = PADDLE_HEIGHT;
    paddle01.x = PADDLE01_DEFAULT_X_POS; 
    paddle01.y = PADDLE01_DEFAULT_Y_POS;

    paddle02.w = PADDLE_WIDTH; 
    paddle02.h = PADDLE_HEIGHT;
    paddle02.x = PADDLE02_DEFAULT_X_POS; 
    paddle02.y = PADDLE02_DEFAULT_Y_POS; 

    ball.w = 16;
    ball.h = 16;
    ball.x = 800 * 0.5;
    ball.y = (600 * 0.5) - (128 * 0.5);
   
    startFrameTime = SDL_GetTicks();
    while(isRunning)
    {

        currentFrameTime = SDL_GetTicks();
        /*-----------------------------------------------------------------------------------
        INPUT...
        --------------------------------------------------------------------------------------*/
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
                    if(gameState == GET_READY)
                        playerState |= PLAYER_1_READY;

                    paddle01State = PADDLE_STATE_UP;
                } 

                if(event.key.keysym.sym == SDLK_s)
                {
                    paddle01State = PADDLE_STATE_DOWN;
                }

                if(event.key.keysym.sym == SDLK_UP)
                {
                    if(gameState == GET_READY)
                        playerState |= PLAYER_2_READY;

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
        /*----------------------------------------------------------------------------------------
        GAME-LOGIC     
        ----------------------------------------------------------------------------------------*/

        //Handle Menu State...
        HandleGameStateMenu(&gameState); 

        //Handle Ready State...
        HandleReadyState(&gameState, playerState);

        if(gameState != GET_READY)
        { 
            //Player 01 Paddle movement...
            if(paddle01State == PADDLE_STATE_UP) 
            {
                if(paddle01.y > 0)
                    paddle01.y -= paddleSpeed * deltaTime; 
            }
            else if (paddle01State == PADDLE_STATE_DOWN)
            {
                if(paddle01.y < 600 - paddle01.h)
                    paddle01.y += paddleSpeed * deltaTime; 
            }

            if(paddle02State == PADDLE_STATE_UP)
            {
                if(paddle02.y > 0)
                    paddle02.y -= paddleSpeed * deltaTime;
            }
            else if (paddle02State == PADDLE_STATE_DOWN)
            {
                if(paddle02.y < 600 - paddle02.h)
                    paddle02.y += paddleSpeed * deltaTime; 
            }

            if(ball.y < 0) 
            {
                currentBallSpeed.y = ballSpeed.y;
            } 
            else if(ball.y + ball.h > 600)
            {
                currentBallSpeed.y = -ballSpeed.y;
            }

            //Increase score...
            if(ball.x < 0)
            {
                if(ResetData(&ball, &currentBallSpeed, &ballBoost, &playerState, &paddle01, &paddle02) 
                        == FALSE)
                {
                    printf("Unable to reset data...\n");
                }

                IncreaseScore(&scoreP2, scoreP2Text);

                InitializeText(fontMedium, scoreP2Text, fontColor, renderer, &scoreP2FontTexture, 800-200, 50, &scoreP2FontRect);
            } 

            if ( ball.x + ball.w > 800)
            {
                if(ResetData(&ball, &currentBallSpeed, &ballBoost, &playerState, &paddle01, &paddle02) 
                        == FALSE)
                {
                    printf("Unable to reset data...\n");
                }
                IncreaseScore(&scoreP1, scoreP1Text);

                InitializeText(fontMedium, scoreP1Text, fontColor, renderer, &scoreP1FontTexture, 200, 50, &scoreP1FontRect);
            }

            if(CheckCollision(ball,paddle02)) 
            {
                ballBoost +=0.1 ;
                currentBallSpeed.x = -ballSpeed.x - ballBoost;

                //Bounce ball diagonally...
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

            ball.x += currentBallSpeed.x * deltaTime;
            ball.y += currentBallSpeed.y * deltaTime;
        }

        /*-----------------------------------------------------------------------------------------
        //RENDER LOOP...
        ----------------------------------------------------------------------------------------- */
         
        SDL_SetRenderDrawColor(renderer, GREY);
        SDL_RenderClear(renderer);
    
        SDL_SetRenderDrawColor(renderer, WHITE);
        
        SDL_RenderCopy(renderer, scoreP1FontTexture, NULL, &scoreP1FontRect);
        SDL_RenderCopy(renderer, scoreP2FontTexture, NULL, &scoreP2FontRect);

        if(gameState == GET_READY)
        {
            SDL_RenderCopy(renderer, p1ReadyPromptText.texture, NULL, &p1ReadyPromptText.rect);
            SDL_RenderCopy(renderer, p1ReadyText.texture, NULL, &p1ReadyText.rect);

            SDL_RenderCopy(renderer, p2ReadyPromptText.texture, NULL, &p2ReadyPromptText.rect);
            SDL_RenderCopy(renderer, p2ReadyText.texture, NULL, &p2ReadyText.rect);
        }

        SDL_RenderFillRect(renderer, &paddle01);
        SDL_RenderFillRect(renderer, &paddle02);

        SDL_SetRenderDrawColor(renderer, 255,0,0,1);

        SDL_RenderFillRect(renderer, &ball);

        SDL_RenderPresent(renderer);

        deltaTime = (currentFrameTime - startFrameTime) * 0.001f;
        //printf("DeltaTime: %f\n", deltaTime);

        frameTime = SDL_GetTicks() - currentFrameTime;
        fpsCollector += frameTime;
        startFrameTime = currentFrameTime;

        frameCounter++;
        if(frameCounter >= 100)
        {
            printf("Raw FPS: %.2f\n\n", 100/(fpsCollector * 0.001f));
           
            frameCounter = 0;
            fpsCollector = 0;
        }
        if(frameTime < targetFPS)
        {
            SDL_Delay(targetFPS - frameTime); 
        }
    }

    //Clean up...
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(fontMedium);
    SDL_DestroyTexture(scoreP1FontTexture);
    SDL_DestroyTexture(scoreP2FontTexture);
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


int InitializeText(TTF_Font *font,char* text, SDL_Color textColor, SDL_Renderer *renderer, 
        SDL_Texture **fontTexture, int x, int y, SDL_Rect *fontRect)
{
    SDL_Color fontColor = {WHITE};

    SDL_Surface *fontSurface = TTF_RenderText_Blended(font, text, fontColor);
    if(fontSurface == NULL)
    {
        printf("Font surface initialization error!\n");
        return FALSE;
    }

    *fontTexture = SDL_CreateTextureFromSurface(renderer, fontSurface);
    if(fontTexture == NULL)
    {
        printf("Font texture initialization error!\n");
        return FALSE;
    }

    fontRect->x = x; //Center align the text...
    fontRect->y = y;
    fontRect->w  = fontSurface->w;
    fontRect->h = fontSurface->h;

    fontRect->x -= fontRect->w * 0.5; //Center align the text...

    SDL_FreeSurface(fontSurface);

    return TRUE;
}

void HandleReadyState(char * gameState, char playerState)
{
    if((*gameState) == GET_READY)
    {
        //If both players are ready...
        if(playerState & PLAYER_1_READY && playerState & PLAYER_2_READY)
        {
            *gameState = IN_GAME;
        }
    }
}

void HandleGameStateMenu(char * gameState)
{
    //Set game state to READY...
    (*gameState) =  GET_READY; 
}

int ResetData(SDL_Rect *ball, Speed *currentBallSpeed, float *ballBoost,char * playerState
        ,SDL_Rect *paddle01, SDL_Rect *paddle02)
{
    ball->x = 800 * 0.5 - ball->w * 0.5;
    ball->y = 600 * 0.5 - ball->h * 0.5;

    currentBallSpeed->y = 0;
    currentBallSpeed->x = 350;

    (*ballBoost) = 1;

    *playerState = PLAYERS_NOT_READY;

    paddle01->x = PADDLE01_DEFAULT_X_POS; 
    paddle01->y = PADDLE01_DEFAULT_Y_POS; 

    paddle02->x = PADDLE02_DEFAULT_X_POS; 
    paddle02->y = PADDLE02_DEFAULT_Y_POS; 

    return TRUE;
}

int CreateFont(TTF_Font **font, unsigned int fontSize)
{
    *font = TTF_OpenFont("../Resources/8-bit-pusab.ttf", fontSize); 

    if(font == NULL)
    {
        printf("Error: Unable to initialize font! | ERROR: %s\n", SDL_GetError());
        return FALSE;
    }

    return TRUE;
}
