#ifndef _MESSAGE_H_
#define _MESSAGE_H_

int convert_msg_id(unsigned char *msg, int player_id);
void process_msg(int player_id, unsigned char *id_buf, int msg_type);

#endif
