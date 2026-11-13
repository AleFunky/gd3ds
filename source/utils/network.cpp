#include <3ds.h>
#include "network.hpp"
#include "curl/system.h"
#include "main.hpp"
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <unistd.h>
#include "string_helpers.hpp"

#include <fcntl.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utils.hpp"
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

namespace gd3ds
{

static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

struct MemoryStruct {
    char *memory;
    size_t size;
};

static int cancelCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    GenericTask *task = (GenericTask *)clientp;

    if (task->cancelled) {
        return 1;
    }

    return 0;
}

size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL)
        return 0;  // out of memory

    mem->memory = ptr;

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int socInit() {
    int ret;

    // allocate buffer for SOC service
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

    if(SOC_buffer == NULL) {
        printf("memalign: failed to allocate\n");
    }

    // Now intialise soc:u service
    if ((ret = ::socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
        printf("socInit: 0x%08X\n", (unsigned int)ret);
    }
    return ret;
}

int getLevelFromID(GenericTask *task, char **outData, int id, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_LEVEL_API : ROBTOP_LEVEL_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        char data[64];
        snprintf(data, 63, "levelID=%d&secret=Wmfd2893gb7", id);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode code = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code) {
            return code;
        }

        printf("(code %d) Response (%d): %s\n", code, chunk.size, chunk.memory);

        if (chunk.memory[0] == '-') {
            return atoi(chunk.memory);
        }
        
        *outData = chunk.memory;

        return 0;
    }
    return 2;
}

static void unpackBitfieldDigits(int field, int bitCount, char *string, int offset) {
    int pos = 0;

    for (int i = 0; i < bitCount; i++) {
        if (field & (1 << i)) {
            if (pos > 0) {
                string[pos++] = ',';
            }

            string[pos++] = i + '0' + offset;
        }
    }

    string[pos] = '\0';
}

int getSearchResults(GenericTask *task, char **outData, int gameVer, SearchFilters f, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_SEARCH_API : ROBTOP_SEARCH_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        char data[512];

        int pos = snprintf(data,
            sizeof(data) - 1, 
            "secret=Wmfd2893gb7&gameVersion=%d&type=%d&page=%d&original=%d&noStar=%d&star=%d&featured=%d&epic=%d", 
            gameVer, 
            f.searchType, 
            f.currentPage, 
            f.original, 
            f.noStar, 
            f.star,
            f.featured,
            f.super);

        pos += snprintf(data + pos, sizeof(data) - pos, "&str=%s", f.searchQuery);

        if(f.lengthFilters){
            char lengths[15] = "";
            unpackBitfieldDigits(f.lengthFilters, 5, lengths, 0);
            pos += snprintf(data + pos, sizeof(data) - pos, "&len=%s", lengths);
        }

        if(f.difficultyFilters || f.isNA || f.isAuto || f.isDemon){
            if(f.isNA){
                pos += snprintf(data + pos, sizeof(data) - pos, "&diff=%d", -1);
            } else if(f.isAuto){
                pos += snprintf(data + pos, sizeof(data) - pos, "&diff=%d", -3);
            } else if(f.isDemon){
                pos += snprintf(data + pos, sizeof(data) - pos, "&diff=%d", -2);
                if(f.difficultyFilters){
                    char difficulties[16] = "";
                    unpackBitfieldDigits(f.difficultyFilters, 5, difficulties, 1);
                    pos += snprintf(data + pos, sizeof(data) - pos, "&demonFilter=%s", difficulties);
                }
            } else{
                char difficulties[16] = "";
                unpackBitfieldDigits(f.difficultyFilters, 5, difficulties, 1);
                pos += snprintf(data + pos, sizeof(data) - pos, "&diff=%s", difficulties);
            }
        }

        if(f.songFilter){
            if(f.customSong){
                pos += snprintf(data + pos, sizeof(data) - pos, "&customSong=%d", f.customSong);
                pos += snprintf(data + pos, sizeof(data) - pos, "&song=%s", f.customSongQuery);
            } else {
                pos += snprintf(data + pos, sizeof(data) - pos, "&song=%d", f.mainSong + 1);
            }
        }

        if(f.uncompleted || f.completed){
            pos += snprintf(data + pos, sizeof(data) - pos, "&onlyCompleted=%d&uncompleted=%d&completedLevels=(6508283,4454123,27732941)", f.completed, f.uncompleted);
        }

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode code = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code) {
            return code;
        }

        printf("(code %d) Response (%d): %s\n", code, chunk.size, chunk.memory);

        if (chunk.memory[0] == '-') {
            return atoi(chunk.memory);
        }
        
        *outData = chunk.memory;

        return 0;
    }
    return 2;
}

int getCommentsFromID(GenericTask *task, char **outData, int id, int page, int mode, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_COMMENTS_API : ROBTOP_COMMENTS_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        char data[128];
        snprintf(data, sizeof(data), "levelID=%d&page=%d&mode=%d&binaryVersion=49&secret=Wmfd2893gb7", id, page, mode);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode code = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code) {
            return code;
        }

        printf("(code %d) Response (%d): %s\n", code, chunk.size, chunk.memory);

        if (chunk.memory[0] == '-') {
            return atoi(chunk.memory);
        }
        
        *outData = chunk.memory;

        return 0;
    }
    return 2;
}

int getSongInfoFromID(GenericTask *task, char **outData, int songId, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_SONGS_API : ROBTOP_SONGS_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        char data[64];
        snprintf(data, sizeof(data), "songID=%d&secret=Wmfd2893gb7", songId);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        CURLcode code = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code) {
            return code;
        }

        printf("(code %d) Response (%d): <%s>\n", code, chunk.size, chunk.memory);

        if (chunk.memory[0] == '-') {
            return atoi(chunk.memory);
        }
        
        *outData = chunk.memory;

        return 0;
    }
    return 2;
}

static int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    DownloadTask *task = (DownloadTask *)clientp;
    float progress = 0;

    if (task->cancelled) {
        return 1;
    }

    u64 now = svcGetSystemTick();

    u64 elapsedTicks = now - task->lastTime;
    
    float time = ((elapsedTicks / CPU_TICKS_PER_MSEC));

    if (time >= 1000.f) {
        curl_off_t bytes = dlnow - task->lastBytes;

        float seconds = time / 1000.f;

        int speed = (int)(bytes / seconds);

        task->speed = speed;

        task->lastTime = now;
        task->lastBytes = dlnow;
    }

    if (dltotal > 0) {
        progress = (dlnow * 100.f) / dltotal;
    }

    task->progress = progress;
    return 0;
}

static int downloadFile(DownloadTask *task) {
    const char *path = task->path;
    const char *url = task->url;
    char *fileName = task->fileName;
    char *extension = task->extension;
    
    // Init
    CURL *curl = curl_easy_init();

    if (curl) {
        char *decodedURL = urlDecode(url);
        // url_convert_to_http(decoded_url);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.4.0");
        curl_easy_setopt(curl, CURLOPT_URL, decodedURL);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);

        char tmpFile[273];
        snprintf(tmpFile, sizeof(tmpFile), "%s%s.%s.tmp", path, fileName, extension);
        FILE* f = fopen(tmpFile, "wb");
        if (!f) {
            free(decodedURL);
            curl_easy_cleanup(curl);
            return -3;
        }
        
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

        CURLcode code = curl_easy_perform(curl);

        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        
        if (httpCode != 200) {
            remove(tmpFile);
            free(decodedURL);
            fclose(f);
            curl_easy_cleanup(curl);
            return -4;
        }

        if (code) {
            remove(tmpFile);
            free(decodedURL);
            fclose(f);
            curl_easy_cleanup(curl);
            return code;
        }

        free(decodedURL);
        fclose(f);
        curl_easy_cleanup(curl);
        
        char actualFile[273];
        snprintf(actualFile, sizeof(actualFile), "%s%s.%s", path, fileName, extension);
        rename(tmpFile, actualFile);

        return 0;
    }
    return -2;
}

static void downloadThread(void *arg) {
    DownloadTask *task = (DownloadTask *)arg;

    task->result = downloadFile(task);

    task->running = false;
    task->finished = true;
}

int getCommits(GenericTask *task, char **outData, const char *repoOwner, const char *repoName, const char *repoBranch) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        headers = curl_slist_append(headers, "X-GitHub-Api_Version: 2026-03-10");

        // github actually requires a useragent Whoops
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.4.0");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        char url[128];
        snprintf(url, sizeof(url) - 1, "https://api.github.com/repos/%s/%s/commits?sha=%s", repoOwner, repoName, repoBranch);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        CURLcode code = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code) {
            return code;
        }

        printf("(code %d) Response (%d): %s\n", code, chunk.size, chunk.memory);

        if (chunk.memory[0] == '-') {
            return atoi(chunk.memory);
        }
        
        *outData = chunk.memory;

        return 0;
    }
    return 2;
}

int getReleases(GenericTask *task, char **outData, const char *repoOwner, const char *repoName) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        headers = curl_slist_append(headers, "X-GitHub-Api_Version: 2026-03-10");

        // github requires a useragent 
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.4.0");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        
        char url[128];
        snprintf(url, sizeof(url) - 1, "https://api.github.com/repos/%s/%s/releases", repoOwner, repoName);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        CURLcode code = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code) {
            return code;
        }

        printf("(code %d) Response (%d): %s\n", code, chunk.size, chunk.memory);

        if (chunk.memory[0] == '-') {
            return atoi(chunk.memory);
        }
        
        *outData = chunk.memory;

        return 0;
    }
    return 2;
}

int getActions(GenericTask *task, char **outData, const char *repoOwner, const char *repoName, const char *actionName) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = (char *)malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        headers = curl_slist_append(headers, "X-GitHub-Api_Version: 2026-03-10");

        // github actually requires a useragent Whoops
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/8.4.0");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0l); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, TIMEOUT_DURATION);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        char url[128];
        snprintf(url, sizeof(url) - 1, "https://api.github.com/repos/%s/%s/actions/artifacts?name=%s", repoOwner, repoName, actionName);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        CURLcode code = curl_easy_perform(curl);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (code) {
            return code;
        }

        printf("(code %d) Response (%d): %s\n", code, chunk.size, chunk.memory);

        if (chunk.memory[0] == '-') {
            return atoi(chunk.memory);
        }
        
        *outData = chunk.memory;

        return 0;
    }
    return 2;
}

Thread createFileDownloadThread(DownloadTask *task) {
    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority += 1;
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority; 
    
    task->progress = 0;
    task->speed = 0;
    task->lastBytes = 0;
    task->lastTime = 0;
    task->cancelled = false;

    task->finished = false;
    task->running = true;
    
    return threadCreate(
        downloadThread,
        task,
        32 * 1024,
        priority,
        (isN3Ds ? 2 : 0),
        true
    );
}

void shutdownSoc() {
    close(sock);
    socExit();
    free(SOC_buffer);
}

}
