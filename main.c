#include <stdio.h>
#include <stdbool.h>
#include "foreign/raylib.h"

#include "foreign/tinyfiledialogs.h"

#define SMANIP_IMPLEMENTATION
#include "include/smanip.h"

#define RAYGUI_IMPLEMENTATION
#include "foreign/raygui.h"

#define SV_FORMAT(sv) TextFormat(SV_FMT, SV_ARG(sv))

#define PROGRAM_NAME "Image Cropper"


static const int START_WINDOW_WIDTH = 1920/2;
static const int START_WINDOW_HEIGHT = 1080/2;
static const int IMG_MAX_WIDTH = 1280;
static const int IMG_MAX_HEIGHT = 720;
static const int IMG_X_OFFSET = 20;
static const int FPS = 60;
static const int PADDING = 5;
static const int TEXT_BOX_W_PADDING = 5;
static const int TEXT_BOX_H_PADDING = 8;

static const Color BG_COLOR = {0x18, 0x18, 0x18, 0xFF};
static const Color VIEW_COLOR = {0x10, 0x10, 0x10, 0xFF};
static const Color TXT_SEL_COLOR = {0, 120, 215, 150};

#define BUTTON_WIDTH 120
#define BUTTON_HEIGHT 30
#define BUTTON_PADDING 10
// #define SELECT_WIDTH 225
// #define SELECT_HEIGHT 300
// #define SELECT_RES ((float)SELECT_WIDTH / SELECT_HEIGHT)

#define MAX_INPUT_CHARS 100


float globalScale = 1.0f;
bool debug_info = true;


Color SELECT_COLOR = {100, 100, 100, 150};
Color FADED_BLACK = {50, 50, 50, 100};

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


// int WINDOW_RESOLUTION = WINDOW_WIDTH/WINDOW_HEIGHT;

typedef struct {
    Rectangle text_box;

    Font font;

    String_Builder str;

    int font_sp;
    int font_size;

    int mouse_cursor;

    int backspace_frames;


    int sel_start;
    int sel_end;
    bool sel_update;
    int click_frames;

    bool hovering;
    bool writable;
    int blink_frames;
} WritableTextBox;

typedef struct {
    Image image;
    Texture2D texture;
    Rectangle as_rect;

    Vector2 texture_pos;
    Vector2 center;

    float scale;
    float target_scale;

    bool attached;
    Vector2 attach_offset;
} TextureView;

typedef struct {
    int width;
    int height;

    Rectangle rect;
    Vector2 center;

    float scale;
    float target_scale;

    bool attached;
    Vector2 attach_offset;
} SelectionRect;

int GetNumFromWBox(WritableTextBox wtxtbx) {

    String_View sv = sb_to_sv(&wtxtbx.str);
    return (int)strtol(sv.data, NULL, 10);

}

float aspect_ratio(Vector2 obj){
    return obj.x / obj.y;
}

void force_rect_inside_view(SelectionRect *sel, Rectangle rect){
    if (sel->rect.x <= rect.x) {sel->rect.x = rect.x;}
    if ((sel->rect.x + sel->rect.width) >= (rect.x + rect.width)) {sel->rect.x = rect.x + rect.width - sel->rect.width;}
    if (sel->rect.y <= rect.y) {sel->rect.y = rect.y;}
    if ((sel->rect.y + sel->rect.height) >= (rect.y + rect.height)){sel->rect.y = rect.y + rect.height - sel->rect.height;}
    sel->center.x = sel->rect.x + sel->rect.width/2.0f;
    sel->center.y = sel->rect.y + sel->rect.height/2.0f;
}
bool is_inside_img(Vector2 pos, Rectangle rect){
    if ((pos.x < rect.x) ||
       (pos.x > (rect.x + rect.width)) ||
       (pos.y < rect.y) ||
       (pos.y > (rect.y + rect.height))){
        return false;
        }
    return true;
}

bool is_img_fit_boundary(Image img){
    if (img.width <= IMG_MAX_WIDTH && img.height <= IMG_MAX_HEIGHT)
        return true;
    else{
        return false;
    }
}

void img_resize_to_boundary(Image *img){
    int w = img->width;
    int h = img->height;
    float img_res = (float)w/h;
    if (IMG_MAX_HEIGHT*img_res <= IMG_MAX_WIDTH){
        ImageResize(img, IMG_MAX_HEIGHT*img_res, IMG_MAX_HEIGHT);
    }else{
        ImageResize(img, (int)IMG_MAX_WIDTH, (int)IMG_MAX_WIDTH/img_res);

    }
}

Image load_img_from_file(const char *fileName) {
    Image img = LoadImage(fileName);
    if (!IsImageValid(img)) {
        fprintf(stderr, "Image has failed to load");
        UnloadImage(img);
        return (Image){0};
    }
    if (!is_img_fit_boundary(img)){
        printf("-------------------------------\n");
        printf("Real img size = (%d, %d)\n", img.width, img.height);
        img_resize_to_boundary(&img);
        printf("New img size = (%d, %d)\n", img.width, img.height);
        printf("-------------------------------\n");
    }
    return img;
}

Image query_and_load_img_from_file() {
    const char *filters[] = {"*.png", "*.jpg"};
    const char *fileName = tinyfd_openFileDialog("Open file:", "./", 2, filters, "Image files", 0);

    printf("-------------------------------\n");
    if (fileName) {printf("Path: %s\n", fileName);}
    printf("-------------------------------\n");

    return load_img_from_file(fileName);

}


bool save_select_as_img(TextureView texV, Rectangle crop_rect, const char* fileName, int width, int height){

    RenderTexture2D rendered = LoadRenderTexture(width, height);
    BeginTextureMode(rendered);
    ClearBackground(BLANK);
    Rectangle img_prev = {0, 0, width, height};

    DrawTexturePro(texV.texture, crop_rect, img_prev, (Vector2){0, 0}, 0, WHITE);

    EndTextureMode();
    Image img = LoadImageFromTexture(rendered.texture);
    ImageFlipVertical(&img);

    if (!ExportImage(img, fileName)) {
            fprintf(stderr, "MERROR: Could not save image!\n");
            UnloadImage(img);
            return false;
        }
    return true;
}

bool query_and_save_select_as_img(TextureView texV, Rectangle crop_rect, int width, int height){

    const char *filters[] = {"*.png", "*.jpg"};
    const char *fileName = tinyfd_saveFileDialog("Save image to:", "./", ARR_SIZE(filters), filters, NULL);
    if (!fileName) {return false;}

    if (!save_select_as_img(texV, crop_rect, fileName, width, height)) {return false;}
    return true;

}


void draw_thirds_lines(Rectangle *rect){
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

void draw_texture_centered(Texture2D texture, Vector2 centerPos, Color tint){
    float x = centerPos.x - (texture.width/2.0f);
    float y = centerPos.y - (texture.height/2.0f);

    DrawTexture(texture, x, y, tint);
}

void scale_rect(Rectangle *rect, float scale){
    rect->height *= scale;
    rect->width *= scale;
}


void setup_texture(TextureView *texV){
    texV->scale = 1.0f;
    texV->target_scale = 1.0f;
    texV->attached = false;

    float center_y = GetScreenHeight() / 2.0f;
    texV->texture_pos = (Vector2){IMG_X_OFFSET, center_y - texV->texture.height/2.0f};
    texV->center = (Vector2){
        texV->texture_pos.x + texV->texture.width/2.0f,
        texV->texture_pos.y + texV->texture.height/2.0f,

    };
    texV->as_rect = (Rectangle){
        texV->texture_pos.x,
        texV->texture_pos.y,
        texV->texture.width,
        texV->texture.height,
    };

}

WritableTextBox setup_wtxtbx(const Rectangle text_box_rect, char *def_str, Font font){
    WritableTextBox wtxtbx = {0};
    wtxtbx.text_box = text_box_rect; 
    sb_append_cstr(&wtxtbx.str, def_str);

    wtxtbx.font = font;
    wtxtbx.font_sp = 1; // FONT SPACING
    wtxtbx.sel_start = wtxtbx.str.count; // SETTING THIS FOR NOW TO WORK WITH CTRL + LEFT AND CTRL + RIGHT
    wtxtbx.sel_end = wtxtbx.str.count;
    return wtxtbx;
}


void DrawWritableTextBox(WritableTextBox *wtxtbx) {
    wtxtbx->font_size = wtxtbx->text_box.height - 10;
    if (CheckCollisionPointRec(GetMousePosition(), wtxtbx->text_box)) {
        wtxtbx->hovering = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {wtxtbx->writable = true;}
    } else {
        wtxtbx->hovering = false;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {wtxtbx->writable = false;}
    }
    if (IsKeyPressed(KEY_ENTER)) {wtxtbx->writable = false;} // Leave if pressed enter

    if (wtxtbx->writable){
        int key; 
        while ((key = GetCharPressed()) > 0) { 
            if ('0' <= key && key <= '9') {
                sb_append(&wtxtbx->str, (char)key);
                char cstr[1024];
                sb_to_cstr(wtxtbx->str, cstr);
                Vector2 next_width = MeasureTextEx(wtxtbx->font, cstr, wtxtbx->font.baseSize, wtxtbx->font_sp);
                if (next_width.x >= (wtxtbx->text_box.width - TEXT_BOX_W_PADDING)) {
                    sb_pop(&wtxtbx->str);
                }
            }

        }
        if (IsKeyPressed(KEY_BACKSPACE)){ // Erase char
            if (wtxtbx->str.count != 0) {
                    sb_pop(&wtxtbx->str);
                }
        }
        if (IsKeyDown(KEY_BACKSPACE)) { // Hold backspace behavior
            wtxtbx->backspace_frames++;
            if (wtxtbx->backspace_frames > 40 && wtxtbx->backspace_frames % 5 == 0) {
                if (wtxtbx->str.count != 0) {
                        sb_pop(&wtxtbx->str);
                    }
            }

        } else {
            wtxtbx->backspace_frames = 0;

        }
    }
    DrawRectangleRec(wtxtbx->text_box, WHITE);
    int tx = (int)wtxtbx->text_box.x;
    int ty = (int)wtxtbx->text_box.y;
    String_View sv = sb_to_sv(&wtxtbx->str);
    DrawTextEx(wtxtbx->font,
               TextFormat(SV_FMT, SV_ARG(sv)),
               (Vector2) {tx + TEXT_BOX_W_PADDING, ty},
               wtxtbx->font.baseSize,
               wtxtbx->font_sp,
               MAROON
    );
    if (wtxtbx->writable) {wtxtbx->blink_frames++;} else {wtxtbx->blink_frames=0;}
    if ((((wtxtbx->blink_frames / 30) + 1) % 2) == 0) {
         Vector2 blink_start = {
            tx 
            + 2*TEXT_BOX_W_PADDING 
            + MeasureTextEx(wtxtbx->font,
                            TextFormat(SV_FMT, SV_ARG(sv)),
                            wtxtbx->font.baseSize,
                            wtxtbx->font_sp).x,
            ty + TEXT_BOX_H_PADDING/2.0f
        };

        DrawLineEx(
            blink_start,
            (Vector2){blink_start.x,
            blink_start.y + wtxtbx->text_box.height - TEXT_BOX_H_PADDING},
            3.0f,
            MAROON
        );
    }
}


void DrawSelectionOnWritableTextBox(WritableTextBox *wtxtbx) {    
    // TODO: Add some sort of way to not have to compute this every frame
    //
    // if (!wtxtbx->sel_update) {return;}
    // wtxtbx->sel_update = false;
    // if (end > wtxtbx->letter_count) {
    //     if (debug_info) fprintf(stderr, "`end` (%d) should be bigger than the length of the string\n", end);
    //     return;
    // }
    // if (start > end) {
    //     if (debug_info) fprintf(stderr, "`end` (%d) should be bigger than `start` (%d)\n", end, start);
    //     return;
    // }
    if (wtxtbx->sel_start == wtxtbx->sel_end) {return;}
    // TODO: Cleanup this function, probably with sel_start and sel_end

    String_View before_select = sb_to_sv(&wtxtbx->str);
    String_View select = sb_to_sv(&wtxtbx->str);
    before_select = sv_strip_right(before_select, wtxtbx->sel_start);
    select = sv_strip_right(select, wtxtbx->sel_end);

    int xoff = MeasureTextEx(
        wtxtbx->font,
        SV_FORMAT(before_select),
        wtxtbx->font.baseSize,
        wtxtbx->font_sp
    ).x;

    int xwidth = MeasureTextEx(
        wtxtbx->font,
        SV_FORMAT(select),
        wtxtbx->font.baseSize,
        wtxtbx->font_sp
    ).x;

    DrawRectangle(
        wtxtbx->text_box.x + TEXT_BOX_W_PADDING - 2 + xoff, 
        wtxtbx->text_box.y + TEXT_BOX_H_PADDING,
        xwidth - xoff,
        wtxtbx->text_box.height - 2*TEXT_BOX_H_PADDING,
        TXT_SEL_COLOR
    );

}

void handle_selection(WritableTextBox *wtxtbx) {
    if (wtxtbx->writable) {
        if (IsKeyPressed(KEY_LEFT) && IsKeyDown(KEY_LEFT_CONTROL)){
            wtxtbx->sel_start -= 1;
            if (wtxtbx->sel_start < 0) {wtxtbx->sel_start = 0;}
        }
        if (IsKeyPressed(KEY_RIGHT) && IsKeyDown(KEY_LEFT_CONTROL)){
            wtxtbx->sel_start += 1;
            if (wtxtbx->sel_start > (int)wtxtbx->str.count) {wtxtbx->sel_start = (int)wtxtbx->str.count;}
        }
        if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_CONTROL)){
            wtxtbx->sel_start = 0;
            wtxtbx->sel_end = (int)wtxtbx->str.count;
        }
    }
    DrawSelectionOnWritableTextBox(wtxtbx);
    // TODO: Implement double clicking to select all using `click_frames`
    if ((IsKeyPressed(KEY_C) && IsKeyDown(KEY_LEFT_CONTROL) && wtxtbx->writable)){
        String_View clip = sb_to_sv(&wtxtbx->str);
        clip = sv_strip_sides(clip, wtxtbx->sel_start, wtxtbx->sel_end);
        SetClipboardText(TextFormat(SV_FMT, SV_ARG(clip)));
    }

}



#define DRAW_DEBUG(fmt, ...) \
    DrawText(TextFormat(fmt, ##__VA_ARGS__),\
            GetScreenWidth()-500,\
            GetScreenHeight()/2 + 160 + (dbg_line++)*20,\
            20, WHITE);

void show_debug(TextureView *mt, SelectionRect *sel) {
    Vector2 mouse_pos = GetMousePosition();
    int dbg_line = 0;
    DRAW_DEBUG("moup: x=%.2f, y=%.2f", mouse_pos.x, mouse_pos.y);
    DRAW_DEBUG("mt.tp: x=%.2f, y=%.2f", mt->texture_pos.x, mt->texture_pos.y);
    DRAW_DEBUG("mt.ao: x=%.2f, y=%.2f", mt->attach_offset.x, mt->attach_offset.y);
    DRAW_DEBUG("mt.a: %b", mt->attached);
    DRAW_DEBUG("mt.sc: %f", mt->target_scale);
    DRAW_DEBUG("selsize: w=%.2f h=%.2f", sel->rect.width, sel->rect.height);
} 

int main(int argc, char* argv[]) { // Config and start

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(START_WINDOW_WIDTH, START_WINDOW_HEIGHT, PROGRAM_NAME);
    MaximizeWindow();
    //testing
    
    // Load font
    Font main_font = LoadFontEx("./fonts/Inter-VariableFont_opsz,wght.ttf", 40, 0, 250);

    // Setting up the main view rectangle
    float center_y = GetScreenHeight()/2.0f;
    Rectangle view_rect = {
        IMG_X_OFFSET - PADDING,
        center_y - IMG_MAX_HEIGHT/2.0f - PADDING,
        IMG_MAX_WIDTH + 2*PADDING,
        IMG_MAX_HEIGHT + 2*PADDING
        };
    
    TextureView main_tex = {0};
    SelectionRect selection = {0};
    if (argc > 1){
        char *arg_file_name = argv[1];
        main_tex.image = load_img_from_file(arg_file_name);
        main_tex.texture = LoadTextureFromImage(main_tex.image);

        selection.target_scale = 1.0f;
    }

    // Selection rect settings
    selection.rect =  (Rectangle){0, 0, 225, 300};
    selection.center = (Vector2){
        selection.rect.x + selection.rect.width/2.0f,
        selection.rect.y + selection.rect.height/2.0f,
        };
    selection.scale = 1.0f;
    selection.target_scale = 1.0f;
    selection.attached = false;

    bool mouse_is_hovering = false; // If mouse is hovering any text box
    bool is_writing = false;        // If we are writing in any box 

    WritableTextBox width_wtxtbx = setup_wtxtbx((Rectangle){GetScreenWidth()-350, 500, 100, 40}, "225", main_font);
    selection.width = GetNumFromWBox(width_wtxtbx);

    WritableTextBox height_wtxtbx = setup_wtxtbx((Rectangle){GetScreenWidth()-350, 550, 100, 40}, "300", main_font);
    selection.height = GetNumFromWBox(height_wtxtbx);
    
    setup_texture(&main_tex);
    SetTargetFPS(FPS);
    while (!WindowShouldClose()) { // MAIN LOOP START

        BeginDrawing();
        ClearBackground(BG_COLOR);
        // Draw the main view Rectangle
        DrawRectangleRec(view_rect, VIEW_COLOR);
        

        // Selection rectangle logic
        // Using keys:
        if (!width_wtxtbx.writable && IsKeyUp(KEY_LEFT_CONTROL)) {
            int key_offset;
            if (IsKeyDown(KEY_LEFT_SHIFT)){key_offset = 10;}else{key_offset = 1;}

            if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {selection.center.x += key_offset;} 
            if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))  {selection.center.x -= key_offset;}
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))  {selection.center.y += key_offset;} 
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))    {selection.center.y -= key_offset;} 
        }

        float dt = GetFrameTime();
        float wheel = GetMouseWheelMove();
        Vector2 mouse_pos = GetMousePosition();


        if (IsKeyDown(KEY_LEFT_CONTROL) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)){ // Moving image mode
            selection.attached = false;

            main_tex.target_scale += wheel * 0.1f * main_tex.target_scale;
            if (main_tex.target_scale < 0.1f) {main_tex.target_scale = 0.1f;}
            main_tex.scale += (main_tex.target_scale - main_tex.scale) * 20.0f * dt;

            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)){
                if (CheckCollisionPointRec(mouse_pos, view_rect)){
                    if (!main_tex.attached) {
                        main_tex.attached = true;
                        main_tex.attach_offset.x = (mouse_pos.x - main_tex.texture_pos.x)/main_tex.target_scale;
                        main_tex.attach_offset.y = (mouse_pos.y - main_tex.texture_pos.y)/main_tex.target_scale;
                    }
                }
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {main_tex.attached = false;}
            if (main_tex.attached) {
                main_tex.texture_pos.x = mouse_pos.x - (main_tex.attach_offset.x)*main_tex.target_scale;
                main_tex.texture_pos.y = mouse_pos.y - (main_tex.attach_offset.y)*main_tex.target_scale;
            }
            
        } else { // Moving selection mode
            main_tex.attached = false;
            // Using mouse:
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON)){
                        // if not on rect, move it to mouse
                    if (!CheckCollisionPointRec(mouse_pos, selection.rect)){
                        selection.center = mouse_pos;
                    } else {
                    // if on rect, anchor to mouse
                    if (!selection.attached){
                        selection.attached = true;
                        selection.attach_offset.x = mouse_pos.x - selection.center.x;
                        selection.attach_offset.y = mouse_pos.y - selection.center.y;
                    }
                }
            }

            // Stop anchoring
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)){selection.attached = false;}
            if (!is_inside_img(mouse_pos, view_rect)){
                selection.center.x = selection.rect.x + selection.rect.width/2.0f;
                selection.center.y = selection.rect.y + selection.rect.height/2.0f;

            }
            if (selection.attached){
                selection.center.x = mouse_pos.x - selection.attach_offset.x;
                selection.center.y = mouse_pos.y - selection.attach_offset.y;
            }


            selection.target_scale += wheel * 0.1f;

            float sel_max_scale_x = view_rect.width / selection.width;
            float sel_max_scale_y = view_rect.height / selection.height;
            float max_scale = (sel_max_scale_x < sel_max_scale_y) ? sel_max_scale_x : sel_max_scale_y;
            if (selection.target_scale > max_scale){
            selection.target_scale = max_scale;
            }

            if (selection.target_scale < 0.1f){
                selection.target_scale = 0.1f;
            }

        };
        if (IsKeyPressed(KEY_F5)) {setup_texture(&main_tex);}
        selection.scale += (selection.target_scale - selection.scale) * 20.0f * dt;

        selection.rect.width = selection.width * selection.scale;
        selection.rect.height = selection.height * selection.scale;
        selection.rect.x = selection.center.x - selection.rect.width/2.0f;
        selection.rect.y = selection.center.y - selection.rect.height/2.0f;

        force_rect_inside_view(&selection, view_rect);

        Rectangle crop_rect = {
            (selection.rect.x - main_tex.texture_pos.x)/main_tex.target_scale,
            (selection.rect.y - main_tex.texture_pos.y)/main_tex.target_scale,
            selection.rect.width/main_tex.target_scale,
            selection.rect.height/main_tex.target_scale
        };




        // Draw stuff
        
        // OPEN IMAGE BUTTON
        if (GuiButton((Rectangle){100, GetScreenHeight() - 100, BUTTON_WIDTH, BUTTON_HEIGHT}, "#012# Open image")) { 
            main_tex.image = query_and_load_img_from_file();
            main_tex.texture = LoadTextureFromImage(main_tex.image);
            setup_texture(&main_tex);
            selection.target_scale = 1.0f;
        }
        // Refresh image button
        if (GuiButton((Rectangle){100 + BUTTON_WIDTH + BUTTON_PADDING, GetScreenHeight() - 100, BUTTON_WIDTH, BUTTON_HEIGHT}, "#012# Reset position")) {setup_texture(&main_tex);}

        Rectangle img_preview = {GetScreenWidth() - 410, 100, selection.width, selection.height};
        // SAVE IMAGE BUTTON
        if (IsImageValid(main_tex.image)) {if (GuiButton((Rectangle){img_preview.x + img_preview.width/2.0f - BUTTON_WIDTH/2.0f, img_preview.y + img_preview.height + 20, BUTTON_WIDTH, BUTTON_HEIGHT }, "#002# Save as Image")) query_and_save_select_as_img(main_tex, crop_rect, selection.width, selection.height);}

        // Rectangle stuff

        BeginScissorMode(view_rect.x, view_rect.y, view_rect.width, view_rect.height);
        DrawTextureEx(main_tex.texture, main_tex.texture_pos, 0.0f, main_tex.target_scale, WHITE); // MAIN IMAGE
        EndScissorMode();

        DrawRectangleLinesEx(view_rect, 2, RED);

        DrawTexturePro(main_tex.texture, crop_rect, img_preview, (Vector2){0, 0}, 0, WHITE);





        DrawRectangleRec(selection.rect, SELECT_COLOR);
        DrawRectangleLinesEx(selection.rect, 1, BLACK);
        draw_thirds_lines(&selection.rect);

        DrawWritableTextBox(&width_wtxtbx);
        DrawWritableTextBox(&height_wtxtbx);
        handle_selection(&width_wtxtbx);
        handle_selection(&height_wtxtbx);
        
        if (IsKeyPressed(KEY_ENTER)){
            selection.width = GetNumFromWBox(width_wtxtbx);
            selection.height = GetNumFromWBox(height_wtxtbx);
        }
        mouse_is_hovering = false;
        mouse_is_hovering |= width_wtxtbx.hovering;
        mouse_is_hovering |= height_wtxtbx.hovering;
        SetMouseCursor(mouse_is_hovering ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);
        is_writing = false;
        is_writing |= width_wtxtbx.writable;
        is_writing |= height_wtxtbx.writable;
        if (IsKeyPressed(KEY_J) && !is_writing) {debug_info = !debug_info;}
        if (IsKeyPressed(KEY_Q) && !width_wtxtbx.writable) {CloseWindow();}

        
        if (debug_info) {show_debug(&main_tex, &selection);}

        if (IsWindowResized()){
            // screenCenter = (Vector2){GetScreenWidth()/2.0f, GetScreenHeight()/2.0f};
        }
        EndDrawing();
    }
    
    UnloadImage(main_tex.image);
    UnloadTexture(main_tex.texture);
    CloseWindow();
    return 0;

}
