#pragma once

#include <stdio.h>

void send400(int fd, FILE *sockfile);
void send404(int fd, FILE *sockfile);
void send501(int fd, FILE *sockfile);
void sendResponseStringOnly(int fd, FILE *sockfile, char *response_string);
void drainBuffer(int fd, FILE *sockfile);
void handle_signal(int signal);
void handle_signal_sigpipe(int signal);