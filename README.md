First place in Tetris 99 using computer vision, classical AI, and a whole lot of free time
======

Tetris 99 is a video game for the Nintendo Switch that combines the thrill of battle royale games with the gameplay of, well, Tetris. The rules are similar to traditional Tetris, but in addition to the game speeding up over time, there is an additional challenge: when players perform high-scoring moves, the game sends lines of blocks to opponents' boards, which fill up the bottom of their play area, pushing their blocks upwards and bringing them closer to a game over. The last player remaining, of course, is the victor, and attains the coveted title of Tetris Maximus.

In the time-honored tradition of programmers, while my roommate [Ben](https://github.com/bpinzone) was playing one night, [I](https://github.com/spschul) thought "you know, it wouldn't be too hard to write a bot to play this, without even hacking the Switch. You'd just have to...well...

1. Somehow see the screen and interpret the board state
2. Write a fast Tetris simulation, write an algorithm to choose a good place to put the next block
3. Convert that placement into a sequence of button presses, and somehow communicate that to the Switch

...none of which sounds that hard, right?"

Easy.

Ninety-nine percent of the time, it would've been an idle thought that went nowhere, but this particular idle thought occurred during May 2020. Everything was shut down due to COVID, and Ben and I had both just graduated and were bored out of our minds. So we decided to give it a shot.

A few months later, we recorded this video of our Tetris-playing algorithm—dubbed "Jeff" in honor of [this video of the 2016 Tetris World Championship](https://www.youtube.com/watch?v=QV_0CcF9-RM)—getting first place (against human players—my thoughts about pitting this AI against human players are [in the "takeaways" section of the writeup](#try-to-avoid-side-projects-where-success-will-make-you-feel-a-bit-bad)). Feel free to skip around.

<video controls>
  <source src="public/first-place.mp4" type="video/mp4" />
  Your browser does not support the video tag.
</video>

So...how does it work?

Overview
------

Jeff has three components: his eyes (to see the game state from the screen), his brain (to pick the next move), and his hands (to input the move into the Switch).

Feel free to check out [the implementation](https://github.com/bpinzone/TetrisAI), but know that the README is outdated and TODO.

The Eyes
------

The goal of Jeff's eyes is take the HDMI output of the Switch—the gameplay video stream—as input and to produce a text-based representation of that game state to pass along to Jeff's brain.

Input: ![A Tetris 99 screenshot](public/example-input.png)

Output: 

```
presented
b
queue
roycyp
board
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
..........
......xxx.
.....xxxxx
in_hold
.
just_swapped
false
board_obscured
false
```

When we started, we (mostly I) were beset by a youthful folly: I wanted the algorithm to work without needing any wired connection. Specifically, I wanted the algorithm to run on a laptop that was sitting in front of a TV, reading the game state from the webcam and controlling the game via Bluetooth, because I thought that would be cool. But the webcam's image wasn't great, and we kept having to mess with the lighting in the room, and before every test I had to carefully click the corners on the "hold" area, the board area, and the queue area to specify where the algorithm should look. It was a huge hassle. As is too common in life, everything I was doing would become much simpler if I simply compromised on my principles—alas, in this case, I gave in and we bought an HDMI splitter and a capture card to read the video stream from the Switch directly into my laptop.

The direct video stream was much simpler. The 20x10 Tetris grid appears in the same place onscreen every time, so we manually specified the section of the image where it appears, split it into a 20x10 grid, and classified each square image independently. A square is full (there's a piece there) if over 50% of the pixels in it are "bright", and a pixel is "bright" if the grayscale value (0-255) was above 80 (the blue pieces can be fairly dark).

However, we also needed to see what piece we had available in the "hold" (not present in classic Tetris—the "hold" allows you to stash a piece away, and then exchange future pieces for your held piece) and the pieces upcoming in the queue, to allow us to plan our next moves. The only differences between pieces in Tetris are color and shape, and at first we decided to try to recognize pieces based on color alone.

Separating by color proved to be a headache: sure, we could determine RGB thresholds for different colors, but especially when we were still using the webcam, sometimes blues would look purple or vice versa, yellow and orange were similar...it was a good idea, but not precise enough.

We ended up opting for a different approach: we relied instead on the differing shapes of the pieces. Because pieces always appear in the same place, we decided to save an image of each possible piece in every possible space it could appear in (the hold or any of the 6 positions in the queue) and simply compare them to the actual image we were seeing in the video stream. In theory, the section of the image we're seeing live should exactly match one of our reference photos. In practice, it's not that simple (for reasons I'll discuss later), so we used "empty/full" image masks like we used in our analysis of the board (effectively making the image black or white) and then compare what we saw in each image to each possible piece, then choose the best match.

| Observed piece                  | Observed piece (B&W)    |  Proposed match         | Proposed piece (B&W)        | Matching pixels   | Best match? |
|---------------------------------|-------------------------|-------------------------|-----------------------------|-------------------|-------------|
| ![Green Piece](public/green-piece-upscaled.png) |   ![Green black and white](public/green-piece-bw.png)          | ![Purple Piece](public/purple-piece-upscaled.png) | ![Purple black and white](public/purple-piece-bw.png) | ![Green/purple matches](public/is-it-purple.png) | No |
| ![Green Piece](public/green-piece-upscaled.png) |   ![Green black and white](public/green-piece-bw.png)          | ![Orange Piece](public/orange-piece-upscaled.png) | ![Orange black and white](public/orange-piece-bw.png) | ![Green/purple matches](public/is-it-orange.png) | No |
... (all other pieces) ...
| ![Green Piece](public/green-piece-upscaled.png) |   ![Green black and white](public/green-piece-bw.png)          | ![Green Piece](public/green-piece-upscaled.png) | ![Green black and white](public/green-piece-bw.png) | ![Green/purple matches](public/is-it-green.png) | Yes |

With these techniques, we were able to read the board, the hold, and the upcoming queue. But there was one more caveat: in addition to knowing what the state of the board was, Jeff's eyes serve as our algorithm's clock. To a human playing the game, it's obvious when they're 'allowed' to place their next block, but our algorithm needed to provide the next game state only after the previous play had finished, so we act (only) when the game is ready. Fortunately, there was an easy fix: every time the game is ready for the player to make their next move, the queue shifts to present the player with the next piece, so we just needed to output the state every time the queue changed.

Unfortunately, there was one major issue that made all of this harder, that informed a lot of the design decisions we made: sparkles.

### The Sparkle Scourge

Back in the classic Tetris days, video games were made right: they used simple colors, simple shapes, and nothing else. But modern games insist on adding gameplay effects, such as sparkly bursts of light that fly everywhere every time you clear a line, and show up in droves every time you get a Tetris (a "Tetris" is when you clear four lines at once with a long bar).

Jeff gets a lot of Tetrises, and when he does, it can look like this.

![Sparkles fly across the screen in Tetris 99, obscuring the board and the queue](public/the-sparkle-scourge.png)

Yes, the sparkles can make us read the board incorrectly—but more concerningly, when the queue shifts, we assume the game is ready for us to make our next block placement. So if the queue *appears* to change, we might provide an incorrect new board and mess up the timing on our gameplay. We needed to get clever about requiring the observed queue to be stable, so we required that queue to be "locked in"—which is when 95% of the (pure black-and-white) observed image matches the (pure B&W) template image. With that logic in place, our algorithm was very consistent about only updating when the queue shifted.

Of course, the sparkles and other effects could still cause problems with reading the board. We tried several methods to mitigate it, but the most successful, by far, wasn't a computer vision technique. In fact, it might be the opposite of a computer vision technique.

<blockquote>
The student stumbled out of the Maze of Illusions and approached his teacher, defeated.

"Teacher, I have failed. After a hundred attempts, I still have not learned to tell what is real from what is fake."

"Ah, but that is not what you are here to learn," the teacher replied. "You are here to learn to close your eyes."

—Roboticist koan
</blockquote>

We know the move Jeff intends to make, and how the board will look when that piece lands, so we know what the board should look like the next time we get to move. When Jeff is about to clear some lines and cause the sparkles, or when the board is just unusually bright, we figured we were better off using what we think the board will look like next rather than a sparkly mess.

There are pitfalls to this "going blind" approach: your previous reading of the game might be wrong, the game can insert lines of blocks sent by your opponents (though it seems to do so only after placements that didn't clear any lines), and you might misplace a block. Still, we found "going blind" in sparkly scenarios to be very useful, and we ended up with a computer vision pipeline that, for our purposes, was good enough.

The Brain
-------

The goal of Jeff's brain is to take a text-based representation of the game state and determine what move should come next. Then it determines which buttons Jeff's hands need to press and passes the list of buttons on to Jeff's hands.

Our plan was simple: write a fast Tetris implementation that would allow us to provide our board state (including the upcoming pieces for our next few moves), then search all possible sequences of moves we could make, find the one that ended up with the best board, and perform the first block placement of that optimal move sequence. There were only two small problems: we didn't know how to define what board state was "best", and even if we did, there were too many possibilities to search through while still running in real time.

### What is a 'good' game state?

We needed something like a utility function: a way to determine a score for each possible board state and then pick the best one. Most people familiar with Tetris can glance at a board and know generally if the game is going well—the taller the blocks are stacked, the worse it's going—but we found it difficult to precisely quantify.

I should note: neither Ben nor I are very knowledgeable about Tetris. There are probably all sorts of terms we don't know and strategies for making a good board that we aren't familiar with; we decided to try to figure out a workable approach ourselves rather than look at other peoples' methods.

We tried many, many iterations of our utility function. Writing the utility function was basically an exercise in [Goodhart's Law](https://en.wikipedia.org/wiki/Goodhart%27s_law). Want the tallest column to stay low? Jeff tries to build a flat board where he'll never be able to clear lines. Want the pieces to neatly fit together, not leaving any hard-to-fill gaps? Jeff will refuse to make holes even when he needs to cut his losses and leave some spots unfilled to prevent the board from getting too high.

Our final utility function was ~~overengineered~~ very technical and full details are beyond the scope of this explanation; however, it had a few core priorities: have only one "trench" (a place for a long piece to be inserted), avoid "holes" (gaps in the stack of blocks), and try to stay in "Tetris mode", which is when the highest column on the board is no taller than 6 blocks (otherwise, Jeff enters "survival mode" and tries to clear rows).

![Tetris 99 screenshot with the empty gaps in the stack of blocks indicated as "holes"](public/holes-example.png)

### What game states should you not explore?

Ideally, we'd consider every possible sequence of actions, but if you want to search several block placements into the future—we liked to consider what the game state would be in 4 or 5 placements—it takes too long to consider every possibility while running in real time. Ideally, we'd notice "hopeless cases"—where the board is in such a definitively bad state that it's not worth checking whether a good state can come out of it—and prune them from the search tree early.

However, just like utility, it can be hard to definitively say what kind of state is "hopeless", and you don't want to stop looking if there was a brilliant play you could've made. To understand the delicate balance you have to maintain while determining which states are deemed "not promising", consider this: for a long time, we pruned boards that added over 2 new holes to the game state. Holes, after all, are difficult for Jeff to get rid of (our setup isn't nearly sophisticated enough to let us slide pieces laterally into specific locations).

But limiting ourselves to 2 new holes would've meant that Jeff would never have considered this move sequence:

![Jeff considers what to do with a long cyan piece](public/jeff-big-brain-pt-0.png)

![Bizarrely, Jeff places a long cyan piece horizontally, creating 3 holes underneath](public/jeff-big-brain-pt-1.png)

![Jeff places another piece...](public/jeff-big-brain-pt-2.png)

![...and another piece...](public/jeff-big-brain-pt-3.png)

![...and another piece...](public/jeff-big-brain-pt-4.png)

![and finally gets himself to a much better board state with a clever line clear.](public/jeff-big-brain-pt-5.png)

So it's nice to be able to consider sequences of moves create more holes, like the 3 holes created in this play. However, pruning states with lots of holes saves us a ton of time when searching. We ended up with a weird compromise where we discouraged adding new holes above the existing max height of the board, but did not penalize adding new holes at or below the current max height.

#### What about just making the program run more quickly?

We did that as well! We did a lot of profiling (which is not, of course, to say that it's 'done' or that there weren't any major bottlenecks we missed), leading to a lot of optimizations and refactors to speed up our Tetris simulation. We used a fairly simple depth-first search to consider the millions of possible move sequences, and redesigned it whenever we thought we could get a significant performance benefit. We made a work queue and multithreaded it, which made it a few times faster. Ben wanted to GPU-accelerate it, though we both knew it wouldn't really help—Jeff's limitations were elsewhere.

### Sending Instructions

Finally, now that we knew where to place the block, we computed the list of buttons that would need to be pressed and passed them to Jeff's hands. There's not much to say about this, other than that we had a lot of fun optimizing for "least buttons pressed"—for example, we found that if the game received a command to shift the block laterally or rotate it on the same controller input frame as it received the 'up' input that would drop the piece, it would do the command before inserting the piece.

### An Unusually Morbid Bug

While testing Jeff's brain, we found an unusual bug: Jeff would play perfectly fine for a while, and then rapidly stack pieces on top of each other to reach the top, ending the game. The bug would usually happen when the board was starting to get taller and would be less apparent when we reduced the depth of the search—that is, looked fewer moves ahead while considering our next block placement.

*(If you're the type to want to come up with the answer yourself, feel free to stop and hypothesize.)*

The issue was in our utility function. At the time, we had an explicit utility function that produced a numerical value (our later iterations were more like a comparison of various board properties). We applied rewards for some things and punished others, and, critically, it was common for the available states to all have negative utility scores, and yet the 'game over' state had a value of 0. So, the moment Jeff saw a path to game over, he took it. And that's why you should always set your 'Game Over' state's utility to `std::numeric_limits<double>::lowest()` (and NOT `std::numeric_limits<double>::min()`)!

### What about reinforcement learning?

If I squint, I can peer into the future and foresee that some people will ask: what about using machine learning—specifically, reinforcement learning—to learn the best move, rather than hand-coding a utility function? Didn't I read [The Bitter Lesson](http://www.incompleteideas.net/IncIdeas/BitterLesson.html)? Aren't I bitter-pilled?

Yeah, it would've been really cool, and it would've avoided basically all the pitfalls of the utility-function-based approach. I kind of wish we had tried—I've done some reinforcement learning, but that would've been the most advanced project I'd done with it. But that would've turned this side project into a much more complex research project—I don't know if we could've produced a good model on a hobbyist's allotment of time and money.

The Hands
------

The goal of Jeff's hands is to take a text list of button presses and convert them into USB signals to send to the Switch as our microcontroller pretends to be a USB controller.

For a while, we tried to communicate with the Switch via Bluetooth using [this wonderful repo](https://github.com/mart1nro/joycontrol), but for some reason, we just had too many issues getting Bluetooth to work easily and consistently. After a long time, we instead decided to connect via USB and control the Switch using [this other wonderful repo](https://github.com/Phroon/switch-controller). As described in that repo's README, you use a USB-to-serial converter and a microcontroller (we used a TODO) to send commands from a Python program running on your computer to the microcontroller to the Switch.

I don't have much to say about Jeff's hands, but it's not because they were easy or they worked well—far from it. It's more that I accept that the natural state of the world is that Jeff's hands are the part of the project that would inexplicably be incredibly difficult. In theory, all we need to is pretend to be a controller and provide a few commands; in practice, there was a constant tension between speed and missed inputs. We thought we'd be able to operate at ridiculously superhuman speeds; in actuality, when we tried to speed up the rate of button presses over USB, it'd start missing button presses, and the hands were frustratingly slow to get started when we needed them to move quickly.

We never really figured out why. Jeff's hands were fast enough to get him into first place, which let him have his moment of glory as the Tetris Maximus—but like a true Roman conqueror, they weren't fast enough to keep him there.

Conclusion, Part 1
------

The video of Jeff getting first place showcases Jeff at his best. He'd usually lose in the later stages of the game when the pieces started falling fast because he couldn't get the pieces to where he wanted them in time.

I'm reasonably confident that, with effort, Jeff could become nigh-unbeatable. We kept meaning to work more on the project, but as time passed it became more and more unlikely that we would, and besides, we had a lot of fun writing Jeff, but I don't want to take away well-deserved victories from human Tetris 99 players. So, at this point, Ben and I are happy to call Jeff a successful side project.

A few miscellaneous takeaways:

### Pair Programming is Unexpectedly Interesting

We pair-programmed almost the entirety of the C++ Tetris simulation and move selector, usually on Ben's computer, and it was surprising just how productive that arrangement felt. Most of our time was spent catching bugs, and it was much, much easier to catch errors early when one person didn't have to type and could instead devote their brainpower to asking themselves "is this going to work?" Also, we benefitted immensely from being able to discuss program design live as we were writing the program. I'm sure it would depend on the person—Ben was an excellent co-programmer—but regret that I haven't had many other chances to pair program since this project.

### It's Helpful To Define A "Light Speed"

One of the things that Ben brought up while doing the project, that I probably never would've thought about, was that we should define what Nvidia calls "light speed" for Jeff's skill at Tetris—a theoretical 'best' that it is impossible to exceed, so that you can tell how much it's possible for your program to improve. In our case, Ben wanted to define what Jeff would ideally do. After researching how Tetris 99 decides how many lines you'll send your opponents, we determined that Jeff should just try to get as many Tetrises (4-line clears) as possible. We reasoned that because each piece is made of 4 little blocks and a Tetris clears 40 blocks, then the maximum possible rate of Tetris per block placements is 10%.

I don't think I would've thought of the 'Tetris percent' metric on my own, but it helped us a lot when comparing utility functions, and it also helped us realize when our utility function couldn't get much better.

### Always Write a Visualization Program

I eventually added a visualization script to Jeff's eyes that would show what Jeff's eyes thought the board state was, which was an incredible decision and I should've done it much earlier. Writing visualizations is the kind of thing that I always put off because I think I'm 'almost done'.

### Faulty Optimums Still Kinda Look Optimal

Any algorithm that performs some kind of optimization, from gradient descent to A*, suffers the same difficulty while being debugged: the fact that my puny human eyes are too weak to fathom the vast depths of the possibility space to see which brilliant maneuvers went overlooked. The algorithm produces an output, and I give it a squint and an "LGTM". If there was a slight inaccuracy the utility function, how would I know?

I'm not sure if there are good ways around this. Rigorous testing, maybe, but it's hard to improve your algorithm by observation once it's not making obvious mistakes. This is, perhaps, one of the promises of using reinforcement learning without human play training: if your algorithm is able to achieve human-level performance, it's probably correct enough to continue well into superhuman performance.

### Try To Avoid Side Projects Where Success Will Make You Feel A Bit Bad

This project was always about having fun and seeing if we could get Jeff to work at all—I don't want to ruin the fun and competition of Tetris 99 for its players. Maybe it's for the best, then, that he only won a handful of times, and that we're not pushing his abilities further—I don't want to be the cause for comments like [this](https://www.reddit.com/r/Tetris99/comments/13wqfpx/bot_farming_in_tetris99/).

Conclusion, Part 2—also, hire me?
------

I'm starting to look for work in the Midwest of the USA right now (January 2025—what are the odds that I'd find myself finally making a writeup about an interesting, years-old project at exactly the same time I start looking for work? 😛). If you think I'd be a good fit for an opportunity and you'd like me to know about it, feel free to contact me at TODO.

Anyway, a final thought about Jeff: you can understand each part of a system individually and still find it stunning when all the parts move in tandem together. We'd watch Jeff slam piece after piece into place and struggle to really believe that it could actually *work*. It was a bright light in my 2020 landscape, a world in which there was everything to watch but nothing to do, and I'm grateful to Jeff for giving us a challenge which granted more texture to the slurry of days.