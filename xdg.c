// Copyright (c) 2016 Roman Chistokhodov
// Copyright (c) 2025 Eduardo Meli
/*
This file is part of fex.

fex is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

fex is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with fex.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int findExecutable(const char* baseName, char* buf, size_t size)
{
    char* envPath;
    char* part;
    size_t length;
    size_t baseNameLength;
    size_t needTrailingSlash;
    
    if (!baseName || !baseName[0]) {
        return 0;
    }
    
    envPath = getenv("PATH");
    if (!envPath) {
        return 0;
    }
    
    baseNameLength = strlen(baseName);
    while(*envPath) {
        part = strchr(envPath, ':');
        if (part) {
            length = part - envPath;
        } else {
            length = strlen(envPath);
        }
        
        if (length > 0) {
            needTrailingSlash = (envPath[length-1] == '/') ? 0 : 1;
            if (length + baseNameLength + needTrailingSlash < size) {
                strncpy(buf, envPath, length);
                strncpy(buf + length, "/", needTrailingSlash);
                strncpy(buf + length + needTrailingSlash, baseName, baseNameLength);
                buf[length + needTrailingSlash + baseNameLength] = '\0';
                
                if (access(buf, X_OK) == 0) {
                    return 1;
                }
            }
        }
        
        envPath += length;
        if (*envPath == ':') {
            envPath++;
        }
    }
    return 0;
}

void execProcess(const char* executable, char* const args[])
{
    //close all file descriptors
    //Possible optimization: use poll and close only valid descriptors.
    struct rlimit r;
    if (getrlimit(RLIMIT_NOFILE, &r) != 0) {
        int maxDescriptors = (int)r.rlim_cur;
        int i;
        for (i=0; i<maxDescriptors; ++i) {
            close(i);
        }
    }
    
    //set standard streams to /dev/null
    int devNull = open("/dev/null", O_RDWR);
    if (devNull > 0) {
        dup2(devNull, STDIN_FILENO);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
    }
    close(devNull);
    
    //detach from process group of parent
    setsid();
    
    execv(executable, args);
    _exit(1);
}

int openFile(char* path)
{
    char xdgOpen[256];
    if (findExecutable("xdg-open", xdgOpen, sizeof(xdgOpen))) {
        char* argv[] = {xdgOpen, path, NULL};
        pid_t pid = fork();
        if (pid == 0) {
            pid_t doubleFork = fork();
            if (doubleFork == 0) {
                execProcess(xdgOpen, argv);
            } else if (doubleFork > 0) {
                _exit(0);
            } else {
                _exit(1);
            }
        } else if (pid < 0) {
            perror("Could not fork");
            return 1;
        } else {
            int status;
            //wait for the first fork
            waitpid(pid, &status, 0);
            return status;
        }
    } else {
        fprintf(stderr, "Could not find xdg-open utility\n");
        return 1;
    }
    return 0;
}
