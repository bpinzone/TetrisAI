# TetrisAI

Meet Jeff, the Tetris 99 playing AI.
  
Created by myself and my magnificent roommate [Steven Schulte!](https://github.com/spschul)
  
[See Jeff in action!](https://www.youtube.com/watch?v=4Jm-x91pVQY)
* He typically makes it to the final 15 players, battling against 98 other human players online in real time!
  
"Jeff's Brain" was pair programmed by the two of us in C++.
* Multi-threaded DFS with work stealing.
* Highly profiled and optimized code.
* Jeff achieves 95% of all theoretically possible tetrises when he looks 6 moves into the future!
  
"Jeff's Eyes" were implemented by Steven in Python using OpenCV.
* Uses a webcam, so no physical connection to the TV or Switch is required!
* Waits for a stable image before reading the game state. This allows us to ignore the firework VFX when getting line clears, among other visual obstructions.
* Detects when the queue of blocks shifts so we know when it is time to act.
* Uses template matching to figure out what blocks are in the queue.
  
"Jeff's Hands" are a slightly modified version of [this repository.](https://github.com/mart1nro/joycontrol)
* All credit to Robert Martin (mart1nro) and other contributors.
* Allows us to communicate with the Nintendo Switch via a Bluetooth connection on the laptop.
  
Watch Jeff play in your terminal:
* $ make
* "Usage: <mode: wsmr> <block generation: seed# or i> <lookahead> <queue size> <game length> <num threads> <e to see board log, anything else otherwise>"
* For example: ./main w 0 7 6 100 20 e
* Notes:
    * If you want to speed him up or slow him down, change how many moves he looks ahead.
    * "Tetris percent" is the percentage of his block placements that result in a tetris.
        * The best theoretically possible tetris percent is 10%.
