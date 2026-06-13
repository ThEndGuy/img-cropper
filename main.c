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

float aspectRatio(Vector2 obj){
    return obj.x / obj.y;
}

void ForceRectInsideImg(Rectangle *rect, Vector2 *rectCent, Texture2D *tex, Vector2 pos){
    if (rect->x <= pos.x) {rect->x = pos.x;}
    if ((rect->x + rect->width) >= (pos.x + tex->width)) {rect->x = pos.x + tex->width - rect->width;}
    if (rect->y <= pos.y) {rect->y = pos.y;}
    if ((rect->y + rect->height) >= (pos.y + tex->height)){rect->y = pos.y + tex->height - rect->height;}
    rectCent->x = rect->x + rect->width/2.0f;
    rectCent->y = rect->y + rect->height/2.0f;
}
bool IsInsideImage(Vector2 pos, Texture2D *tex, Vector2 imgPos){
    if ((pos.x < imgPos.x) |
       (pos.x > (imgPos.x + tex->width)) |
       (pos.y < imgPos.y) |
       (pos.y > (imgPos.y + tex->height))){
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
    const char *filters[] = {"*.png", "*.jpg"};
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

    const char *filters[] = {"*.png", "*.jpg"};
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
    
    // Image mainImg = QueryAndLoadImageFromFile();
    Image mainImg = {0};
    Texture2D mainImgTex = LoadTextureFromImage(mainImg);


    // Selection rect settings
    Rectangle selectRect =  { 0, 0, 225, 350};
    Vector2 selectRectCenter = {
        selectRect.x + selectRect.width/2.0f,
        selectRect.y + selectRect.height/2.0f,
        };
    float selectRectScale = 1.0f;
    float selectRectTargetScale = 1.0f;
    Vector2 attachOffset = {0};
    bool selectRectAttached = false;
    //


    SetTargetFPS(FPS);
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Q)) {CloseWindow();}
        float centerY = GetScreenHeight() / 2.0f;
        Vector2 imagePos = {IMG_X_OFFSET, centerY - (mainImgTex.height/2.0f)};
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Draw image
        
        DrawTextureEx(mainImgTex, imagePos, 0.0f, globalScale, WHITE);

        // Selection rectangle logic
        // Using keys:
       
        int keyOffset;
        if (IsKeyDown(KEY_LEFT_SHIFT)){keyOffset = 10;}else{keyOffset = 1;}

        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {selectRectCenter.x += keyOffset;} 
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  {selectRectCenter.x -= keyOffset;}
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  {selectRectCenter.y += keyOffset;} 
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    {selectRectCenter.y -= keyOffset;} 

        // Using mouse:
        Vector2 mousePos = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
                    // if not on rect, move it to mouse
                if (!CheckCollisionPointRec(mousePos, selectRect)){
                    selectRectCenter = mousePos;
            } else {
                // if on rect, anchor to mouse
                if (!selectRectAttached){
                    selectRectAttached = true;
                    attachOffset.x = mousePos.x - selectRectCenter.x;
                    attachOffset.y = mousePos.y - selectRectCenter.y;
                }
            }
        }

        // Stop anchoring
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){selectRectAttached = false;}
        if (!IsInsideImage(mousePos, &mainImgTex, imagePos)){
            selectRectCenter.x = selectRect.x + selectRect.width/2.0f;
            selectRectCenter.y = selectRect.y + selectRect.height/2.0f;

        }
        if (selectRectAttached){
            selectRectCenter.x = mousePos.x - attachOffset.x;
            selectRectCenter.y = mousePos.y - attachOffset.y;
        }

        float wheel = GetMouseWheelMove();

        selectRectTargetScale += wheel * 0.1f;
        float dt = GetFrameTime();

        float maxScaleX = (float)mainImgTex.width  / SELECT_WIDTH;
        float maxScaleY = ((float)mainImgTex.height) / SELECT_HEIGHT;
        float maxScale = (maxScaleX < maxScaleY) ? maxScaleX : maxScaleY;
        if (selectRectTargetScale > maxScale){
        selectRectTargetScale = maxScale;
        }

        if (selectRectTargetScale < 0.1f){
            selectRectTargetScale = 0.1f;
        }

        selectRectScale += (selectRectTargetScale - selectRectScale) * 20.0f * dt;

        selectRect.width = SELECT_WIDTH * selectRectScale;
        selectRect.height = SELECT_HEIGHT * selectRectScale;
        selectRect.x = selectRectCenter.x - selectRect.width/2.0f;
        selectRect.y = selectRectCenter.y - selectRect.height/2.0f;
        if (selectRect.height > mainImgTex.height){selectRect.height = mainImgTex.height;}

        ForceRectInsideImg(&selectRect, &selectRectCenter, &mainImgTex, imagePos);

        Rectangle cropRect = {selectRect.x - imagePos.x, selectRect.y - imagePos.y, selectRect.width, selectRect.height};
        Rectangle imgPreview = {GetScreenWidth() - 410, 100, SELECT_WIDTH, SELECT_HEIGHT};
        DrawTexturePro(mainImgTex, cropRect, imgPreview, (Vector2){0, 0}, 0, WHITE);


        // Draw stuff
        // OPEN IMAGE BUTTON
        if (GuiButton((Rectangle){100, GetScreenHeight() - 100, BUTTON_WIDTH, BUTTON_HEIGHT}, "#012# Open image")) { 
            mainImg = QueryAndLoadImageFromFile();
            mainImgTex = LoadTextureFromImage(mainImg);
            selectRectTargetScale = 1.0f;
        }

        // SAVE IMAGE BUTTON
        if (IsImageValid(mainImg)) {if (GuiButton((Rectangle){imgPreview.x + imgPreview.width/2.0f - BUTTON_WIDTH/2.0f, imgPreview.y + imgPreview.height + 20, BUTTON_WIDTH, BUTTON_HEIGHT }, "#002# Save as Image")) QueryAndSaveSelectAsImage(cropRect, mainImg);}
        DrawRectangleRec(selectRect, SELECT_COLOR);
        DrawRectangleLinesEx(selectRect, 1, BLACK);
        DrawThirdsLines(&selectRect);
        DrawText(TextFormat("%f, %f", selectRectCenter.x, selectRectCenter.y), GetScreenWidth()-500, GetScreenHeight()/2 + 200, 20, BLACK);


        if (IsWindowResized()){
            // screenCenter = (Vector2){GetScreenWidth()/2.0f, GetScreenHeight()/2.0f};
        }
        EndDrawing();
    }
    
    UnloadImage(mainImg);
    UnloadTexture(mainImgTex);
    CloseWindow();
    return 0;

}
