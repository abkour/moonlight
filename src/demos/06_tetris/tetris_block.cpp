#include "tetris_block.hpp"
#include "../../utility/random_number.hpp"

namespace moonlight
{

TetrisBlock::TetrisBlock(BlockType type)
    : block_type(type)
{
    switch (type)
    {
    case BlockType::Long:
        color_id = 1;
        break;
    case BlockType::L:
        color_id = 2;
        break;
    case BlockType::T:
        color_id = 3;
        break;
    case BlockType::L2:
        color_id = 4;
        break;
    case BlockType::S:
        color_id = 5;
        break;
    case BlockType::Square:
        color_id = 6;
        break;
    case BlockType::S2:
        color_id = 7;
        break;
    }
}

TetrisBlock::TetrisBlock(const TetrisBlock& other)
{
    b0 = other.b0;
    b1 = other.b1;
    b2 = other.b2;
    b3 = other.b3;
    block_type = other.block_type;
    rotate_position = other.rotate_position;
    color_id = other.color_id;
}

TetrisBlock& TetrisBlock::operator=(const TetrisBlock& other)
{
    b0 = other.b0;
    b1 = other.b1;
    b2 = other.b2;
    b3 = other.b3;
    block_type = other.block_type;
    rotate_position = other.rotate_position;
    color_id = other.color_id;

    return *this;
}

void TetrisBlock::descend()
{
    for (int i = 0; i < 4; ++i)
        b[i].y++;
}

int TetrisBlock::x(int i) const
{
    return b[i].x;
}

int TetrisBlock::y(int i) const
{
    return b[i].y;
}

void TetrisBlock::move_left(const std::vector<std::vector<unsigned>>& simulation_grid)
{
    if (b0.x - 1 < 0 || b1.x - 1 < 0 || b2.x - 1 < 0 || b3.x - 1 < 0)
    {
        return;
    }

    for (int i = 0; i < 4; ++i)
    {
        int nx = b[i].x - 1;

        if (b[i].y < 0)
            continue;

        if (simulation_grid[b[i].y][nx] != CELL_EMPTY)
        {
            return;
        }
    }

    b0.x--; b1.x--; b2.x--; b3.x--;
}

void TetrisBlock::move_right(const std::vector<std::vector<unsigned>>& simulation_grid)
{
    if (b0.x + 1 >= tetris_width ||
        b1.x + 1 >= tetris_width ||
        b2.x + 1 >= tetris_width ||
        b3.x + 1 >= tetris_width)
    {
        return;
    }

    for (int i = 0; i < 4; ++i)
    {
        int nx = b[i].x + 1;

        if (b[i].y < 0)
            continue;

        if (simulation_grid[b[i].y][nx] != CELL_EMPTY)
        {
            return;
        }
    }

    b0.x++; b1.x++; b2.x++; b3.x++;
}

int TetrisBlock::is_outside_left_bound()
{
    int max_extent = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (b[i].x < 0)
        {
            max_extent = std::min(max_extent, b[i].x);
        }
    }

    return -max_extent;
}

int TetrisBlock::is_outside_right_bound()
{
    int max_extent = 0;

    for (int i = 0; i < 4; ++i)
    {
        if (b[i].x >= tetris_width)
        {
            max_extent = std::max(max_extent, b[i].x - (static_cast<int>(tetris_width) - 1));
        }
    }

    return -max_extent;
}


TetrisBlock create_square_block()
{
    TetrisBlock square_block(BlockType::Square);

    square_block.b0.x = tetris_width / 2 - 1;
    square_block.b0.y = -1;

    square_block.b1.x = tetris_width / 2;
    square_block.b1.y = -1;

    square_block.b2.x = tetris_width / 2 - 1;
    square_block.b2.y = -2;

    square_block.b3.x = tetris_width / 2;
    square_block.b3.y = -2;

    return square_block;
}

TetrisBlock create_long_block()
{
    TetrisBlock long_block(BlockType::Long);

    long_block.b0.x = tetris_width / 2;
    long_block.b0.y = -1;

    long_block.b1.x = tetris_width / 2;
    long_block.b1.y = -2;

    long_block.b2.x = tetris_width / 2;
    long_block.b2.y = -3;

    long_block.b3.x = tetris_width / 2;
    long_block.b3.y = -4;

    return long_block;
}

TetrisBlock create_l_shaped_block()
{
    TetrisBlock l_shaped_block(BlockType::L);

    l_shaped_block.b0.x = tetris_width / 2 - 1;
    l_shaped_block.b0.y = -1;

    l_shaped_block.b1.x = tetris_width / 2 - 1;
    l_shaped_block.b1.y = -2;

    l_shaped_block.b2.x = tetris_width / 2 - 1;
    l_shaped_block.b2.y = -3;

    l_shaped_block.b3.x = tetris_width / 2;
    l_shaped_block.b3.y = -1;

    return l_shaped_block;
}

TetrisBlock create_l2_shaped_block()
{
    TetrisBlock l_shaped_block(BlockType::L2);

    l_shaped_block.b0.x = tetris_width / 2 - 1;
    l_shaped_block.b0.y = -1;

    l_shaped_block.b1.x = tetris_width / 2 - 1;
    l_shaped_block.b1.y = -2;

    l_shaped_block.b2.x = tetris_width / 2 - 1;
    l_shaped_block.b2.y = -3;

    l_shaped_block.b3.x = tetris_width / 2 - 2;
    l_shaped_block.b3.y = -1;

    return l_shaped_block;
}

TetrisBlock create_t_shaped_block()
{
    TetrisBlock t_shaped_block(BlockType::T);

    t_shaped_block.b0.x = tetris_width / 2 - 2;
    t_shaped_block.b0.y = -1;

    t_shaped_block.b1.x = tetris_width / 2 - 1;
    t_shaped_block.b1.y = -1;

    t_shaped_block.b2.x = tetris_width / 2;
    t_shaped_block.b2.y = -1;

    t_shaped_block.b3.x = tetris_width / 2 - 1;
    t_shaped_block.b3.y = -2;

    return t_shaped_block;
}

TetrisBlock create_s_shaped_block()
{
    TetrisBlock s_shaped_block(BlockType::S);

    s_shaped_block.b0.x = tetris_width / 2 - 1;
    s_shaped_block.b0.y = -3;

    s_shaped_block.b1.x = tetris_width / 2 - 1;
    s_shaped_block.b1.y = -2;

    s_shaped_block.b2.x = tetris_width / 2;
    s_shaped_block.b2.y = -2;

    s_shaped_block.b3.x = tetris_width / 2;
    s_shaped_block.b3.y = -1;

    return s_shaped_block;
}

TetrisBlock create_s2_shaped_block()
{
    TetrisBlock s_shaped_block(BlockType::S2);

    s_shaped_block.b0.x = tetris_width / 2;
    s_shaped_block.b0.y = -3;

    s_shaped_block.b1.x = tetris_width / 2;
    s_shaped_block.b1.y = -2;

    s_shaped_block.b2.x = tetris_width / 2 - 1;
    s_shaped_block.b2.y = -2;

    s_shaped_block.b3.x = tetris_width / 2 - 1;
    s_shaped_block.b3.y = -1;

    return s_shaped_block;
}

TetrisBlock create_block()
{
    int r = random_in_range_int(0, 6);
    switch (r)
    {
    case 0:
        return create_square_block();
    case 1:
        return create_long_block();
    case 2:
        return create_l_shaped_block();
    case 3:
        return create_t_shaped_block();
    case 4:
        return create_s_shaped_block();
    case 5:
        return create_l2_shaped_block();
    case 6:
        return create_s2_shaped_block();
    }

    // Just so the compiler shuts up, this case doesn't happen.
    return create_square_block();
}

TetrisBlock rotate_square_block(const TetrisBlock& block)
{
    return block;
}

TetrisBlock rotate_l_block(const TetrisBlock& block)
{
    TetrisBlock nb(block);

    switch (nb.rotate_position)
    {
    case 0:
    {
        nb.b0.y++;
        nb.b1.x++;
        nb.b1.y++;
        nb.b2.x += 2;
        nb.b2.y += 2;
        nb.b3.x--;
        nb.rotate_position = 1;
    }
    break;
    case 1:
    {
        nb.b0.y--;
        nb.b1.y++;
        nb.b2.x--;
        nb.b2.y += 2;
        nb.b3.x++;
        nb.rotate_position = 2;
    }
    break;
    case 2:
    {
        nb.b0.x++;
        nb.b1.x--;
        nb.b1.y--;
        nb.b2.x -= 2;
        nb.b2.y -= 2;
        nb.b3.y--;
        nb.rotate_position = 3;
    }
    break;
    case 3:
    {
        nb.b0.x--;
        nb.b1.y--;
        nb.b2.x++;
        nb.b2.y -= 2;
        nb.b3.y++;
        nb.rotate_position = 0;
    }
    break;
    default:
        break;
    }

    return nb;
}

TetrisBlock rotate_l2_block(const TetrisBlock& block)
{
    TetrisBlock nb(block);

    switch (nb.rotate_position)
    {
    case 0:
    {
        nb.b0.x--;
        nb.b1.y++;
        nb.b2.x++;
        nb.b2.y += 2;
        nb.b3.y--;
        nb.rotate_position = 1;
    }
    break;
    case 1:
    {
        nb.b0.y--;
        nb.b1.x--;
        nb.b2.x -= 2;
        nb.b2.y++;
        nb.b3.x++;
        nb.rotate_position = 2;
    }
    break;
    case 2:
    {
        nb.b0.x++;
        nb.b1.y--;
        nb.b2.x--;
        nb.b2.y -= 2;
        nb.b3.y++;
        nb.rotate_position = 3;
    }
    break;
    case 3:
    {
        nb.b0.y++;
        nb.b1.x++;
        nb.b2.x += 2;
        nb.b2.y--;
        nb.b3.x--;
        nb.rotate_position = 0;
    }
    break;
    default:
        break;
    }

    return nb;
}

TetrisBlock rotate_t_block(const TetrisBlock& block)
{
    TetrisBlock nb(block);

    switch (nb.rotate_position)
    {
    case 0:
    {
        nb.b0.x++;
        nb.b0.y--;
        nb.b2.x--;
        nb.b2.y++;
        nb.b3.x++;
        nb.b3.y++;
        nb.rotate_position = 1;
    }
    break;
    case 1:
    {
        nb.b0.x++;
        nb.b0.y++;
        nb.b2.x--;
        nb.b2.y--;
        nb.b3.x--;
        nb.b3.y++;
        nb.rotate_position = 2;
    }
    break;
    case 2:
    {
        nb.b0.x--;
        nb.b0.y++;
        nb.b2.x++;
        nb.b2.y--;
        nb.b3.x--;
        nb.b3.y--;
        nb.rotate_position = 3;
    }
    break;
    case 3:
    {
        nb.b0.x--;
        nb.b0.y--;
        nb.b2.x++;
        nb.b2.y++;
        nb.b3.x++;
        nb.b3.y--;
        nb.rotate_position = 0;
    }
    break;
    default:
        break;
    }

    return nb;
}

TetrisBlock rotate_s_block(const TetrisBlock& block)
{
    TetrisBlock nb(block);

    switch (nb.rotate_position)
    {
    case 0:
    {
        nb.b0.x += 2;
        nb.b0.y++;
        nb.b1.x++;
        nb.b2.y++;
        nb.b3.x--;
        nb.rotate_position = 1;
    }
    break;
    case 1:
    {
        nb.b0.x -= 2;
        nb.b0.y--;
        nb.b1.x--;
        nb.b2.y--;
        nb.b3.x++;
        nb.rotate_position = 0;
    }
    break;
    default:
        break;
    }

    return nb;
}

TetrisBlock rotate_s2_block(const TetrisBlock& block)
{
    TetrisBlock nb(block);

    switch (nb.rotate_position)
    {
    case 0:
    {
        nb.b0.x++;
        nb.b0.y++;
        nb.b2.x++;
        nb.b2.y--;
        nb.b3.y -= 2;
        nb.rotate_position = 1;
    }
    break;
    case 1:
    {
        nb.b0.x--;
        nb.b0.y--;
        nb.b2.x--;
        nb.b2.y++;
        nb.b3.y += 2;
        nb.rotate_position = 0;
    }
    break;
    default:
        break;
    }

    return nb;
}

TetrisBlock rotate_long_block(const TetrisBlock& block)
{
    TetrisBlock nb(block);

    switch (nb.rotate_position)
    {
    case 0:
    {
        nb.b0.x--;
        nb.b0.y--;
        nb.b2.x++;
        nb.b2.y++;
        nb.b3.x += 2;
        nb.b3.y += 2;
        nb.rotate_position = 1;
    }
    break;
    case 1:
    {
        nb.b0.x++;
        nb.b0.y++;
        nb.b2.x--;
        nb.b2.y--;
        nb.b3.x -= 2;
        nb.b3.y -= 2;
        nb.rotate_position = 0;
    }
    break;
    default:
        break;
    }

    return nb;
}

TetrisBlock rotate_block(const TetrisBlock& block, const std::vector<std::vector<unsigned>>& simulation_grid)
{
    TetrisBlock nb(block.block_type);

    switch (block.block_type)
    {
    case BlockType::Square:
        nb = rotate_square_block(block);
        break;
    case BlockType::Long:
        nb = rotate_long_block(block);
        break;
    case BlockType::T:
        nb = rotate_t_block(block);
        break;
    case BlockType::S:
        nb = rotate_s_block(block);
        break;
    case BlockType::L:
        nb = rotate_l_block(block);
        break;
    case BlockType::L2:
        nb = rotate_l2_block(block);
        break;
    case BlockType::S2:
        nb = rotate_s2_block(block);
        break;
    }

    int max_extent_left = nb.is_outside_left_bound();
    int max_extent_right = nb.is_outside_right_bound();
    if (max_extent_left != 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            nb.b[i].x += max_extent_left;
        }
        return nb;
    }

    if (max_extent_right != 0)
    {
        for (int i = 0; i < 4; ++i)
        {
            nb.b[i].x += max_extent_right;
        }
        return nb;
    }

    for (int i = 0; i < 4; ++i)
    {
        if (nb.b[i].y >= tetris_height)
        {
            return block;
        }

        if (nb.b[i].y >= 0 && nb.b[i].y < tetris_height)
        {
            if (simulation_grid[nb.b[i].y][nb.b[i].x] != CELL_EMPTY)
            {
                return block;
            }
        }
    }

    return nb;
}


}