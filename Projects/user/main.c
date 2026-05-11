#include "app_main.h"
#include "bsp_board.h"

int main(void)
{
    board_init();
    app_main();

    return 0;
}
