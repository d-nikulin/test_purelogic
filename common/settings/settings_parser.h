// (C) 2020, Никулин Д.А., dan-gubkin@mail.ru

#ifndef SETTINGS_PARSER_H_
#define SETTINGS_PARSER_H_


#include <stdbool.h>


typedef struct{
    void* (*parse)(const char* str);     // parse serialized settings
    char* (*to_string)(const void* obj);  // serialize settings
    void* (*get)(const void* obj, const char* key);  // returns complex value of a key (array or document)
    const char* (*get_string)(const void* obj, const char* key); // returns string value
    long (*get_long)(const void* obj, const char* key, bool* found); // returns long value
    bool (*get_bool)(const void* obj, const char* key, bool* found); // // returns bool value
    void (*free_string)(char*); // free returns to_string
    void (*free)(void*); // clears returns parse && get_object
}SettingsParser;


SettingsParser* create_settings_parser();


void free_settings_parser();


#endif

