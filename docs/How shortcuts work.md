# How Shortcuts Work

Credit to Pipe Wizard (@711138971756920833).

The game does not actually "compile" pipe paths from shortcut dots. Instead, each dot is a possible point that a pipe-traveling object can proceed to.

A pipe-traveling object stores its forward direction (I don't know if it applies to entrances and endpoints. I guess it does, in which case the outermost rows of dots in the above image would be redundant.). If there is a dot in that direction, it continues forwards.
Otherwise, it prefers going down over up and left over right. It never goes directly backwards just because of level geometry.
It is impossible to make a pipe-traveling object choose between directions that are not directly opposite, because one of them will inevitably be the forwards or backwards direction.

The type of entrance in-game (and consequently the symbol if any) can be determined by simulating where a pipe-traveling object will end up if it goes into that entrance:
-if it goes into a dead end (a dot with nothing in every direction except for backwards) or an endless loop, it becomes an dummy entrance.
-if it goes into an endpoint, it behaves in accordance to its endpoint.
-it it goes into another entrance, it becomes an in-level shortcut.
Therefore, it is possible to connect multiple endpoints per entrance and to differently-typed entrances.

I don't know what happens if a creature's den does not have an entrance leading into it.
I do not know what happens if a pipe-traveling object is forced into an endless loop by the intermediate room trick. Dummy entrances blink when a player goes over them, and that blink effect does get trapped in a loop.
I do NOT know what happens if a pipe-traveling object of incorrect type is forced into an incompatible endpoint by the intermediate room trick.
If the player exits out of an entrance that is not deemed a shortcut entrance or a room entrance, they are violently expulsed from it. It is the only phenomenon that does not require unorthodox level geometry, as it is possible to connect a room to one. I do not know if this applies to all entities that exit out of an entrance with an incompatible endpoint.

The direction of an entrance is not determined by where it connects to a path, but instead by the geometry surrounding it. This is the priority order for entrance directions: left down right up.