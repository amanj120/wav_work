#include <stdio.h>
#include <ncurses.h>

int main() {

	initscr();
	cbreak();
	noecho();
	nodelay(stdscr, TRUE);
	// scrollok(stdscr, TRUE);

	while(1) {
		int ch = getch();

		if (ch != ERR) {
			printw("\n\tkey pressed: it was: %c (ascii: %d)\n", (char) ch, ch);
			if (ch == 'c') {
				break;
			}
		} else {
			printw("\rwaiting for a keypress (press 'c' to exit): ");
		}

		refresh();
	}

	// char c = getc();//(stdin);
	// printf("your char: %c (ascii: %d)\n", c, (int)c);
	endwin();
	return 0;
}