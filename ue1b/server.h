#pragma once

#include <stdio.h>

void send400(int fd, FILE *sockfile);
void send404(int fd, FILE *sockfile);
void send501(int fd, FILE *sockfile);
void drainBuffer(int fd, FILE *sockfile);