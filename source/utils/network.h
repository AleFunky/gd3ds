#pragma once

int soc_init();

int get_level_from_id(char **out_data, int id);

int get_search_results(char **out_data, int gameVer, int type, char *query, int page, bool uncompleted, bool onlyCompleted, char *completedList, bool featured, bool original, bool noStar, bool customSong, int customSongId);

int get_comments_from_id(char **out_data, int id, int page, int mode);

void soc_exit();
