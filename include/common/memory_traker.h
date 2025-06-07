#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H
#include <iostream>
#include <cstdlib>
#include <sys/resource.h>

void measureMemory(const char* label) {
    static char buf[256];
    static long base = 0;
    
    // 获取当前进程内存信息
    pid_t pid = getpid();
    snprintf(buf, sizeof(buf), "/proc/%d/statm", pid);
    
    FILE* f = fopen(buf, "r");
    if (f) {
        long size, resident, shared, text, lib, data, dt;
        fscanf(f, "%ld %ld %ld %ld %ld %ld %ld", 
              &size, &resident, &shared, &text, &lib, &data, &dt);
        fclose(f);
        
        // 计算实际物理内存占用
        long page_size = sysconf(_SC_PAGE_SIZE);
        long current = resident * page_size;
        
        if (base == 0) {
            base = current;
        }
        
        printf("%s: %+ld KB (total: %ld KB)\n", 
               label, (current - base) / 1024, current / 1024);
    }
}

#endif// MEMORY_TRACKER_H