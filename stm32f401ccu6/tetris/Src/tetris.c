#include "main.h"
#include "printf.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "stm32_adafruit_lcd.h"
#include "stm32f4xx_hal.h"
#include "string.h"
#include "usbd_cdc_if.h"

#define TICK_MS 300

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 24
#define BOARD_SIZE (BOARD_WIDTH * BOARD_HEIGHT)
#define IS_BOARD_OOB(x, y) ((uint8_t)(x) >= BOARD_WIDTH || (uint8_t)(y) >= BOARD_HEIGHT)

#define DISPLAY_WIDTH 10
#define DISPLAY_HEIGHT 20
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)

#define TEXT_PADDING 4

#define NUM_OF_TETRIMINOS 7
enum Tetrimino {
    Tetrimino_Empty = 0,
    Tetrimino_I,
    Tetrimino_J,
    Tetrimino_L,
    Tetrimino_O,
    Tetrimino_S,
    Tetrimino_T,
    Tetrimino_Z,
    Falling_Tetrimino = 0x08,  // used to set falling piece
    Falling_Tetrimino_I,
    Falling_Tetrimino_J,
    Falling_Tetrimino_L,
    Falling_Tetrimino_O,
    Falling_Tetrimino_S,
    Falling_Tetrimino_T,
    Falling_Tetrimino_Z,
    Visual_Tetrimino = 0x10,  // used to visualize where falling piece hard drops
};

#define IS_FALLING_TETRIMINO(T) (T & Falling_Tetrimino)
#define IS_STATIONARY_TETRIMINO(T) (!IS_FALLING_TETRIMINO(T) && T != Tetrimino_Empty)
#define CONVERT_FALLING_TETRIMINO(T) (T | Falling_Tetrimino)
#define CONVERT_STATIONARY_TETRIMINO(T) (T & ~Falling_Tetrimino)

#define TETRIMINO_WIDTH 4
#define TETRIMINO_HEIGHT 4
#define TETRIMINO_SIZE (TETRIMINO_WIDTH * TETRIMINO_HEIGHT)
#define TETRIMINO_PX_SIZE 24
#define TETRIMINO_PX_TOP_LEFT_PADDING 2
#define TETRIMINO_PX_BOT_RIGHT_PADDING 1

// clang-format off
const uint8_t Tetrimino_Shape_I[TETRIMINO_SIZE * 4] = {
    0, 0, 0, 0, 
    1, 1, 1, 1,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 0, 1, 0,
    0, 0, 1, 0,
    0, 0, 1, 0,
    0, 0, 1, 0,
    
    0, 0, 0, 0,
    0, 0, 0, 0,
    1, 1, 1, 1,
    0, 0, 0, 0,
    
    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 1, 0, 0,
};
const uint8_t Tetrimino_Shape_J[TETRIMINO_SIZE * 4] = {
    1, 0, 0, 0,
    1, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 1, 1, 0,
    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
    
    0, 0, 0, 0,
    1, 1, 1, 0,
    0, 0, 1, 0,
    0, 0, 0, 0,
    
    0, 1, 0, 0,
    0, 1, 0, 0,
    1, 1, 0, 0,
    0, 0, 0, 0,
};
const uint8_t Tetrimino_Shape_L[TETRIMINO_SIZE * 4] = {
    0, 0, 1, 0,
    1, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,

    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 1, 1, 0,
    0, 0, 0, 0,

    0, 0, 0, 0,
    1, 1, 1, 0,
    1, 0, 0, 0,
    0, 0, 0, 0,
    
    1, 1, 0, 0,
    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
};
const uint8_t Tetrimino_Shape_O[TETRIMINO_SIZE * 4] = {
    0, 1, 1, 0,
    0, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 1, 1, 0,
    0, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 1, 1, 0,
    0, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 1, 1, 0,
    0, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};
const uint8_t Tetrimino_Shape_S[TETRIMINO_SIZE * 4] = {
    0, 1, 1, 0,
    1, 1, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 1, 0, 0,
    0, 1, 1, 0,
    0, 0, 1, 0,
    0, 0, 0, 0,

    0, 0, 0, 0,
    0, 1, 1, 0,
    1, 1, 0, 0,
    0, 0, 0, 0,
    
    1, 0, 0, 0,
    1, 1, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
};
const uint8_t Tetrimino_Shape_T[TETRIMINO_SIZE * 4] = {
    0, 1, 0, 0,
    1, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 1, 0, 0,
    0, 1, 1, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
    
    0, 0, 0, 0,
    1, 1, 1, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
    
    0, 1, 0, 0,
    1, 1, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
};
const uint8_t Tetrimino_Shape_Z[TETRIMINO_SIZE * 4] = {
    1, 1, 0, 0,
    0, 1, 1, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    
    0, 0, 1, 0,
    0, 1, 1, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,

    0, 0, 0, 0,
    1, 1, 0, 0,
    0, 1, 1, 0,
    0, 0, 0, 0,
    
    0, 1, 0, 0,
    1, 1, 0, 0,
    1, 0, 0, 0,
    0, 0, 0, 0,
};
// clang-format on

const uint8_t* Tetrimino_Shapes[] = {
    [Tetrimino_Empty] = NULL,
    [Tetrimino_I] = Tetrimino_Shape_I,
    [Tetrimino_J] = Tetrimino_Shape_J,
    [Tetrimino_L] = Tetrimino_Shape_L,
    [Tetrimino_O] = Tetrimino_Shape_O,
    [Tetrimino_S] = Tetrimino_Shape_S,
    [Tetrimino_T] = Tetrimino_Shape_T,
    [Tetrimino_Z] = Tetrimino_Shape_Z,
    [Falling_Tetrimino] = NULL,
    [Falling_Tetrimino_I] = Tetrimino_Shape_I,
    [Falling_Tetrimino_J] = Tetrimino_Shape_J,
    [Falling_Tetrimino_L] = Tetrimino_Shape_L,
    [Falling_Tetrimino_O] = Tetrimino_Shape_O,
    [Falling_Tetrimino_S] = Tetrimino_Shape_S,
    [Falling_Tetrimino_T] = Tetrimino_Shape_T,
    [Falling_Tetrimino_Z] = Tetrimino_Shape_Z,
};

const uint16_t Tetrimino_Colors[] = {
    [Tetrimino_Empty] = 0,
    [Tetrimino_I] = LCD_COLOR_CYAN,
    [Tetrimino_J] = LCD_COLOR_BLUE,
    [Tetrimino_L] = 0xFC00,
    [Tetrimino_O] = LCD_COLOR_YELLOW,
    [Tetrimino_S] = LCD_COLOR_GREEN,
    [Tetrimino_T] = LCD_COLOR_MAGENTA,
    [Tetrimino_Z] = LCD_COLOR_RED,
    [Falling_Tetrimino] = 0,
    [Falling_Tetrimino_I] = LCD_COLOR_CYAN,
    [Falling_Tetrimino_J] = LCD_COLOR_BLUE,
    [Falling_Tetrimino_L] = 0xFC00,
    [Falling_Tetrimino_O] = LCD_COLOR_YELLOW,
    [Falling_Tetrimino_S] = LCD_COLOR_GREEN,
    [Falling_Tetrimino_T] = LCD_COLOR_MAGENTA,
    [Falling_Tetrimino_Z] = LCD_COLOR_RED,
};

// we use enum Tetrimino to color the squares later
enum Tetrimino board[BOARD_SIZE];  // pos = x + y * BOARD_WIDTH
enum Tetrimino prev_board[BOARD_SIZE];
enum Tetrimino cur_tetrimino = Tetrimino_Empty;
enum Tetrimino next_tetrimino = Tetrimino_Empty;

// current rotation and position of falling tetrimino
uint8_t cur_rotation = 0;
uint8_t cur_x = 0;
uint8_t cur_y = 0;

// descending list of y values that were cleared in the previous game tick
uint8_t clines[TETRIMINO_HEIGHT];
uint8_t clines_len = 0;

// how many ticks to wait before collision check & gravity
uint8_t wait_ticks = 0;

uint32_t cur_score = 0;
uint32_t lines_cleared = 0;

// get position of square based on tetrimino's coordinates
uint8_t get_cur_pos(uint8_t tx, uint8_t ty) {
    return tx + cur_x + (ty + cur_y) * BOARD_WIDTH;
}

void debug_print_board() {
    for (uint8_t y = 0; y < BOARD_HEIGHT; y++) {
        char row[BOARD_WIDTH + 2];
        for (uint8_t x = 0; x < BOARD_WIDTH; x++) {
            enum Tetrimino cell = board[x + y * BOARD_WIDTH];
            if (cell == 0) {
                row[x] = ' ';
            } else if (cell > 0 && cell < 8) {
                row[x] = cell + '0';
            } else {
                row[x] = cell + 'a';
            }
        }
        row[BOARD_WIDTH] = '\n';
        row[BOARD_WIDTH + 1] = 0;
        PRINTF("%s", row);
    }
}

void draw(uint8_t x, uint8_t y, volatile uint16_t color) {
    uint16_t px_x = x * TETRIMINO_PX_SIZE + TETRIMINO_PX_TOP_LEFT_PADDING;
    uint16_t px_y = y * TETRIMINO_PX_SIZE + TETRIMINO_PX_TOP_LEFT_PADDING;
    uint16_t px_length = TETRIMINO_PX_SIZE - TETRIMINO_PX_TOP_LEFT_PADDING - TETRIMINO_PX_BOT_RIGHT_PADDING;

    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(px_x, px_y, px_length, px_length);
}

void update_screen(bool force) {
    // draw board
    for (uint8_t y = 0; y < DISPLAY_HEIGHT; y++) {
        for (uint8_t x = 0; x < DISPLAY_WIDTH; x++) {
            uint8_t pos = x + (y + 4) * BOARD_WIDTH;
            if (board[pos] != prev_board[pos] || force) {
                prev_board[pos] = board[pos];
                draw(x, y, Tetrimino_Colors[board[pos]]);
            }
        }
    }

    static enum Tetrimino prev_next_tetrimino = Tetrimino_Empty;

    if (prev_next_tetrimino != next_tetrimino || force) {
        prev_next_tetrimino = next_tetrimino;
        const uint8_t* cur_shape = Tetrimino_Shapes[next_tetrimino];
        const uint16_t size = TETRIMINO_PX_SIZE / 4 * 3;
        const uint16_t draw_size = size - 2;

        for (uint8_t ty = 0; ty < 2; ty++) {
            for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
                uint16_t px_x = TETRIMINO_PX_SIZE * DISPLAY_WIDTH + 2 * TEXT_PADDING + tx * size;
                uint16_t px_y = 1 * TETRIMINO_PX_SIZE + ty * size + 1;

                if (cur_shape[tx + ty * TETRIMINO_WIDTH]) {
                    BSP_LCD_SetTextColor(Tetrimino_Colors[next_tetrimino]);
                } else {
                    BSP_LCD_SetTextColor(Tetrimino_Colors[Tetrimino_Empty]);
                }
                BSP_LCD_FillRect(px_x, px_y, draw_size, draw_size);
            }
        }
    }

    static uint32_t prev_score = 0;
    // update score
    if (prev_score != cur_score || force) {
        prev_score = cur_score;
        char score_str[12];  // -ve sign, 10 digits, nul term
        sprintf(score_str, "%lu", cur_score);
        char lines_str[12];
        sprintf(lines_str, "%lu", lines_cleared);
        char levels_str[12];
        sprintf(levels_str, "%lu", lines_cleared / 10 + 1);
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        BSP_LCD_SetFont(&Font24);
        BSP_LCD_DisplayStringAt(TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, 4 * TETRIMINO_PX_SIZE, (uint8_t*)score_str, LEFT_MODE);
        BSP_LCD_DisplayStringAt(TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, 6 * TETRIMINO_PX_SIZE, (uint8_t*)lines_str, LEFT_MODE);
        BSP_LCD_DisplayStringAt(TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, 8 * TETRIMINO_PX_SIZE, (uint8_t*)levels_str, LEFT_MODE);

        PRINTF("update score %s\n", score_str);
    }
}

// return true if collision
bool check_collision(uint8_t new_x, uint8_t new_y, uint8_t new_rot) {
    const uint8_t* new_shape = Tetrimino_Shapes[cur_tetrimino] + new_rot * TETRIMINO_SIZE;

    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            if (!new_shape[tx + ty * TETRIMINO_WIDTH]) continue;

            if (IS_BOARD_OOB(tx + new_x, ty + new_y)) {
                PRINTF("OOB: (%u, %u) (%u, %u)\n", tx, ty, new_x, new_y);
                return true;
            }

            uint8_t new_pos = tx + new_x + (ty + new_y) * BOARD_WIDTH;
            if (IS_STATIONARY_TETRIMINO(board[new_pos])) return true;
        }
    }
    return false;
}

bool Tetris_SpawnPiece() {
    PRINTF("spawn piece\n");
    cur_tetrimino = next_tetrimino;
    next_tetrimino = (enum Tetrimino)(rand() % NUM_OF_TETRIMINOS + 1);
    cur_rotation = 0;

    const uint8_t* cur_shape = Tetrimino_Shapes[cur_tetrimino];  // rotation = 0
    cur_x = BOARD_WIDTH / 2 - TETRIMINO_WIDTH / 2;
    cur_y = 4;

    if (check_collision(cur_x, cur_y, cur_rotation))
        return false;

    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t board_pos = get_cur_pos(tx, ty);

            if (cur_shape[tx + ty * TETRIMINO_WIDTH]) {
                board[board_pos] = CONVERT_FALLING_TETRIMINO(cur_tetrimino);
            }
        }
    }
    return true;
}

bool Tetris_Move(int8_t dx, int8_t dy) {
    // check collision
    if (check_collision(cur_x + dx, cur_y + dy, cur_rotation))
        return false;

    // remove old tetrimino
    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t pos = get_cur_pos(tx, ty);

            if (IS_FALLING_TETRIMINO(board[pos]))
                board[pos] = Tetrimino_Empty;
        }
    }

    const uint8_t* cur_shape = Tetrimino_Shapes[cur_tetrimino] + cur_rotation * TETRIMINO_SIZE;

    // place new tetrimino
    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t pos = get_cur_pos(tx + dx, ty + dy);

            if (cur_shape[tx + ty * TETRIMINO_WIDTH]) {
                board[pos] = CONVERT_FALLING_TETRIMINO(cur_tetrimino);
            }
        }
    }

    // commit changes
    cur_x += dx;
    cur_y += dy;
    wait_ticks = 1;
    update_screen(false);
    return true;
}

void stick_cur_tetrimino() {
    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t pos = get_cur_pos(tx, ty);
            if (pos >= BOARD_SIZE) return;

            board[pos] &= ~Falling_Tetrimino;
        }
    }
}

bool clear_lines() {
    for (uint8_t y = cur_y + TETRIMINO_HEIGHT; y-- > cur_y;) {
        if (IS_BOARD_OOB(0, y)) continue;
        bool full_line = true;
        for (uint8_t x = 0; x < BOARD_WIDTH; x++) {
            uint8_t pos = x + y * BOARD_WIDTH;
            if (board[pos] == Tetrimino_Empty) {
                full_line = false;
                break;
            }
        }
        if (full_line) clines[clines_len++] = y;
    }
    if (clines_len == 0) return false;

    for (uint8_t i = 0; i < clines_len; i++) {
        PRINTF("cleared line %i\n", clines[i]);
        for (uint8_t x = 0; x < BOARD_WIDTH; x++) {
            uint8_t pos = x + clines[i] * BOARD_WIDTH;
            board[pos] = Tetrimino_Empty;
        }
    }
    update_screen(false);

    return true;
}

void drop_lines() {
    if (clines_len == 0) return;

    uint8_t i = 0;
    for (uint8_t y = clines[i]; y >= 1; y--) {
        if (i < clines_len && y == clines[i]) {
            PRINTF("len %u drop %u\n", clines_len, y);
            i++;
            continue;
        }

        bool empty = true;
        for (uint8_t x = 0; x < BOARD_WIDTH; x++) {
            uint8_t pos1 = x + y * BOARD_WIDTH;
            uint8_t pos2 = x + (y + i) * BOARD_WIDTH;

            if (IS_FALLING_TETRIMINO(board[pos1])) continue;
            if (board[pos1] == Tetrimino_Empty) empty = false;
            board[pos2] = board[pos1];
        }
        if (empty) break;
    }

    // update score
    switch (clines_len) {
        case 1:
            cur_score += 40 * (lines_cleared / 10 + 1);
            break;
        case 2:
            cur_score += 100 * (lines_cleared / 10 + 1);
            break;
        case 3:
            cur_score += 300 * (lines_cleared / 10 + 1);
            break;
        case 4:
            cur_score += 1200 * (lines_cleared / 10 + 1);
            break;
    }
    lines_cleared += clines_len;
    clines_len = 0;
}

void Tetris_StartGame() {
    PRINTF("start game\n");
    memset(board, Tetrimino_Empty, BOARD_SIZE);
    memset(prev_board, Tetrimino_Empty, BOARD_SIZE);
    clines_len = 0;
    lines_cleared = 0;
    cur_tetrimino = (enum Tetrimino)(rand() % NUM_OF_TETRIMINOS + 1);
    next_tetrimino = (enum Tetrimino)(rand() % NUM_OF_TETRIMINOS + 1);
    cur_rotation = 0;
    cur_x = 0;
    cur_y = 0;
    wait_ticks = 0;
    cur_score = 0;

    BSP_LCD_Clear(LCD_COLOR_BLACK);

    // draw vertical lines
    for (int x = 0; x <= DISPLAY_WIDTH; x++) {
        if (x == 0 || x == DISPLAY_WIDTH)
            BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        else
            BSP_LCD_SetTextColor(0x4208);

        BSP_LCD_DrawLine(x * TETRIMINO_PX_SIZE, 0, x * TETRIMINO_PX_SIZE, DISPLAY_HEIGHT * TETRIMINO_PX_SIZE);
    }

    // draw horizontal lines
    for (int y = 0; y <= DISPLAY_HEIGHT; y++) {
        if (y == 0 || y == DISPLAY_HEIGHT)
            BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        else
            BSP_LCD_SetTextColor(0x4208);
        BSP_LCD_DrawLine(0, y * TETRIMINO_PX_SIZE, DISPLAY_WIDTH * TETRIMINO_PX_SIZE, y * TETRIMINO_PX_SIZE);
    }

    // draw score
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font20);
    BSP_LCD_DisplayStringAt(TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, TEXT_PADDING, (uint8_t*)"NEXT", LEFT_MODE);
    BSP_LCD_DisplayStringAt(TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, 3 * TETRIMINO_PX_SIZE, (uint8_t*)"SCORE", LEFT_MODE);
    BSP_LCD_DisplayStringAt(TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, 5 * TETRIMINO_PX_SIZE, (uint8_t*)"LINES", LEFT_MODE);
    BSP_LCD_DisplayStringAt(TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, 7 * TETRIMINO_PX_SIZE, (uint8_t*)"LEVEL", LEFT_MODE);

    update_screen(true);
}

void Tetris_Tick() {
    // debug_print_board();
    if (cur_tetrimino == Tetrimino_Empty) {
        bool spawn_ok = Tetris_SpawnPiece();
        if (!spawn_ok) return Tetris_StartGame();

        wait_ticks = 1;
        update_screen(false);
        return;
    }

    if (clines_len > 0) drop_lines();

    bool move_ok = Tetris_Move(0, 1);
    // if (move_ok) PRINTF("move down\n");

    if (wait_ticks > 0) {
        // PRINTF("wait\n");
        wait_ticks--;
        return;
    }

    if (!move_ok) {
        // PRINTF("stick\n");
        stick_cur_tetrimino();
        bool lc = clear_lines();

        if (lc) wait_ticks = 1;

        cur_tetrimino = Tetrimino_Empty;
        return;
    }
}

void Tetris_Loop() {
    static uint32_t last_tick_ts = 0;

    uint32_t now = HAL_GetTick();
    if (now - last_tick_ts > TICK_MS) {
        last_tick_ts = now;
        Tetris_Tick();
        // PRINTF("tick\n");
    }
}

bool Tetris_RotatePiece(bool clockwise) {
    uint8_t new_rotation = (cur_rotation + (clockwise ? 1 : 3)) % 4;

    const uint8_t* new_shape = Tetrimino_Shapes[cur_tetrimino] + new_rotation * TETRIMINO_SIZE;

    // check rotation collision
    if (check_collision(cur_x, cur_y, new_rotation))
        return false;

    // carry out rotation
    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t pos = get_cur_pos(tx, ty);

            if (IS_FALLING_TETRIMINO(board[pos]))
                board[pos] = Tetrimino_Empty;

            if (new_shape[tx + ty * TETRIMINO_WIDTH])
                board[pos] = CONVERT_FALLING_TETRIMINO(cur_tetrimino);
        }
    }

    // commit changes
    cur_rotation = new_rotation;
    wait_ticks = 1;
    update_screen(false);
    return true;
}
