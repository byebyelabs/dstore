#pragma once

// Client function to set a key-value pair
// char *key: string key
// char *value: string value 
void dset(char *key, char *value);

// Client function to get a key-value pair at key
// char *key: string key
// char *value_out: save value output here 
// (remember to allocate enough space for value_out,
//  we reccomend MAX_VALUE_LENGTH from config.h)
void dget(char *key, char *value_out);

// Client function to delete key-value pair at key
// char *key: string key
// out of scope for the current lab
void ddel(char *key);
