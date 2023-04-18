#pragma once
#include "tetris_block.hpp"

namespace moonlight
{

struct Playfield
{
    Playfield();

    void clear(const TetrisBlock& block);

    void display(const TetrisBlock& block);

    bool collide(const TetrisBlock& block);

    void drop_block(const TetrisBlock& block, const int line);

    int clear_lines();

    void quick_drop(TetrisBlock& block, bool space_bar_pressed);
    void quick_drop2(const TetrisBlock& highlight_block);

    TetrisBlock quick_drop_highlight(const TetrisBlock& block);
    TetrisBlock quick_drop_highlight2(const TetrisBlock& block);

    void synchronize_grids();

    // We use unsigned rather than bool or uint8_t.
    // This is because it makes instanced rendering easier to work with.
    // HLSL does not support integer types below 4 bytes, which is why
    // I made this decision.
    std::vector<std::vector<unsigned>> grid;
    std::vector<std::vector<unsigned>> simulation_grid;

    bool is_game_valid = true;
};

}