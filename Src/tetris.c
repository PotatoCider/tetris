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

#define BASE_TICK_MS 1000
#define MIN_TICK_MS 20
#define LEVEL_TICK_MULTIPLIER_1_3 150
#define LEVEL_TICK_MULTIPLIER_4_10 50
#define LEVEL_TICK_MULTIPLIER_11_INF 10

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 24
#define BOARD_SIZE (BOARD_WIDTH * BOARD_HEIGHT)
#define IS_BOARD_OOB(x, y) \
    ((uint8_t)(x) >= BOARD_WIDTH || (uint8_t)(y) >= BOARD_HEIGHT)

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
    Visual_Tetrimino =
        0x10,  // used to visualize where falling piece hard drops
};

#define IS_FALLING_TETRIMINO(T) (T & Falling_Tetrimino)
#define IS_VISUAL_TETRIMINO(T) (T & Visual_Tetrimino)
#define IS_STATIONARY_TETRIMINO(T)                          \
    (!IS_FALLING_TETRIMINO(T) && !IS_VISUAL_TETRIMINO(T) && \
     T != Tetrimino_Empty)
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

const uint8_t *Tetrimino_Shapes[] = {
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
enum Tetrimino hold_tetrimino = Tetrimino_Empty;

bool already_held_tetrimino = false;

// current rotation and position of falling tetrimino
uint8_t cur_rotation = 0;
uint8_t cur_x = 0;
uint8_t cur_y = 0;

// descending list of y values that were cleared in the previous game tick
uint8_t clines[TETRIMINO_HEIGHT];
uint8_t clines_len = 0;

uint32_t droptick_period = BASE_TICK_MS;

// how many ticks to wait before collision check & gravity
uint8_t wait_ticks = 0;

uint32_t cur_score = 0;
uint32_t lines_cleared = 0;

// get position of square based on tetrimino's coordinates
uint8_t get_cur_pos(uint8_t tx, uint8_t ty) {
    return tx + cur_x + (ty + cur_y) * BOARD_WIDTH;
}

uint32_t get_cur_level() { return lines_cleared / 10 + 1; }

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
    uint16_t px_length = TETRIMINO_PX_SIZE - TETRIMINO_PX_TOP_LEFT_PADDING -
                         TETRIMINO_PX_BOT_RIGHT_PADDING;

    BSP_LCD_SetTextColor(color);
    BSP_LCD_FillRect(px_x, px_y, px_length, px_length);
}

void draw_tetrimino_at(enum Tetrimino tetrimino, uint16_t rel_x, uint16_t rel_y,
                       uint16_t size, volatile uint16_t color) {
    const uint8_t *cur_shape = Tetrimino_Shapes[tetrimino];

    for (uint8_t ty = 0; ty < 2; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint16_t px_x = rel_x + tx * size + 1;
            uint16_t px_y = rel_y + ty * size + 1;

            if (cur_shape[tx + ty * TETRIMINO_WIDTH]) {
                BSP_LCD_SetTextColor(color);
            } else {
                BSP_LCD_SetTextColor(Tetrimino_Colors[Tetrimino_Empty]);
            }
            BSP_LCD_FillRect(px_x, px_y, size - 2, size - 2);
        }
    }
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
    static enum Tetrimino prev_hold_tetrimino = Tetrimino_Empty;

    // draw next piece
    if (prev_next_tetrimino != next_tetrimino || force) {
        prev_next_tetrimino = next_tetrimino;

        draw_tetrimino_at(next_tetrimino,
                          TETRIMINO_PX_SIZE * DISPLAY_WIDTH + 2 * TEXT_PADDING,
                          1 * TETRIMINO_PX_SIZE, TETRIMINO_PX_SIZE / 4 * 3,
                          Tetrimino_Colors[next_tetrimino]);
    }

    if (prev_hold_tetrimino != hold_tetrimino || force) {
        prev_hold_tetrimino = hold_tetrimino;

        draw_tetrimino_at(hold_tetrimino,
                          TETRIMINO_PX_SIZE * DISPLAY_WIDTH + 2 * TEXT_PADDING,
                          11 * TETRIMINO_PX_SIZE, TETRIMINO_PX_SIZE / 4 * 3,
                          already_held_tetrimino
                              ? LCD_COLOR_GRAY
                              : Tetrimino_Colors[hold_tetrimino]);
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
        sprintf(levels_str, "%lu", get_cur_level());
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        BSP_LCD_SetFont(&Font20);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING,
            4 * TETRIMINO_PX_SIZE, (uint8_t *)score_str, LEFT_MODE);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING,
            6 * TETRIMINO_PX_SIZE, (uint8_t *)lines_str, LEFT_MODE);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING,
            8 * TETRIMINO_PX_SIZE, (uint8_t *)levels_str, LEFT_MODE);

        PRINTF("update score %s\n", score_str);
    }

    if (force) {
        // draw vertical lines
        for (int x = 0; x <= DISPLAY_WIDTH; x++) {
            if (x == 0 || x == DISPLAY_WIDTH)
                BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
            else
                BSP_LCD_SetTextColor(0x4208);

            BSP_LCD_DrawLine(x * TETRIMINO_PX_SIZE, 0, x * TETRIMINO_PX_SIZE,
                             DISPLAY_HEIGHT * TETRIMINO_PX_SIZE);
        }

        // draw horizontal lines
        for (int y = 0; y <= DISPLAY_HEIGHT; y++) {
            if (y == 0 || y == DISPLAY_HEIGHT)
                BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
            else
                BSP_LCD_SetTextColor(0x4208);
            BSP_LCD_DrawLine(0, y * TETRIMINO_PX_SIZE,
                             DISPLAY_WIDTH * TETRIMINO_PX_SIZE,
                             y * TETRIMINO_PX_SIZE);
        }

        // draw score
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        BSP_LCD_SetFont(&Font20);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING, TEXT_PADDING,
            (uint8_t *)"NEXT", LEFT_MODE);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING,
            3 * TETRIMINO_PX_SIZE, (uint8_t *)"SCORE", LEFT_MODE);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING,
            5 * TETRIMINO_PX_SIZE, (uint8_t *)"LINES", LEFT_MODE);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING,
            7 * TETRIMINO_PX_SIZE, (uint8_t *)"LEVEL", LEFT_MODE);
        BSP_LCD_DisplayStringAt(
            TETRIMINO_PX_SIZE * DISPLAY_WIDTH + TEXT_PADDING,
            10 * TETRIMINO_PX_SIZE, (uint8_t *)"HOLD", LEFT_MODE);
    }
}

// return true if collision
bool check_collision(uint8_t new_x, uint8_t new_y, uint8_t new_rot) {
    const uint8_t *new_shape =
        Tetrimino_Shapes[cur_tetrimino] + new_rot * TETRIMINO_SIZE;

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
    already_held_tetrimino = false;
    cur_tetrimino = next_tetrimino;
    next_tetrimino = (enum Tetrimino)(rand() % NUM_OF_TETRIMINOS + 1);

    cur_rotation = 0;

    const uint8_t *cur_shape = Tetrimino_Shapes[cur_tetrimino];  // rotation = 0
    cur_x = BOARD_WIDTH / 2 - TETRIMINO_WIDTH / 2;
    cur_y = 4;

    if (check_collision(cur_x, cur_y, cur_rotation)) return false;

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
    if (check_collision(cur_x + dx, cur_y + dy, cur_rotation)) return false;

    // remove old tetrimino
    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t pos = get_cur_pos(tx, ty);

            if (IS_FALLING_TETRIMINO(board[pos])) board[pos] = Tetrimino_Empty;
        }
    }

    const uint8_t *cur_shape =
        Tetrimino_Shapes[cur_tetrimino] + cur_rotation * TETRIMINO_SIZE;

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

    // update score based on level
    uint32_t level = get_cur_level();
    switch (clines_len) {
        case 1:
            cur_score += 40 * level;
            break;
        case 2:
            cur_score += 100 * level;
            break;
        case 3:
            cur_score += 300 * level;
            break;
        case 4:
            cur_score += 1200 * level;
            break;
    }
    uint32_t prev_level = get_cur_level();
    lines_cleared += clines_len;
    uint32_t new_level = get_cur_level();

    if (prev_level != new_level) {
        if (prev_level <= 3)
            droptick_period -= LEVEL_TICK_MULTIPLIER_1_3;
        else if (prev_level <= 10)
            droptick_period -= LEVEL_TICK_MULTIPLIER_4_10;
        else
            droptick_period -= LEVEL_TICK_MULTIPLIER_11_INF;

        if (droptick_period < MIN_TICK_MS) droptick_period = MIN_TICK_MS;
        PRINTF("update tick %lu", droptick_period);
    }

    clines_len = 0;
}

void Tetris_StartGame() {
    PRINTF("start game\n");
    memset(board, Tetrimino_Empty, BOARD_SIZE);
    memset(prev_board, Tetrimino_Empty, BOARD_SIZE);
    clines_len = 0;
    lines_cleared = 0;
    droptick_period = BASE_TICK_MS;
    // cur_tetrimino = (enum Tetrimino)(rand() % NUM_OF_TETRIMINOS + 1);
    next_tetrimino = (enum Tetrimino)(rand() % NUM_OF_TETRIMINOS + 1);
    hold_tetrimino = Tetrimino_Empty;
    cur_rotation = 0;
    cur_x = BOARD_WIDTH / 2 - TETRIMINO_WIDTH / 2;
    cur_y = 4;
    wait_ticks = 0;
    cur_score = 0;
    already_held_tetrimino = false;

    BSP_LCD_Clear(LCD_COLOR_BLACK);

    update_screen(true);
}

bool Tetris_HoldPiece() {
    if (already_held_tetrimino) return false;

    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t pos = get_cur_pos(tx, ty);
            if (pos >= BOARD_SIZE) continue;
            if (IS_FALLING_TETRIMINO(board[pos])) board[pos] = Tetrimino_Empty;
        }
    }

    enum Tetrimino next = next_tetrimino;
    next_tetrimino = hold_tetrimino;
    hold_tetrimino = cur_tetrimino;

    if (next_tetrimino == Tetrimino_Empty) {
        next_tetrimino = next;
        next = (enum Tetrimino)(rand() % NUM_OF_TETRIMINOS + 1);
    }

    bool spawn_ok = Tetris_SpawnPiece();
    if (!spawn_ok) Tetris_StartGame();

    already_held_tetrimino = true;
    next_tetrimino = next;

    wait_ticks = 1;
    update_screen(true);

    return true;
}

void Tetris_Tick(bool drop) {
    // debug_print_board();
    if (cur_tetrimino == Tetrimino_Empty) {
        bool spawn_ok = Tetris_SpawnPiece();
        if (!spawn_ok) return Tetris_StartGame();

        wait_ticks = 1;
        update_screen(true);
        return;
    }

    if (clines_len > 0) drop_lines();

    if (drop) Tetris_Move(0, 1);
    // if (move_ok) PRINTF("move down\n");

    if (wait_ticks > 0) {
        PRINTF("wait\n");
        wait_ticks--;
        return;
    }

    if (check_collision(cur_x, cur_y + 1, cur_rotation)) {
        PRINTF("stick\n");
        stick_cur_tetrimino();
        bool lc = clear_lines();

        if (lc) wait_ticks = 1;

        cur_tetrimino = Tetrimino_Empty;
        return;
    }
}

void Tetris_Loop() {
    static uint32_t last_droptick_ts = 0;
    static uint32_t last_tick_ts = 0;

    uint32_t now = HAL_GetTick();
    if (now - last_droptick_ts > droptick_period) {
        last_tick_ts = now;
        last_droptick_ts = now;
        Tetris_Tick(true);
        // PRINTF("tick\n");
    }
    if (now - last_tick_ts > 250) {
        last_tick_ts = now;
        Tetris_Tick(false);
        // PRINTF("tick\n");
    }
}

bool Tetris_RotatePiece(uint8_t rotations) {
    uint8_t new_rotation = (cur_rotation + rotations) % 4;

    const uint8_t *new_shape =
        Tetrimino_Shapes[cur_tetrimino] + new_rotation * TETRIMINO_SIZE;

    // check rotation collision
    if (check_collision(cur_x, cur_y, new_rotation)) return false;

    // carry out rotation
    for (uint8_t ty = 0; ty < TETRIMINO_HEIGHT; ty++) {
        for (uint8_t tx = 0; tx < TETRIMINO_WIDTH; tx++) {
            uint8_t pos = get_cur_pos(tx, ty);

            if (IS_FALLING_TETRIMINO(board[pos])) board[pos] = Tetrimino_Empty;

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
