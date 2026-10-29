#include <GL/glut.h>
#include <cstdlib>
#include <ctime>

// Approximate palette of C64 color components (0.0 to 1.0)
const GLfloat C64_PALETTE[16][3] = {
    {0.0f, 0.0f, 0.0f},       // 0: Black
    {1.0f, 1.0f, 1.0f},       // 1: White
    {0.53f, 0.27f, 0.20f},    // 2: Red
    {0.44f, 0.73f, 0.76f},    // 3: Cyan
    {0.55f, 0.29f, 0.64f},    // 4: Purple
    {0.33f, 0.65f, 0.31f},    // 5: Green
    {0.22f, 0.20f, 0.60f},    // 6: Blue
    {0.73f, 0.79f, 0.43f},    // 7: Yellow
    {0.55f, 0.38f, 0.18f},    // 8: Orange
    {0.40f, 0.31f, 0.00f},    // 9: Brown
    {0.73f, 0.47f, 0.42f},    // 10: Light Red
    {0.31f, 0.31f, 0.31f},    // 11: Dark Grey
    {0.47f, 0.47f, 0.47f},    // 12: Grey
    {0.67f, 0.90f, 0.64f},    // 13: Light Green
    {0.67f, 0.64f, 0.92f},    // 14: Light Blue
    {0.62f, 0.62f, 0.62f}     // 15: Light Grey
};

// Application state
int border_color_idx = 0;
bool is_loading = true;

void display() {
    // 1. Clear the screen with the flashing border color
    GLfloat r = C64_PALETTE[border_color_idx][0];
    GLfloat g = C64_PALETTE[border_color_idx][1];
    GLfloat b = C64_PALETTE[border_color_idx][2];
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Reset coordinate matrix
    glLoadIdentity();

    // 2. Draw the random horizontal loading bands (simulating tape noise)
    if (is_loading) {
        glBegin(GL_QUADS);
        float current_y = -1.0f;
        while (current_y < 1.0f) {
            // Randomly choose height of the band fragment
            float band_height = (rand() % 50 + 5) / 500.0f; 
            int color_idx = rand() % 16;

            glColor3fv(C64_PALETTE[color_idx]);
            
            glVertex2f(-1.0f, current_y);
            glVertex2f(1.0f, current_y);
            glVertex2f(1.0f, current_y + band_height);
            glVertex2f(-1.0f, current_y + band_height);

            current_y += band_height;
        }
        glEnd();
    }

    // 3. Overlay the inner C64 screen rectangle (traditionally dark blue)
    // C64 active area was 320x200 inside a larger border
    glColor3fv(C64_PALETTE[6]); // 6 is C64 Blue
    glBegin(GL_QUADS);
        glVertex2f(-0.7f, -0.6f);
        glVertex2f( 0.7f, -0.6f);
        glVertex2f( 0.7f,  0.6f);
        glVertex2f(-0.7f,  0.6f);
    glEnd();

    glutSwapBuffers();
}

void timer(int value) {
    if (is_loading) {
        // Rapidly change the border color index to create the flicker effect
        border_color_idx = rand() % 16;
        
        // Force a screen redraw
        glutPostRedisplay();
    }
    
    // Call timer again in ~20 milliseconds (~50 FPS)
    glutTimerFunc(20, timer, 0);
}

void handle_keypress(unsigned char key, int x, int y) {
    // Press ESC to exit, Space to pause/unpause the loading flash
    if (key == 27) { 
        exit(0);
    } else if (key == ' ') {
        is_loading = !is_loading;
        if (!is_loading) {
            border_color_idx = 14; // Default light blue C64 border when idle
            glutPostRedisplay();
        }
    }
}

int main(int argc, char** argv) {
    srand(static_cast<unsigned int>(time(NULL)));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Commodore 64 Loading Screen Emulator");

    // Register callbacks
    glutDisplayFunc(display);
    glutTimerFunc(20, timer, 0);
    glutKeyboardFunc(handle_keypress);

    glutMainLoop();
    return 0;
}
