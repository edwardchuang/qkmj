#include <cjson/cJSON.h>
extern float thinktime();
void send_game_log(const char *action, int card, cJSON *extra);