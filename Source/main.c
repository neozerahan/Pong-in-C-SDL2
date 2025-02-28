#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define SDL_MAIN_HANDLED
#include "..\Include\SDL2\SDL.h"
#include "..\Include\SDL2\SDL_ttf.h"
#include "..\Include\SDL2\SDL_mixer.h"

//COLORS
#define WHITE   255,255,255,255
#define BLACK   0,0,0,255
#define GREY    80,80,80,255

//PADDLE STATES
#define PADDLE_STATE_NONE 0
#define PADDLE_STATE_UP 1
#define PADDLE_STATE_DOWN 2

#define TRUE 1
#define FALSE 0

#define GAME_POINT 3

//GAME STATES
#define MENU         0b1 
#define GET_READY   0b10 
#define IN_GAME    0b100 
#define RESULT    0b1000 
#define PAUSE    0b10000 

//GAME SCENE
#define INTRO 1
#define GAME  2   
#define TITLE 3

//PLAYER STATES
#define PLAYERS_NOT_READY   0b0
#define PLAYER_1_READY      0b1
#define PLAYER_2_READY      0b10

//SCREEN RESOLUTION
#define SCREEN_RES_HEIGHT       600
#define SCREEN_RES_WIDTH        800

//PADDLE DIMENSIONS
#define PADDLE_WIDTH            16 
#define PADDLE_HEIGHT           128 

//PADDLE POSITION
#define PADDLE01_DEFAULT_X_POS  100 - (PADDLE_WIDTH * 0.5) 
#define PADDLE01_DEFAULT_Y_POS  (SCREEN_RES_HEIGHT * 0.5) - (PADDLE_HEIGHT * 0.5)

#define PADDLE02_DEFAULT_X_POS  SCREEN_RES_WIDTH - 100 - (PADDLE_WIDTH * 0.5)
#define PADDLE02_DEFAULT_Y_POS  (SCREEN_RES_HEIGHT * 0.5) - (PADDLE_HEIGHT * 0.5)

//-----------------------------------------------------------------------------------------------//
//                                          OBJECTS
//-----------------------------------------------------------------------------------------------//

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

typedef struct sfx{
    Mix_Chunk *paddleHit;
    Mix_Chunk *gainScore;
}sfx;

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

//-----------------------------------------------------------------------------------------------//

void ResetScore(int * scoreP1,char * scoreP1Text ,int * scoreP2, char * scoreP2Text);

int ResetData(SDL_Rect *ball, Speed *currentBallSpeed, float *ballBoost, char *playerState, 
        SDL_Rect *paddle01, SDL_Rect *paddle02);

void IncreaseScore(int *score, char * scoreText, char *gameState);

int InitializeText(TTF_Font *font,char* text, SDL_Color textColor, SDL_Renderer *renderer,
        SDL_Texture **fontTexture, int x, int y, SDL_Rect *fontRect);

void HandleGameStateMenu(char * gameState);

void HandleReadyState(char * gameState, char playerState, char * sceneID);

int CreateFont(TTF_Font **font, unsigned int fontSize);

//-----------------------------------------------------------------------------------------------//

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

    char gameState = MENU;

    char playerState = 0; 

    char sceneID = INTRO;

    float sceneIntroTime = 1.0f;
    float currentSceneIntroTime = 0.0f;

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

    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("Unable to initialize SDL Video!\n");
        return 1;
    }

    //-------------------------------------------------------------------------------------------//
    //                             SDL_AUDIO INITIALIZATION 
    //-------------------------------------------------------------------------------------------//

    if(SDL_Init(SDL_INIT_AUDIO) < 0){
        printf("Unable to initialize SDL Audio!\n");
        return 1;
    }

    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("Unable to open audio!\n");
        return 1;
    }
    Mix_Music * bgMusic = NULL;

    bgMusic = Mix_LoadMUS("../Resources/PongBGMusic.mp3");

    if(bgMusic == NULL)
    {
        printf("Unable to load bg music!\n");
        return 1;
    }

    sfx SFXs = {NULL};

    SFXs.paddleHit = Mix_LoadWAV("../Resources/SFXPaddleHit.wav");

    if(SFXs.paddleHit == NULL){
        printf("unable to load SFXpaddleHit.wav!\n");
        return 1;
    }

    SFXs.gainScore = Mix_LoadWAV("../Resources/SFXScoreGain.wav");

    if(SFXs.gainScore == NULL){
        printf("unable to load SFXScoreGain.wav!\n");
        return 1;
    }

    //-------------------------------------------------------------------------------------------//

    //Creates window...
    window = SDL_CreateWindow("PONG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            SCREEN_RES_WIDTH, SCREEN_RES_HEIGHT, 0);

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

    //Set Window Icon...
    SDL_Surface *winIconSurface = SDL_LoadBMP("../Resources/icon.bmp");

    if(winIconSurface == NULL)
    {
        printf("Unable to load the game icon!\n");
        return 1;
    }

    SDL_SetWindowIcon(window, winIconSurface);
    SDL_FreeSurface(winIconSurface);

    //-------------------------------------------------------------------------------------------//
    //                                          FONT
    //-------------------------------------------------------------------------------------------//

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


    //-------------------------------------------------------------------------------------------//
    //                                          TEXT 
    //-------------------------------------------------------------------------------------------//

    TextObject p1ReadyText = {NULL, "Press 'W'", {0}};

    InitializeText(fontSmall, p1ReadyText.text, fontColor, renderer, &p1ReadyText.texture, 
            PADDLE01_DEFAULT_X_POS, PADDLE01_DEFAULT_Y_POS + 150, &p1ReadyText.rect);

    //-------------------------------------------------------------------------------------------//
    
    TextObject p2ReadyText = {NULL, "Press 'UP'", {0}};

    InitializeText(fontSmall, p2ReadyText.text, fontColor, renderer, &p2ReadyText.texture, 
            PADDLE02_DEFAULT_X_POS, PADDLE02_DEFAULT_Y_POS + 150, &p2ReadyText.rect);

    //-------------------------------------------------------------------------------------------//
    
    TextObject p1ReadyPromptText = {NULL, "Ready?", {0}};

    InitializeText(fontSmall, p1ReadyPromptText.text, fontColor, renderer, &p1ReadyPromptText.texture, 
            PADDLE01_DEFAULT_X_POS, p1ReadyText.rect.y + 40, &p1ReadyPromptText.rect);

    //-------------------------------------------------------------------------------------------//

    TextObject p2ReadyPromptText = {NULL, "Ready?", {0}};

    InitializeText(fontSmall, p2ReadyPromptText.text, fontColor, renderer, &p2ReadyPromptText.texture, 
            PADDLE02_DEFAULT_X_POS, p2ReadyText.rect.y + 40, &p2ReadyPromptText.rect);

    //-------------------------------------------------------------------------------------------//

    TextObject victoryText01 = {NULL, "P1 wins!!!", {0}};

    InitializeText(fontSmall, victoryText01.text, fontColor, renderer, &victoryText01.texture, 
                   SCREEN_RES_WIDTH * 0.5f, SCREEN_RES_HEIGHT * 0.5f, &victoryText01.rect);

    //-------------------------------------------------------------------------------------------//
    
    TextObject victoryText02 = {NULL, "P2 wins!!!", {0}};

    InitializeText(fontSmall, victoryText02.text, fontColor, renderer, &victoryText02.texture, 
                   SCREEN_RES_WIDTH * 0.5f, SCREEN_RES_HEIGHT * 0.5f, &victoryText02.rect);

    //-------------------------------------------------------------------------------------------//
    
    TextObject restartText = {NULL, "Press 'R' to restart!", {0}};

    InitializeText(fontSmall, restartText.text, fontColor, renderer, &restartText.texture, 
                   SCREEN_RES_WIDTH * 0.5f, SCREEN_RES_HEIGHT * 0.5f + victoryText01.rect.h + 5, 
                   &restartText.rect);

    //-------------------------------------------------------------------------------------------//
    
    TextObject quitText = {NULL, "Press 'Q' to quit!", {0}};

    InitializeText(fontSmall, quitText.text, fontColor, renderer, &quitText.texture, 
                   SCREEN_RES_WIDTH * 0.5f, restartText.rect.y + restartText.rect.h, &quitText.rect);

    //-------------------------------------------------------------------------------------------//
    
    TextObject developerText = {NULL, "OHS ENGINE", {0}};

    InitializeText(fontSmall, developerText.text, fontColor, renderer, &developerText.texture, 
                   SCREEN_RES_WIDTH * 0.5f, SCREEN_RES_HEIGHT * 0.5f, &developerText.rect);

    //-------------------------------------------------------------------------------------------//
    
    TextObject titleText = {NULL, "PONG", {0}};

    InitializeText(fontMedium, titleText.text, fontColor, renderer, &titleText.texture, 
                   SCREEN_RES_WIDTH * 0.5f,50, &titleText.rect);

    //-------------------------------------------------------------------------------------------//
    TextObject pauseText = {NULL, "PAUSED!", {0}};

    InitializeText(fontMedium, pauseText.text, fontColor, renderer, &pauseText.texture, 
                   SCREEN_RES_WIDTH * 0.5f, SCREEN_RES_HEIGHT * 0.5, &pauseText.rect);

    //-------------------------------------------------------------------------------------------//
    
    SDL_Texture *scoreP1FontTexture = NULL;
    SDL_Rect scoreP1FontRect = {0};
    int scoreP1 = 0;
    char scoreP1Text[2] = "0";

    InitializeText(fontMedium, scoreP1Text, fontColor, renderer, &scoreP1FontTexture, 200, 50, 
            &scoreP1FontRect);

    //-------------------------------------------------------------------------------------------//

    SDL_Texture *scoreP2FontTexture = NULL;
    SDL_Rect scoreP2FontRect = {0};
    int scoreP2= 0;
    char scoreP2Text[2] = "0";

    InitializeText(fontMedium, scoreP2Text, fontColor, renderer, &scoreP2FontTexture, 
            SCREEN_RES_WIDTH-200, 50, &scoreP2FontRect);

    //-------------------------------------------------------------------------------------------//

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
    ball.x = SCREEN_RES_WIDTH * 0.5;
    ball.y = (SCREEN_RES_HEIGHT * 0.5) - (PADDLE_HEIGHT * 0.5);
   
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
            //--------------------------------------------------------------------------------
            //PADDLE CONTROLS...
            //--------------------------------------------------------------------------------

                if(event.key.keysym.sym == SDLK_q)
                {
                    if(gameState == RESULT)
                    {
                        isRunning = 0;
                        SDL_Quit();
                    }
                }
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

                //--------------------------------------------------------------------------------
                //RESTART GAME...
                //--------------------------------------------------------------------------------
                if(event.key.keysym.sym == SDLK_r)
                {
                    if(gameState == RESULT)
                    {
                        ResetScore(&scoreP1, scoreP1Text, &scoreP2, scoreP2Text);
                        gameState = GET_READY; 
                        sceneID = TITLE;

                        InitializeText(fontMedium, scoreP1Text, fontColor, renderer, 
                                &scoreP1FontTexture, 200, 50, &scoreP1FontRect);

                        InitializeText(fontMedium, scoreP2Text, fontColor, renderer, 
                                &scoreP2FontTexture, SCREEN_RES_WIDTH - 200, 50, &scoreP2FontRect);
                    }
                }
                    if(event.key.keysym.sym == SDLK_p)
                    {
                        if(gameState == IN_GAME)
                        {
                            gameState = PAUSE;
                            printf("Game Paused!!!\n");
                        }
                        else if(gameState == PAUSE)
                        {
                            gameState = IN_GAME;
                            printf("Game Unpaused!!!\n");
                        }
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
        if(sceneID == INTRO){
            currentSceneIntroTime += deltaTime;
            if(currentSceneIntroTime > sceneIntroTime){
                sceneID = TITLE;
            }
        }
        //Handle Menu State...
        if(gameState != PAUSE)
        {
            HandleGameStateMenu(&gameState); 
        }

        //Handle Ready State...
        if(gameState != PAUSE)
        {
            HandleReadyState(&gameState, playerState, &sceneID);
        }
        
        if(sceneID == TITLE){
            if(Mix_PlayingMusic() == 0){
                printf("Playing Music...\n");
                Mix_PlayMusic(bgMusic, -1);
            }
        }

        if(gameState != GET_READY && gameState != RESULT && gameState != PAUSE)
        { 
            //Player 01 Paddle movement...
            if(paddle01State == PADDLE_STATE_UP) 
            {
                if(paddle01.y > 0)
                    paddle01.y -= paddleSpeed * deltaTime; 
            }
            else if (paddle01State == PADDLE_STATE_DOWN)
            {
                if(paddle01.y < SCREEN_RES_HEIGHT- paddle01.h)
                    paddle01.y += paddleSpeed * deltaTime; 
            }

            if(paddle02State == PADDLE_STATE_UP)
            {
                if(paddle02.y > 0)
                    paddle02.y -= paddleSpeed * deltaTime;
            }
            else if (paddle02State == PADDLE_STATE_DOWN)
            {
                if(paddle02.y < SCREEN_RES_HEIGHT - paddle02.h)
                    paddle02.y += paddleSpeed * deltaTime; 
            }

            //Upper and Lower screen bound collision check...
            if(ball.y < 0) 
            {
                currentBallSpeed.y = ballSpeed.y;

                Mix_PlayChannel(-1, SFXs.paddleHit, 0);
            } 
            else if(ball.y + ball.h > SCREEN_RES_HEIGHT)
            {
                currentBallSpeed.y = -ballSpeed.y;

                Mix_PlayChannel(-1, SFXs.paddleHit, 0);
            }

            //Increase score...
            if(ball.x < 0)
            {
                if(ResetData(&ball, &currentBallSpeed, &ballBoost, &playerState, &paddle01, 
                            &paddle02) 
                        == FALSE)
                {
                    printf("Unable to reset data...\n");
                }

                IncreaseScore(&scoreP2, scoreP2Text, &gameState);

                InitializeText(fontMedium, scoreP2Text, fontColor, renderer, &scoreP2FontTexture, 
                        SCREEN_RES_WIDTH - 200, 50, &scoreP2FontRect);

                Mix_PlayChannel(-1,SFXs.gainScore, 0);
            } 

            if ( ball.x + ball.w > SCREEN_RES_WIDTH)
            {
                if(ResetData(&ball, &currentBallSpeed, &ballBoost, &playerState, &paddle01, 
                            &paddle02 ) 
                        == FALSE)
                {
                    printf("Unable to reset data...\n");
                }
                IncreaseScore(&scoreP1, scoreP1Text, &gameState);

                InitializeText(fontMedium, scoreP1Text, fontColor, renderer, &scoreP1FontTexture, 
                        200, 50, &scoreP1FontRect);

                Mix_PlayChannel(-1,SFXs.gainScore, 0);
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

                //Play SFX
                Mix_PlayChannel(-1, SFXs.paddleHit, 0);
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

                Mix_PlayChannel(-1, SFXs.paddleHit, 0);
            }

            ball.x += currentBallSpeed.x * deltaTime;
            ball.y += currentBallSpeed.y * deltaTime;
        }

        /*-----------------------------------------------------------------------------------------
        //RENDER LOOP...
        ----------------------------------------------------------------------------------------- */

        //Intro Scene...
        if(sceneID == INTRO)
        {
            SDL_SetRenderDrawColor(renderer, BLACK);
            SDL_RenderClear(renderer);

            SDL_RenderCopy(renderer, developerText.texture, NULL, &developerText.rect);
        }

        if(sceneID == GAME || sceneID == TITLE) 
        {
            SDL_SetRenderDrawColor(renderer, GREY);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, WHITE);

            //RENDER PLAYER SCORES...
            if(sceneID == GAME)
            {
                SDL_RenderCopy(renderer, scoreP1FontTexture, NULL, &scoreP1FontRect);
                SDL_RenderCopy(renderer, scoreP2FontTexture, NULL, &scoreP2FontRect);
            }

            //RENDER PAUSED TEXT...
            if(gameState == PAUSE)
            {
                SDL_RenderCopy(renderer, pauseText.texture, NULL, &pauseText.rect);
            }
            
            //RENDER READY TEXT AND OBJECTS...
            if(gameState == GET_READY)
            {
                if(sceneID == TITLE)
                {
                    SDL_RenderCopy(renderer, titleText.texture,NULL, &titleText.rect);

                    SDL_Rect  quitRect = quitText.rect;
                    quitRect.y = SCREEN_RES_HEIGHT * 0.8; 

                    SDL_RenderCopy(renderer, quitText.texture,NULL, &quitRect);
                }

                SDL_RenderCopy(renderer, p1ReadyPromptText.texture, NULL, &p1ReadyPromptText.rect);
                SDL_RenderCopy(renderer, p1ReadyText.texture, NULL, &p1ReadyText.rect);

                SDL_RenderCopy(renderer, p2ReadyPromptText.texture, NULL, &p2ReadyPromptText.rect);
                SDL_RenderCopy(renderer, p2ReadyText.texture, NULL, &p2ReadyText.rect);
            }

            //RENDER RESULT SCREEN...
            else if(gameState == RESULT)
            {
                if(scoreP1 > scoreP2)
                {
                    SDL_RenderCopy(renderer, victoryText01.texture, NULL, &victoryText01.rect);
                }
                else 
                {
                    SDL_RenderCopy(renderer, victoryText02.texture, NULL, &victoryText02.rect);
                }

                SDL_RenderCopy(renderer, restartText.texture, NULL, &restartText.rect);
                SDL_RenderCopy(renderer, quitText.texture, NULL, &quitText.rect);
            }
            
            //RENDER PADDLES...
            SDL_RenderFillRect(renderer, &paddle01);
            SDL_RenderFillRect(renderer, &paddle02);

            //Don't render the ball in Result screen...
            if(gameState != RESULT)
            {
                SDL_SetRenderDrawColor(renderer, 255,0,0,1);
                SDL_RenderFillRect(renderer, &ball);
            }
        }

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

void IncreaseScore(int *score, char * scoreText, char *gameState)
{
    *score += 1;

    //48 = 0 in ASCII table. So it starts from 0...
    scoreText[0] = 48 + (*score);

    //Win condition...
    if(*score >= GAME_POINT) 
    { 
        *gameState = RESULT;
    }
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

    if(*fontTexture != NULL)
    {
        SDL_DestroyTexture(*fontTexture);
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

void HandleReadyState(char * gameState, char playerState, char * sceneID)
{
    if((*gameState) == GET_READY)
    {
        //If both players are ready...
        if(playerState & PLAYER_1_READY && playerState & PLAYER_2_READY)
        {
            *gameState = IN_GAME;
            *sceneID = GAME;
        }
    }
}

void HandleGameStateMenu(char * gameState)
{
    if(*gameState == RESULT) 
        return;

    (*gameState) =  GET_READY; 
}

void ResetScore(int * scoreP1,char * scoreP1Text ,int * scoreP2, char * scoreP2Text)
{
    *scoreP1 = 0;
    scoreP1Text[0] = '0';

    *scoreP2 = 0;
    scoreP2Text[0] = '0';
}

int ResetData(SDL_Rect *ball, Speed *currentBallSpeed, float *ballBoost,char * playerState
        ,SDL_Rect *paddle01, SDL_Rect *paddle02)
{
    ball->x = SCREEN_RES_WIDTH * 0.5 - ball->w * 0.5;
    ball->y = SCREEN_RES_HEIGHT * 0.5 - ball->h * 0.5;

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
