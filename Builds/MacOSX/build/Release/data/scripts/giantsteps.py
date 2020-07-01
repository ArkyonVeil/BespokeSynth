import notecanvas

notes = [('F#4', .5),
         ('D4', .5),
         ('B3', .5),
         ('G3', .5-.125),
         ('Bb3', 1.125),
         ('B3', .375),
         ('A3', .625)]

canvas = notecanvas.get("notecanvas")
canvas.clear()

pos = 0
for k in range(2):
   for i in range(len(notes)):
      length = notes[i][1] * .5
      for j in range(4):
         canvas.add_note(pos,bespoke.name_to_pitch(notes[i][0])-j*5-k*4,80,length)
      pos = pos + length
   




 