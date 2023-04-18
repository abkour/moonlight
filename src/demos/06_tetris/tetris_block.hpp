#pragma once
#include "../../simple_math.hpp"
#include <vector>

#define CELL_EMPTY 0x00

constexpr int tetris_height = 20;
constexpr int tetris_width = 10;
constexpr int tetirs_border_xorigin = -tetris_width / 2;
constexpr int tetris_border_yorigin = -tetris_height / 2;

namespace moonlight
{

enum class BlockType
{
    Long = 0,
    L,
    T,
    Square,
    S,
    L2,
    S2
};

struct TetrisBlock
{
    TetrisBlock(BlockType type);
    TetrisBlock(const TetrisBlock& other);
    TetrisBlock& operator=(const TetrisBlock& other);

    void descend();

    int x(int i) const;
    int y(int i) const;

    void move_left(const std::vector<std::vector<unsigned>>& simulation_grid);
    void move_right(const std::vector<std::vector<unsigned>>& simulation_grid);
    int is_outside_left_bound();
    int is_outside_right_bound();

    union
    {
        struct
        {
            Vector2<int> b0, b1, b2, b3;
        };
        Vector2<int> b[4];
    };

    BlockType block_type;
    int rotate_position = 0;
    int color_id = 0x00;
};

TetrisBlock create_square_block();

TetrisBlock create_long_block();

TetrisBlock create_l_shaped_block();

TetrisBlock create_l2_shaped_block();

TetrisBlock create_t_shaped_block();

TetrisBlock create_s_shaped_block();

TetrisBlock create_s2_shaped_block();

TetrisBlock create_block();

TetrisBlock rotate_square_block(const TetrisBlock& block);

TetrisBlock rotate_l_block(const TetrisBlock& block);

TetrisBlock rotate_l2_block(const TetrisBlock& block);

TetrisBlock rotate_t_block(const TetrisBlock& block);

TetrisBlock rotate_s_block(const TetrisBlock& block);

TetrisBlock rotate_s2_block(const TetrisBlock& block);

TetrisBlock rotate_long_block(const TetrisBlock& block);

TetrisBlock rotate_block(const TetrisBlock& block, const std::vector<std::vector<unsigned>>& simulation_grid);


}