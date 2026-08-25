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

static void unpack_bitfield_digits(int field, int bit_count, char *string, int offset) {
    int pos = 0;

    for (int i = 0; i < bit_count; i++) {
        if (field & (1 << i)) {
            if (pos > 0) {
                string[pos++] = ',';
            }

            string[pos++] = i + '0' + offset;
        }
    }

    string[pos] = '\0';
}

int get_search_results(char **out_data, int gameVer, SearchFilters f) {
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

        int pos = snprintf(data,
            sizeof(data) - 1, 
            "secret=Wmfd2893gb7&gameVersion=%d&type=%d&page=%d&original=%d&noStar=%d&star=%d&featured=%d", 
            gameVer, 
            f.searchType, 
            f.currentPage, 
            f.original, 
            f.noStar, 
            f.star,
            f.featured);

        pos += snprintf(data + pos, sizeof(data) - pos, "&str=%s", f.searchQuery);

        if(f.lengthFilters){
            char lengths[15] = "";
            unpack_bitfield_digits(f.lengthFilters, 5, lengths, 0);
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
                    unpack_bitfield_digits(f.difficultyFilters, 5, difficulties, 1);
                    pos += snprintf(data + pos, sizeof(data) - pos, "&demonFilter=%s", difficulties);
                }
            } else{
                char difficulties[16] = "";
                unpack_bitfield_digits(f.difficultyFilters, 5, difficulties, 1);
                pos += snprintf(data + pos, sizeof(data) - pos, "&diff=%s", difficulties);
            }
        }

        if(f.songFilter){
            if(f.customSong){
                pos += snprintf(data + pos, sizeof(data) - pos, "&customSong=%d", f.customSong);
                pos += snprintf(data + pos, sizeof(data) - pos, "&song=%s", f.customSongQuery);
            } else{
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