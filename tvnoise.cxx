//g++ -O3 main.cpp -o tv_sim -lSDL2 -lSDL2_image -lGL

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

// Window Dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Helper to convert an SDL_Surface into an OpenGL texture
GLuint loadTextureFromPNG(const char* filename) {
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        std::cerr << "Failed to load image: " << IMG_GetError() << std::endl;
        return 0;
    }

    // Determine format
    GLenum format = GL_RGB;
    if (surface->format->BytesPerPixel == 4) {
        format = GL_RGBA;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters for legacy OpenGL
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Upload surface pixel data to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);

    SDL_FreeSurface(surface);
    return textureID;
}

// Draw a simple quad that spans the whole screen or viewport
void drawQuad() {
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f); // Bottom-Left
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f); // Bottom-Right
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f); // Top-Right
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f); // Top-Left
    glEnd();
}

int main(int argc, char* argv[]) {
    // 1. Initialize SDL2 and SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return -1;

    // 2. Create SDL2 Window with OpenGL support
    SDL_Window* window = SDL_CreateWindow("Analogue Static & Overlay (SDL2 + Legacy GL)", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    
    SDL_GLContext glContext = SDL_GL_ someContext;
    glContext = SDL_GL_CreateContext(window);

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 3. Setup the Noise Texture (Grayscale / Luminance)
    GLuint noiseTextureID;
    glGenTextures(1, &noiseTextureID);
    glBindTexture(GL_TEXTURE_2D, noiseTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Buffer for static pixels (using single channel GL_LUMINANCE for optimization)
    std::vector<GLubyte> noiseBuffer(WIDTH * HEIGHT);

    // 4. Load the Overlay PNG Texture
    GLuint overlayTextureID = loadTextureFromPNG("overlay.png");
    if (overlayTextureID == 0) return -1;

    // Enable basic texturing and blending for transparency
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool running = true;
    SDL_Event event;

    // 5. Application Main Loop
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // --- UPDATE: Populate noise buffer with random pixels ---
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            noiseBuffer[i] = std::rand() % 256;
        }

        // --- DRAW ---
        glClear(GL_COLOR_BUFFER_BIT);

        // Layer 1: Background Analogue Static
        glBindTexture(GL_TEXTURE_2D, noiseTextureID);
        // Using SubImage2D is faster than re-allocating memory with glTexImage2D
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, WIDTH, HEIGHT, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, noiseBuffer.data());
        drawQuad();

        // Layer 2: Foreground Overlay (Transparency handled by glBlendFunc)
        glBindTexture(GL_TEXTURE_2D, overlayTextureID);
        drawQuad();

        SDL_GL_SwapWindow(window);
        SDL_Delay(16); // Target roughly 60 FPS
    }

    // Cleanup
    glDeleteTextures(1, &noiseTextureID);
    glDeleteTextures(1, &overlayTextureID);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
