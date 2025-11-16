# TODOs

* moving around should only recompute the new area
* change the color palette
* compute more detailed where required, otherwise interpolate

# Perf
Always 1000 iter
- 2.4 s: default compute everything everytime
- 4.2 s: added distance estimate, yikes
- 1.1 s: optimized computations, ray march and details flagging, too big steps artifacts