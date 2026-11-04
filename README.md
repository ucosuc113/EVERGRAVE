# evergrave

wip clicker about humanity getting really good at making energy. it starts with a few meters and a steam turbine and (eventually) ends with a dyson sphere, an artificial star and proxima centauri. prequel to AN∅MALY.

you look at earth from space (almost top down, slightly tilted). it spins, it orbits the sun, and you click stuff. clicking the earth gives you research points, you spend them in a tech tree, and techs unlock new things you can click (the sun, a laser pulse button...). not everything is clickable from the start, that's kind of the whole point.

## where it's at

super early. it's a prototype: the earth is two flat colors and the sun is a yellow ball. the tech tree has 14 techs and the balance is basically nonexistent, you can finish the whole thing in under two minutes. i know.

what works:

- click the earth (or hit space) -> points
- tech tree (T): a map with one lane per branch that opens up as you research (drag or scroll to move, hover for name and cost, click for details)
- shop (B): buildings you buy again and again (weather stations, satellites, steam plants...), each a bit pricier than the last. they unlock from the tech tree, produce points on their own, and show up around the earth (satellites orbit it, lights appear on the night side). owning 10, 25, 50... of one multiplies what it makes
- three places to play: the earth (first 43% of the game), the moon (33%) and the sun (23%). you always start on the earth; techs open the system map (a 2.5d view from above), then the moon, then the sun. the tech tree is shared, but each body has its own shop. drag to spin the camera around the body, wheel to zoom, 1/2/3 to switch body, M for the map
- the sun and the laser pulse are locked until you research them, clicking them early just tells you what you're missing
- F5 / F9 to save / load (windows only)
- pixel art look, same idea as AN∅MALY but 360p instead of 180p

## running it

you can't, sorry. it's built on S.T.R.AC.E, my own engine, and the engine isn't public, so none of this will open anywhere else. the repo is mostly here so the code and the scene are backed up and readable.

if you just want to read code: `Assets/Scripts/Core` is the game logic and it's plain c++ with no engine in it. everything else is glue for the engine.

## layout, roughly

- `Assets/Scripts/Core` - game state, resources, tech tree, saving
- `Assets/Scripts/Data` - tech + interaction definitions (generated from a spreadsheet i keep locally, don't edit by hand)
- `Assets/Scripts/View` - ui and the earth / sun scene stuff
- `Assets/Shaders`, `Assets/Materials`, `Assets/Meshes` - the earth, the sun, the post-process
- `Scenes/Main.strscene` - the only scene
