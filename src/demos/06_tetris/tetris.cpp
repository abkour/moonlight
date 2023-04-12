#include "tetris.hpp"
#include "../../simple_math.hpp"
#include "../../utility/random_number.hpp"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace
{

constexpr unsigned tetris_height = 20;
constexpr unsigned tetris_width  = 10;
constexpr int tetirs_border_xorigin = -static_cast<int>(tetris_width) / 2;
constexpr int tetris_border_yorigin = -static_cast<int>(tetris_height) / 2;

static const float quad_vertices[] =
{
    -1.f, -1.f,    1.f, -1.f,    1.f, 1.f,
    -1.f, -1.f,    1.f, 1.f,    -1.f, 1.f
};

// 10 width x 20 height

static const float borders[] =
{
    // Horizontal line at the bottom
    tetirs_border_xorigin, tetris_border_yorigin,
    tetirs_border_xorigin + tetris_width, tetris_border_yorigin,

    // Vertical line at the left
    tetirs_border_xorigin, tetris_border_yorigin,
    tetirs_border_xorigin, tetris_border_yorigin + tetris_height,

    // Vertical line at the right
    tetirs_border_xorigin + tetris_width, tetris_border_yorigin,
    tetirs_border_xorigin + tetris_width, tetris_border_yorigin + tetris_height
};

}

namespace moonlight
{

static struct ScenePipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE root_signature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT input_layout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitive_topology;
    CD3DX12_PIPELINE_STATE_STREAM_VS vs;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rs;
    CD3DX12_PIPELINE_STATE_STREAM_PS ps;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsv_format;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtv_formats;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL ds_desc;
} scene_pipeline_state_stream;

#define CELL_EMPTY 0x00

enum class BlockType
{
    Long = 0,
    L,
    T,
    Square,
    S,
    L2,
    S2
    
    /*
    
        Description:
            S      Sq.   Long    T      L

        	x      x x    x      x      x
	        x x    x x    x    x x x    x
	          x           x             x x
					      x
    */
};

struct TetrisBlock
{
    TetrisBlock(BlockType type)
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

    TetrisBlock(const TetrisBlock& other)
    {
        b0 = other.b0;
        b1 = other.b1;
        b2 = other.b2;
        b3 = other.b3;
        block_type = other.block_type;
        rotate_position = other.rotate_position;
        color_id = other.color_id;
    }

    TetrisBlock& operator=(const TetrisBlock& other)
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

    void descend()
    {
        for (int i = 0; i < 4; ++i)
            b[i].y++;
    }

    union
    {
        struct
        {
            Vector2<int> b0, b1, b2, b3;
        };
        Vector2<int> b[4];
    };

    int x(int i) const
    {
        return b[i].x;
    }

    int y(int i) const
    {
        return b[i].y;
    }

    void move_left(const std::vector<std::vector<unsigned>>& simulation_grid)
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

    void move_right(const std::vector<std::vector<unsigned>>& simulation_grid)
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

    BlockType block_type;
    int rotate_position = 0;
    int color_id = 0x00;
};

TetrisBlock create_square_block()
{
    TetrisBlock square_block(BlockType::Square);

    square_block.b0.x = tetris_width / 2 - 1;
    square_block.b0.y = -1;

    square_block.b1.x = tetris_width / 2;
    square_block.b1.y = -1;

    square_block.b2.x = tetris_width / 2 -1;
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
        nb.b1.x++;
        nb.b1.y++;
        nb.b2.x += 2;
        nb.b2.y += 2;
        nb.b3.x--;
        nb.b3.y++;
        nb.rotate_position = 1;
    }
    break;
    case 1:
    {
        nb.b0.y -= 2;
        nb.b1.x--;
        nb.b1.y--;
        nb.b2.x -= 2;
        nb.b3.x--;
        nb.b3.y -= 3;
        nb.rotate_position = 2;
    }
    break;
    case 2:
    {
        nb.b0.x += 2;
        nb.b0.y += 2;
        nb.b1.x++;
        nb.b1.y++;
        nb.b3.x += 3;
        nb.b3.y++;
        nb.rotate_position = 3;
    }
    break;
    case 3:
    {
        nb.b0.x -= 2;
        nb.b0.y += 2;
        nb.b1.x--;
        nb.b1.y++;
        nb.b3.x--;
        nb.b3.y += 3;
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

    for (int i = 0; i < 4; ++i)
    {
        if (nb.b[i].x < 0 || nb.b[i].x >= tetris_width)
        {
            return block;
        }

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

struct Tetris::Playfield
{
    Playfield()
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

    void clear(const TetrisBlock& block)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (block.b[i].y >= 0)
            {
                grid[block.b[i].y][block.b[i].x] = CELL_EMPTY;
            }
        }
    }

    void display(const TetrisBlock& block)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (block.b[i].y >= 0)
            {
                grid[block.b[i].y][block.b[i].x] = block.color_id;
            }
        }
    }

    bool collide(const TetrisBlock& block)
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

    void drop_block(const TetrisBlock& block, const int line)
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

            simulation_grid[block.y(i)][block.x(i)] = block.color_id;
        }
    }

    void clear_lines()
    {
        for (int y = 0; auto& grid_line : simulation_grid)
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
            }

            ++y;
        }
    }

    void quick_drop(TetrisBlock& block, bool space_bar_pressed)
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

    TetrisBlock quick_drop_highlight(const TetrisBlock& block)
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
        
        TetrisBlock highlight_block(block);

        for (int i = 0; i < 4; ++i)
        {
            int y = block.b[i].y + min_diff - 1;
            grid[y][block.b[i].x] = 0x08;

            highlight_block.b[i].y += min_diff - 1;
        }

        return highlight_block;
    }

    void synchronize_grids()
    {
        for (int i = 0; i < tetris_height; ++i)
        {
            for (int j = 0; j < tetris_width; ++j)
            {
                grid[i][j] = simulation_grid[i][j];
            }
        }
    }

    bool is_game_valid = true;

    // We use unsigned rather than bool or uint8_t.
    // This is because it makes instanced rendering easier to work with.
    // HLSL does not support integer types below 4 bytes, which is why
    // I made this decision.
    std::vector<std::vector<unsigned>> grid;
    std::vector<std::vector<unsigned>> simulation_grid;
};

Tetris::Tetris(HINSTANCE hinstance)
    : IApplication(hinstance)
{
    m_window = std::make_unique<Window>(
        hinstance,
        L"DX12MoonlightApplication",
        L"DX12_Demo_Template",
        1600,
        800,
        &Tetris::WindowMessagingProcess,
        this
    );

    SetCursor(NULL);
    initialize_raw_input_devices();

    ComPtr<IDXGIAdapter4> most_sutiable_adapter = _pimpl_create_adapter();
    m_device = _pimpl_create_device(most_sutiable_adapter);
    m_command_queue = std::make_unique<CommandQueue>(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_allocator = _pimpl_create_command_allocator(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_command_list_direct = _pimpl_create_command_list(m_device, m_command_allocator, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_dsv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2
    );

    m_srv_descriptor_heap = std::make_unique<DescriptorHeap>(
        m_device.Get(),
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1
    );

    m_swap_chain = std::make_unique<SwapChain>(
        m_device.Get(),
        m_command_queue->get_underlying(),
        m_window->width(),
        m_window->height(),
        m_window->handle
    );

    m_scissor_rect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    m_viewport = CD3DX12_VIEWPORT(
        0.f,
        0.f,
        static_cast<float>(m_window->width()),
        static_cast<float>(m_window->height())
    );

    m_depth_buffer = _pimpl_create_dsv(
        m_device,
        m_dsv_descriptor_heap->cpu_handle(),
        m_window->width(), m_window->height()
    );

    load_assets();

    m_application_initialized = true;
}

Tetris::~Tetris()
{
}

void Tetris::flush() 
{
    m_command_queue->flush();
}

void Tetris::on_mouse_move(LPARAM) 
{
}

void Tetris::render() 
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();

    // Clear
    {
        m_swap_chain->transition_to_rtv(m_command_list_direct.Get());

        // Clear backbuffer
        const FLOAT clear_color[] = { 0.05f, 0.05f, 0.05f, 1.f };
        m_command_list_direct->ClearRenderTargetView(
            m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx),
            clear_color,
            0,
            NULL
        );

        m_command_list_direct->ClearDepthStencilView(
            m_dsv_descriptor_heap->cpu_handle(),
            D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL
        );
    }

    record_command_list(m_command_list_direct.Get());
    record_gui_commands(m_command_list_direct.Get());

    // Present
    {
        m_swap_chain->transition_to_present(m_command_list_direct.Get());

        m_command_list_direct->Close();
        ID3D12CommandList* command_lists[] =
        {
            m_command_list_direct.Get()
        };

        m_command_queue->execute_command_list(command_lists, 1);
        m_command_queue->signal();
        m_swap_chain->present();
        m_command_queue->wait_for_fence();

        ThrowIfFailed(m_command_allocator->Reset());
        ThrowIfFailed(m_command_list_direct->Reset(m_command_allocator.Get(), nullptr));
    }
}

void Tetris::resize() 
{
}

void Tetris::update() 
{
    float aspect_ratio = static_cast<float>(m_window->width()) / static_cast<float>(m_window->height());

    static float near_clip_distance = 0.1f;
    static float far_clip_distance = 500.f;
    XMMATRIX projection_matrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(45.f),
        aspect_ratio,
        near_clip_distance,
        far_clip_distance
    );

    static float simulation_scale = 1.f;
    static float render_threshold_time = 0.f;
    static float ms_threshold_time = 0.f;
    static float total_time = 0.f;
    static auto t0 = std::chrono::high_resolution_clock::now();
    auto t1 = std::chrono::high_resolution_clock::now();
    m_elapsed_time = (t1 - t0).count() * 1e-9;
    t0 = t1;
    total_time += m_elapsed_time;
    ms_threshold_time += m_elapsed_time;
    render_threshold_time += m_elapsed_time;

    float bsx = 20.f / m_window->width();
    float bsy = 20.f / m_window->height();
    m_mvp_matrix = XMMatrixScaling(bsx, bsy, 0.f);

    float pfx = 0.f;
    float pfy = 0.5f;
    XMMATRIX translate_matrix = XMMatrixTranslation(pfx, pfy, 0.f);
    m_mvp_matrix = XMMatrixMultiply(m_mvp_matrix, translate_matrix);

    if (m_field->is_game_valid)
    {
        static TetrisBlock tetris_block = create_block();
        static bool is_collide = false;
        
        if (render_threshold_time > 0.0166f)
        {
            TetrisBlock highlight_block = m_field->quick_drop_highlight(tetris_block);
            m_field->display(tetris_block);
            m_field->clear_lines();
            update_instanced_buffer();
            m_field->clear(tetris_block);
            m_field->clear(highlight_block);
            render_threshold_time = 0.f;
        }

        if (ms_threshold_time > 0.1f)
        {
            if (m_keyboard_state['A'])
            {
                tetris_block.move_left(m_field->simulation_grid);
            }
            if (m_keyboard_state['D'])
            {
                tetris_block.move_right(m_field->simulation_grid);
            }
            if (m_keyboard_state['S'])
            {
                simulation_scale = 0.25f;
            }
            if (!m_keyboard_state['S'])
            {
                simulation_scale = 1.f;
            }
            if (m_keyboard_state['W'])
            {
                tetris_block = rotate_block(tetris_block, m_field->simulation_grid);
            }

            m_field->quick_drop(tetris_block, m_keyboard_state[KeyCode::Spacebar]);
            
            ms_threshold_time = 0.f;
        }

        if (total_time > 0.33f * simulation_scale)
        {
            is_collide = m_field->collide(tetris_block);

            m_field->synchronize_grids();

            if (is_collide)
            {
                tetris_block = create_block();
                is_collide = false;
            } 
            else
                tetris_block.descend();

            total_time = 0.f;
        }
    }
}

void Tetris::update_instanced_buffer()
{
    for (int y = 0; y < m_field->grid.size(); ++y)
    {
        for (int x = 0; x < m_field->grid[y].size(); ++x)
        {
            int i = y * tetris_width + x;
            instances_buffer[i] = m_field->grid[y][x];
        }
    }

    instance_buffer_rsc->update(
        m_device.Get(), m_command_list_direct.Get(),
        instances_buffer.data(), sizeof(unsigned) * instances_buffer.size(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    D3D12_BUFFER_SRV buffer_desc = {};
    buffer_desc.FirstElement = 0;
    buffer_desc.NumElements = m_field->grid.size();
    buffer_desc.StructureByteStride = sizeof(unsigned);

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer = buffer_desc;
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    m_device->CreateShaderResourceView(
        instance_buffer_rsc->get_underlying(),
        &srv_desc,
        m_srv_descriptor_heap->cpu_handle()
    );
}

void Tetris::record_gui_commands(ID3D12GraphicsCommandList* command_list)
{
}

void Tetris::record_command_list(ID3D12GraphicsCommandList* command_list)
{
    uint8_t backbuffer_idx = m_swap_chain->current_backbuffer_index();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_dsv_descriptor_heap->cpu_handle();
    UINT rtv_inc_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE backbuffer_rtv_handle =
        m_swap_chain->backbuffer_rtv_descriptor_handle(backbuffer_idx);

    //
    // Render Scene to Texture
    command_list->SetPipelineState(m_scene_pso.Get());
    command_list->SetGraphicsRootSignature(m_scene_root_signature.Get());
    command_list->SetGraphicsRootShaderResourceView(1, instance_buffer_rsc->gpu_virtual_address());

    D3D12_VERTEX_BUFFER_VIEW vb_views[] = { m_vertex_buffer_view };
    command_list->IASetVertexBuffers(0, _countof(vb_views), vb_views);
    command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    command_list->RSSetViewports(1, &m_viewport);
    command_list->RSSetScissorRects(1, &m_scissor_rect);
    command_list->OMSetRenderTargets(1, &backbuffer_rtv_handle, FALSE, &dsv_handle);
    command_list->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / sizeof(float), &m_mvp_matrix, 0);
    command_list->DrawInstanced(sizeof(quad_vertices) / (sizeof(float) * 2), tetris_width * tetris_height, 0, 0);
}

void Tetris::load_assets()
{
    m_field = std::make_unique<Tetris::Playfield>();
    instances_buffer.resize(tetris_width * tetris_height, 0xFF);
    load_scene_shader_assets();
}

void Tetris::load_scene_shader_assets()
{
    {
        m_vertex_buffer = std::make_unique<DX12Resource>();
        m_vertex_buffer->upload(m_device.Get(), m_command_list_direct.Get(),
            (float*)quad_vertices,
            sizeof(quad_vertices),
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        );

        m_vertex_buffer_view.BufferLocation = m_vertex_buffer->gpu_virtual_address();
        m_vertex_buffer_view.SizeInBytes = sizeof(quad_vertices);
        m_vertex_buffer_view.StrideInBytes = sizeof(float) * 2;
    }

    // Instance data SRV
    {
        instance_buffer_rsc = std::make_unique<DX12Resource>();
        instance_buffer_rsc->upload(
            m_device.Get(), m_command_list_direct.Get(),
            instances_buffer.data(), sizeof(unsigned) * instances_buffer.size(),
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );

        D3D12_BUFFER_SRV buffer_desc = {};
        buffer_desc.FirstElement = 0;
        buffer_desc.NumElements = m_field->grid.size();
        buffer_desc.StructureByteStride = sizeof(unsigned);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer = buffer_desc;
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        m_device->CreateShaderResourceView(
            instance_buffer_rsc->get_underlying(),
            &srv_desc,
            m_srv_descriptor_heap->cpu_handle()
        );
    }

    ComPtr<ID3DBlob> vs_blob;
    ComPtr<ID3DBlob> ps_blob;
    {
        std::wstring vspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/06_tetris/shaders/tetris_vs.cso";
        std::wstring pspath = std::wstring(ROOT_DIRECTORY_WIDE) + L"/src/demos/06_tetris/shaders/tetris_ps.cso";
        ThrowIfFailed(D3DReadFileToBlob(vspath.c_str(), &vs_blob));
        ThrowIfFailed(D3DReadFileToBlob(pspath.c_str(), &ps_blob));
    }

    D3D12_INPUT_ELEMENT_DESC input_layout[] = 
    {
        {   "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
        } 
    };

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
    {
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 root_parameters[2];
    // Three float4x4
    root_parameters[0].InitAsConstants(sizeof(XMMATRIX) / sizeof(float), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    root_parameters[1].InitAsShaderResourceView(0);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
    // TODO: Is this call correct, if the highest supported version is 1_0?
    root_signature_desc.Init_1_1(
        _countof(root_parameters), root_parameters,
        0, nullptr,
        root_signature_flags
    );

    ComPtr<ID3DBlob> root_signature_blob;
    // TODO: What is the error blob=
    ComPtr<ID3DBlob> error_blob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(
        &root_signature_desc,
        feature_data.HighestVersion,
        &root_signature_blob,
        &error_blob
    ));

    ThrowIfFailed(m_device->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&m_scene_root_signature)
    ));

    D3D12_RT_FORMAT_ARRAY rtv_formats = {};
    rtv_formats.NumRenderTargets = 1;
    rtv_formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    CD3DX12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    CD3DX12_DEFAULT default_initializer;
    CD3DX12_DEPTH_STENCIL_DESC dsv_desc(default_initializer);
    dsv_desc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    scene_pipeline_state_stream.dsv_format = DXGI_FORMAT_D32_FLOAT;
    scene_pipeline_state_stream.input_layout = { input_layout, _countof(input_layout) };
    scene_pipeline_state_stream.primitive_topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    scene_pipeline_state_stream.ps = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    scene_pipeline_state_stream.root_signature = m_scene_root_signature.Get();
    scene_pipeline_state_stream.rs = rasterizer_desc;
    scene_pipeline_state_stream.rtv_formats = rtv_formats;
    scene_pipeline_state_stream.vs = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    scene_pipeline_state_stream.ds_desc = dsv_desc;

    D3D12_PIPELINE_STATE_STREAM_DESC pss_desc = {
        sizeof(ScenePipelineStateStream),
        &scene_pipeline_state_stream
    };

    ThrowIfFailed(m_device->CreatePipelineState(&pss_desc, IID_PPV_ARGS(&m_scene_pso)));
}

}