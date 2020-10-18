// (C) 2020, d.nikulin@sk-shd.ru

#include "common/json/bson_parser.h"
#include "common/settings/settings_parser.h"

#include <stdlib.h>


SettingsParser* create_settings_parser(){
    SettingsParser* jp = (SettingsParser*) calloc (1,  sizeof(SettingsParser) );
    if (!jp){
        return NULL;
    }
    jp->parse = parse_bson;
    jp->to_string = to_string_bson;
    jp->get = get_bson;
    jp->get_string = get_string_bson;
    jp->get_long = get_long_bson;
    jp->get_bool = get_bool_bson;
    jp->free_string = free_string_bson;
    jp->free = free_bson;
    return jp;
}


void free_settings_parser(SettingsParser* jp){
    free( jp );
}
