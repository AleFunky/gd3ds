#include <3ds.h>
#include "network.h"
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

struct MemoryStruct {
    char *memory;
    size_t size;
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL)
        return 0;  // out of memory

    mem->memory = ptr;

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int soc_init() {
    int ret;

    // allocate buffer for SOC service
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

    if(SOC_buffer == NULL) {
        printf("memalign: failed to allocate\n");
    }

    // Now intialise soc:u service
    if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
        printf("socInit: 0x%08X\n", (unsigned int)ret);
    }
    return ret;
}

int get_level_from_id(char **out_data, int id) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, "http://www.boomlings.com/database/downloadGJLevel22.php");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");

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
        
        *out_data = chunk.memory;

        return 0;
    }
    return 2;
}

int get_search_results(char **out_data, int gameVer, int type, char *query, int page, bool uncompleted, bool onlyCompleted, char *completedList, bool featured, bool original, bool noStar, bool customSong, int customSongId) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, "http://www.boomlings.com/database/getGJLevels21.php");
        // curl_easy_setopt(curl, CURLOPT_URL, "https://19gdps.com/gdapi/getGJLevels21.php");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");

        char data[512];

        snprintf(data, sizeof(data) - 1, "secret=Wmfd2893gb7&gameVersion=%d&type=%d&str=%s&page=%d&uncompleted=%d&onlyCompleted=%d&completedLevels=%s&featured=%d&original=%d&noStar=%d&song=%d&customSong=%d", gameVer, type, query, page, uncompleted, onlyCompleted, completedList, featured, original, noStar, customSong, customSongId);
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
        
        *out_data = chunk.memory;

        return 0;
    }
    return 2;
}

int get_comments_from_id(char **out_data, int id, int page, int mode) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, "http://www.boomlings.com/database/getGJComments21.php");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");

        char data[64];
        snprintf(data, 63, "levelID=%d&page=%d&mode=%d&secret=Wmfd2893gb7", id, page, mode);
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
        
        *out_data = chunk.memory;

        return 0;
    }
    return 2;
}

void soc_exit() {
    close(sock);
    socExit();
}