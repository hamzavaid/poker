#ifndef POKER_GUI_H
#define POKER_GUI_H
 
// call once after connecting to server
void launch_poker_window(int server_fd);
 
// call these from poker.c when server messages arrive
void poker_gui_set_pot(int amount);
void poker_gui_set_stack(int amount);
void poker_gui_set_status(const char *msg);
void poker_gui_set_community_card(int idx, const char *path);
void poker_gui_set_my_card(int idx, const char *path);
void poker_gui_update_slot(int slot, const char *name, const char *bet_str, int is_active, int folded);
 
// builds asset path: "ace_of_spades" -> "assets/ace_of_spades.png"
void get_card_path(char *buf, int bufsize, const char *value, const char *suit);
 
#endif
