#ifndef NCURSES_MENU_H
#define NCURSES_MENU_H
#include <menu.h>
#include <curses.h>
#include <libconfig.h>
#include "ncurses_panel.h"
#include "ncurses_shared.h"

void set_keyboard_menu();
void create_main_menu(MENU **menu, config_t *lib_config);
void display_main_menu(MENU *menu, WINDOW *sub);
void process_menu_input(MENU *menu, WINDOW *window_header, WINDOW *window_body);

#endif //NCURSES_MENU_H
