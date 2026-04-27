/*
 * FTGL example - renders text using all FTGL font types.
 *
 * Controls:
 *   Left / Right arrow  - cycle through font types
 *   ESC / q             - quit
 */

#ifdef _WIN32
#  include <windows.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <FTGL/ftgl.h>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

#ifndef FONT_PATH
#  ifdef _WIN32
#    define FONT_PATH "C:/Windows/Fonts/arial.ttf"
#  elif defined(__APPLE__)
#    define FONT_PATH "/Library/Fonts/Arial.ttf"
#  else
#    define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#  endif
#endif

static const unsigned int FONT_SIZE = 48;
static const char *SAMPLE_TEXT      = "Hello, FTGL!";

// 3-D rotation state (used for FTExtrudeFont)
static float rotX = 20.0f;
static float rotY = -30.0f;
static int   lastMouseX = 0;
static int   lastMouseY = 0;
static bool  mouseDown  = false;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

// Font types enum
enum FontType
{
    FONT_BITMAP = 0,
    FONT_PIXMAP,
    FONT_OUTLINE,
    FONT_POLYGON,
    FONT_EXTRUDE,
    FONT_TEXTURE,
    FONT_BUFFER,
    FONT_TYPE_COUNT
};

static FTFont *fonts[FONT_TYPE_COUNT] = {};
static FontType current = FONT_TEXTURE;

static const char *fontNames[FONT_TYPE_COUNT] = {
    "FTBitmapFont",
    "FTPixmapFont",
    "FTOutlineFont",
    "FTPolygonFont",
    "FTExtrudeFont",
    "FTTextureFont",
    "FTBufferFont"
};

static int winW = 800, winH = 200;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static FTFont *createFont(FontType type, const char *path)
{
    FTFont *f = nullptr;
    switch (type)
    {
        case FONT_BITMAP:   f = new FTBitmapFont(path);  break;
        case FONT_PIXMAP:   f = new FTPixmapFont(path);  break;
        case FONT_OUTLINE:  f = new FTOutlineFont(path); break;
        case FONT_POLYGON:  f = new FTPolygonFont(path); break;
        case FONT_EXTRUDE:  f = new FTExtrudeFont(path); break;
        case FONT_TEXTURE:  f = new FTTextureFont(path); break;
        case FONT_BUFFER:   f = new FTBufferFont(path);  break;
        default: break;
    }
    if (!f || f->Error())
    {
        fprintf(stderr, "FTGL: failed to load font '%s' (error %d)\n",
                path, f ? f->Error() : -1);
        delete f;
        return nullptr;
    }
    if (!f->FaceSize(FONT_SIZE))
    {
        fprintf(stderr, "FTGL: failed to set face size\n");
        delete f;
        return nullptr;
    }
    if (type == FONT_EXTRUDE)
    {
        // Ascender() reads face metrics set by FaceSize() — no glyph is cached yet,
        // so Depth() is applied to every character including the first.
        f->Depth(f->Ascender());
    }
    return f;
}

// ---------------------------------------------------------------------------
// GLUT callbacks
// ---------------------------------------------------------------------------

// Set up projection for the current font type and window size.
static void setProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (current == FONT_EXTRUDE)
    {
        double aspect = (winH > 0) ? static_cast<double>(winW) / winH : 1.0;
        // 45-degree perspective; near/far chosen to comfortably contain the text
        gluPerspective(45.0, aspect, 1.0, 10000.0);
    }
    else
    {
        glOrtho(0, winW, 0, winH, -100, 100);
    }
    glMatrixMode(GL_MODELVIEW);
}

static void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    FTFont *f = fonts[current];
    if (!f)
    {
        glutSwapBuffers();
        return;
    }

    glLoadIdentity();

    if (current == FONT_EXTRUDE)
    {
        // 3-D perspective view
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        static const GLfloat lightPos[]  = { 200.f, 400.f, 600.f, 1.f };
        static const GLfloat ambient[]   = { 0.2f, 0.2f, 0.2f, 1.f };
        static const GLfloat diffuse[]   = { 0.9f, 0.9f, 0.9f, 1.f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
        glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);

        // Place the camera so the text fills the view comfortably
        FTBBox bb = f->BBox(SAMPLE_TEXT);
        float textW = (bb.Upper() - bb.Lower()).Xf();
        float textH = (bb.Upper() - bb.Lower()).Yf();
        float cx = bb.Lower().Xf() + textW * 0.5f;
        float cy = bb.Lower().Yf() + textH * 0.5f;
        float dist = textW * 1.2f;  // pull back enough to see the text

        gluLookAt(cx, cy, dist,   // eye
                  cx, cy, 0.0,   // centre
                  0.0, 1.0, 0.0);// up

        // Apply mouse-drag rotation around the text centre
        glTranslatef(cx, cy, 0.0f);
        glRotatef(rotX, 1.0f, 0.0f, 0.0f);
        glRotatef(rotY, 0.0f, 1.0f, 0.0f);
        glTranslatef(-cx, -cy, 0.0f);

        glColor3f(0.4f, 0.8f, 1.0f);
        f->Render(SAMPLE_TEXT);

        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_COLOR_MATERIAL);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        // Centre the string in the 2-D ortho view
        FTBBox bb = f->BBox(SAMPLE_TEXT);
        float textW = static_cast<float>((bb.Upper() - bb.Lower()).Xf());
        float textH = static_cast<float>((bb.Upper() - bb.Lower()).Yf());
        float x = (static_cast<float>(winW) - textW) * 0.5f;
        float y = (static_cast<float>(winH) - textH) * 0.5f;

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(x, y);   // needed for bitmap / pixmap fonts
        glTranslatef(x, y, 0.0f);
        f->Render(SAMPLE_TEXT);
    }

    // Print the current font name in the window title bar
    char title[128];
    snprintf(title, sizeof(title),
             current == FONT_EXTRUDE
                 ? "FTGL example  [%d/%d] %s  (Left/Right to switch | drag to rotate)"
                 : "FTGL example  [%d/%d] %s  (Left/Right to switch)",
             current + 1, FONT_TYPE_COUNT, fontNames[current]);
    glutSetWindowTitle(title);

    glutSwapBuffers();
}

static void reshape(int w, int h)
{
    winW = w;
    winH = (h > 0) ? h : 1;
    glViewport(0, 0, winW, winH);
    setProjection();
    glLoadIdentity();
}

static void keyboard(unsigned char key, int /*x*/, int /*y*/)
{
    switch (key)
    {
        case 27:  // ESC
        case 'q':
        case 'Q':
            for (int i = 0; i < FONT_TYPE_COUNT; ++i) { delete fonts[i]; fonts[i] = nullptr; }
            exit(0);
    }
    glutPostRedisplay();
}

static void specialKey(int key, int /*x*/, int /*y*/)
{
    switch (key)
    {
        case GLUT_KEY_LEFT:
            current = static_cast<FontType>(
                (current + FONT_TYPE_COUNT - 1) % FONT_TYPE_COUNT);
            break;
        case GLUT_KEY_RIGHT:
            current = static_cast<FontType>(
                (current + 1) % FONT_TYPE_COUNT);
            break;
    }
    // Rebuild the projection for the new font type
    setProjection();
    glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        mouseDown  = (state == GLUT_DOWN);
        lastMouseX = x;
        lastMouseY = y;
    }
}

static void motion(int x, int y)
{
    if (mouseDown && current == FONT_EXTRUDE)
    {
        rotY += static_cast<float>(x - lastMouseX) * 0.5f;
        rotX += static_cast<float>(y - lastMouseY) * 0.5f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char **argv)
{
    const char *fontPath = (argc > 1) ? argv[1] : FONT_PATH;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("FTGL example");

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Load all font types up front
    for (int i = 0; i < FONT_TYPE_COUNT; ++i)
    {
        fonts[i] = createFont(static_cast<FontType>(i), fontPath);
        if (!fonts[i])
        {
            fprintf(stderr, "Could not create %s from '%s'\n", fontNames[i], fontPath);
        }
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKey);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    printf("Left/Right arrow = cycle font type   q/ESC = quit\n");
    printf("When FTExtrudeFont is active: drag with mouse to rotate\n");
    printf("Font: %s\n", fontPath);

    glutMainLoop();
    return 0;
}
