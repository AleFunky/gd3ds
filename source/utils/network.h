#pragma once

int soc_init();
int get_level_from_id(char **out_data, int id);
int get_search_results(char **out_data, char *query, int type);
void soc_exit();
