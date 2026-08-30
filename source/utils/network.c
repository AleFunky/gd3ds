#include <3ds.h>
#include "network.h"
#include "curl/system.h"
#include "main.h"
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <unistd.h>
#include "string_helpers.h"

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

static int cancelCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    NetworkTask *task = clientp;

    if (task->cancelled) {
        return 1;
    }

    return 0;
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
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

int get_level_from_id(NetworkTask *task, char **out_data, int id, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_LEVEL_API : ROBTOP_LEVEL_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/certs.pem");

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

int get_search_results(NetworkTask *task, char **out_data, int gameVer, SearchFilters f, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_SEARCH_API : ROBTOP_SEARCH_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/certs.pem");

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
        
        *out_data = chunk.memory;

        return 0;
    }
    return 2;
}

int get_comments_from_id(NetworkTask *task, char **out_data, int id, int page, int mode, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_COMMENTS_API : ROBTOP_COMMENTS_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/certs.pem");

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

int get_song_info_from_id(NetworkTask *task, char **out_data, int songId, bool useGdps) {
    // Init
    CURL *curl = curl_easy_init();
    struct curl_slist *headers = NULL;

    if (curl) {
        struct MemoryStruct chunk;
        chunk.memory = malloc(1);
        chunk.size = 0;
        headers = curl_slist_append(headers,
            "Content-Type: application/x-www-form-urlencoded");

        curl_easy_setopt(curl, CURLOPT_URL, useGdps ? GDPS_SONGS_API : ROBTOP_SONGS_API);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, cancelCallback);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/certs.pem");

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
        
        *out_data = chunk.memory;

        return 0;
    }
    return 2;
}

static int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    DownloadTask *task = clientp;
    float progress = 0;

    if (task->cancelled) {
        return 1;
    }

    u64 now = svcGetSystemTick();

    u64 elapsed_ticks = now - task->last_time;
    
    float time = ((elapsed_ticks / CPU_TICKS_PER_MSEC));

    if (time >= 1000.f) {
        curl_off_t bytes = dlnow - task->last_bytes;

        float seconds = time / 1000.f;

        int speed = (int)(bytes / seconds);

        task->speed = speed;

        task->last_time = now;
        task->last_bytes = dlnow;
    }

    if (dltotal > 0) {
        progress = (dlnow * 100.f) / dltotal;
    }

    task->progress = progress;
    return 0;
}

static int download_song(DownloadTask *task) {
    char *path = task->path;
    char *url = task->url;
    char *song_id = task->song_id;
    
    // Init
    CURL *curl = curl_easy_init();

    if (curl) {
        char *decoded_url = url_decode(url);
        // url_convert_to_http(decoded_url);

        curl_easy_setopt(curl, CURLOPT_URL, decoded_url);
        
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, task);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); // Enable progress data
        curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/certs.pem"); // Certificate slop

        char tmp_file[273];
        snprintf(tmp_file, sizeof(tmp_file), "%s/%s.tmp", path, song_id);
        FILE* f = fopen(tmp_file, "wb");
        if (!f) {
            free(decoded_url);
            return -3;
        }
        
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

        CURLcode code = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        if (http_code != 200) {
            remove(tmp_file);
            free(decoded_url);
            curl_easy_cleanup(curl);
            return -4;
        }

        if (code) {
            remove(tmp_file);
            free(decoded_url);
            curl_easy_cleanup(curl);
            return code;
        }

        free(decoded_url);
        fclose(f);
        curl_easy_cleanup(curl);
        
        char actual_file[273];
        snprintf(actual_file, sizeof(actual_file), "%s/%s.mp3", path, song_id);
        rename(tmp_file, actual_file);

        return 0;
    }
    return -2;
}

static void network_thread(void *arg) {
    NetworkTask *task = arg;

    task->finished = false;
    task->running = true;
    task->cancelled = false;

    task->result = task->func(task);

    task->running = false;
    task->finished = true;
}

Thread create_network_thread(NetworkTask *task) {
    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority += 1;
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;
    
    return threadCreate(
        network_thread,
        task,
        32 * 1024,
        priority,
        (is_N3DS ? 2 : 0),
        true
    );
}

static void download_thread(void *arg) {
    DownloadTask *task = arg;

    task->progress = 0;
    task->speed = 0;
    task->last_bytes = 0;
    task->last_time = 0;
    task->cancelled = false;

    task->finished = false;
    task->running = true;

    task->result = download_song(task);

    task->running = false;
    task->finished = true;
}

Thread create_download_song_thread(DownloadTask *task) {
    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    priority += 1;
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;
    
    return threadCreate(
        download_thread,
        task,
        32 * 1024,
        priority,
        (is_N3DS ? 2 : 0),
        true
    );
}

void soc_exit() {
    close(sock);
    socExit();
}