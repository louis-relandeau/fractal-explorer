# TODOs

* change the color palette
* smooth iteration coloring first
* perturbation theory next
* series approximation
* zoom with box selection?
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
- structure for DE with std_optional and tuple: 55 ms
- structure for DE with std_optional and pair: 49 ms
- structure for DE with double and pair: 41 ms ok
- with not conservative DE: 32/50 ms -> loosing a bit at high zoom and not really worth it
