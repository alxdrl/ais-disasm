#include <ncurses.h>

void wdisasm(WINDOW *w, ais_vma address)
{
	int l = 0;
	while ( l < = getmaxy(w) - 1) {
		printr_adress
		print_hex
		count = print_data
		address += count
		
	}	
}

int main(int argc, char **argv)
{
    WINDOW *ais_window;
    int ch;
    int curx, cury;

    initscr();
    keypad(stdscr, TRUE);
    noecho();
    refresh();
    box(stdscr, 0, 0);
    ais_window = newwin(LINES - 2, COLS - 2, 1, 1);
    scrollok(ais_window, TRUE);
    idlok(ais_window, TRUE);
    mvprintw(0, 1, "%s (%d, %d)", argv[0], getmaxx(ais_window), getmaxy(ais_window));
    refresh();
    wmove(ais_window, 0, 0);
    wrefresh(ais_window);
    while((ch = getch()) != KEY_F(10)) {
	getyx(ais_window, cury, curx);
	switch (ch) {
		case KEY_RESIZE:
			wresize(ais_window, LINES - 2, COLS - 2);
			box(stdscr, 0, 0);
			getyx(ais_window, cury, curx);
			break;
		case KEY_UP:
			cury--;
			break;	
		case KEY_DOWN:
			cury++;
			break;	
		case KEY_LEFT:
			curx--;
			break;	
		case KEY_RIGHT:
			curx++;
			break;	
		case KEY_PPAGE:
			cury -= 10;
			break;	
		case KEY_NPAGE:
			cury += 10;
			break;
		case KEY_HOME:
			curx = 0;
			break;
		case KEY_END:
			curx = getmaxx(ais_window);
			break;
	}
	
	if (curx >= getmaxx(ais_window) -1 )
		curx = getmaxx(ais_window) - 1;
	if (curx < 0)
		curx = 0;
	if (cury >= getmaxy(ais_window) - 1)
		cury = getmaxy(ais_window) - 1;
	if (cury < 0)
		cury = 0;
	if (cury < 0)
		cury = 0;
    	mvprintw(0, 1, "%s (%d:%d, %d:%d)    ", argv[0], curx, getmaxx(ais_window), cury, getmaxy(ais_window));
    	refresh();
	wmove(ais_window, cury, curx);
	wrefresh(ais_window);
    }
    delwin(ais_window);
 
    endwin();
 
    return 0;
}
