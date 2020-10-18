// (C) 2020, d.nikulin@sk-shd.ru

#include "common/json/bson_parser.h"


// Disable 'sign-conversion' warning produced by included header because this
// kind of warning is not allowed with project configuration (treated as an error)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <bson.h>
#pragma GCC diagnostic pop


#include <stdbool.h>
#include <stddef.h>


void* parse_bson(const char* json){
    bson_error_t berr;
    void* bson = bson_new_from_json((const uint8_t*)json, -1, &berr);
    if (!bson){
        printf("Error parsing json: %s\n", berr.message);
        //printf("json: %s\n", json);
    }
    return bson;
}


char* to_string_bson(const void* obj){
    if (!obj){
        return NULL;
    }
    return bson_as_json( (const bson_t*)obj, NULL);
}

/*
 * возвращает значене ключа, которое является сложной/составной сущностью
 */
void* get_bson(const void* obj, const char* key){
    const bson_t* bdoc = (const bson_t*) obj;
    bson_iter_t iter;
    if (!bson_iter_init (&iter, bdoc)) {
        return NULL;
    }
    if (!bson_iter_find (&iter, key)) {
        return NULL;
    }
    const bson_value_t* val = bson_iter_value(&iter);
    if ( !val ){
        return NULL;
    }
    if (val->value_type==BSON_TYPE_DOCUMENT ){
        uint32_t len = 0;
        const uint8_t *data = NULL;
        bson_iter_document(&iter, &len, &data);
        return bson_new_from_data(data, len);
    }else if ( val->value_type==BSON_TYPE_ARRAY ){
        uint32_t len = 0;
        const uint8_t *data = NULL;
        bson_iter_array(&iter, &len, &data);
        return bson_new_from_data(data, len);

    }
    return NULL;
}


/*
 * возвращает скалярное значение ключа
 */
const char* get_string_bson(const void* obj, const char* key){
    const bson_t* bdoc = (const bson_t*) obj;
    bson_iter_t iter;
    if (!bson_iter_init (&iter, bdoc)) {
        return NULL;
    }
    if (!bson_iter_find (&iter, key)) {
        return NULL;
    }
    const bson_value_t* val = bson_iter_value(&iter);
    if (val && val->value_type==BSON_TYPE_UTF8){
        return val->value.v_utf8.str;
    }
    return NULL;
}


long get_long_bson(const void* obj, const char* key, bool* found){
    const bson_t* bdoc = (const bson_t*) obj;
    bson_iter_t iter;
    if (found){
        *found = false;
    }
    if (!bson_iter_init (&iter, bdoc)) {
        return 0;
    }
    if (!bson_iter_find (&iter, key)) {
        return 0;
    }
    const bson_value_t* val = bson_iter_value(&iter);
    if ( !val ){
        return 0;
    }
    if (val->value_type==BSON_TYPE_INT32){
        if (found){
            *found = true;
        }
        return val->value.v_int32;
    }else if(val->value_type==BSON_TYPE_INT64){
        if (found){
            *found = true;
        }
        return val->value.v_int64;
    }
    return 0;
}


bool get_bool_bson(const void* obj, const char* key, bool* found){
    const bson_t* bdoc = (const bson_t*) obj;
    bson_iter_t iter;
    if (found){
        *found = false;
    }
    if (!bson_iter_init (&iter, bdoc)) {
        return false;
    }
    if (!bson_iter_find (&iter, key)) {
        return false;
    }
    const bson_value_t* val = bson_iter_value(&iter);
    if ( val && val->value_type==BSON_TYPE_BOOL){
        if (found){
            *found = true;
        }
        return val->value.v_bool;
    }
    return false;

}


void free_string_bson(char* json){
    bson_free( json );
}


void free_bson(void* obj){
    bson_destroy( (bson_t*) obj);
}

