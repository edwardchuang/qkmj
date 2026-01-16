#include <cjson/cJSON.h>
extern float thinktime();
void send_game_log(const char *action, int card, cJSON *extra);
void broadcast_game_result(int winner_sit, int loser_sit, const char* tai_str, long* money_diff);