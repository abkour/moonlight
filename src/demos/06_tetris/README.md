## Tetris

This is a standalone tetris clone.

![frustum-culling](https://github.com/abkour/moonlight/blob/main/src/demos/06_tetris/res/tetris.gif)

### How it works

Tetris is a simple game. The game consists of a playfield, which is 20 blocks tall and 10 blocks wide.
A tetris block consists of 4 four mini blocks. There are a total of seven tetris blocks, differentiated
by their shape.
The game is never won, rather the player loses when a block reaches past the top of the playfield.

### Implementation

#### Tetris block

A tetris block is defined by four tuples. Each tuple defines the location of a 2D miniblock on the 
playfield. Additionally, a tetris block contains a block id, that is used to identify what shape it is,
a color id that identifies what color it has and a rotation id, that identifies in what rotational
state it is in. 

#### Block creation

Each block is created above the playing field, therefore the y-position is initially negative.
The blocks are offset to be within the middle of a grid line.
To create a block, we randomize between 0 and 6 inclusively. That number is used initialize 
the appropriate block.

#### Block rotation

To rotate a block, we use the rotation id. The initial rotation state of any tetris block is 0.
When a rotation takes place, we increment that value by one and rotate the block appropriately.
The actual rotation is hard-coded. Care has to be taken to ensure that the rotation does not
change the momentum of the block in any way.

#### Playfield

The playfield is implemented as two two-dimensional vectors. Each of which is 20 units tall
and 10 units wide. Each unit is an unsigned integer. The unsigned integer corresponds to the 
state of a grid cell. This value can be zero, meaning the cell is empty. When it is not zero,
it is the color_id of some block.

As to why there are two two-dimensional vectors. This was done for simplcity. We use the 
"simulation grid" and the "visualization grid". The simulation grid contains the blocks 
that have already felt on the bottom of the field or have collided with other blocks in the 
simulation grid. The visualization grid is the simulation grid in addition to the currently
dropping tetris block and the projection of that block to the nearest block in the simulation grid.

When it comes to rendering, we simply flatten out the 2D visualization grid into a 1D grid of 
unsigned integers.
For collision detection, we iterate over the simulation grid to find overlapping blocks. If those
are found, we handle the collision and update the simulation grid. If not, we simply proceed with
rendering. Once a collision takes place we have to synchronize with the visualization grid.

#### Game loop

The player input is queued in a short input buffer. That input buffer is fully read every X milliseconds.

### Knwon bugs

- There is a bug with the highlight block function. When an in-flight block is underneath a block
in the simulation grid, the highlight appears above said block. The correct behavior is that the 
block should be underneath it.