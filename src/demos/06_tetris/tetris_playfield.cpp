#include "tetris_playfield.hpp"

namespace moonlight
{

Playfield::Playfield()
{
    grid.resize(tetris_height);
    simulation_grid.resize(tetris_height);
    for (auto& grid_line : grid)
    {
        grid_line.resize(tetris_width, CELL_EMPTY);
    }
    for (auto& grid_line : simulation_grid)
    {
        grid_line.resize(tetris_width, CELL_EMPTY);
    }
}

void Playfield::clear(const TetrisBlock& block)
{
    for (int i = 0; i < 4; ++i)
    {
        if (block.b[i].y >= 0)
        {
            grid[block.b[i].y][block.b[i].x] = CELL_EMPTY;
        }
    }
}

void Playfield::display(const TetrisBlock& block)
{
    for (int i = 0; i < 4; ++i)
    {
        if (block.b[i].y >= 0)
        {
            grid[block.b[i].y][block.b[i].x] = block.color_id;
        }
    }
}

bool Playfield::collide(const TetrisBlock& block)
{
    for (int y = 0; auto & grid_line : simulation_grid)
    {
        for (int x = 0; auto & cell : grid_line)
        {
            for (int i = 0; i < 4; ++i)
            {
                if (block.y(i) + 1 == y) // Read above explanation!
                {
                    if (block.x(i) == x && cell != CELL_EMPTY)
                    {
                        // Drop the block in place
                        drop_block(block, y);
                        return true;
                    }
                }
            }

            ++x;
        }
        ++y;
    }

    for (int i = 0; i < 4; ++i)
    {
        if (block.b[i].y == tetris_height - 1)
        {
            drop_block(block, tetris_height - 1);
            return true;
        }
    }

    return false;
}

void Playfield::drop_block(const TetrisBlock& block, const int line)
{
    int current_line = line;
    for (int i = 0; i < 4; ++i)
    {
        if (block.y(i) < current_line)
        {
            current_line = block.y(i);
        }
        if (current_line < 0)
        {
            is_game_valid = false;
            // Don't exit the function yet. Update the state of the grid, to illustrate
            // to the player that the block is over the playing field.
        }

        if (block.y(i) >= 0)
            simulation_grid[block.y(i)][block.x(i)] = block.color_id;
    }
}

int Playfield::clear_lines()
{
    int result = 0;

    for (int y = 0; auto & grid_line : simulation_grid)
    {
        bool fully_occupied = true;
        for (const auto& cell : grid_line)
        {
            if (cell == CELL_EMPTY)
            {
                fully_occupied = false;
                break;
            }
        }

        if (fully_occupied)
        {
            for (auto& cell : grid_line)
            {
                cell = CELL_EMPTY;
            }

            // Drop all lines above grid_lines by one
            for (int i = y; i > 0; --i)
            {
                auto& upper_line = simulation_grid[i - 1];
                std::swap(upper_line, simulation_grid[i]);
            }

            result++;
        }

        ++y;
    }

    return result;
}

void Playfield::quick_drop(TetrisBlock& block, bool space_bar_pressed)
{
    int y = 0;
    for (; y < tetris_height; ++y)
    {
        bool is_collision = false;
        for (int i = 0; i < 4; ++i)
        {
            unsigned cell = simulation_grid[y][block.b[i].x];
            if (cell != CELL_EMPTY)
                is_collision = true;
        }
        if (is_collision)
            break;
    }

    int min_diff = std::numeric_limits<int>::max();
    for (int i = 0; i < 4; ++i)
    {
        int diff = y - block.b[i].y;
        min_diff = std::min(min_diff, diff);
    }

    if (space_bar_pressed)
    {
        for (int i = 0; i < 4; ++i)
        {
            block.b[i].y += min_diff - 1;
        }
    }
}

void Playfield::quick_drop2(const TetrisBlock& highlight_block)
{
    for (int i = 0; i < 4; ++i)
    {
        int bx = highlight_block.b[i].x;
        int by = highlight_block.b[i].y;
        simulation_grid[by][bx] = highlight_block.color_id;
    }
}

TetrisBlock Playfield::quick_drop_highlight(const TetrisBlock& block)
{
    int y = 0;
    for (; y < tetris_height; ++y)
    {
        bool is_collision = false;
        for (int i = 0; i < 4; ++i)
        {
            unsigned cell = simulation_grid[y][block.b[i].x];
            if (cell != CELL_EMPTY)
            {
                is_collision = true;
            }
        }

        if (is_collision)
            break;
    }

    int min_diff = std::numeric_limits<int>::max();
    for (int i = 0; i < 4; ++i)
    {
        int diff = 100;
        if (y < tetris_height)
        {
            unsigned cell = simulation_grid[y][block.b[i].x];
            if (cell != CELL_EMPTY)
            {
                diff = y - block.b[i].y;
                min_diff = std::min(min_diff, diff);
            }
        } else
        {
            diff = tetris_height - block.b[i].y;
            min_diff = std::min(min_diff, diff);
        }
    }

    TetrisBlock highlight_block(block);

    for (int i = 0; i < 4; ++i)
    {
        int yy = block.b[i].y + min_diff - 1;
        grid[yy][block.b[i].x] = 0x08;

        highlight_block.b[i].y += min_diff - 1;
    }

    return highlight_block;
}

TetrisBlock Playfield::quick_drop_highlight2(const TetrisBlock& block)
{
    int min_diff = std::numeric_limits<int>::max();
    int y = 0;
    for (; y < tetris_height; ++y)
    {
        for (int i = 0; i < 4; ++i)
        {
            unsigned cell = simulation_grid[y][block.b[i].x];
            if (cell != CELL_EMPTY)
            {
                int diff = y - block.b[i].y;
                min_diff = std::min(min_diff, diff);
            }
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        int diff = tetris_height - block.b[i].y;
        min_diff = std::min(min_diff, diff);
    }

    TetrisBlock highlight_block(block);

    for (int i = 0; i < 4; ++i)
    {
        int yy = block.b[i].y + min_diff - 1;
        if (yy < 0)
            continue;
        grid[yy][block.b[i].x] = block.color_id | 0x8000'0000;

        highlight_block.b[i].y += min_diff - 1;
    }

    return highlight_block;
}

void Playfield::synchronize_grids()
{
    for (int i = 0; i < tetris_height; ++i)
    {
        for (int j = 0; j < tetris_width; ++j)
        {
            grid[i][j] = simulation_grid[i][j];
        }
    }
}

}