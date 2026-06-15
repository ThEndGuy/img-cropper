#include <stdio.h>
#include <stdbool.h>
#include "foreign/raylib.h"

#include "foreign/tinyfiledialogs.h"

#define RAYGUI_IMPLEMENTATION
#include "foreign/raygui.h"


#define PROGRAM_NAME "Image Cropper"


static const int START_WINDOW_WIDTH = 1920/2;
static const int START_WINDOW_HEIGHT = 1080/2;
static const int IMG_MAX_WIDTH = 1280;
static const int IMG_MAX_HEIGHT = 720;
static const int IMG_X_OFFSET = 20;
static const int FPS = 60;

#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT  30
#define SELECT_WIDTH 225
#define SELECT_HEIGHT 350
#define SELECT_RES ((float)SELECT_WIDTH / SELECT_HEIGHT)
float globalScale = 1.0f;


Color SELECT_COLOR = {100, 100, 100, 150};
Color FADED_BLACK = {50, 50, 50, 100};

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


// int WINDOW_RESOLUTION = WINDOW_WIDTH/WINDOW_HEIGHT;


typedef struct {
    Image image;
    Texture2D texture;
    Vector2 texturePos;
} TextureView;

typedef struct {
    Rectangle rect;
    Vector2 center;

    float scale;
    float targetScale;

    bool attached;
    Vector2 attachOffset;
} SelectionRect;

float aspectRatio(Vector2 obj){
    return obj.x / obj.y;
}

void ForceRectInsideImg(SelectionRect *sel, TextureView *texV){
    if (sel->rect.x <= texV->texturePos.x) {sel->rect.x = texV->texturePos.x;}
    if ((sel->rect.x + sel->rect.width) >= (texV->texturePos.x + texV->texture.width)) {sel->rect.x = texV->texturePos.x + texV->texture.width - sel->rect.width;}
    if (sel->rect.y <= texV->texturePos.y) {sel->rect.y = texV->texturePos.y;}
    if ((sel->rect.y + sel->rect.height) >= (texV->texturePos.y + texV->texture.height)){sel->rect.y = texV->texturePos.y + texV->texture.height - sel->rect.height;}
    sel->center.x = sel->rect.x + sel->rect.width/2.0f;
    sel->center.y = sel->rect.y + sel->rect.height/2.0f;
}
bool IsInsideImage(Vector2 pos, TextureView *texV){
    if ((pos.x < texV->texturePos.x) ||
       (pos.x > (texV->texturePos.x + texV->texture.width)) ||
       (pos.y < texV->texturePos.y) ||
       (pos.y > (texV->texturePos.y + texV->texture.height))){
        return false;
        }
    return true;
}

bool IsImageFitBoundary(Image img){
    if (img.width <= IMG_MAX_WIDTH && img.height <= IMG_MAX_HEIGHT)
        return true;
    else{
        return false;
    }
}

void ImageResizeToBoundary(Image *img){
    int w = img->width;
    int h = img->height;
    float img_res = (float)w/h;
    if (IMG_MAX_HEIGHT*img_res <= IMG_MAX_WIDTH){
        ImageResize(img, IMG_MAX_HEIGHT*img_res, IMG_MAX_HEIGHT);
    }else{
        ImageResize(img, (int)IMG_MAX_WIDTH, (int)IMG_MAX_WIDTH/img_res);

    }
}

Image LoadImageFromFile(const char *fileName) {
    Image img = LoadImage(fileName);
    if (!IsImageValid(img)) {
        fprintf(stderr, "Image has failed to load");
        return img;
    }
    if (!IsImageFitBoundary(img)){
        printf("-------------------------------\n");
        printf("Real img size = (%d, %d)\n", img.width, img.height);
        ImageResizeToBoundary(&img);
        printf("New img size = (%d, %d)\n", img.width, img.height);
        printf("-------------------------------\n");
    }
    return img;
}

Image QueryAndLoadImageFromFile() {
    const char *filters[] = {".jpeg", "*.png", "*.jpg"};
    const char *fileName = tinyfd_openFileDialog("Open file:", "./", 2, filters, "Image files", 0);

    printf("-------------------------------\n");
    if (fileName) {printf("Path: %s\n", fileName);}
    printf("-------------------------------\n");

    return LoadImageFromFile(fileName);

}


bool SaveSelectAsImage(Rectangle cropRect, Image img, const char* fileName){
    if (!IsImageValid(img)) {
        fprintf(stderr, "Invalid image loaded!");
        return false;
    }
    Image croppedImg = ImageCopy(img);
    ImageCrop(&croppedImg, cropRect);
    ImageResize(&croppedImg, SELECT_WIDTH, SELECT_HEIGHT);
    if (!ExportImage(croppedImg, fileName)) {
            fprintf(stderr, "Could not save image");
            UnloadImage(croppedImg);
            return false;
        }
    return true;
}

bool QueryAndSaveSelectAsImage(Rectangle cropRect, Image img){

    const char *filters[] = {".jpeg", "*.png", "*.jpg"};
    const char *fileName = tinyfd_saveFileDialog("Save image to:", "./", ARR_SIZE(filters), filters, NULL);
    if (!fileName) {return false;}

    if (!SaveSelectAsImage(cropRect, img, fileName)) {return false;}
    return true;

}

void DrawThirdsLines(Rectangle *rect){
    float x, y, w, h;
    x = rect->x;
    y = rect->y;
    h = rect->height;
    w = rect->width;

    DrawLine(x + w/3, y, x + w/3, y + h, FADED_BLACK);
    DrawLine(x + 2*w/3, y, x + 2*w/3, y + h, FADED_BLACK);
    DrawLine(x, y + h/3, x + w, y + h/3 , FADED_BLACK);
    DrawLine(x, y + 2*h/3, x + w, y + 2*h/3 , FADED_BLACK);
}

void DrawTextureCentered(Texture2D texture, Vector2 centerPos, Color tint){
    float x = centerPos.x - (texture.width/2.0f);
    float y = centerPos.y - (texture.height/2.0f);

    DrawTexture(texture, x, y, tint);
}

void ScaleRect(Rectangle *rect, float scale){
    rect->height *= scale;
    rect->width *= scale;
}






int main()
{
    // Config and start
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(START_WINDOW_WIDTH, START_WINDOW_HEIGHT, PROGRAM_NAME);
    MaximizeWindow();


// Opening image 
    
    TextureView mainTex = {0};
    SelectionRect selection = {0};


    // Selection rect settings
    selection.rect =  (Rectangle){0, 0, 225, 350};
    selection.center = (Vector2){
        selection.rect.x + selection.rect.width/2.0f,
        selection.rect.y + selection.rect.height/2.0f,
        };
    selection.scale = 1.0f;
    selection.targetScale = 1.0f;
    selection.attached = false;


    SetTargetFPS(FPS);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Q)) {CloseWindow();}
        float centerY = GetScreenHeight() / 2.0f;
        mainTex.texturePos = (Vector2){IMG_X_OFFSET, centerY - (mainTex.texture.height/2.0f)};
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Draw image
        
        DrawTextureEx(mainTex.texture, mainTex.texturePos, 0.0f, globalScale, WHITE);

        // Selection rectangle logic
        // Using keys:
       
        int keyOffset;
        if (IsKeyDown(KEY_LEFT_SHIFT)){keyOffset = 10;}else{keyOffset = 1;}

        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {selection.center.x += keyOffset;} 
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  {selection.center.x -= keyOffset;}
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  {selection.center.y += keyOffset;} 
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    {selection.center.y -= keyOffset;} 

        // Using mouse:
        Vector2 mousePos = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
                    // if not on rect, move it to mouse
                if (!CheckCollisionPointRec(mousePos, selection.rect)){
                    selection.center = mousePos;
            } else {
                // if on rect, anchor to mouse
                if (!selection.attached){
                    selection.attached = true;
                    selection.attachOffset.x = mousePos.x - selection.center.x;
                    selection.attachOffset.y = mousePos.y - selection.center.y;
                }
            }
        }

        // Stop anchoring
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){selection.attached = false;}
        if (!IsInsideImage(mousePos, &mainTex)){
            selection.center.x = selection.rect.x + selection.rect.width/2.0f;
            selection.center.y = selection.rect.y + selection.rect.height/2.0f;

        }
        if (selection.attached){
            selection.center.x = mousePos.x - selection.attachOffset.x;
            selection.center.y = mousePos.y - selection.attachOffset.y;
        }

        float wheel = GetMouseWheelMove();

        selection.targetScale += wheel * 0.1f;
        float dt = GetFrameTime();

        float maxScaleX = (float)mainTex.image.width  / SELECT_WIDTH;
        float maxScaleY = ((float)mainTex.image.height) / SELECT_HEIGHT;
        float maxScale = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
        if (selection.targetScale > maxScale){
        selection.targetScale = maxScale;
        }

        if (selection.targetScale < 0.1f){
            selection.targetScale = 0.1f;
        }

        selection.scale += (selection.targetScale - selection.scale) * 20.0f * dt;

        selection.rect.width = SELECT_WIDTH * selection.scale;
        selection.rect.height = SELECT_HEIGHT * selection.scale;
        selection.rect.x = selection.center.x - selection.rect.width/2.0f;
        selection.rect.y = selection.center.y - selection.rect.height/2.0f;
        if (selection.rect.height > mainTex.image.height){selection.rect.height = mainTex.image.height;}

        ForceRectInsideImg(&selection, &mainTex);

        Rectangle cropRect = {selection.rect.x - mainTex.texturePos.x, selection.rect.y - mainTex.texturePos.y, selection.rect.width, selection.rect.height};
        Rectangle imgPreview = {GetScreenWidth() - 410, 100, SELECT_WIDTH, SELECT_HEIGHT};
        DrawTexturePro(mainTex.texture, cropRect, imgPreview, (Vector2){0, 0}, 0, WHITE);


        // Draw stuff
        // OPEN IMAGE BUTTON
        if (GuiButton((Rectangle){100, GetScreenHeight() - 100, BUTTON_WIDTH, BUTTON_HEIGHT}, "#012# Open image")) { 
            mainTex.image = QueryAndLoadImageFromFile();
            mainTex.texture = LoadTextureFromImage(mainTex.image);
            selection.targetScale = 1.0f;
        }

        // SAVE IMAGE BUTTON
        if (IsImageValid(mainTex.image)) {if (GuiButton((Rectangle){imgPreview.x + imgPreview.width/2.0f - BUTTON_WIDTH/2.0f, imgPreview.y + imgPreview.height + 20, BUTTON_WIDTH, BUTTON_HEIGHT }, "#002# Save as Image")) QueryAndSaveSelectAsImage(cropRect, mainTex.image);}
        DrawRectangleRec(selection.rect, SELECT_COLOR);
        DrawRectangleLinesEx(selection.rect, 1, BLACK);
        DrawThirdsLines(&selection.rect);
        DrawText(TextFormat("%f, %f", selection.center.x, selection.center.y), GetScreenWidth()-500, GetScreenHeight()/2 + 200, 20, BLACK);


        if (IsWindowResized()){
            // screenCenter = (Vector2){GetScreenWidth()/2.0f, GetScreenHeight()/2.0f};
        }
        EndDrawing();
    }
    
    UnloadImage(mainTex.image);
    UnloadTexture(mainTex.texture);
    CloseWindow();
    return 0;

}
