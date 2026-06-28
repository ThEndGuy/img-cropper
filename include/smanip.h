#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#define SB_INIT_CAP 16
#define SB_MIN_CAP 8

typedef struct {
    char *items;
    size_t count;
    size_t cap;
} String_Builder;


typedef struct {
    const char *data;
    size_t len;
} String_View;

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int)sv.len, sv.data

void sb_manage_memory(String_Builder *sb, size_t exp_mem);
void sb_append(String_Builder *sb, char c);
char sb_pop(String_Builder *sb);
void sb_to_cstr(String_Builder sb, char *out);
void sb_printf(String_Builder sb);
void sb_append_cstr(String_Builder *sb, char *str);
String_View sb_to_sv(String_Builder *sb);
String_View sv_strip_right(String_View sv, size_t n) ;
String_View sv_strip_left(String_View sv, size_t n) ;
String_View sv_strip_sides(String_View sv, size_t start, size_t end);

#ifdef SMANIP_IMPLEMENTATION

// STRING BUILD STUFF
void sb_manage_memory(String_Builder *sb, size_t exp_mem){
    if (exp_mem >= sb->cap) {
        if (sb->cap == 0) {
            sb->cap = SB_INIT_CAP;
            sb->items = malloc(sb->cap * sizeof(*sb->items));
        } else {
            sb->cap *= 2;
            printf("[INFO] Doubled size of string to %lld\n", sb->cap);
            sb->items = realloc(sb->items, sb->cap* sizeof(*sb->items));
        }
    } else if ((exp_mem < sb->cap/4) && (exp_mem > SB_MIN_CAP)) {
        sb->cap /= 2;
        printf("[INFO] Halfed size of string to %lld\n", sb->cap);
        sb->items = realloc(sb->items, sb->cap* sizeof(*sb->items));
    }
}


void sb_append(String_Builder *sb, char c){
    sb_manage_memory(sb, sb->count+1);
    sb->items[sb->count] = c;
    sb->count += 1;
}

char sb_pop(String_Builder *sb) {
    if (sb->count == 0) {
        fprintf(stderr, "[ERROR] Tried to pop from empty string");
        exit(1);
    }
    char popped = sb->items[sb->count - 1];
    sb_manage_memory(sb, sb->count - 1); 
    sb->count -= 1;
    return popped;
    
}

void sb_to_cstr(String_Builder sb, char *out){
    for (size_t i = 0; i < sb.count; i++){
        out[i] = sb.items[i];
    }
    out[sb.count] = '\0';
}

void sb_printf(String_Builder sb){
    char str[sb.count + 1];
    for (size_t i = 0; i < sb.count; i++){
        str[i] = sb.items[i];
    }
    str[sb.count] = '\0';

    printf("%s\n", str);
}

void sb_append_cstr(String_Builder *sb, char *str){
    for (size_t i = 0; i < strlen(str); i++) {
        sb_append(sb, str[i]);
    }
}

String_View sb_to_sv(String_Builder *sb){
    String_View sv;
    sv.data = sb->items;
    sv.len = sb->count;
    return sv;
}

// STRING VIEW STUFF

String_View sv_strip_right(String_View sv, size_t n) {
    String_View newsv;
    newsv.data = sv.data;
    newsv.len = sv.len - n;
    return newsv;
}

String_View sv_strip_left(String_View sv, size_t n) {
    String_View newsv;
    newsv.data = sv.data + n;
    newsv.len = sv.len - n;
    return newsv;
}

String_View sv_strip_sides(String_View sv, size_t start, size_t end){
    String_View newsv;
    newsv.data = sv.data + start;
    newsv.len = end - start;
    return newsv;
}

#endif // SMANIP_IMPLEMENTATION
