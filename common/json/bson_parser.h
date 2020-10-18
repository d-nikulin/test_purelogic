// (C) 2020, d.nikulin@sk-shd.ru

#ifndef BSON_PARSER_H_
#define BSON_PARSER_H_


#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

void* parse_bson(const char* json);


char* to_string_bson(const void* obj);


/*
 * возвращает значене ключа, которое является сложной/составной сущностью
 */
void* get_bson(const void* obj, const char* key);


const char* get_string_bson(const void* obj, const char* key);


long get_long_bson(const void* obj, const char* key, bool* found);


bool get_bool_bson(const void* obj, const char* key, bool* found);


void free_string_bson(char* str);


void free_bson(void* obj);


#ifdef __cplusplus
}
#endif


#endif

