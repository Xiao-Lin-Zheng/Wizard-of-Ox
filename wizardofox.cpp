/******************************************
* Environment/Compiler: XCode 15.4

* Interactions:
* Press the d key to open the sliding door.
* Press the c key to close the sliding door.
* Press the l key to toggle the ceiling white light on/off.
* Press the g key to activate green light and Oz head glow.
* Press the r key to deactivate green light, toggle bubbles, and toggle heel clicking.
* Press the b key to make the broom fly.
* Press the a key to increase global ambient lighting.
* Press the A key to decrease global ambient lighting.
* Press the arrow keys to move around (↑ forward, ↓ backward, ← turn left, → turn right).
* Press ESC to exit the program.
* reference: https://stackoverflow.com/questions/63358101/how-to-visualize-a-spot-light-in-opengl
* reference: https://learnopengl.com/Lighting/Light-casters
*******************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
using namespace std;

// globals
GLuint texture[1]; // [0]=grass
float camX = 0.0f, camY = 2.0f, camZ = 15.0f;
float angle = 0.0f;
float moveSpeed = 0.5f;
float turnSpeed = 0.1f;
float doorOffset = 0.0f;
const float maxDoorSlide = 2.0f;
bool doorOpening = false;
bool doorClosing = false;

// Room boundary
float roomX1 = -5.0f, roomX2 = 5.0f;
float roomZ1 = -10.0f, roomZ2 = 0.0f;

//Lighting
bool ceilingLightOn = true;
float globalAmbientLevel = 0.3f;
bool heelClicking = false;
float heelOffset = 0.0f;
float heelDir = 1.0f;
bool broomFlying = false;
float broomOffsetY = 0.0f;
float broomDir = 1.0f;

// Table bounds
float tableMinX = -1.0f, tableMaxX =1.0f;
float tableMinZ = -6.5f, tableMaxZ = -5.5f;

// Broom bounds
float broomMinX = -4.5f, broomMaxX = -3.6f;
float broomMinZ = -9.0f, broomMaxZ = -9.1f;

// Spotlight
GLUquadric* spotlightCone = gluNewQuadric();

// g key toggle
bool whiteGlowOn = true;
bool greenMode = false;

// r key toggled, bubble floating
const int NUM_BUBBLES = 30;
float bubbleX[NUM_BUBBLES];
float bubbleY[NUM_BUBBLES];
float bubbleZ[NUM_BUBBLES];
float bubbleSpeed[NUM_BUBBLES];
bool bubblesActive = false;


// texture
struct BitMapFile {
   int sizeX;
   int sizeY;
   unsigned char *data;
};

BitMapFile *getBMPData(string filename) {
   BitMapFile *bmp = new BitMapFile;
   unsigned int size, offset, headerSize;
   ifstream infile(filename.c_str(), ios::binary);
   infile.seekg(10); infile.read((char*)&offset, 4);
   infile.read((char*)&headerSize, 4);
   infile.seekg(18);
   infile.read((char*)&bmp->sizeX, 4);
   infile.read((char*)&bmp->sizeY, 4);
   size = bmp->sizeX * bmp->sizeY * 3;
   bmp->data = new unsigned char[size];
   infile.seekg(offset);
   infile.read((char*)bmp->data, size);
   for (int i = 0; i < size; i += 3) {
      unsigned char temp = bmp->data[i];
      bmp->data[i] = bmp->data[i + 2];
      bmp->data[i + 2] = temp;
   }
   return bmp;
}

// texture
void loadGrassTexture() {
   BitMapFile *image = getBMPData("Textures/grass.bmp");
   glBindTexture(GL_TEXTURE_2D, texture[0]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->sizeX, image->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, image->data);
}

// draw outdoor
void drawOutdoorScene() {
       glDisable(GL_LIGHTING);
       glEnable(GL_TEXTURE_2D);
       // Only draw grass
       glBindTexture(GL_TEXTURE_2D, texture[0]);
       glBegin(GL_QUADS);
       glTexCoord2f(0, 0); glVertex3f(-50, 0, -50);
       glTexCoord2f(5, 0); glVertex3f(50, 0, -50);
       glTexCoord2f(5, 5); glVertex3f(50, 0, 50);
       glTexCoord2f(0, 5); glVertex3f(-50, 0, 50);
       glEnd();
       glDisable(GL_TEXTURE_2D);
       glEnable(GL_LIGHTING);
    }

// lighting
void updateLighting() {
   GLfloat ambient[] = {globalAmbientLevel, globalAmbientLevel, globalAmbientLevel, 1.0f};
   glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
   if (ceilingLightOn) glEnable(GL_LIGHT1);
   else glDisable(GL_LIGHT1);
}

void drawTable() {
    GLfloat woodAmbient[] = {0.4f, 0.2f, 0.0f, 1.0f};
    GLfloat woodDiffuse[] = {0.8f, 0.5f, 0.2f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, woodAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, woodDiffuse);
    // Table top
    glPushMatrix();
    glTranslatef(0.0f, 2.5f, -5.0f);
    glScalef(4.0f, 0.2f, 3.0f);
    glutSolidCube(1.0f);
    glPopMatrix();
    // Legs
    float legX[] = {-1.8f, 1.8f};
    float legZ[] = {-1.3f, 1.3f};
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            glPushMatrix();
            glTranslatef(legX[i], 1.25f, -5.0f + legZ[j]);
            glScalef(0.2f, 2.5f, 0.2f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }
    }
}

void drawRubySlippers() {
    float tableTopY = 2.5f; // height of table top
    // materials
    GLfloat redAmbient[] = {0.4f, 0.0f, 0.0f, 1.0f};
    GLfloat redDiffuse[] = {1.0f, 0.0f, 0.0f, 1.0f};
    GLfloat redSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat shininess[] = {100.0f};
    float offset = fabs(heelOffset);
    // left shoe
    glPushMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, redAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, redDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, redSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glTranslatef(-0.4f, tableTopY + 0.2f + offset, -5.0f);
    glRotatef(15, 1.0f, 0.0f, 0.0f);
    glScalef(0.6f, 0.2f, 1.2f);
    glutSolidCube(1.0f);
    glPopMatrix();
    // left sparkle
    for (int i = 0; i < 10; i++) {
        float sx = ((rand() % 100) / 500.0f) - 0.1f;
        float sy = 0.2f + ((rand() % 100) / 500.0f);
        float sz = ((rand() % 100) / 500.0f) - 0.1f;
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, white);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);
        glPushMatrix();
        glTranslatef(-0.4f + sx, tableTopY + sy + offset, -5.0f + sz);
        glutSolidSphere(0.02f, 8, 8);
        glPopMatrix();
    }
    
    glPushMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, redAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, redDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, redSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glTranslatef(-0.4f, tableTopY + 0.1f, -5.6f);
    glScalef(0.2f, 0.4f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();
    // right shoe
    glPushMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, redAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, redDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, redSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glTranslatef(0.4f, tableTopY + 0.2f + offset, -5.0f);
    glRotatef(15, 1.0f, 0.0f, 0.0f);
    glScalef(0.6f, 0.2f, 1.2f);
    glutSolidCube(1.0f);
    glPopMatrix();
    // right sparkle
    for (int i = 0; i < 10; i++) {
        float sx = ((rand() % 100) / 500.0f) - 0.1f;
        float sy = 0.2f + ((rand() % 100) / 500.0f);
        float sz = ((rand() % 100) / 500.0f) - 0.1f;
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, white);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 100.0f);
        glPushMatrix();
        glTranslatef(0.4f + sx, tableTopY + sy + offset, -5.0f + sz);
        glutSolidSphere(0.02f, 8, 8);
        glPopMatrix();
    }
    // right heel
    glPushMatrix();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, redAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, redDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, redSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    glTranslatef(0.4f, tableTopY + 0.1f, -5.6f);
    glScalef(0.2f, 0.4f, 0.2f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawLampOnTable() {
    float tableTopY = 2.5f;
    float baseX = -0.8f;  // left side of table
    float lampZ = -5.0f;
    float verticalHeight = 0.8f;
    float horizontalLength = 0.8f;

    GLfloat darkGray[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat bulbColor[] = {1.0f, 0.9f, 0.6f, 1.0f};

    GLUquadric* quad = gluNewQuadric();

    // base
    glPushMatrix();
    glTranslatef(baseX, tableTopY + 0.025f, lampZ);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // flat on table
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, darkGray);
    gluDisk(quad, 0.0f, 0.2f, 32, 1);
    glPopMatrix();

    // Vertical Arm
    glPushMatrix();
    glTranslatef(baseX, tableTopY + verticalHeight / 2.0f, lampZ);
    glScalef(0.05f, verticalHeight, 0.05f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, darkGray);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Horizontal Arm
    glPushMatrix();
    glTranslatef(baseX + horizontalLength / 2.0f, tableTopY + verticalHeight, lampZ);
    glScalef(horizontalLength, 0.05f, 0.05f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, darkGray);
    glutSolidCube(1.0f);
    glPopMatrix();

    //  Cone Lampshade
    float lampHeadX = baseX + horizontalLength;
    float lampHeadY = tableTopY + verticalHeight - 0.1f;
    glPushMatrix();
    glTranslatef(lampHeadX, lampHeadY, lampZ);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, bulbColor);
    gluCylinder(quad, 0.15f, 0.0f, 0.25f, 20, 4);
    glPopMatrix();

    glPushMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 1.0f, 0.15f);

    float coneBase = 1.0f;
    float coneTip = 0.05f;
    float coneHeight = lampHeadY - 2.5f;

    glTranslatef(lampHeadX, lampHeadY, lampZ);
    glRotatef(-90.0f, -1.0f, 0.0f, 0.0f);
    gluCylinder(quad, coneTip, coneBase, coneHeight, 16, 1);

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glPopMatrix();

    gluDeleteQuadric(quad);
}

void drawLeaningBroom() {
    float baseX = -4.6f;
    float baseZ = -9.6f;
    float baseY = 0.3f + broomOffsetY;
    float handleLength = 2.0f;
    GLfloat handleColor[] = {0.4f, 0.2f, 0.1f, 1.0f};
    GLfloat bristleColor[] = {0.9f, 0.8f, 0.3f, 1.0f};
    GLUquadric* quad = gluNewQuadric();
    // bristle
    glPushMatrix();
    glTranslated(0.0f, -0.5f, 0.0f);
    glPushMatrix();
    glTranslatef(baseX+0.5f, baseY+0.3f, baseZ);
    glRotatef(-70.0f, 1.0f, 0.0f, 0.0f); // Lean backward
    glRotatef(-15.0f, 0.0f, 1.0f, 0.0f); // Side tilt
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, bristleColor);
    gluCylinder(quad, 0.15f, 0.05f, 0.3f, 16, 3);
    glPopMatrix();
    glPopMatrix();
    // handle
    glPushMatrix();
    glTranslated(0.0f, -0.5f, 0.0f);
    glPushMatrix();
    glTranslatef(baseX+0.5f, baseY+0.3f, baseZ);
    glRotatef(-70.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(-15.0f, 0.0f, 1.0f, 0.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, handleColor);
    gluCylinder(quad, 0.05f, 0.05f, handleLength, 12, 3);
    glPopMatrix();
    glPopMatrix();
    gluDeleteQuadric(quad);
}

void drawCeilingLightFixture() {
    float centerX = 0.0f;
    float centerY = 4.8f; // near the ceiling
    float centerZ = -5.0f;
    float radius = 0.2f;

    // Material properties
    GLfloat whiteAmbientGlow[]  = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat whiteAmbientDull[]  = {0.2f, 0.2f, 0.2f, 1.0f};

    GLfloat whiteDiffuseGlow[]  = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat whiteDiffuseDull[]  = {0.5f, 0.5f, 0.5f, 1.0f};

    GLfloat whiteEmissionGlow[] = {0.7f, 0.7f, 0.7f, 1.0f};
    GLfloat whiteEmissionDull[] = {0.0f, 0.0f, 0.0f, 1.0f};

    GLfloat* whiteAmbient  = whiteGlowOn ? whiteAmbientGlow  : whiteAmbientDull;
    GLfloat* whiteDiffuse  = whiteGlowOn ? whiteDiffuseGlow  : whiteDiffuseDull;
    GLfloat* whiteEmission = whiteGlowOn ? whiteEmissionGlow : whiteEmissionDull;

    // Green sphere
    GLfloat greenAmbientGlow[]   = {0.0f, 0.3f, 0.0f, 1.0f};
    GLfloat greenAmbientDark[]   = {0.0f, 0.2f, 0.0f, 1.0f};

    GLfloat greenDiffuseGlow[]   = {0.2f, 0.8f, 0.2f, 1.0f};
    GLfloat greenDiffuseDark[]   = {0.0f, 0.4f, 0.0f, 1.0f};

    GLfloat greenEmissionGlow[]  = {0.0f, 0.6f, 0.0f, 1.0f};
    GLfloat greenEmissionDark[]  = {0.0f, 0.0f, 0.0f, 1.0f};
    
    GLfloat* greenAmbient  = heelClicking ? greenAmbientDark  : greenAmbientGlow;
    GLfloat* greenDiffuse  = heelClicking ? greenDiffuseDark  : greenDiffuseGlow;
    GLfloat* greenEmission = heelClicking ? greenEmissionDark : greenEmissionGlow;

    GLfloat noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};

    // Draw string
    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINES);
    glVertex3f(centerX, centerY + radius + 0.1f, centerZ);
    glVertex3f(centerX, centerY, centerZ);
    glEnd();
    glEnable(GL_LIGHTING);

    // Top white glowing sphere
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, whiteAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, whiteDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, whiteEmission);
    glPushMatrix();
    glTranslatef(centerX, centerY, centerZ);
    glutSolidSphere(radius, 20, 20);
    glPopMatrix();

    // Bottom green glowing sphere
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, greenAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, greenDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, greenEmission);
    glPushMatrix();
    glTranslatef(centerX, centerY - radius * 2.0f, centerZ);
    glutSolidSphere(radius, 20, 20);
    glPopMatrix();

    // Bottom green sphere (Oz head)
    if (heelClicking) {
        GLfloat darkGreenAmbient[] = {0.0f, 0.2f, 0.0f, 1.0f};
        GLfloat darkGreenDiffuse[] = {0.0f, 0.5f, 0.0f, 1.0f};
        GLfloat noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, darkGreenAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, darkGreenDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);
    } else {
        // Glowing green
        GLfloat greenAmbient[] = {0.0f, 0.3f, 0.0f, 1.0f};
        GLfloat greenDiffuse[] = {0.2f, 0.8f, 0.2f, 1.0f};
        GLfloat greenEmission[] = {0.0f, 0.6f, 0.0f, 1.0f};

        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, greenAmbient);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, greenDiffuse);
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, greenEmission);
    }

    glPushMatrix();
    glTranslatef(centerX, centerY - radius * 2.0f, centerZ);
    glutSolidSphere(radius, 20, 20);
    glPopMatrix();
    
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);
  
    if (greenMode) {
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, greenEmission);
    } else {
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);
    }

}

void drawRoomBox() {
    float w = 10.0f, h = 5.0f, d = 10.0f;
    float x1 = -w / 2, x2 = w / 2;
    float y1 = 0.01f, y2 = h;
    float z1 = -d / 2 - 5, z2 = d / 2 - 5;
    float doorW = 2.0f, doorH = 3.0f;
    // Materials
    GLfloat brownAmbient[]  = {0.2f, 0.1f, 0.0f, 1.0f};
    GLfloat brownDiffuse[]  = {0.6f, 0.3f, 0.1f, 1.0f};
    GLfloat whiteAmbient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat whiteDiffuse[]  = {0.9f, 0.9f, 0.9f, 1.0f};
    GLfloat blueAmbient[]   = {0.1f, 0.1f, 0.2f, 1.0f};
    GLfloat blueDiffuse[]   = {0.5f, 0.5f, 1.0f, 1.0f};
    GLfloat pinkAmbient[]   = {0.2f, 0.1f, 0.1f, 1.0f};
    GLfloat pinkDiffuse[]   = {1.0f, 0.7f, 0.6f, 1.0f};
    GLfloat greenAmbient[]  = {0.1f, 0.2f, 0.1f, 1.0f};
    GLfloat greenDiffuse[]  = {0.6f, 1.0f, 0.6f, 1.0f};
    GLfloat yellowAmbient[] = {0.2f, 0.2f, 0.1f, 1.0f};
    GLfloat yellowDiffuse[] = {1.0f, 1.0f, 0.6f, 1.0f};
    GLfloat doorAmbient[] = {0.3f, 0.3f, 0.0f, 1.0f};
    GLfloat doorDiffuse[] = {1.0f, 1.0f, 0.2f, 1.0f};
    // Floor
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, brownAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, brownDiffuse);
    glNormal3f(0, 1, 0);
    glBegin(GL_QUADS);
    glVertex3f(x1,y1,z1); glVertex3f(x2,y1,z1);
    glVertex3f(x2,y1,z2); glVertex3f(x1,y1,z2);
    glEnd();
    // Ceiling
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, whiteAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, whiteDiffuse);
    glNormal3f(0, 1, 0);
    glBegin(GL_QUADS);
    glVertex3f(x1,y2,z1); glVertex3f(x1,y2,z2);
    glVertex3f(x2,y2,z2); glVertex3f(x2,y2,z1);
    glEnd();
    // Walls
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, blueAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, blueDiffuse);
    glNormal3f(0, 0, 1);
    glBegin(GL_QUADS);  // Back
    glVertex3f(x1,y1,z1); glVertex3f(x1,y2,z1);
    glVertex3f(x2,y2,z1); glVertex3f(x2,y1,z1);
    glEnd();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, pinkAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pinkDiffuse);
    glNormal3f(1, 0, 0);
    glBegin(GL_QUADS);  // Left
    glVertex3f(x1,y1,z1); glVertex3f(x1,y1,z2);
    glVertex3f(x1,y2,z2); glVertex3f(x1,y2,z1);
    glEnd();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, greenAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, greenDiffuse);
    glNormal3f(-1, 0, 0);
    glBegin(GL_QUADS);  // Right
    glVertex3f(x2,y1,z2); glVertex3f(x2,y1,z1);
    glVertex3f(x2,y2,z1); glVertex3f(x2,y2,z2);
    glEnd();
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, yellowAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, yellowDiffuse);
    glNormal3f(0, 0, -1);
    glBegin(GL_QUADS);  // Front left of door
    glVertex3f(x1,y1,z2);
    glVertex3f(x1 + (w - doorW)/2.0f,y1,z2);
    glVertex3f(x1 + (w - doorW)/2.0f,y2,z2);
    glVertex3f(x1,y2,z2);
    glEnd();
    glBegin(GL_QUADS);  // Front right of door
    glVertex3f(x2,y1,z2);
    glVertex3f(x2 - (w - doorW)/2.0f,y1,z2);
    glVertex3f(x2 - (w - doorW)/2.0f,y2,z2);
    glVertex3f(x2,y2,z2);
    glEnd();
    glBegin(GL_QUADS);  // Front above door
    glVertex3f(x1 + (w - doorW)/2.0f, doorH, z2);
    glVertex3f(x2 - (w - doorW)/2.0f, doorH, z2);
    glVertex3f(x2 - (w - doorW)/2.0f, y2, z2);
    glVertex3f(x1 + (w - doorW)/2.0f, y2, z2);
    
    glEnd();
    
    // light switch
    float switchWidth = 0.2f;
    float switchHeight = 0.4f;
    float switchX = -1.8f;
    float switchY = 2.0f;
    float switchZ = -0.1f;

    GLfloat switchAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat switchDiffuse[] = {0.6f, 0.6f, 0.6f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, switchAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, switchDiffuse);

    glPushMatrix();
    glTranslatef(switchX, switchY, switchZ);
    glScalef(switchWidth, switchHeight, 0.05f);
    glutSolidCube(1.0f);
    glPopMatrix();
    
    // Sliding door
    float doorWidth =2.0f;
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, doorAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, doorDiffuse);
    glNormal3f(0, 0, -1);
    glBegin(GL_QUADS);
    glVertex3f(-doorWidth/2.0f + doorOffset, y1, z2);
    glVertex3f(doorWidth/2.0f + doorOffset, y1, z2);
    glVertex3f(doorWidth/2.0f + doorOffset, doorH, z2);
    glVertex3f(-doorWidth/2.0f + doorOffset, doorH, z2);
    glEnd();

}

void drawBubbles() {
    GLfloat bubbleColor[] = {0.8f, 0.9f, 1.0f, 0.6f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, bubbleColor);

    for (int i = 0; i < NUM_BUBBLES; i++) {
        glPushMatrix();
        glTranslatef(bubbleX[i], bubbleY[i], bubbleZ[i]);
        glutSolidSphere(0.1f, 12, 12);
        glPopMatrix();
    }
}

void drawSun() {
    GLfloat sunEmission[] = {1.0f, 0.85f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, sunEmission);
    glPushMatrix();
    glTranslatef(20.0f, 20.0f, -20.0f);
    glutSolidSphere(2.0f, 30, 30);
    glPopMatrix();

    // Reset emission so it doesn't affect other objects
    GLfloat noEmission[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, noEmission);
}

void drawTeapotOnCube() {
    // cube
    GLfloat cubeColor[] = {0.2f, 0.4f, 0.6f, 1.0f};  // A blueish cube
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, cubeColor);
    glutSolidCube(1.0f);

    // teapot
    GLfloat potColor[] = {0.8f, 0.2f, 0.2f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, potColor);
    glPushMatrix();
    glTranslatef(0.0f, 0.6f, 0.0f);
    glutSolidTeapot(0.4f);
    glPopMatrix();
}

void setupSunlight() {
    glEnable(GL_LIGHT3);
    GLfloat sunDirection[] = {-1.0f, -1.0f, 1.0f, 0.0f};
    GLfloat sunAmbient[]   = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat sunDiffuse[]   = {1.0f, 0.9f, 0.5f, 1.0f};
    GLfloat sunSpecular[]  = {1.0f, 0.9f, 0.6f, 1.0f};

    glLightfv(GL_LIGHT3, GL_POSITION, sunDirection);
    glLightfv(GL_LIGHT3, GL_AMBIENT, sunAmbient);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, sunDiffuse);
    glLightfv(GL_LIGHT3, GL_SPECULAR, sunSpecular);
}

void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camX, camY, camZ, camX + sin(angle), camY, camZ - cos(angle), 0.0f, 1.0f, 0.0f);
    updateLighting();
    //glEnable(GL_LIGHT3);
    drawOutdoorScene();
    drawSun();
    //glDisable(GL_LIGHT3);
    
    drawRoomBox();
    
    glPushMatrix();
    glTranslatef(4.0f, 0.5f, -9.0f);
    glScalef(1.0f, 1.0f, 1.0f);
    drawTeapotOnCube();
    glPopMatrix();

    if (greenMode) {
        GLfloat normalAmbient[] = {globalAmbientLevel, globalAmbientLevel, globalAmbientLevel, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, normalAmbient);
    }

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -5.0f);
    glScalef(0.6f, 0.6f, 0.6f);
    drawTable();
    drawRubySlippers();
    glPopMatrix();

    if (greenMode) {
        GLfloat greenAmbient[] = {0.2f, 0.5f, 0.2f, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, greenAmbient);
    }
    
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -5.0f);
    glScalef(0.6f, 0.6f, 0.6f);
    drawTable();
    drawRubySlippers();
    glPopMatrix();
    
    if (greenMode) {
        GLfloat greenAmbient[] = {0.2f, 0.5f, 0.2f, 1.0f};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, greenAmbient);
    }
    
    glPushMatrix();
    glTranslatef(0.0f, -0.4f, -3.5f);
    glScalef(0.8f, 0.8f, 0.8f);
    drawLampOnTable();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, -0.4f, -4.0f);
    glScalef(0.8f, 0.8f, 0.8f);
    glPopMatrix();
    drawLeaningBroom();
    drawCeilingLightFixture();
    
    if (bubblesActive) {
        drawBubbles();
    }

    glutSwapBuffers();
}

bool isColliding(float newX, float newZ) {
    // 1. Wall collision
    if (newZ > roomZ2 + 0.1f) return false;
    if (newX < roomX1 || newX > roomX2 || newZ < roomZ1 || newZ > roomZ2)
        return true;

    // 2. Table collision
    if (newX > tableMinX && newX < tableMaxX &&
        newZ > tableMinZ && newZ < tableMaxZ)
        return true;

    // 3. Broom collision
    if (newX > broomMinX && newX < broomMaxX &&
        newZ > broomMinZ && newZ < broomMaxZ)
        return true;

    return false;
}

//movement
void handleArrowKeys(int key, int x, int y) {
    float newX = camX;
    float newZ = camZ;
    
    if (key == GLUT_KEY_LEFT) angle -= turnSpeed;
    if (key == GLUT_KEY_RIGHT) angle += turnSpeed;
    if (key == GLUT_KEY_UP) {
        newX += moveSpeed * sin(angle);
        newZ -= moveSpeed * cos(angle);
    }
    if (key == GLUT_KEY_DOWN) {
        newX -= moveSpeed * sin(angle);
        newZ += moveSpeed * cos(angle);
    }
    
    if (!isColliding(newX, newZ)) {
        camX = newX;
        camZ = newZ;
    }
    
    glutPostRedisplay();
}
void mouseClick(int button, int state, int x, int y) {

        }

void keyboard(unsigned char key, int, int) {
    if (key == 'd') doorOpening = true;
    if (key == 'c') doorClosing = true;
    if (key == 'g') {
        greenMode = !greenMode;
        whiteGlowOn = !whiteGlowOn;

        if (greenMode) {
            glDisable(GL_LIGHT1);
            glEnable(GL_LIGHT3);
        } else {
            glEnable(GL_LIGHT1);
            glDisable(GL_LIGHT3);
        }
    }

    if (key == 'b') {
        if (!broomFlying && broomOffsetY == 0.0f) {
            broomFlying = true;
            broomDir = 1.0f;
        }
    }
    if (key == 'l') ceilingLightOn = !ceilingLightOn;
    if (key == 'r') {
        heelClicking = !heelClicking;
        bubblesActive = heelClicking;
    }

    if (key == 'a') {
       globalAmbientLevel += 0.25f;
       if (globalAmbientLevel > 1.0f) globalAmbientLevel = 1.0f;
    }
    if (key == 'A') {
       globalAmbientLevel -= 0.25f;
       if (globalAmbientLevel < 0.0f) globalAmbientLevel = 0.0f;
    }
    glutPostRedisplay();
}
void update(int value) {
    if (doorOpening && doorOffset < maxDoorSlide) {
        doorOffset += 0.1f;
        if (doorOffset >= maxDoorSlide) {
            doorOffset = maxDoorSlide;
            doorOpening = false;
        }
    }
    if (doorClosing && doorOffset > 0.0f) {
        doorOffset -= 0.1f;
        if (doorOffset <= 0.0f) {
            doorOffset = 0.0f;
            doorClosing = false;
        }
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
    
    if (heelClicking) {
        heelOffset += heelDir * 0.01f;
        if (heelOffset > 0.1f || heelOffset < -0.1f) {
            heelDir = -heelDir;
        }
    }
    
    if (broomFlying) {
        broomOffsetY += broomDir * 0.01f;
        if (broomOffsetY > 1.0f) broomDir = -1.0f;
        if (broomOffsetY < 0.0f) {
            broomDir = 1.0f;
            broomOffsetY = 0.0f;
            broomFlying = false;
        }
    }
    
    if (bubblesActive) {
        for (int i = 0; i < NUM_BUBBLES; i++) {
            bubbleY[i] += bubbleSpeed[i];
            if (bubbleY[i] > 5.0f) bubbleY[i] = 0.5f;
        }
    }

}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    //glEnable(GL_COLOR_MATERIAL);
 
    GLfloat lightPos[] = {0.0f, 5.0f, 10.0f, 1.0f};
    GLfloat ambientLight[] = {0.3f, 0.3f, 0.3f, 1.0f};
    GLfloat diffuseLight[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glClearColor(0.6f, 0.85f, 1.0f, 1.0f);
    glGenTextures(1, texture);
    
    GLfloat ambient[] = {globalAmbientLevel, globalAmbientLevel, globalAmbientLevel, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

    GLfloat ceilingPos[] = {0.0f, 10.0f, -5.0f, 1.0f};

    GLfloat whiteDiffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat whiteSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat whiteAmbient[]  = {0.2f, 0.2f, 0.2f, 1.0f};
    
    GLfloat greenLightDiffuse[] = {0.2f, 0.6f, 0.2f, 1.0f};
    GLfloat greenLightAmbient[] = {0.1f, 0.4f, 0.1f, 1.0f};
    GLfloat greenLightPos[]     = {0.0f, 5.0f, -5.0f, 1.0f};

    glLightfv(GL_LIGHT3, GL_POSITION, greenLightPos);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, greenLightDiffuse);
    glLightfv(GL_LIGHT3, GL_AMBIENT, greenLightAmbient);
    glLightfv(GL_LIGHT3, GL_SPECULAR, greenLightDiffuse);
    glEnable(GL_LIGHT3);

    glLightfv(GL_LIGHT1, GL_POSITION, ceilingPos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, whiteAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, whiteDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, whiteSpecular);

    glEnable(GL_LIGHT1);
    glClearColor(0.6f, 0.85f, 1.0f, 1.0f);
    
    spotlightCone = gluNewQuadric();
    
    loadGrassTexture();
    for (int i = 0; i < NUM_BUBBLES; i++) {
        bubbleX[i] = ((rand() % 100) / 10.0f) - 5.0f;   // between -5 and 5
        bubbleY[i] = 0.5f + ((rand() % 50) / 10.0f);     // between 0.5 and 5.5
        bubbleZ[i] = -8.0f + ((rand() % 100) / 20.0f);   // between -10 and 0
        bubbleSpeed[i] = 0.005f + ((rand() % 10) / 1000.0f);
    }

}
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float aspect = (float)w / h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void interaction() {
    cout << "\n==== KEYBOARD INTERACTIONS ====\n";
    cout << "d - Open Door\n";
    cout << "c - Close Door\n";
    cout << "l - Toggle Ceiling Light On/Off\n";
    cout << "g - Toggle Green Room Mode (ambient green lighting)\n";
    cout << "r - Click Ruby Slippers (heels click, Oz head stops glowing, bubbles appear)\n";
    cout << "b - Make Broom Fly (bobs up and down)\n";
    cout << "a - Increase Ambient Light\n";
    cout << "Shift + A - Decrease Ambient Light\n";
    cout << "Arrow Keys - Move and Turn Camera\n";
    cout << "===============================\n";
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Assignment4");
    init();
    glutDisplayFunc(drawScene);
    glutReshapeFunc(reshape);
    glutSpecialFunc(handleArrowKeys);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseClick);
    interaction();
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}
