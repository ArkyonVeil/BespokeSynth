Tag: songcanvas-exp4.5
Title: SongCanvas Exp V4.5

- Fixed Crash when playing on measures higher than 40 #26 #25
- Fixed Crash when grid is on 16n, 32n or 64n and elements are not placed in 4n measures
- Real time display is now loaded properly. (Plus a lot of other options menu only configs are also updated) #23
- Faded canvas note placement preview now correctly reflects the size of notes when on non 4n intervals.
- Can't replicate #16 "Via click dragging, its possible to display untrunctated real time". Marking as fixed.
- Fixed Loop Region's wrong overfill bounds. #15
- Resolved another visual bug with loop region clipping.
- Pressing reset in Transport or SC now factors lookahead for downbeat enabling #
- SC now implements proper lookahead for its modules. Handling downbeats properly.
- Overhauled Transport advancing to be far more granular. Supporting jumps 1/1 measures to 1/64 measures.
- SC Loop regions now handle resetting smoothly.
- Overhauled the time keeping from the SC modules to be more like the EventCanvas, hopefully this trims on some bugs down the line.
- Transport's reset now relies entirely on SetQueuedMeasure for proper resets. (This might break stuff, hopefully not)
- Resolved visual misalignment on stacked vertical racks. #10