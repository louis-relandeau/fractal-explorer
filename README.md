# TODOs

* change the color palette
* compute more detailed where required, otherwise interpolate
* zoom with box selection?
* analytical soution for in bulbs
* multithreading
* dynamic wallpaper

# Perf
Always 1000 iter
- 2.4 s: default compute everything everytime
- 4.2 s: added distance estimate, yikes
- 1.1 s: optimized computations, ray march and details flagging, too big steps artifacts

New structure
- complex: 1.1 s
- doubles: 545 ms
- unrolling by 4: 520 ms -> not worth it for now
- using AVX for 4 pixels: 500 ms -> also not for now too constraining
- some analytical instant returns and symmetry: 39 ms -> zoom meh