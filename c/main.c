#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "c_png_reader.h"

int main(int argc, char **argv)
{
    int ret = -1;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL is active!", 100, 100, 512, 512, 0);
    if (!window)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        goto quit;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!renderer)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        goto quit;
    }

    int width;
    int height;
    int channels;
    unsigned char* pixels = get_pixel_data("image.png", &width, &height, &channels);
    
    if (!pixels)
    {
        SDL_Log("Unable to open image");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        goto quit;
    }
    
    SDL_Log("Image width: %d height: %d channhels: %d", width, height, channels);

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture)
    {
        SDL_Log("Unable to create texture: %s", SDL_GetError());
        free(pixels);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        goto quit;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(texture, NULL, pixels, width * channels);
    free(pixels);

    ret = 0;

    int running = 1;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                running = 0;
            }
        }

        SDL_SetRenderDrawColor(renderer , 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_Rect target_rect = {100, 100, 256, 256};
        SDL_RenderCopy(renderer, texture, NULL, &target_rect);
        SDL_RenderPresent (renderer);
    }
    
    quit:
        SDL_Quit();
        return ret;
    return 0;
}