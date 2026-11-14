#pragma once
#include "arena.hpp"
#include "objects.hpp"
#include "text.hpp"
#include "utils/server_utils.hpp"
#include <3ds.h>

#define MAX_GROUPS_PER_OBJECT 20

#define SECTION_HASH_SIZE 1024

#define SECTION_SIZE 128

#define MAX_PULSES_PER_GROUP 5

#define MAX_TEXT_LEN 64

namespace gd3ds
{

typedef struct Color {
    unsigned char r,g,b;

    u32 toColor32(){
        return C2D_Color32(r, g, b, 255);
    }
} Color;

typedef struct {
    float h;
    float s;
    float v;
    bool sChecked;
    bool vChecked;
} HSV;

typedef enum {
    GD_VAL_INT,
    GD_VAL_FLOAT,
    GD_VAL_BOOL,
    GD_VAL_HSV,
    GD_VAL_INT_ARRAY,
    GD_VAL_STRING,
    GD_VAL_UNKNOWN
} GDValueType;

typedef union {
    int i;
    float f;
    bool b;
    HSV hsv;
    short intArray[MAX_GROUPS_PER_OBJECT];
    char *str;
} GDValue;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    float trigDuration;
    int targetGroup;
    unsigned char trigColorR, trigColorG, trigColorB;
    bool p1Color, p2Color;
    bool tintGround;
    bool blending;
    unsigned short targetColorID;
    int copiedColorID;
    float opacity;
    HSV copiedHSV;
} ColorTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    bool targetMode;
    float trigDuration;
    int targetGroup;
    int targetGroup2;
    int centerGroup;
    float moveOffsetX;
    float moveOffsetY;
    int moveEasing;
    bool lockToPlayerX;
    bool lockToPlayerY;
} MoveTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    bool activateGroup;
    bool multiActivate;
    int targetGroup;
    int itemID;
    int targetCount;
} CountTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    bool activateGroup;
    int targetGroup;
    int itemID;
    int targetCount;
    int instantCountMode;
} InstantCountTrigger;

#define COUNTER_LABEL 1615
#define MAX_COUNTER_TEXT 12

typedef struct {
    int itemID;
    int lastValue;
    char text[MAX_COUNTER_TEXT + 1];
    unsigned char layoutDone;
    unsigned char glyphCount;
    TextGlyphPlacement glyphs[MAX_COUNTER_TEXT];
} CounterLabel;

typedef struct {
    int itemID;
    bool isPickup;
    bool subCount;
} PickupItem;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    int targetGroup;
    float followSpeed;
    float followDelay;
    float followOffset;
    float followMaxSpeed;
} FollowPlayerYTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    int targetGroup;
    int followGroup;
    float xMod;
    float yMod;
    float trigDuration;
} FollowTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    float trigDuration;
    int targetGroup;
    float triggerOpacity;
} AlphaTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    float trigDuration;
    int targetGroup;
    unsigned char trigColorR, trigColorG, trigColorB;
    unsigned short targetColorID;
    float opacity;
    int copiedColorID;
    HSV copiedHSV;

    float pulseFadeIn;
    float pulseHold;
    float pulseFadeOut;
    int pulseMode;

    int pulseTargetType;
    bool pulseMainOnly;
    bool pulseDetailOnly;
} PulseTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    int targetGroup;
    bool activateGroup;
    bool spawnOnly;
} ToggleTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    bool holdMode;
    unsigned char toggleMode;
    int targetGroup;
} TouchTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    int targetGroup;
} StopTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    bool activate;
    int targetGroup;
} OnDeathTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
    int targetGroup;
    float spawnDelay;
    float trigDuration;
} SpawnTrigger;

typedef struct {
    bool spawnTriggered;
    bool multiTriggered;
} SimpleTrigger;

typedef struct {
    char text[MAX_TEXT_LEN + 1];
    unsigned char len;
    unsigned char layoutDone;
    unsigned char glyphCount;
    TextGlyphPlacement glyphs[MAX_TEXT_LEN];
} TextObject;

typedef struct {
    void *data;
    size_t count;
    size_t capacity;
} TriggerPool;

typedef enum {
    FLAG_TOGGLED    = (1 << 0),
    FLAG_DONT_FADE  = (1 << 1),
    FLAG_DONT_ENTER = (1 << 2),
    FLAG_CAN_BE_X_MOVED = (1 << 3),
    FLAG_MULTI_ACTIVATE = (1 << 4),
    FLAG_PICKUP = (1 << 5)
} ObjectFlags;

typedef struct {
    int count;

    int *random;

    int *id;
    float *x, *y;
    float *lastY;
    float *rotation;
    float *visualRotation;
    int *zlayer, *zorder;
    float *opacity;
    float *alphaTriggerOpacity;

    float *width, *height;

    unsigned short *v1p9ColChannel;
    unsigned short *colChannel;
    unsigned short *detailColChannel;

    int *triggerIndex;

    float *originalX;
    float *originalY;

    u8 *numMainPulses;
    u8 *numDetailPulses;
    bool *mainBeingPulsed;
    bool *detailBeingPulsed;
    Color *mainNonPulseColor;
    Color *detailNonPulseColor;
    Color *mainColor;
    Color *detailColor;

    bool *mainColHSVEnabled;
    bool *detailColHSVEnabled;
    HSV *mainColHSV;
    HSV *detailColHSV;

    Color *cachedMainHSVSrcColor;
    Color *cachedDetailHSVSrcColor;
    Color *cachedMainHSVColor;
    Color *cachedDetailHSVColor;
    bool *cachedMainHSVValid;
    bool *cachedDetailHSVValid;

    float *scaleX, *scaleY;
    float *originalScaleX, *originalScaleY;

    int *childObject;
    float *tpYOffset;
    int *sectionX;
    int *sectionY;

    unsigned char *transitionApplied;
    unsigned char *orientation;
    unsigned char *hitboxCounter;
    union {
        bool *touchTriggered;
        u8 *coinID;
    };
    bool *flippedH, *flippedV;

    short (*groups)[MAX_GROUPS_PER_OBJECT];
    int *groupCombinationIndex;

    u8 *groupCount;
    u8 *flags;

    u8 *activated;
    u8 *collided;
} ObjectsArray;

typedef struct {
    int fromRed;
    int fromGreen;
    int fromBlue;
    int playerColor;
    bool blending;
    int channelID;
    float fromOpacity;
    bool toggleOpacity;
    int inheritedChannelID;
    HSV hsv;
    int toRed;
    int toGreen;
    int toBlue;
    float deltaTime;
    float toOpacity;
    float duration;
    bool copyOpacity;
} GDColorChannel;


typedef struct SectionTriggers {
    int count, capacity;
    int indices[];
} SectionTriggers;

typedef struct Section {
    int *objects;
    int objectCount;
    int objectCapacity;
    SectionTriggers *triggers;

    int x, y; // Section coordinates
    struct Section *next; // For chaining in hash map
} Section;

typedef struct {
    float lastObjX;
    float wallX;
    float wallY;

    int pulsingType;
    size_t songID;
    int customSongID;
    float songOffset;
    bool completing;
    int backgroundID;
    int groundID;
    int initialGamemode;
    bool initialMini;
    unsigned char initialSpeed;
    bool initialDual;
    bool initialUpsidedown;

    bool twoPlayerMode;
    bool lowDetail;

    char levelName[256];
    char creatorName[256];
} LoadedLevelInfo;



typedef struct {
    unsigned char *compressedData;
    size_t compressedLen;
    const char *rawData;
} LevelStreamInput;

typedef int (*LevelStreamCallback)(const unsigned char *data, size_t len, void *context);

typedef struct {
    char *section;
    size_t sectionLen;
    size_t sectionCapacity;
    char *metadata;
    int sectionCount;
    int highDetailCount;
    int extraSlots;
    int portalMatch;
} LevelStreamScan;

typedef struct {
    char *section;
    size_t sectionLen;
    size_t sectionCapacity;
    int sectionCount;
    int objectIndex;
    int parseCapacity;
} LevelStreamParser;

typedef enum {
    LOAD_NO_ERROR,
    LOAD_INVALID_GMD,
    LOAD_INVALID_COMPRESSED_DATA,
    LOAD_LEVEL_STRING_MISSING_SECTIONS,
    LOAD_OUT_OF_MEMORY,
    LOAD_COULDNT_PARSE_OBJECTS,
    LOAD_INVALID_BASE64,
    LOAD_ERROR_COUNT,
} LevelLoadError;

extern TriggerPool colPool;
extern TriggerPool movePool;
extern TriggerPool alphaPool;
extern TriggerPool pulsePool;
extern TriggerPool togglePool;
extern TriggerPool touchPool;
extern TriggerPool spawnPool;
extern bool hasFollowPlayerY;

ColorTrigger *getColorTrigger(int obj);
StopTrigger *getStopTrigger(int obj);
OnDeathTrigger *getOnDeathTrigger(int obj);
PulseTrigger *getPulseTrigger(int obj);
AlphaTrigger *getAlphaTrigger(int obj);
MoveTrigger *getMoveTrigger(int obj);
CountTrigger *getCountTrigger(int obj);
InstantCountTrigger *getInstantCountTrigger(int obj);
CounterLabel *getCounterLabel(int obj);
PickupItem *getPickupItem(int obj);
FollowPlayerYTrigger *getFollowPlayerYTrigger(int obj);
FollowTrigger *getFollowTrigger(int obj);
ToggleTrigger *getToggleTrigger(int obj);
TouchTrigger *getTouchTrigger(int obj);
SpawnTrigger *getSpawnTrigger(int obj);
TextObject *getTextObject(int obj);

bool isSimpleTriggerObject(int id);

bool triggerIsSpawnTriggered(int id, int obj);
bool triggerIsMultiTriggered(int id, int obj);

extern const char *errorStrings[LOAD_ERROR_COUNT - 1];

extern LoadedLevelInfo levelInfo;

extern const char *defaultName;

extern const char *levelLengths[5];

#define BG_COUNT 20
#define G_COUNT 17

extern char *currLevelString;

extern ObjectsArray objects;

extern int channelCount;
extern GDColorChannel *colorChannels;

char *readFile(const char *filepath, size_t *outSize);
char *readFileArena(Arena *arena, const char *filepath, size_t *outSize);
char *decompressLevel(char *data, int *outCode);

int loadLevel(const char *path);
int loadOnlineLevel(char *levelString);
bool isLevelArenaPtr(const void *ptr);
void reloadLevel();
void unloadLevel();

void fixBase64URL(char *b64);
int base64Decode(const char *in, unsigned char *out);

Section *getSection(int x, int y);
Section *getOrCreateSection(int x, int y);
void assignObjectToSection(int obj);
void updateObjectSection(int obj);
bool objHasMain(const GameObject *obj);
bool objHasDetail(const GameObject *obj);

bool isValidObject(int id);
bool isTriggerObject(int id);
bool isComplexTriggerObject(int id);
bool isColorTrigger(int id);
int getLevelCoinCount(void);

char *getLevelName(char *dataPtr);
char *loadUserSong(int id, size_t *outSize); 
bool checkSong(int id);
char *extractGMDKey(const char *data, const char *key, const char *type);

char **splitString(const char *str, char delimiter, int *outCount, bool ignoreZeroLength);
char **splitStringInPlace(char *str, char delimiter, int *outCount, bool ignoreZeroLength);
char **splitStringStrDel(const char *str, const char *delimiter, int *outCount, bool ignoreZeroLength);
void freeStringArray(char **arr, int count);

char *convertIfGdlvl(char *gmd, const char *path);

}
