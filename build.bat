ECHO OFF
gcc .\Source\main.c -mwindows -o .\Bin\game.exe .\Bin\SDL2.dll .\Bin\SDL2_ttf.dll .\Bin\SDL2_mixer.dll -I .\Includes

