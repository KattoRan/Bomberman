/* client/handlers/session.h */
#ifndef SESSION_H
#define SESSION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Session file path
extern char session_file_path[256];

// Functions
void determine_session_path(int argc, char *argv[]);
void save_session_token(const char *token);
int load_session_token(char *buffer);
void clear_session_token();

#endif