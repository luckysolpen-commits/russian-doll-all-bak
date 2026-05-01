
import re
import json
from collections import deque, defaultdict
import os

BOOK_DIR = "nGL-CYOA-BOOK1"
LINK_PATTERN = re.compile(r"((?:turn to|Turn to) page )(\d+)(\.?)") # Captures prefix, number, and optional period
FILENAME_PATTERN = re.compile(r"page_(\d+)\.txt")

# --- Content from previous read_file calls ---
# Ensured all multiline strings are properly formatted with triple quotes and escaped correctly if necessary.
page_contents = {
    "nGL-CYOA-BOOK1/outline.txt": '''# Choose Your Own Adventure: FLEXTAINK

## Introduction: The Field Trip Gone Wrong

You are a student on a once-in-a-lifetime field trip to the Grand Lumina Mall in Angland, a marvel of futuristic underwater architecture and home to the legendary FLEXTANK – a window into countless universes. But a mysterious surge of energy engulfs your tour group, and your world dissolves into a chaotic kaleidoscope of light and pressure.

## The Awakening: New Bodies, New Reality

You awaken not as yourself, but within the scaled body of an Angleri – a humanoid angler fish native to the abyssal depths of Aquilon Prime. Your surroundings are alien yet strangely familiar: you're inside a vast, luminescent chamber, a universe contained within a shimmering, pressurized field. This is the FLEXTANK, and you and your classmates are now its newest, unwilling inhabitants.

## First Choice: Navigating the Abyss

Panic threatens to overwhelm you. The familiar world is gone, replaced by glowing alien forms and a crushing sense of displacement. You and your fellow students must decide how to face this terrifying new reality.

*   If you decide to **hide and observe**, trying to understand your new form and surroundings before acting, turn to page 2.
*   If you decide to **seek immediate answers and assistance**, looking for any authority figures or signs of help, turn to page 3.
*   If you attempt to **explore the immediate vicinity** to gauge the escape routes or potential dangers, turn to page 4.

## Branching Paths

### Path of the Lure (Angler Force / Order)

Driven by a desire for structure and safety, you and your allies gravitate towards the established order of Angland. You might join the Angler Force or its enforcement arm, the XVS, learning to wield phosphorus spright swords and navigate the complex politics of the Angler Empire.

*   **Focus:** Law, order, combat, political maneuvering, upholding the "Lure the Light from the Dark" motto.

### Path of the Void (Unlit Tribe / Rebellion)

Embracing the unknown and rejecting the established powers, you seek a way out of the FLEXTANK. This path may lead you to discover the secrets of the "Unlit Void" and ally with rebel factions like the Unlit Tribe, challenging the Angler Empire's dominion.

*   **Focus:** Evasion, resistance, uncovering hidden truths, understanding the "Unlit Void," fighting for freedom.

### Path of the Current (Smuggling / Bounty Hunting)

Survival and self-reliance become paramount. You might find yourself in the gritty Abyssal Forge District, taking on lucrative but dangerous bounties, smuggling illicit goods across the FLEXTANK's contained universes, or simply trying to make a living in the lawless fringes.

*   **Focus:** Resourcefulness, combat, risk-taking, profit, navigating criminal underworlds.

## Potential Mid-Game Scenarios

*   **Saving a Planet:** A contained universe within the FLEXTANK is in crisis. Do you intervene?
*   **Political Intrigue:** Caught between factions within the Angler Empire or between the Empire and rebel groups.
*   **Mythical Artifacts:** Discovering powerful items like the "Shell" or "Pearls of Wisdom."
*   **Inter-Species Conflict:** Navigating tensions between races like the Niveans and Noahites within or across FLEXTANK universes.

## Possible Endings

*   **Escape:** Finding a true way out of the FLEXTANK and returning to your original reality.
*   **Ascension:** Becoming a respected commander in the Angler Force or a legendary figure in Angland.
*   **Dominion:** Seizing control of a universe within the FLEXTANK.
*   **The Void:** Becoming one with the "Unlit Void" or lost to the "Darkness."
*   **Bounty Hunter Legend:** Becoming the most feared or renowned bounty hunter in the FLEXTANK.
*   **Tragic End:** Succumbing to the dangers, corruption, or internal conflicts of the FLEXTANK.

---
## Initial Pages

### Page 1: The Field Trip Disaster

You remember the shimmering exterior of the Grand Lumina Mall, the excited chatter of your classmates, and the strange, pulsating hum of the FLEXTANK exhibit. It was supposed to be an educational excursion to Angland, a city thriving in the deepest oceans of Aquilon Prime. Then, a blinding flash of emerald light erupted from the FLEXTANK's core. The floor beneath you dissolved, and a disorienting sensation of being pulled apart and reformed washed over you.

Now, you blink in the dim, bioluminescent light. Your limbs feel alien – scaled, webbed, and ending in sharp, claw-like digits. A strange, fleshy lure dangles from your forehead, pulsing faintly. You look at your classmates, who share your horrifying transformation. Around you, immense, translucent walls curve, revealing not the ocean depths, but swirling nebulae and alien cityscapes. You are inside the FLEXTANK.

Suddenly, heavy, armored figures with glowing lures approach, their massive jaws opening. "Containment breach detected," a metallic voice echoes. "Unauthorized biological entities within Sector 7-G. Bubble Wardens, engage!"

What do you do?

*   If you **command your classmates to scatter and hide** among the bizarre, coral-like structures of the FLEXTANK's interior, turn to page 2.
*   If you **attempt to reason with the approaching figures**, hoping they are benevolent guardians, turn to page 3.
*   If you focus on **finding the nearest "escape" route** or weakness in the FLEXTANK's shimmering barrier, turn to page 4.
''',
    "nGL-CYOA-BOOK1/page_01.txt": '''You are a student on a once-in-a-lifetime field trip to the Grand Lumina Mall in Angland, a marvel of futuristic underwater architecture and home to the legendary FLEXTANK – a window into countless universes. But a mysterious surge of energy engulfs your tour group, and your world dissolves into a chaotic kaleidoscope of light and pressure.

You awaken not as yourself, but within the scaled body of an Angleri – a humanoid angler fish native to the abyssal depths of Aquilon Prime. Your surroundings are alien yet strangely familiar: you're inside a vast, luminescent chamber, a universe contained within a shimmering, pressurized field. This is the FLEXTANK, and you and your classmates are now its newest, unwilling inhabitants.

Panic threatens to overwhelm you. The familiar world is gone, replaced by glowing alien forms and a crushing sense of displacement. You and your fellow students must decide how to face this terrifying new reality.

Suddenly, heavy, armored figures with glowing lures approach, their massive jaws opening. "Containment breach detected," a metallic voice echoes. "Unauthorized biological entities within Sector 7-G. Bubble Wardens, engage!"

What do you do?

*   If you **command your classmates to scatter and hide** among the bizarre, coral-like structures of the FLEXTANK's interior, turn to page 2.
*   If you **attempt to reason with the approaching figures**, hoping they are benevolent guardians, turn to page 3.
*   If you focus on **finding the nearest "escape" route** or weakness in the FLEXTANK's shimmering barrier, turn to page 4.
''',
    "nGL-CYOA-BOOK1/page_010.txt": '''You gather a handful of small, phosphorescent pebbles from the chamber floor. Drawing on some instinct you didn't know you possessed, you concentrate, focusing your strange new Angleri energy. The pebbles begin to glow brighter, emitting a high-pitched whine. You hurl them towards the Bubble Wardens' path.

The pebbles detonate in a flash of blinding light and a piercing sonic shriek, disorienting the Wardens and momentarily scrambling their helmet sensors. Their lures flicker wildly, and their synchronized movements falter. "Interference! Shields are down!" one yells. This is your chance.

Do you:

*   **Charge the disoriented Wardens** while they're vulnerable, hoping to incapacitate them? Turn to page 16.
*   **Use the chaos to escape** and find a better hiding spot? Turn to page 5.
''',
    "nGL-CYOA-BOOK1/page_011.txt": '''You scan the alien environment. The strange coral structures are not just decorative; they seem to pulse with a faint internal light and have hollows and tunnels within them. You notice a particularly large, branching coral formation that leads into deeper darkness. It looks like it could conceal you and your classmates effectively, at least for a while.

You signal to your fellow students, gesturing towards the coral. They cautiously follow you into its depths, the bioluminescent light of the main chamber fading as you go. The air, or fluid, grows colder, and a faint, rhythmic pulsing emanates from the walls. You’ve found a temporary sanctuary, but you can still hear the faint hum of the Bubble Wardens' scanners somewhere in the distance.

What is your next move?

*   Attempt to **explore deeper into the coral structure**, looking for a way out or a more secure location? Turn to page 17.
*   Try to **use the coral's natural features to set a trap** or barricade, should the Wardens find you? Turn to page 18.
''',
    "nGL-CYOA-BOOK1/page_012.txt": '''"This is a joint initiative," you continue, trying to project confidence. "The Angler Empire and the Galactic Federation's Xenological Study Program. We are here to establish cultural exchange and understanding through direct experiential immersion. Your role is to facilitate our transition and integration." You flick your lure, mimicking a gesture you've seen on inter-dimensional broadcasts. "We are pioneers of cosmic understanding!"

The lead Warden's lure flashes a series of complex patterns, presumably communicating with its superiors. It then turns back to you. "Your bioluminescent signature is still anomalous, but... my superior is relaying a priority directive. Acknowledging potential 'Xeno-Sentience Integration Study' parameters. You will be escorted to the nearest orientation facility. Do not deviate. Compliance is mandatory."

Relief washes over you. It worked. For now. You and your classmates are to be escorted, not apprehended. This could be your first step towards understanding this new world, or a gilded cage.

*   Follow the Bubble Wardens to the orientation facility. Turn to page 19.
''',
    "nGL-CYOA-BOOK1/page_013.txt": '''You plunge into the shimmering, unstable membrane. For a heart-stopping moment, you feel an intense pressure, a disorienting shift, and then… nothing. You tumble onto a surface that feels like soft, yielding moss, but glows with an inner light. The air is thick with the scent of exotic blossoms.

Looking around, you see your companions have also made it through, gasping for breath. The chaotic energy of the barrier is gone, replaced by a serene, alien landscape. Towering, crystalline trees reach towards a sky swirling with pastel colors. Strange, winged creatures flit between them. You are no longer in the sterile containment chamber. You have escaped the immediate threat of the Bubble Wardens, but where in the cosmos have you landed?

*   Cautiously explore your new surroundings. Turn to page 20.
''',
    "nGL-CYOA-BOOK1/page_014.txt": '''You decide that a direct leap might be too risky. Instead, you scan the immediate area for anything that might help. You spot several of the larger, phosphorescent flora nearby, their stalks thick and sturdy. You also notice the strange, coral-like structures seem to channel faint energy currents.

You signal to your classmates. "We need to focus our energy!" you urge. You then point to the flora. "Try to channel your new Angleri energy into these plants. Maybe we can overload that section of the barrier, create a wider opening, or at least distract the Wardens!"

It's a long shot, but it's a plan. You and your classmates gather around the nearest thick-stalked plants, focusing your will and the strange, new energies coursing through your bodies. The plants begin to glow brighter, their inner light intensifying.

What happens next?

*   The plants surge with energy, creating a powerful wave that might breach the barrier or stun the Wardens. Turn to page 14.
*   The energy backfires, causing a localized explosion that alerts the Wardens to your position. Turn to page 21.
''',
    "nGL-CYOA-BOOK1/page_015.txt": '''You sprint around the fallen crystal, using the distraction to your advantage. The Bubble Wardens are momentarily confused, their sensors likely overloaded by the impact and debris. You see an opportunity to flank them.

As you round a large coral outcrop, you notice a small maintenance conduit barely visible behind the Wardens. It seems to lead into the deeper structure of the FLEXTANK. It might be a trap, or it might be a way to escape their immediate line of sight.

Do you:

*   Attempt to **slip into the maintenance conduit**, hoping it leads somewhere safe or offers a new path? Turn to page 22.
*   Try to **use the Wardens' confusion to make a direct run** for the perimeter of the chamber, looking for any signs of egress? Turn to page 4.
''',
    "nGL-CYOA-BOOK1/page_016.txt": '''With the Wardens disoriented, you seize the moment. You and your classmates surge forward, a wave of desperate, transformed students. You feel an unfamiliar strength in your new Angleri limbs, a primal instinct guiding your movements. You knock aside stunned Wardens, their armor clanking uselessly against your unexpected ferocity.

One Warden recovers and raises its weapon, but before it can fire, another student lunges, tackling it and sending the energy beam wildly off course. The chaotic energy washes over the chamber, striking a nearby console. Alarms blare, and the lights of the FLEXTANK flicker erratically.

You and your group have escaped the immediate danger, but you've also caused significant disruption. You are now fugitives, and the entire FLEXTANK system is on high alert.

*   Use the ongoing chaos to find a hidden path or escape route. Turn to page 23.
''',
    "nGL-CYOA-BOOK1/page_017.txt": '''You signal to your classmates, and together you push deeper into the branching coral structure. The passages narrow, forcing you to move in single file. The rhythmic pulsing grows stronger, and you begin to hear a faint, melodic hum. It feels… organic.

You emerge into a vast cavern within the coral. It's filled with a soft, pulsating light, and at its center is a pool of shimmering, viscous fluid. Strange, glowing tendrils descend from the ceiling, bathing the pool in luminescence. This seems to be a core chamber of the coral itself, perhaps even a biological hub for the FLEXTANK.

Do you:

*   Investigate the shimmering pool? Turn to page 24.
*   Look for exits or other passages leading away from this chamber? Turn to page 25.
''',
    "nGL-CYOA-BOOK1/page_018.txt": '''You decide to fortify your position. Using your new Angleri strength and the strange, pliable nature of the coral, you and your classmates begin to twist and weave the softer parts of the structure, blocking off the entrance you used. You also look for anything that might serve as a makeshift weapon or defense. You find several sharp, crystalline shards embedded in the coral walls.

As you work, you hear the distinct *clank* of Warden armor echoing from outside. They've found your general location. The sounds of them attempting to probe the coral, to find an entrance, grow louder.

Do you:

*   **Prepare to ambush the Wardens** when they inevitably break through your defenses? Turn to page 26.
*   **Attempt to find an alternative escape route** within the coral structure while they are still outside? Turn to page 17.
''',
    "nGL-CYOA-BOOK1/page_019.txt": '''You attempt to convince the lead Angler commander that you are part of a sanctioned research program. The commander's lure pulses yellow as it accesses the Nexus Database. After a tense moment, it relays a priority directive: "Acknowledging potential 'Xeno-Sentience Integration Study' parameters. You will be escorted to the nearest orientation facility. Do not deviate. Compliance is mandatory."

You and your classmates are escorted to a large, domed building. Inside, other beings in unfamiliar forms are also being processed. A stern-looking Angler addresses you: "Welcome, anomalies. Your previous forms are irrelevant. Your new purpose begins now. You will be trained and assigned roles that benefit the Angler Empire."

*   Accept the Angler's terms and inquire about training. Turn to page 27.
*   Protest, demanding to know how to return home. Turn to page 28.
''',
    "nGL-CYOA-BOOK1/page_02.txt": '''You hiss, the sound a strange clicking in your new throat. "Scatter!" you manage to croak, your voice a distorted echo of its former self. Your classmates, equally terrified and disoriented in their new Angleri forms, heed your command. Some dart into the flickering shadows cast by the strange, coral-like growths that litter the chamber. Others scramble behind pulsating, bioluminescent flora.

The Bubble Wardens, imposing figures in their pressurized suits, advance with unnerving speed, their glowing lures pulsing with authoritative light. They deploy shimmering energy nets, attempting to ensnare the fleeing students. The air, or rather the dense quantum fluid of the FLEXTANK, crackles with energy. You press yourself against a cool, smooth surface that feels strangely like polished coral, trying to remain unseen. The immediate danger is the Wardens, but the larger question looms: where are you, and how do you get out?

Do you:

*   Try to **slip away and find a more permanent hiding spot** deeper within this alien environment? Turn to page 5.
*   Attempt to **observe the Bubble Wardens** to understand their tactics and weaknesses? Turn to page 6.
''',
    "nGL-CYOA-BOOK1/page_020.txt": '''You find yourselves in a serene, alien forest. Crystalline trees shimmer, their leaves like delicate glass chimes, and the air hums with a gentle energy. The ground is covered in a soft, glowing moss. Small, winged creatures with iridescent scales flit past, their chirps sounding like tiny bells. The sky above is a breathtaking canvas of swirling colors.

As you cautiously move forward, you notice the flora seems to react to your presence, the moss glowing brighter as you step on it, the crystalline leaves tinkling in response to your movements. You also spot a clear, winding path leading deeper into the forest.

Do you:

*   Follow the **winding path**, hoping it leads to civilization or answers? Turn to page 29.
*   Attempt to **examine the unique flora and fauna** more closely, looking for clues about this world or potential resources? Turn to page 30.
''',
    "nGL-CYOA-BOOK1/page_021.txt": '''Your attempt to channel energy into the plants causes an uncontrolled surge. The flora ignites with a blinding flash, accompanied by a deafening roar. The energy wave washes outwards, but instead of breaching the barrier, it seems to be absorbed by it, causing the FLEXTANK's field to flare violently.

The Bubble Wardens, momentarily stunned by the initial blast, are now fully alerted and enraged. "Containment breach escalated! Lethal force authorized!" their synthesized voices boom. They advance, weapons charged with deadly intent. The very structure of the FLEXTANK seems to groan under the strain. You've made things much worse.

*   Your only option now is to fight or flee. Turn to page 16.
''',
    "nGL-CYOA-BOOK1/page_022.txt": '''You quickly slip into the narrow maintenance conduit. It's dark, cramped, and smells faintly of ozone and something metallic. The conduit is slick with condensation, and you have to use your new Angleri claws to gain traction as you move. You can hear the Bubble Wardens searching nearby, their voices muffled by the conduit walls.

The conduit twists and turns, descending into the unknown depths of the FLEXTANK's infrastructure. You’re not sure if you’re heading towards safety or a more dangerous part of the system. After a few minutes of crawling, you come to a junction. One path leads further down, while the other seems to curve upwards.

Do you:

*   Take the path **leading further down**? Turn to page 31.
*   Take the path **curving upwards**? Turn to page 32.
''',
    "nGL-CYOA-BOOK1/page_023.txt": '''You push through the access panel and find yourselves in a maze of humming conduits, pulsing energy nodes, and whirring machinery. This is the engine room of the FLEXTANK, the hidden infrastructure that maintains the illusion of contained universes. It's grimy, utilitarian, and dangerous, a stark contrast to the exhibit hall.

You see several large conduits labeled with cryptic symbols. One seems to lead to what looks like a high-speed transport system. Another goes deeper into the technological heart of the FLEXTANK. There's also a smaller hatch that seems to lead to a maintenance crawlspace.

Do you:

*   Head for the **high-speed transport conduit**? Turn to page 46.
*   Venture into the **deeper technological heart** of the FLEXTANK? Turn to page 47.
*   Try the **maintenance crawlspace hatch**? Turn to page 48.
''',
    "nGL-CYOA-BOOK1/page_024.txt": '''You decide to investigate the shimmering pool. As your hand touches the fluid, visions flood your mind: galaxies forming and collapsing, civilizations rising and falling, the vast interconnected web of the FLEXTANK. You understand it's a cosmic terrarium, and your old world is just a droplet. You also sense the pool's ancient solitude and gain fragmented knowledge of the Angler Empire's motives and the "Unlit Void."

*   You now possess a deeper understanding of the FLEXTANK. Turn to page 49.
''',
    "nGL-CYOA-BOOK1/page_025.txt": '''You decide to look for other passages leading out of the coral chamber. Scanning the walls, you find a narrow opening hidden behind glowing moss. It leads deeper into the coral structure.

*   Continue through the narrow passage. Turn to page 36.
''',
    "nGL-CYOA-BOOK1/page_026.txt": '''You press the attack, determined to incapacitate the Bubble Warden. Your group surges forward, overwhelming the creature with a flurry of blows from your improvised coral shards. The Warden stumbles, its armor groaning under the impact, and collapses.

However, the sounds of more Wardens approaching are now deafening. You've bought yourselves precious seconds, but the tide is turning. You need to escape this coral stronghold.

*   Try to find a new escape route within the coral structure. Turn to page 17.
*   Make a desperate charge out of the coral and into the main chamber, hoping to lose them in the chaos. Turn to page 23.
on** and fight your way out? Turn to page 50.
*   Use the momentary advantage to **find an alternative escape route** while the Wardens are regrouping? Turn to page 17.
''',
    "nGL-CYOA-BOOK1/page_027.txt": '''Your training begins. Days turn into weeks. You learn to navigate the complex currents of the FLEXTANK, to identify basic energy signatures, and to operate your Angleri bodies with growing proficiency. You practice with phosphor-blade simulators, your lures flaring as you learn to control their intensity for communication and combat.

Your first assignment is a reconnaissance mission into a newly stabilized pocket universe – a serene, forest-like world brimming with exotic flora. Your objective: observe and report on any potential threats or resources, and discreetly gauge the local inhabitants' disposition towards the Angler Empire. You are given a standard-issue spright-saber.

*   Embark on your first mission. Turn to page 57.
e path ahead is uncertain, but it is a path, and for now, that feels like a victory.

*   Begin your training and await your assignment. Turn to page 39.
''',
    "nGL-CYOA-BOOK1/page_028.txt": '''You decide to play along, feigning acceptance of your new fate. "Of course," you say, your lure flickering contritely. "We understand. We wish to serve the Empire well. Where do we begin?" You hope that by appearing compliant, you can gather information and find an opportunity to escape later.

The Angler commander's lure pulses a steady, satisfied blue. "Excellent. Your assimilation process will begin immediately. You will be assigned to initial training and evaluation. Your aptitude will determine your path within the Empire. Report to Orientation Chamber B for your preliminary assessments."

You are escorted to another chamber, where a series of tests await you, designed to gauge your physical abilities, intelligence, and psychological profile. This is your new reality.

*   Begin your preliminary assessments. Turn to page 39.

ative"? Turn to page 40.
''',
    "nGL-CYOA-BOOK1/page_029.txt": '''You follow the winding path through the serene alien forest. The flora reacts to your presence, glowing brighter as you move. You eventually reach a clearing with a majestic, crystalline tree and a group of robed figures tending to shimmering pearls. They introduce themselves as guardians of the "Grove of Whispers" and offer "Pearls of Wisdom."

The central figure offers you a pearl that glows silver, stating it holds knowledge of escape routes, but requires understanding the "Unlit Void."

*   Ask about the "Unlit Void" and how to obtain the Pearl of Escape. Turn to page 52.
*   Express skepticism and ask about more direct escape methods. Turn to page 53.
''',
    "nGL-CYOA-BOOK1/page_03.txt": '''"Wait!" you call out, your voice a guttural rasp that surprises even yourself. "We're not a threat! We're students! There's been a mistake!" You step forward, hands (or rather, webbed fins) held up in a gesture of peace. The other students, frozen by your actions, watch with bated breath.

The Bubble Wardens pause, their multi-faceted eyes focusing on you. The lead Warden, its lure pulsing a steady, deep blue, tilts its head. "Students? From where?" its voice is synthesized, amplified by its helmet. "This is Sector 7-G of the FLEXTANK. Unauthorized biologicals are classified as anomalies. Anomalies are to be contained or neutralized."

Another Warden, its lure flashing an impatient red, raises a weapon that looks like a solidified beam of light. "The Prime Directive is clear. No deviations."

You realize that appealing to reason might be futile. These beings operate on strict protocols.

Do you:

*   **Try to bluff your way through**, claiming you are part of a sanctioned inter-universal research program? Turn to page 7.
*   **Make a sudden dash for cover**, realizing negotiation is failing? Turn to page 2.
''',
    "nGL-CYOA-BOOK1/page_030.txt": '''You follow the winged creature. It flies in short bursts, chirping encouragingly, and waits for you to catch up. The path it leads you on is winding and passes through areas of the alien forest that are denser and more mysterious. The ambient glow of the flora seems to intensify as you go deeper.

Eventually, the creature leads you to a clearing where a majestic, crystalline tree stands. At its base, several figures cloaked in shadow are gathered around a pool of light, tending to shimmering, pearl-like fruits. The creature lands on your shoulder and chirps, then flies towards the figures.

*   Approach the figures at the base of the tree. Turn to page 41.
has inhabitants who might offer help.

Do you:

*   Try to **communicate with the creature**, hoping it can guide you? Turn to page 43.
*   Use the fleeting vision as a clue, looking for signs of civilization or a similar city? Turn to page 29.
''',
    "nGL-CYOA-BOOK1/page_031.txt": '''You decide to head towards the Abyssal Forge District. The journey through the FLEXTANK's service conduits is dangerous and disorienting, but you eventually find your way to the industrial heart of Angland. The air is thick with the smell of metal and heat, and the sounds of industry echo all around.

You are immediately confronted by a hulking, armored Angler. "Oi! You lot! New recruits for the Forge, or lost fry? State your business before you get melted!"

*   Claim you are new recruits. Turn to page 44.
*   Explain you are lost students seeking escape. Turn to page 45.
''',
    "nGL-CYOA-BOOK1/page_032.txt": '''You emerge from the conduit into a serene, alien forest. Towering crystalline trees shimmer, and the ground is carpeted with soft, glowing moss. The air is thick with the scent of exotic blossoms and carries the gentle tinkling of leaves. Small, winged creatures flit through the air, their chirps like tiny bells.

You are clearly not in the industrial heart of Angland. This place feels peaceful, perhaps even sacred. You see a winding path leading deeper into the forest.

*   Follow the winding path into the forest. Turn to page 29.
 This is it. You're Angleri now, or whatever you are. Best learn to live with it."

It gestures dismissively with a clawed hand. "Look, if you ain't recruits, you're just a drain on resources. Try the outer sectors. Maybe some smugglers or traders passing through know somethin'. Or try the Grand Lumina Mall. That's where the fancy folks with the answers hang out. Now get lost."

*   Head towards the outer sectors, looking for smugglers or traders. Turn to page 55.
*   Attempt to find your way back to the Grand Lumina Mall. Turn to page 56.
''',
    "nGL-CYOA-BOOK1/page_033.txt": '''You proceed into the service area, a maze of humming conduits, pulsing energy nodes, and whirring machinery. The air is thick with the smell of ozone and lubricant. It's a stark contrast to the pristine exhibit hall, but it feels more real, more functional.

You see several large conduits labeled with cryptic symbols. One seems to lead to what looks like a high-speed transit system. Another goes deeper into the technological heart of the FLEXTANK. There's also a smaller hatch that seems to lead to a maintenance crawlspace.

*   Enter the high-speed transport conduit. Turn to page 46.
*   Venture down the conduit marked with the fractured planet. Turn to page 47.
*   Investigate the maintenance crawlspace hatch. Turn to page 48.
''',
    "nGL-CYOA-BOOK1/page_036.txt": '''You continue through the narrow passage, your Angleri bodies agile as you squeeze through the tight confines. The passage opens up into a larger chamber, its walls pulsating with a soft, rhythmic light. In the center, a pool of shimmering fluid is illuminated by tendrils descending from the ceiling. This is the heart of the coral structure.

*   Investigate the shimmering pool. Turn to page 24.
*   Look for other passages leading away from this chamber. Turn to page 25.
''',
    "nGL-CYOA-BOOK1/page_04.txt": '''Your mind races. Escape. That's the only priority. You scan the immediate area. The shimmering barrier of the FLEXTANK hums with power, but near the cluster of bizarre coral growths, you notice a section that seems to flicker more intensely, as if it's thinner or unstable. It could be a point of entry, or perhaps an exit.

You gesture frantically to your closest classmates, trying to convey your intent without attracting the attention of the approaching Bubble Wardens. They seem to be focusing on the students who scattered. This might be your only chance.

Do you:

*   **Make a break for the flickering section of the barrier**, hoping it's an exit, with a few brave classmates? Turn to page 8.
*   **Try to create a diversion** to draw the Wardens away from the potential exit, giving others more time to escape? Turn to page 9.
''',
    "nGL-CYOA-BOOK1/page_043.txt": '''You follow the winged creature. It leads you through the alien forest to a clearing with a majestic, crystalline tree. At its base, robed figures tend to shimmering pearls. The creature chirps and nudges you forward.

*   Approach the figures and ask for help. Turn to page 41.
''',
    "nGL-CYOA-BOOK1/page_044.txt": '''You claim to be new recruits for the Abyssal Forge. The armored Angler, after a skeptical glance, accepts your story and leads you into the industrial district. You are assigned to ore sorting in Vent Sector 3. The work is hot, dirty, and dangerous, but you are now inside the industrial heart of Angland, hidden among the workers.

*   Begin your work in the Abyssal Forge, looking for opportunities. Turn to page 54.
''',
    "nGL-CYOA-BOOK1/page_045.txt": '''You explain you are lost students seeking to escape the FLEXTANK. The armored Angler dismisses you, suggesting you try the outer sectors for smugglers or return to the Grand Lumina Mall.

*   Head towards the outer sectors, seeking smugglers. Turn to page 55.
*   Attempt to return to the Grand Lumina Mall. Turn to page 56.
''',
    "nGL-CYOA-BOOK1/page_046.txt": '''You decide to enter the high-speed transport conduit. The tunnel seals, and with a surge, you're propelled forward at incredible speed through the FLEXTANK's systems. You emerge in a bustling transit hub, filled with diverse beings and holographic advertisements.

*   Explore the transit hub for clues or allies. Turn to page 60.
er a short, intense ride, the transport slows, and you are deposited in a bustling hub. Holographic advertisements flicker, and various beings – Angleri, and others you don't recognize – move about. It seems to be a nexus point for travel within the FLEXTANK. You see signs pointing towards different sectors and potential universes.

*   Explore this new hub, looking for clues or allies. Turn to page 60.
''',
    "nGL-CYOA-BOOK1/page_047.txt": '''You decide to venture down the conduit marked with the fractured planet, leading towards the FLEXTANK's core. The passage resonates with immense power, and a holographic warning appears: "DANGER: CORE REALITY INTERFACE. UNSTABLE DIMENSIONAL FLUX."

*   Brave the flux and enter the core interface. Turn to page 700.
*   Turn back and choose another path from the service area. Turn to page 33.
RE REALITY INTERFACE. UNSTABLE DIMENSIONAL FLUX. PROCEED AT OWN RISK. IMPERIAL SANCTIONS APPLY." It's clearly a highly restricted area, but it also implies a direct connection to the FLEXTANK's fundamental workings.

*   Brave the unstable dimensional flux and enter the core interface. Turn to page 700.
*   Turn back, deciding the core is too dangerous, and try another path from the service area. Turn to page 33.
''',
    "nGL-CYOA-BOOK1/page_048.txt": '''You squeeze into the maintenance crawlspace. It's a disorienting maze of pipes and wires. After crawling for some time, you find a grimy access panel. Pushing it open reveals a bustling transit hub.

*   Explore the transit hub. Turn to page 60.
your classmates squeeze in, pulling the hatch shut behind you. You are now in the bowels of the FLEXTANK's infrastructure. The sounds of the exhibit hall are muffled, replaced by the hum of unseen machinery. You can navigate through this space, potentially bypassing patrols, but it's also disorienting and offers no clear path forward.

*   Attempt to navigate through the crawlspace, looking for an exit or a way to understand the FLEXTANK's systems. Turn to page 701.
''',
    "nGL-CYOA-BOOK1/page_049.txt": '''You feel a growing sense of understanding and purpose. The knowledge gained from the pool has opened your eyes to the true nature of the FLEXTANK and the Angler Empire. You decide to seek out the "Unlit Void," hoping to find allies and the truth that lies beyond the Empire's manufactured light.

*   Set course for the Unlit Void, guided by your new understanding. Turn to page 72.
ire's control.

This knowledge comes with a heavy burden. You can no longer be just a simple student, seeking only to return home. You must now decide how to use this awareness.

*   Seek out the "Unlit Void" to understand its potential and perhaps find allies. Turn to page 72.
*   Look for ways to exploit the Angler Empire's control systems, using your knowledge to find an escape. Turn to page 102.
*   Share this knowledge with your classmates and decide on a collective course of action. Turn to page 702.
''',
    "nGL-CYOA-BOOK1/page_05.txt": '''You decide the best approach is stealth. Staying low, you weave through the strange, phosphorescent flora and the jagged, coral-like formations that dot the FLEXTANK's interior. The dense quantum fluid makes movement sluggish, but your new Angleri bodies seem adapted to it, allowing for a surprisingly fluid glide. You can hear the sounds of pursuit behind you – the clicking of Warden armor, the hiss of energy containment fields.

You need a place to truly disappear. The shadows here are deep, the bioluminescent lights casting shifting patterns that could conceal you, but also reveal you. You spot a darker, cavernous opening in a large coral structure ahead. It looks like it might lead deeper into whatever this place is.

Do you:

*   **Venture into the dark, cavernous opening**, hoping for a secure hiding spot? Turn to page 5.
*   **Continue to hug the periphery**, looking for a less enclosed space to observe the Wardens from a safer distance? Turn to page 6.
''',
    "nGL-CYOA-BOOK1/page_050.txt": '''You grip the spright-saber, its energy blade humming with power. The incoming Bubble Wardens advance, their weapons glowing. You meet their charge head-on, the alien weapon feeling surprisingly natural in your webbed hand. The fight is intense, a blur of light and metal.

You manage to deflect several energy blasts and land a solid hit on one Warden, disabling its weapon. But more are converging, their coordinated tactics overwhelming your desperate defense. You are fighting valiantly, but you are outnumbered.

*   Push through the Wardens and attempt to escape into the coral. Turn to page 17.
*   Fight to the last, hoping to disable as many Wardens as possible. Turn to page 68.
''',
    "nGL-CYOA-BOOK1/page_055.txt": '''You decide to head towards the outer sectors, seeking smugglers or traders. After a disorienting journey through the FLEXTANK's service conduits, you arrive at a bustling transit hub. You spot a gruff-looking Angler leaning against a cargo container, cleaning an energy pistol.

*   Approach the Angler and ask for passage off the beaten path. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_056.txt": '''You decide to head towards the Grand Lumina Mall, hoping for allies or information. The journey is disorienting, but you eventually reach the vicinity. However, the area is heavily patrolled by Bubble Wardens, making a discreet entry difficult.

*   Attempt a discreet entry into the mall. Turn to page 706.
*   Decide the mall is too risky and seek another path. Turn to page 55.
 enter the mall. Turn to page 706.
*   Decide the mall is too dangerous and seek another path. Turn to page 55.
''',
    "nGL-CYOA-BOOK1/page_057.txt": '''Your reconnaissance mission takes you to a planet bathed in the soft light of twin moons. Lush, alien forests cover the surface, teeming with strange, bioluminescent life. You’ve learned to move with a natural grace in your Angleri form, your lure providing a subtle light as you navigate the dense foliage.

Your data slate buzzes – a proximity alert. A group of indigenous creatures, resembling bipedal lizards with crystalline scales, approaches. They seem curious, not hostile. Your objective is to observe, but a primal urge, perhaps tied to your new Angleri instincts, whispers of something more.

Do you:

*   Attempt to **observe the creatures from a distance**, maintaining stealth? Turn to page 62.
*   Make contact, **offering a gesture of peace** and attempting to communicate? Turn to page 63.
''',
    "nGL-CYOA-BOOK1/page_058.txt": '''You ask the robed figure about the "Pearl of the Rift" and directions to "The Rift." The figure nods gravely. "The Rift is a wound in the fabric of existence," it explains, its voice tinged with sorrow. "A place where the Angler Empire's light does not reach, and where the Unlit Tribe gathers. It is a dangerous path, a place of shadows and harsh truths, but also of freedom from the Empire's control."

The figure plucks a pearl that glows with a deep, swirling indigo light from the tree. "This pearl holds the resonance of the Rift. Hold it close, and let its echoes guide you. Follow the currents of the 'Unlit Void,' where the stars grow dim and wild. Seek the whispers of the Veiled Ones; they will show you the way."

The figure hands you the indigo pearl. It feels cool to the touch, and a faint, unsettling hum emanates from it.

*   Take the Pearl of the Rift and set off towards the Unlit Void. Turn to page 64.
''',
    "nGL-CYOA-BOOK1/page_059.txt": '''"Illusions? Control?" the robed figure echoes, a hint of sadness in its voice. "The Angler Empire projects an image of order and benevolence, the 'Lure of Light.' But this light can blind. They seek to contain all within their vision, suppressing true understanding and individual will. They fear what they cannot control, the 'Unlit Void' you speak of. It is not emptiness, but potential. It is the truth of existence beyond their manufactured reality."

The figure gestures to another pearl, this one shimmering with a soft, opalescent glow. "This pearl offers insight into the Angler Empire's methods of control, their propaganda, and their hidden weaknesses. Understanding these illusions can be a path to freedom, though it may not lead you directly home."

*   Ask for the Pearl of Illusions and learn about the Empire's control methods. Turn to page 65.
*   Reconsider and ask for the Pearl of the Rift, focusing on direct escape. Turn to page 58.
''',
    "nGL-CYOA-BOOK1/page_06.txt": '''You observe the Bubble Wardens intently. They move with synchronized precision, their lures pulsing in coordinated patterns. One Warden uses its lure to project a holographic map of the immediate area, highlighting points of interest and energy signatures. Another deploys a device that emits a low hum, scanning the environment. They seem to be hunting for any anomalies, any deviations from the norm.

You notice one Warden’s lure flickers erratically for a moment, perhaps due to a minor power fluctuation or interference. It seems to be a moment of weakness. Their tactics are efficient, but they appear to rely heavily on their technology and coordinated movements. Disrupting their communication or their sensors might be key.

Do you:

*   Attempt to **disrupt their communication systems** by creating a loud, disorienting noise or light burst? Turn to page 10.
*   Try to **use the environment to your advantage** to create a trap or mislead them? Turn to page 11.
''',
    "nGL-CYOA-BOOK1/page_060.txt": '''You decide to explore the bustling transit hub. Holographic advertisements flicker, showcasing destinations across countless universes. Signs point towards "Angland," "Neutrino Prime," and "Xerxes' Domain," alongside cryptic symbols for less charted regions. You need information.

*   Approach a ticketing kiosk for official information. Turn to page 66.
*   Seek out smugglers or independent traders for less official routes. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_061.txt": '''You fight on, a desperate, rearguard action. The spright-saber is a potent weapon, and you manage to disable two more Wardens, their armor sparking as they fall. But the relentless advance of their brethren is too much. You are being pushed back, surrounded.

Your Angleri form is strong, but the Wardens are trained, relentless, and countless. One by one, your companions fall, or are captured. You are the last one standing, spright-saber blazing, but the odds are impossible.

*   You are overwhelmed and captured. Turn to page 38.
''',
    "nGL-CYOA-BOOK1/page_062.txt": '''You find a concealed spot behind some dense, pulsating foliage and observe the crystalline lizards. They move with an agile, almost playful gait, occasionally pausing to tap the ground with their tails, which causes faint tremors. They seem to be communicating through these tremors and subtle shifts in their scale luminescence.

You notice one of them approaches a cluster of glowing mushrooms, taps it with its tail, and the mushroom emits a puff of fragrant spores. The lizard then consumes some of the spores. It appears to be a symbiotic or at least a functional relationship with the local ecosystem.

*   Try to approach the creatures cautiously, mimicking their peaceful interaction with the environment. Turn to page 63.
*   Use your observation to search for similar resources or hidden paths indicated by the flora. Turn to page 69.
''',
    "nGL-CYOA-BOOK1/page_063.txt": '''You cautiously approach the crystalline lizards, extending a webbed hand in what you hope is a universal gesture of peace. Your lure pulses with a soft, non-threatening light. The lizards stop, their faceted eyes focusing on you. They seem to examine you, their scales shimmering.

One of them steps forward, taps a nearby crystal formation with its tail, and then taps the ground. A faint, low hum emanates from the crystal, and a series of holographic symbols appear in the air – rudimentary images of the forest, a flowing river, and then a question mark. They are trying to communicate.

*   Attempt to respond by drawing simple images in the air with your lure's light. Turn to page 70.
*   Show them the data slate from your Angler Force gear, hoping the universal format will convey your message. Turn to page 71.
''',
    "nGL-CYOA-BOOK1/page_065.txt": '''Armed with the knowledge of the Angler Empire's illusions, you decide to seek out those who operate outside its control. The FLEXTANK is vast, and there are bound to be others who have seen through the Empire's facade. You recall the mention of the "Abyssal Forge District" and "smugglers." These seem like logical places to start looking for discontent and opportunities.

*   Head towards the Abyssal Forge District. Turn to page 31.
*   Seek out information on smugglers and black markets within the FLEXTANK. Turn to page 67.
 of benevolence hides a predatory agenda. You see cracks in their seemingly impenetrable facade. This knowledge is powerful, but it doesn't offer an immediate escape.

*   Use this knowledge to seek out rebel factions or hidden weaknesses within the Angler Empire. Turn to page 73.
*   Combine this knowledge with the desire for a physical escape, looking for ways to exploit the Empire's control systems. Turn to page 66.
''',
    "nGL-CYOA-BOOK1/page_066.txt": '''You ask the kiosk about "inter-universal transit authorization." The synthesized voice responds, "Authorization for inter-universal transit is granted only to accredited research vessels, diplomatic envoys, and high-ranking Imperial officials. Applications are reviewed by the Central Nexus Authority and require extensive background checks and sponsorship from a recognized Inter-Universal Consortium member. Would you like me to initiate a Level 1 inquiry regarding eligibility for displaced sentients?"

It's a bureaucratic maze designed to keep you in place.

*   Initiate the Level 1 inquiry. Turn to page 81.
*   Decide this is a dead end and look for smugglers. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_067.txt": '''You decide to try and find a smuggler. You approach the gruff Angler, who sizes you up with a practiced eye. "Payment, eh?" it grunts. "Kids like you ain't got much. But I can tell you this: if you're lookin' to get anywhere without the Empire sniffin' around, you gotta go through the 'service conduits.' They're the guts of the FLEXTANK. Messy, dangerous, but they bypass the main patrols."

The Angler points a clawed finger towards a dimly lit section of the hub. "There's an access panel over there. Usually leads to the primary conduits. Just... try not to get yourselves vaporized by the plasma relays. And if you find any rare 'Shells' or 'Pearls,' you know who to bring 'em to for a good price."

*   Head towards the access panel to the service conduits. Turn to page 33.
un some contraband?" It chuckles, a rattling sound. "This ain't no joyride. What's in it for me?"

*   Offer to pay them with any valuable items you might have, or promise future payment. Turn to page 76.
*   Try to appeal to their sense of adventure or offer to help with a job. Turn to page 77.
''',
    "nGL-CYOA-BOOK1/page_068.txt": '''You fight on, a desperate, rearguard action. The spright-saber is a potent weapon, and you manage to disable two more Wardens, their armor sparking as they fall. But the relentless advance of their brethren is too much. You are being pushed back, surrounded.

Your Angleri form is strong, but the Wardens are trained, relentless, and countless. One by one, your companions fall, or are captured. You are the last one standing, spright-saber blazing, but the odds are impossible.

*   You are overwhelmed and captured. Turn to page 38.
''',
    "nGL-CYOA-BOOK1/page_069.txt": '''You decide to search for resources or hidden paths indicated by the flora. Recalling how the crystalline lizards interacted with the environment, you examine the glowing mushrooms and the moss. You discover that some of the moss patches, when gently pressed, reveal faint, glowing symbols beneath them – similar to the ones the lizards used.

Following a trail of these symbols, you find a hidden opening in a large, crystalline rock formation. It appears to be a natural tunnel, leading into darkness. It might be a safe passage, or it might lead to unknown dangers.

*   Enter the hidden tunnel. Turn to page 78.
''',
    "nGL-CYOA-BOOK1/page_07.txt": '''"Sanctioned research program," you state firmly, trying to sound authoritative, though your new Angleri vocal cords struggle to maintain the tone. "Designation: 'Xeno-Sentience Integration Study.' We are... experimental subjects. Part of a new inter-universal exchange initiative. Please verify our credentials with Sector Command." You add a slight, nervous flicker to your lure, hoping it conveys sincerity rather than fear.

The lead Warden's lure pulses slowly, a deep, analytical yellow. It appears to be processing your words. The other Warden with the weapon remains tense. "Inter-universal exchange? We have no record of such a program in this sector. Your bioluminescent signatures are… non-standard for this universe."

The lead Warden's lure brightens. "Accessing Nexus Database. Stand by, anomalies." A tense silence fills the chamber, broken only by the hum of the FLEXTANK and the distant sounds of your classmates' panicked movements. This is a gamble. If they find no record, you're in deep trouble.

Do you:

*   **Press your advantage**, elaborating on the "program's" supposed objectives and benefits to the Angler Empire? Turn to page 12.
*   **Seize the moment of their distraction** and make a break for it while they're querying the database? Turn to page 8.
''',
    "nGL-CYOA-BOOK1/page_070.txt": '''You decide to try and communicate using your lure's light. Focusing your new Angleri abilities, you project simple shapes and patterns into the air – a circle for your group, a wavy line for water, a crude representation of your ship, and then a question mark. You attempt to convey that you are lost and seeking help.

The crystalline lizards watch intently, their scales rippling. The lead lizard taps the ground again, and the holographic symbols reappear, but this time they form a clearer image: a map of the immediate area, highlighting a path leading into the forest, and a symbol of a large, glowing tree. It seems they understand and are offering guidance.

*   Follow the path towards the glowing tree. Turn to page 29.
''',
    "nGL-CYOA-BOOK1/page_071.txt": '''You retrieve your data slate, its interface glowing with familiar symbols. You hold it up, showing the lizards the information displayed – your school's logo, a diagram of your planet, and basic universal greetings. The lizards seem fascinated by the technology, their scales flashing with curiosity.

The lead lizard taps the ground again, and the holographic symbols appear. This time, they show a simplified schematic of the FLEXTANK, with your current location marked, and then a series of branching paths. One path leads towards a stylized representation of Angland, another towards a darker, more chaotic region.

*   Study the schematics and choose a path. Turn to page 79.
''',
    "nGL-CYOA-BOOK1/page_0711.txt": '''You cautiously approach the "Exhibit Hall Access" door. It's heavy and reinforced, but to your surprise, it's slightly ajar. Peeking through, you see the main exhibit hall – the very place where your field trip went wrong. It appears to be less patrolled at this moment, likely due to the recent "containment breach" and the ongoing alert.

*   Slip into the main exhibit hall. Turn to page 713.
''',
    "nGL-CYOA-BOOK1/page_0713.txt": '''You slip through the slightly ajar door into the main exhibit hall. The space is vast and eerily quiet compared to the bustling hubbub you’ve glimpsed elsewhere. The FLEXTANK itself still pulses with its contained universes, but the immediate area around it is subdued. You see no Bubble Wardens, but the general atmosphere is one of heightened security.

*   Approach the FLEXTANK, hoping to find answers or a way out. Turn to page 716.
*   Look for an exit from the exhibit hall, aiming to leave the mall entirely. Turn to page 717.
''',
    "nGL-CYOA-BOOK1/page_0714.txt": '''You don your discarded maintenance uniform, which fits surprisingly well over your Angleri forms. The small Glow-Berry provides a subtle light. You head towards the main areas of the mall, hoping to blend in. The uniform, while drab, seems to make other Angleri and service personnel less suspicious of your presence. You pass through bustling food courts and less crowded shopping districts.

*   Continue blending in, seeking information or an opportunity. Turn to page 718.
''',
    "nGL-CYOA-BOOK1/page_0718.txt": '''You continue to blend in, moving through the mall's less crowded areas. You overhear snippets of conversation about security sweeps and the "incident" at the FLEXTANK exhibit. It's clear the Angler Empire is taking the breach seriously. You see a sign for "Angler Force Recruitment."

*   Consider joining the Angler Force for safety and training. Turn to page 27.
*   Continue to seek a way out, avoiding Imperial entanglements. Turn to page 717.
''',
    "nGL-CYOA-BOOK1/page_072.txt": '''You decide to follow the path indicated by the robed figures, heading towards the "Unlit Void." The pearl in your hand hums, guiding you away from the bright lights of the FLEXTANK and into regions where the light dims and the stars seem to grow wild. As you venture deeper, you notice faint, ethereal forms coalescing in the distance – shimmering beings.

*   Approach these shimmering beings, who must be the Veiled Ones. Turn to page 80.
''',
    "nGL-CYOA-BOOK1/page_0721.txt": '''You look for control panels near the FLEXTANK. You find a console, ostensibly for managing the exhibit's lighting and environmental simulations. However, a closer inspection reveals it has access to basic FLEXTANK status reports and diagnostics. You see a flickering section of the barrier highlighted, marked as "Maintenance Cycle - Unstable."

*   Interact with the console to try and stabilize or open the flickering section. Turn to page 724.
*   Study the status reports, looking for information on exits or other realities. Turn to page 725.
''',
    "nGL-CYOA-BOOK1/page_0722.txt": '''You decide to try and gather information from the control center discreetly. You observe the technicians, noting their procedures and the data displayed on their screens. You see schematics for the mall's transit systems and security grids, as well as reports on dimensional stability within certain FLEXTANK sectors.

*   Attempt to download some of this data onto your data slate. Turn to page 726.
*   Try to find a terminal that shows FLEXTANK exit protocols. Turn to page 727.
''',
    "nGL-CYOA-BOOK1/page_0723.txt": '''You look for a way to access a transport system or exit from the control center. You find a conduit labeled "High-Speed Transit Hub - Sector 7." It seems to lead to a major transportation nexus within the FLEXTANK.

*   Enter the High-Speed Transit Hub conduit. Turn to page 46.
''',
    "nGL-CYOA-BOOK1/page_073.txt": '''You decide to use the knowledge of the Angler Empire's illusions to seek out rebels. You recall the mention of the "Abyssal Forge District" and "smugglers," places where discontent might fester.

*   Head towards the Abyssal Forge District. Turn to page 31.
*   Seek out information on smugglers and black markets within the FLEXTANK. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_074.txt": '''You ask the kiosk about obtaining "inter-universal transit authorization." The synthesized voice responds, "Authorization requires Level 5 clearance or sponsorship from a recognized Inter-Universal Consortium member. Applications are processed by the Central Nexus Authority with extensive background checks. Would you like to initiate a Level 1 inquiry regarding eligibility for displaced sentients?"

It's a bureaucratic labyrinth designed to keep you contained.

*   Initiate the Level 1 inquiry. Turn to page 81.
*   Abandon this path and look for smugglers. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_075.txt": '''You ask the kiosk about booking passage to a "stabilized reality for integration." The kiosk displays options: "Assimilation Program: Neutrino Prime," "Resettlement Initiative: Aether's Realm," "Labor Corps: Angler Empire Outposts." Each promises a new life, but within the Empire's structured control.

*   Choose a program that sounds least restrictive, like "Assimilation Program: Neutrino Prime." Turn to page 85.
*   Reject integration and look for smugglers. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_076.txt": '''You approach the gruff Angler smuggler. "We don't have much," you admit, "but we can pay later, or help with a job. We just need a way to get around, off the main routes."

The Angler eyes you up and down, a flicker of interest in its gaze. "Jobs, eh? Always need muscle, or eyes. Alright, listen up. There's a shipment heading to the Abyssal Forge tonight, needs 'escorting' through a few unstable sectors. Dangerous, but the pay is good. You help me, I'll get you to the Forge, and from there, you're on your own. But you'll be closer to folks who might know about 'exits'."

*   Accept the smuggler's offer to escort a shipment. Turn to page 703.
*   Decline the offer, deciding it's too risky, and look for another way. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_077.txt": '''You approach the gruff Angler smuggler. "We're looking for adventure," you say, trying to sound bold. "Or maybe a challenge. If you've got a job that needs doing, something that takes us off the beaten path, we're your crew."

The Angler grins, revealing sharp teeth. "Adventure, eh? I like your spirit. There's a deal going down in the Outer Veils, collecting some rare samples. High risk, high reward. But it means navigating some tricky currents and avoiding Imperial patrols. You in?"

*   Accept the smuggler's job to collect samples in the Outer Veils. Turn to page 704.
*   Decline the job, deciding it's too dangerous. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_078.txt": '''You decide to investigate the hidden tunnel. It's narrow and descends sharply, forcing you to move on your belly. The air grows colder, and the faint luminescence of the forest disappears, replaced by absolute darkness. You can hear a faint dripping sound and the scuttling of unseen things.

After what feels like an eternity, the tunnel widens, and you see a faint glow ahead. You emerge into a vast, underground cavern. Strange, crystalline formations hang from the ceiling, dripping with luminous liquid. In the center of the cavern is a pool of shimmering fluid, and descending tendrils of light. This looks like the heart of some organic network.

*   Investigate the shimmering pool. Turn to page 24.
*   Look for other passages leading away from this chamber. Turn to page 25.
''',
    "nGL-CYOA-BOOK1/page_079.txt": '''You examine the holographic schematics. The path towards Angland is brightly lit, suggesting it's a controlled area. The path towards the chaotic region, however, is marked with skull symbols and flickering warnings. It is clearly dangerous, but it also seems to lead away from any imperial presence.

You decide the darker path is your best bet for finding true freedom, or at least a way to avoid immediate assimilation. You point towards the symbol for the chaotic region. The lead lizard taps the ground, and the holographic map shifts, highlighting a specific route.

*   Follow the path indicated by the lizards towards the chaotic region. Turn to page 72.
''',
    "nGL-CYOA-BOOK1/page_08.txt": '''With a surge of adrenaline, you and the handful of students who understood your gesture sprint towards the flickering section of the FLEXTANK's barrier. The quantum fluid rushes past you, your new Angleri bodies surprisingly adept at propulsion. Behind you, the Bubble Wardens realize what you're doing and their weapons begin to glow menacingly.

You reach the area. It pulses with a chaotic energy, seeming to warp the very fabric of reality around it. It feels less like a solid barrier and more like a volatile membrane. A few of your companions hesitate, fear overriding their desire for escape.

Do you:

*   **Leap through the flickering membrane**, trusting it leads to somewhere – anywhere – else? Turn to page 13.
*   **Try to stabilize the flickering area** first, perhaps by channeling your new Angleri energy or using some of the strange flora around you? Turn to page 14.
''',
    "nGL-CYOA-BOOK1/page_080.txt": '''You approach the Veiled Ones. They are beings of pure, shimmering light, their forms indistinct and ethereal. As you draw near, they seem to coalesce, their faint glow intensifying. They don't speak with words, but you feel their presence directly in your minds – a sense of ancient wisdom and a profound understanding of the forces that shape existence.

They guide you, not with directions, but with a subtle shift in the cosmic currents. You feel yourself being drawn towards a place where the stars themselves seem to dim, a region of profound quietude. It is the Unlit Void, and you are being led towards its heart, a place of true freedom from the Angler Empire's control, but also a place of profound mystery.

*   Continue your journey into the heart of the Unlit Void with the Veiled Ones. Turn to page 82.
''',
    "nGL-CYOA-BOOK1/page_081.txt": '''"Initiate Level 1 inquiry," you say to the kiosk. The holographic globe spins faster, and a new window appears: "Inquiry initiated. Please provide biographical data and origin universe coordinates. Expected response time: 3-5 standard cycles. While you await inquiry results, would you like to browse available integration programs?"

You've entered a system designed to capture and categorize you. The wait time is extensive, and there's no guarantee of a positive outcome.

*   Wait for the inquiry results and browse integration programs. Turn to page 83.
*   Abandon the kiosk and look for smugglers. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_082.txt": '''You follow the Veiled Ones deeper into the Unlit Void. The light of the FLEXTANK's other universes fades entirely, replaced by a gentle, pervasive darkness that feels not empty, but full of potential. The Veiled Ones move with you, their forms like guiding stars in this cosmic night.

You sense that you are nearing a place of power, a refuge for those who reject the Angler Empire's dominion. This is where the Unlit Tribe makes its stand. The path ahead is uncertain, but you feel a growing sense of purpose, a connection to something larger than yourself.

*   You are on the path to finding the Unlit Tribe. Turn to page 84.
''',
    "nGL-CYOA-BOOK1/page_083.txt": '''You decide to wait for the inquiry results and browse the integration programs. The kiosk presents you with a dizzying array of options, from joining Angler Force patrols to working in resource extraction on distant moons, or even participating in xenobiological studies. Each program promises a structured existence within the FLEXTANK, aligned with the Angler Empire's goals.

As you browse, you notice a subtle pattern. Many programs involve contributing to "stabilization efforts" or "containment protocols" for newly discovered or unstable universes. It seems the Angler Empire is not just observing, but actively managing and controlling these pocket realities.

*   Select a program that seems least restrictive or offers potential for learning more about the FLEXTANK's operations. Turn to page 85.
*   Decide this path is too restrictive and look for smugglers. Turn to page 67.
''',
    "nGL-CYOA-BOOK1/page_084.txt": '''You feel a sense of destiny, a calling to the Unlit Void. You are on the path to finding the Unlit Tribe, a group that rejects the superficial light of the Angler Empire for a deeper truth. The Veiled Ones continue to guide you, their ethereal presence a constant comfort in the encroaching darkness.

You are moving towards a place of profound significance, where the very nature of reality is questioned and potentially redefined. Your journey has just begun, and the path ahead promises both great danger and immense revelation.

*   You are heading towards the Unlit Tribe. Turn to page 86.
''',
    "nGL-CYOA-BOOK1/page_085.txt": '''You choose a program that seems to offer the most flexibility: "Xeno-Cultural Exchange Observer." It involves monitoring developing civilizations within the FLEXTANK, reporting on their progress and potential threats to Imperial stability. The allure is the opportunity to see more of the FLEXTANK and understand its workings from within.

The kiosk confirms your provisional acceptance. You are given a temporary identification chip and directed towards a transport hub. You are now a cog in the Angler Empire's machine, a designated observer of worlds. Your journey into the FLEXTANK has officially begun, but it is a path dictated by Imperial decree.

*   Accept your new role and proceed to the transport hub. Turn to page 87.
''',
    "nGL-CYOA-BOOK1/page_086.txt": '''You feel a pull towards the heart of the Unlit Void, guided by the ethereal Veiled Ones. This realm is not empty, but teeming with a different kind of existence, one that thrives beyond the fabricated light of the Angler Empire. You are on the cusp of discovering the Unlit Tribe, rebels who seek truth in the very darkness that the Empire fears.

Your journey has led you to a pivotal moment, where you will confront the true nature of the FLEXTANK and your place within it.

*   You are now deeply involved in the quest for truth beyond the Empire's control. Turn to page 86.
''',
    "nGL-CYOA-BOOK1/page_087.txt": '''You board the ship as an observer. Your destination is Terra Nova, a developing world on the edge of Imperial expansion. The ship warps through dimensional currents, and you arrive at an orbital platform. Your mission is to observe and report on the planet's internal conflicts, subtly nudging events to favor Imperial interests.

*   Begin your observation mission on Terra Nova. Turn to page 88.
''',
    "nGL-CYOA-BOOK1/page_088.txt": '''Your mission on Terra Nova begins. The world is a vibrant, yet volatile, planet teetering on the brink of civil war. Your observation post is a discreet orbital platform, allowing you to monitor communications and troop movements. You are tasked with identifying key factions and predicting potential outcomes, reporting back to your Angler superiors.

The political landscape is complex, with warring factions employing primitive but effective weaponry. You witness skirmishes and diplomatic failures, all while adhering to your directive of non-interference unless Imperial interests are directly threatened. This is your role: an unseen hand, shaping destinies from afar.

*   Continue your observation mission, gathering intelligence on Terra Nova's factions. Turn to page 89.
''',
    "nGL-CYOA-BOOK1/page_089.txt": '''You continue your mission, meticulously recording data on Terra Nova's factions. The planet is a powder keg, and your reports paint a grim picture of impending civil war. You await further directives, your role as an observer feeling increasingly like a passive participant in the planet's fate.

*   Await further orders from the Angler Empire. Turn to page 90.
''',
    "nGL-CYOA-BOOK1/page_09.txt": '''You need to buy your classmates time to hide. Spotting a cluster of large, crystalline growths nearby, you race towards them. With a mighty shove, you topple one of the larger crystals. It crashes to the floor of the FLEXTANK, emitting a cascade of bright, disorienting light and a surprisingly loud *clang*.

The Bubble Wardens immediately turn their attention to the commotion. "Anomaly neutralized!" one shouts, its lure flashing red. They advance towards the fallen crystal, their weapons raised. This buys your classmates precious moments to melt into the shadows. You, however, have revealed your position. You need to move, and fast.

Do you:

*   **Use the distraction to slip away** and find your own hiding place? Turn to page 5.
*   **Use the Wardens' focus on the crystal** to circle around and potentially flank them, or disrupt their advance? Turn to page 15.
''',
    "nGL-CYOA-BOOK1/page_090.txt": '''You meticulously record every detail of Terra Nova's unfolding conflict. Your reports to the Angler Empire are detailed and objective, providing them with a comprehensive understanding of the planet's internal struggles. You are a loyal observer, fulfilling your duty to the letter.

One day, a high-priority directive arrives. The Empire has decided that Terra Nova's current trajectory is detrimental to its long-term interests. You are authorized to take "limited action" to influence the outcome, specifically by providing strategic intelligence to the faction deemed most amenable to Imperial influence. You are no longer just an observer; you are now an agent of change, albeit under Imperial command.

*   Follow the directive and provide intelligence to the favored faction. Turn to page 92.
*   Refuse the directive, deeming it unethical or too risky, and continue only to observe. Turn to page 93.
''',
    "nGL-CYOA-BOOK1/page_091.txt": '''You can no longer stand by and merely observe. The Angler Empire's directive to maintain stability feels like a euphemism for control. You decide to subtly aid the faction that speaks of freedom and self-determination. Using your access to planetary communications, you feed them anonymous tips about enemy movements and supply lines.

Your actions are small, intended to tip the scales without revealing your involvement. You watch as the favored faction gains ground, their revolutionary ideals resonating with the planet's populace. You are walking a dangerous line, but the hope you see flickering in the eyes of the people of Terra Nova fuels your defiance.

*   Continue your covert support for the freedom fighters. Turn to page 94.
''',
    "nGL-CYOA-BOOK1/page_092.txt": '''You transmit the requested intelligence to the designated faction. Their forces, now better informed, launch a successful strategic offensive, shifting the balance of power on Terra Nova. The Angler Empire's influence on the planet is now solidified, with the favored faction poised to win the conflict and establish a pro-Imperial government.

You have successfully executed your orders, ensuring Imperial dominance. However, a small part of you wonders if you've just traded one form of oppression for another.

*   Await your next assignment, having completed this one. Turn to page 95.
''',
    "nGL-CYOA-BOOK1/page_093.txt": '''You refuse the directive. Manipulating a planetary conflict feels like a violation of the very principles of observation you were meant to uphold. You transmit a message back to the Empire stating your inability to comply with the order, citing ethical concerns and potential for unforeseen consequences.

Your defiance does not go unnoticed. Within hours, your orbital platform is boarded by elite Angler Force operatives. They escort you, not to a disciplinary hearing, but to a transport vessel. You are being moved, your mission reassigned, your future uncertain.

*   You are taken to an unknown destination. Turn to page 96.
''',
    "nGL-CYOA-BOOK1/page_094.txt": '''Your covert support for the freedom fighters continues. You feed them crucial intelligence, using your access to the FLEXTANK's systems to mask your tracks. The faction you are aiding gains momentum, their cause bolstered by your anonymous assistance. You see the hope in their eyes, the spark of true self-determination.

However, your actions are not going unnoticed by the Angler Empire. Your unusual data transmissions and the unexpected success of the "rebel" faction have triggered security protocols. You feel a growing sense of unease, as if unseen eyes are beginning to watch you.

*   Continue your operations, but be more cautious. Turn to page 97.
*   Decide the risk is too great and cease your intervention. Turn to page 98.
''',
    "nGL-CYOA-BOOK1/page_095.txt": '''You have successfully executed your orders on Terra Nova, ensuring Imperial dominance. You receive commendations for your role. Your next assignment arrives: a deep-space patrol mission along the fringe of known sectors to monitor for 'Unlit' activity or dimensional breaches.

*   Embark on your deep-space patrol mission. Turn to page 103.
''',
    "nGL-CYOA-BOOK1/page_096.txt": '''Your defiance led to your apprehension. You are escorted onto a vessel by grim-faced Angler Force operatives. They provide no explanation for your destination, only curt orders to board. You are whisked away into the FLEXTANK's vastness, your future uncertain.

*   You are taken to an unknown destination. Turn to page 100.
''',
    "nGL-CYOA-BOOK1/page_097.txt": '''You continue your covert operations, but the Angler Empire's security is tightening. You have identified a weakness in their planetary control systems, a backdoor that could be exploited.

*   Exploit the weakness to further destabilize Imperial control. Turn to page 101.
*   Use the weakness to find a way to escape the FLEXTANK entirely. Turn to page 102.
''',
    "nGL-CYOA-BOOK1/page_098.txt": '''You choose to cease your intervention, returning to passive observation. You continue to file reports, but the guilt of inaction weighs heavily. Your future within the FLEXTANK is now likely one of quiet servitude, a cog in the Imperial machine.

*   Continue your mission as a passive observer. Turn to page 90.
''',
    "nGL-CYOA-BOOK1/page_099.txt": '''Your next assignment arrives: a deep-space patrol mission along the fringe of known sectors. You are to monitor for any signs of 'Unlit' activity or dimensional breaches. It is a standard, if potentially hazardous, duty. You are equipped with a long-range scanner and a combat-ready spright-saber.

*   Embark on your deep-space patrol mission. Turn to page 103.
''',
    "nGL-CYOA-BOOK1/page_100.txt": '''The ship travels for an indeterminate amount of time, warping through different dimensional currents. You are given basic rations and allowed to observe the star fields, a breathtaking, terrifying panorama of the FLEXTANK's immensity. Finally, the ship decelerates, entering a docking bay of a colossal, obsidian-like structure. It hums with a strange, resonant energy, unlike anything you've seen before.

You are led off the ship into a grand hall, where figures cloaked in shadow await you. Their lures pulse with a faint, almost imperceptible light. You sense a profound difference between these beings and the Angler Empire.

*   You have arrived at a new, mysterious location. Turn to page 104.
''',
    "nGL-CYOA-BOOK1/page_101.txt": '''You exploit the weakness in the control systems, not to cause chaos, but to create a diversion. You initiate a massive data surge, momentarily overloading the planetary communications and security grids. This creates a window of opportunity for those who wish to escape the Empire's grasp.

You've also subtly transmitted schematics for a rudimentary escape craft, based on your observations of FLEXTANK technology, to a trusted contact among the freedom fighters. You hope they can use it to break free from Terra Nova and perhaps even the FLEXTANK itself.

*   Your actions have created an escape route. Turn to page 105.
''',
    "nGL-CYOA-BOOK1/page_102.txt": '''Using the backdoor in the control systems, you focus your efforts on finding a way out of the FLEXTANK. You delve into ancient logs and forgotten schematics, searching for any mention of egress points or dimensional gateways. It's a dangerous undertaking, as any anomaly in the system could alert the Empire.

After days of searching, you find it: a reference to a 'Quantum Veil' access point, located in a rarely patrolled sector of the FLEXTANK. It's a theoretical pathway, highly unstable, but it might be your only chance.

*   Attempt to navigate towards the Quantum Veil access point. Turn to page 106.
''',
    "nGL-CYOA-BOOK1/page_103.txt": '''You embark on your patrol mission. The vastness of space in the FLEXTANK is both awe-inspiring and terrifying. Your scanners sweep across nebulae, asteroid fields, and the faint glows of distant, unknown universes. It's a lonely vigil, far from any familiar world.

Days turn into cycles. You encounter minor phenomena – energy fluctuations, rogue asteroid clusters – but no sign of 'Unlit' activity. The silence is broken only by the hum of your ship's engines and the occasional ping of your long-range scanner.

*   Continue your patrol, hoping for a significant discovery. Turn to page 107.
*   Decide to investigate a strange energy anomaly detected on the edge of your scan range. Turn to page 108.
''',
    "nGL-CYOA-BOOK1/page_104.txt": '''You disembark from the transport. The obsidian structure seems to absorb all light, yet it hums with a deep, powerful energy. The cloaked figures greet you not with words, but with a shared sense of understanding. You feel their connection to the "Unlit Void," to the truth beyond the Angler Empire's control.

You have arrived at a sanctuary for those who seek a different path. This is likely the home of the Unlit Tribe, or a place where they gather. Your journey has led you to the heart of rebellion.

*   You have reached a significant point on the path of rebellion. Turn to page 86.
''',
    "nGL-CYOA-BOOK1/page_105.txt": '''You initiate the escape sequence. With the diversion you created, you and your chosen classmates manage to board a small, functional craft. It's crude, built from scavenged parts and jury-rigged systems, but it responds to your command.

As the vessel lurches away from Terra Nova, you see the Imperial forces scrambling to respond. You've made your move, and the FLEXTANK is now your vast, unknown playground. Your future is uncertain, but you are free from immediate control.

*   You have escaped Terra Nova. Turn to page 109.
''',
    "nGL-CYOA-BOOK1/page_106.txt": '''You pilot your vessel towards the Quantum Veil access point. The entry is violent, a chaotic symphony of tearing reality. Your ship struggles, systems flickering. Suddenly, you are expelled into an expanse of utter blackness. The FLEXTANK is gone.

*   You have entered the Quantum Veil. Turn to page 115.
''',
    "nGL-CYOA-BOOK1/page_107.txt": '''Your patrol continues through the vastness of space. Days turn into cycles, marked by sensor readings and the occasional anomaly. You've avoided unstable dimensional rifts and charted nebulae, but found no trace of 'Unlit' activity. The silence of space is profound.

*   Continue your patrol, hoping for a significant discovery. Turn to page 107.
*   Investigate a strange energy anomaly detected on the edge of your scan range. Turn to page 108.
''',
    "nGL-CYOA-BOOK1/page_108.txt": '''You decide to investigate the strange energy anomaly. It's in an 'Uncharted Void' sector. As you approach, faint, coherent signals are detected – unlike any natural phenomenon. The anomaly is a small, cloaked vessel broadcasting a coded message.

*   Attempt to hail the cloaked vessel. Turn to page 112.
*   Scan the vessel for further intelligence before hailing. Turn to page 113.
''',
    "nGL-CYOA-BOOK1/page_109.txt": '''You have escaped Terra Nova, but you are now fugitives in the vast FLEXTANK. Your immediate goal is survival, but you also need a long-term plan.

*   Seek refuge in the Abyssal Forge District. Turn to page 31.
*   Attempt to find your way to the Grand Lumina Mall. Turn to page 56.
*   Head towards less charted regions, seeking freedom. Turn to page 103.
''',
    "nGL-CYOA-BOOK1/page_110.txt": '''With your heart pounding, you pilot your makeshift vessel towards the shimmering distortion of the Quantum Veil. The entry is violent, a chaotic symphony of tearing reality and flashing lights. You feel your ship, and yourselves, being stretched and compressed.

Then, silence. You drift in an unknown void, the FLEXTANK no longer visible. Did you escape, or are you merely in another, more isolated, pocket of the multiverse? The sky is a deep, unending black, devoid of stars.

*   You have entered the Quantum Veil. Turn to page 115.
''',
    "nGL-CYOA-BOOK1/page_111.txt": '''You steer your patrol ship towards the source of the strange energy signature. As you get closer, your sensors pick up faint, coherent signals, unlike any natural phenomenon. The signature leads you to a small, cloaked vessel adrift in the 'Void,' broadcasting a subtle, coded message.

*   Attempt to hail the cloaked vessel. Turn to page 112.
*   Scan the vessel for further intelligence before attempting contact. Turn to page 113.
''',
    "nGL-CYOA-BOOK1/page_112.txt": '''You initiate a hailing frequency, broadcasting a standard greeting. The cloaked vessel responds, its message a complex mix of Angler script and something entirely alien. However, certain phrases stand out: "Unlit," "Refuge," "Seek the shadows."

It seems you've encountered a vessel associated with the Unlit Tribe or one of their allies. They are clearly avoiding Imperial detection.

*   Respond with a coded message indicating you are seeking the "Unlit" path. Turn to page 116.
*   Decide this is too dangerous and disengage, returning to your patrol. Turn to page 107.
''',
    "nGL-CYOA-BOOK1/page_113.txt": '''You activate your ship's advanced scanners, focusing them on the cloaked vessel. The readings are baffling. It seems to be phasing in and out of local reality, utilizing some form of cloaking technology far beyond Imperial designs. The coded message is confirmed to be a signal for allies, seeking passage into a region designated "The Rift."

The vessel appears to be vulnerable, its phasing technology causing intermittent disruptions.

*   Attempt to hail the vessel, identifying yourself as seeking the "Unlit" path. Turn to page 116.
*   Prepare to board the vessel, seeing an opportunity to gain valuable technology or intelligence. Turn to page 117.
''',
    "nGL-CYOA-BOOK1/page_114.txt": '''You have escaped Terra Nova, but you are now fugitives in the vast FLEXTANK. Your immediate goal is survival, but you also need a long-term plan.

*   Seek refuge in the Abyssal Forge District. Turn to page 31.
*   Attempt to find your way to the Grand Lumina Mall. Turn to page 56.
*   Head towards less charted regions, seeking freedom. Turn to page 103.
''',
    "nGL-CYOA-BOOK1/page_115.txt": '''You pilot your vessel towards the Quantum Veil access point. The entrance is as chaotic as predicted, a maelvert of temporal and spatial distortions. Your ship struggles to maintain coherence, systems flickering wildly.

Suddenly, you are ejected into an expanse of utter blackness. No stars, no nebulae, just an infinite, silent void. The FLEXTANK, and all the universes it contained, are gone. You have escaped. But into what? Is this true freedom, or a different kind of prison?

*   You have escaped the FLEXTANK. Turn to page 118.
''',
    "nGL-CYOA-BOOK1/page_116.txt": '''You respond with the coded message: "Seeking the shadows. Path of the Unlit confirmed." The cloaked vessel's broadcast changes, alien script becoming clearer: "Proceed to coordinates Alpha-7-Delta. The Rift awaits. Trust not the light."

*   Set course for the coordinates provided. Turn to page 119.
''',
    "nGL-CYOA-BOOK1/page_117.txt": '''Seeing the vulnerability of the cloaked vessel, you decide to board it. Your ship maneuvers close, and you use a tractor beam to pull it in. As you prepare to breach the hull, you notice the vessel's phasing technology faltering more rapidly, as if your proximity is destabilizing it.

*   Attempt to board the vessel before it phases out completely. Turn to page 120.
''',
    "nGL-CYOA-BOOK1/page_118.txt": '''You have escaped the FLEXTANK. You are in a place of absolute void, a true nothingness. Your journey has ended, or perhaps, it has just begun in a way you could never have imagined. The future is an unknown expanse.

*   The End. Turn to page 121.
''',
    "nGL-CYOA-BOOK1/page_119.txt": '''You decide to set course for the coordinates Alpha-7-Delta, provided by the cloaked vessel. As you travel, the FLEXTANK's usual vibrant displays fade, replaced by a deep, pervasive darkness. The further you venture, the more the light of other universes recedes. You are heading towards "The Rift," a place where the Angler Empire's influence wanes and the Unlit Tribe finds its strength.

*   You are heading towards The Rift. Turn to page 86.
''',
    "nGL-CYOA-BOOK1/page_120.txt": '''You attempt to board the cloaked vessel before it phases out. Your ship locks onto it, and you breach the hull. Inside, the technology is alien and phasing. You find shadowy beings who react with surprise but not alarm. One speaks, "You are Imperial dogs. Yet you seek passage?"

*   Declare your desire to seek the 'Unlit' path. Turn to page 122.
*   Attempt to seize the vessel for the Empire. Turn to page 123.
''',
    "nGL-CYOA-BOOK1/page_121.txt": '''This is the end of your adventure within the FLEXTANK. You have navigated its perils, made choices that shaped your destiny, and found yourself in a place beyond the known. Whether this is a new beginning or an ultimate end, your story concludes here.

THE END
''',
    "nGL-CYOA-BOOK1/page_122.txt": '''You declare your intention to abandon the Empire and seek the "Unlit" path. The shadowy beings agree, granting you passage through a shimmering gateway.

*   Step through the gateway into the unknown. Turn to page 86.
anding.

*   Step through the gateway into the unknown. Turn to page 86.
''',
    "nGL-CYOA-BOOK1/page_123.txt": '''You attempt to seize the vessel, your spright-saber blazing. The shadowy beings react with surprising speed, their forms coalescing into sharp, ephemeral blades. The fight is disorienting, their attacks phasing through your defenses.

You are outmatched. Their mastery over the void is absolute. They do not fight to kill, but to subdue. You are quickly disarmed and overwhelmed.

*   You are subdued by the shadowy beings. Turn to page 124.
''',
    "nGL-CYOA-BOOK1/page_124.txt": '''You attempt to seize the vessel, but the shadowy beings are too skilled. They disarm you and subdue you. "You sought to capture truth," one whispers. "Truth cannot be captured. It can only be embraced."

*   You are left adrift with a choice: seek the Unlit path or return to Imperial sectors. Turn to page 125.
''',
    "nGL-CYOA-BOOK1/page_125.txt": '''You are left adrift, disarmed. The shadowy beings depart, leaving you with a choice: seek the Unlit path or return to Imperial sectors.

*   Seek the Unlit path. Turn to page 86.
*   Attempt to return to Imperial controlled sectors. Turn to page 103.
''',
    "nGL-CYOA-BOOK1/page_700.txt": '''You decide to enter the core reality interface. The vortex pulses with raw power, and your vessel struggles to maintain integrity. You are tossed through dimensions, witnessing sights that defy comprehension – universes in formation, collapsing stars, and cosmic entities beyond mortal understanding.

Suddenly, you are expelled into a seemingly empty void. The FLEXTANK is gone. You are in a place of absolute blackness, devoid of stars or light. You have escaped, but into what?

*   You have entered the Quantum Veil. Turn to page 115.
''',
    "nGL-CYOA-BOOK1/page_701.txt": '''You attempt to navigate through the maintenance crawlspace. It's dark, cramped, and filled with a disorienting maze of pipes and wires. You crawl for what feels like hours, occasionally finding junctions that lead to nowhere, or precarious drops.

Eventually, you stumble upon a small, grimy access panel. Pushing it open, you find yourselves looking out into a bustling transit hub filled with beings of various species and holographic advertisements. You've found a way out of the immediate infrastructure, but you're now in a central nexus of the FLEXTANK.

*   Explore the transit hub. Turn to page 60.
''',
    "nGL-CYOA-BOOK1/page_702.txt": '''You decide to share your knowledge and discuss options with your classmates. After much debate, you agree that seeking the "Unlit Void" offers the best chance for truth and freedom, rather than being assimilated or remaining a fugitive.

*   Set course for the Unlit Void. Turn to page 72.
.

*   Set course for the Unlit Void. Turn to page 72.
''',
    "nGL-CYOA-BOOK1/page_703.txt": '''You decide to join the smugglers for the shipment to the Abyssal Forge. The journey is hazardous, but your new Angleri agility and resilience serve you well. You arrive at the Forge, a gritty industrial district filled with raw power and potential dangers.

*   You have arrived in the Abyssal Forge. Turn to page 31.
trial district teeming with Angleri and other beings.

*   You have arrived in the Abyssal Forge. Turn to page 31.
''',
    "nGL-CYOA-BOOK1/page_704.txt": '''You accept the smuggler's offer for the 'adventure' in the Outer Veils. The mission is challenging, requiring you to navigate treacherous cosmic currents and evade Imperial patrols. You successfully collect the rare samples and return with a profit, earning the smuggler's respect.

*   The impressed smuggler offers you another opportunity. Turn to page 705.
gler's respect.

*   The smuggler, impressed, offers you a new opportunity. Turn to page 705.
''',
    "nGL-CYOA-BOOK1/page_706.txt": '''You decide to attempt a discreet entry into the Grand Lumina Mall. Using your Angleri forms' natural agility and the shadows cast by the bioluminescent architecture, you and your classmates manage to slip past some of the outer patrols. You find yourselves in a less guarded service corridor.

*   Navigate through the service corridors of the mall. Turn to page 707.
''',
    "nGL-CYOA-BOOK1/page_707.txt": '''You find yourselves in a hidden service corridor within the Grand Lumina Mall. The air hums with the distant sounds of commerce and activity. You can see conduits leading deeper into the mall's infrastructure, and signs pointing towards various districts.

*   Explore deeper into the mall's infrastructure. Turn to page 708.
*   Try to find a way back to the main exhibit area, hoping for a less guarded entrance. Turn to page 709.
''',
    "nGL-CYOA-BOOK1/page_708.txt": '''You venture deeper into the mall's infrastructure. The corridors become more utilitarian, filled with humming machinery and storage areas. You pass by areas labeled "Waste Reclamation," "Energy Distribution," and "Bio-Luminescent Cultivation." It seems the mall is a complex ecosystem, with hidden workings supporting its dazzling facade.

*   Continue deeper into the infrastructure, looking for an exit or a way to learn more. Turn to page 708.
''',
    "nGL-CYOA-BOOK1/page_709.txt": '''You decide to try and find a less guarded entrance back to the main exhibit area. You navigate through service tunnels, trying to avoid any signs of official patrol. You eventually find yourselves emerging into a large, open area that seems to be a storage bay for display materials.

*   Use the storage bay to find a way into the main exhibit area. Turn to page 710.
''',
    "nGL-CYOA-BOOK1/page_710.txt": '''You enter the storage bay, finding yourself surrounded by crates filled with holographic projectors, spare parts for exhibits, and bizarre Angler-themed decorations. It seems like a good place to hide and observe. You see a large, double door labeled "Exhibit Hall Access - Authorized Personnel Only."

*   Attempt to open the "Exhibit Hall Access" door. Turn to page 711.
*   Search the storage bay for useful items or alternative exits. Turn to page 712.
''',
    "nGL-CYOA-BOOK1/page_712.txt": '''You search the storage bay. Amidst the crates, you find a discarded maintenance uniform, some basic tools, and a small, glowing orb that seems to be a discarded "Glow-Berry" from the mall's food court. It emits a faint, warm light. You also spot a ventilation shaft that might lead further into the mall's complex.

*   Put on the uniform and try to blend in, heading towards the mall's main areas. Turn to page 714.
*   Investigate the ventilation shaft. Turn to page 715.
''',
    "nGL-CYOA-BOOK1/page_715.txt": '''You investigate the ventilation shaft. It's tight and dusty, but navigable. As you crawl deeper, you hear the faint sounds of machinery and distant voices. It seems to lead towards the mall's central processing or utility areas.

*   Continue through the ventilation shaft. Turn to page 719.
''',
    "nGL-CYOA-BOOK1/page_716.txt": '''You approach the FLEXTANK, the source of your transformation and the cause of your current predicament. The immense, shimmering barrier pulses with contained realities. As you get closer, you notice a section of the barrier that seems to flicker, similar to the one you saw earlier, but this time it's more pronounced, as if it's unstable or designed for maintenance.

*   Examine the flickering section of the FLEXTANK barrier more closely. Turn to page 720.
*   Look for any control panels or interfaces near the FLEXTANK that might offer information. Turn to page 721.
''',
    "nGL-CYOA-BOOK1/page_717.txt": '''You decide to leave the mall. You navigate back through the service corridors, using the knowledge gained from your brief exploration. Your goal is to find a way out of Angland entirely, perhaps into the less controlled sectors of the FLEXTANK.

*   Exit the mall and seek passage to the outer sectors. Turn to page 55.
''',
    "nGL-CYOA-BOOK1/page_719.txt": '''You continue through the ventilation shaft. It leads you to a section of the mall that appears to be a control center. Technicians – mostly Angleri – are working diligently, monitoring various readouts and screens. The air hums with the sound of active systems.

*   Attempt to gather information from the control center discreetly. Turn to page 722.
*   Look for a way to access a transport system or exit from this area. Turn to page 723.
''',
    "nGL-CYOA-BOOK1/page_720.txt": '''You examine the flickering section of the FLEXTANK barrier. It seems to be a temporary maintenance access point, flickering as it cycles through stability phases. It looks like it might be possible to pass through during one of these cycles, but it would be incredibly risky.

*   Attempt to pass through the flickering section of the barrier. Turn to page 13.
*   Decide it's too dangerous and look for another way. Turn to page 721.
''',
    "nGL-CYOA-BOOK1/page_724.txt": '''You interact with the console. Your attempt to stabilize the flickering section fails. Instead, it triggers a localized energy surge, causing the FLEXTANK barrier to flare violently and alarms to blare. Bubble Wardens are alerted.

*   You've triggered an alarm and alerted the Wardens. Turn to page 21.
''',
    "nGL-CYOA-BOOK1/page_725.txt": '''You study the status reports on the console. You see that the FLEXTANK houses countless universes, many designated for "observation" and others for "containment." There are mentions of "inter-universal transit authorization" and "displaced sentients," confirming your situation. You also notice a section detailing "emergency egress points," though they are heavily restricted.

*   Try to access the information on "emergency egress points." Turn to page 728.
*   Focus on finding "inter-universal transit authorization" procedures. Turn to page 74.
''',
    "nGL-CYOA-BOOK1/page_726.txt": '''You attempt to download data onto your data slate. You find an unsecured terminal and begin transferring schematics of the mall's transit systems and security grids, along with reports on dimensional stability. Suddenly, a technician notices your activity. "Hey! What are you doing?"

*   You are caught attempting to download data. Turn to page 729.
''',
    "nGL-CYOA-BOOK1/page_727.txt": '''You search for a terminal showing FLEXTANK exit protocols. You find a heavily encrypted terminal, clearly restricted. However, by observing the technicians, you manage to glean some of the access codes and security measures.

*   Attempt to bypass the encryption and access the exit protocols. Turn to page 730.
''',
    "nGL-CYOA-BOOK1/page_728.txt": '''You focus on the "emergency egress points" section of the console. The information is heavily encrypted, but you manage to find a reference to a seldom-used maintenance tunnel that supposedly leads to a "stabilization nexus" outside the main exhibit. It requires specific clearance codes.

*   Try to find the clearance codes to access the maintenance tunnel. Turn to page 731.
*   Decide it's too risky and look for another way out of the exhibit hall. Turn to page 717.
''',
    "nGL-CYOA-BOOK1/page_729.txt": '''You are caught downloading data by a technician. Before they can fully react, you and your classmates make a break for it, scrambling back into the ventilation shafts. You hear alarms blaring behind you.

*   Escape back into the ventilation system. Turn to page 715.
''',
    "nGL-CYOA-BOOK1/page_730.txt": '''You attempt to bypass the encryption on the exit protocols terminal. It's a complex challenge, but your combined knowledge and desperate efforts pay off. You uncover a hidden subroutine referencing a "Quantum Veil" access point, a theoretical pathway for emergency egress.

*   You have found information about the Quantum Veil. Turn to page 106.
''',
    "nGL-CYOA-BOOK1/page_731.txt": '''You decide to search for the clearance codes needed for the emergency maintenance tunnel. You subtly observe the technicians, noting their patterns and interactions with the console. You manage to glean enough information to deduce a sequence of access codes.

*   Use the codes to attempt to open the maintenance tunnel. Turn to page 732.
''',
    "nGL-CYOA-BOOK1/page_732.txt": '''Using the codes you've acquired, you input them into the console. The system grants you access. A holographic map appears, highlighting a hidden maintenance tunnel leading away from the exhibit hall and towards an unknown area of the FLEXTANK.

*   Enter the maintenance tunnel. Turn to page 733.
''',
    "nGL-CYOA-BOOK1/page_733.txt": '''You enter the maintenance tunnel. It's dark and narrow, but you can hear the faint hum of machinery and see occasional maintenance lights. The tunnel seems to lead away from the main exhibit and towards the deeper systems of the FLEXTANK.

*   Proceed through the maintenance tunnel. Turn to page 734.
''',
    "nGL-CYOA-BOOK1/page_734.txt": '''You proceed through the maintenance tunnel. It twists and turns, eventually opening into a vast, cavernous space filled with immense forges and glowing vats of molten material. Steam billows from vents, illuminated by industrial bioluminescence. You've found your way to the Abyssal Forge District.

*   You have arrived in the Abyssal Forge. Turn to page 31.
'''
}

# List of files from directory listing, filtered for pages and outline
# Assuming 'outline.txt' is not to be renamed but its links need updating.
page_files_and_outline = [
    'outline.txt', # Special handling for outline.txt
    'page_01.txt', 'page_010.txt', 'page_011.txt', 'page_012.txt', 'page_013.txt', 'page_014.txt', 'page_015.txt', 'page_016.txt', 'page_017.txt', 'page_018.txt', 'page_019.txt', 'page_02.txt', 'page_020.txt', 'page_021.txt', 'page_022.txt', 'page_023.txt', 'page_024.txt', 'page_025.txt', 'page_026.txt', 'page_027.txt', 'page_028.txt', 'page_029.txt', 'page_03.txt', 'page_030.txt', 'page_031.txt', 'page_032.txt', 'page_033.txt', 'page_036.txt', 'page_04.txt', 'page_043.txt', 'page_044.txt', 'page_045.txt', 'page_046.txt', 'page_047.txt', 'page_048.txt', 'page_049.txt', 'page_05.txt', 'page_050.txt', 'page_055.txt', 'page_056.txt', 'page_057.txt', 'page_058.txt', 'page_059.txt', 'page_06.txt', 'page_060.txt', 'page_061.txt', 'page_062.txt', 'page_063.txt', 'page_065.txt', 'page_066.txt', 'page_067.txt', 'page_068.txt', 'page_069.txt', 'page_07.txt', 'page_070.txt', 'page_071.txt', 'page_0711.txt', 'page_0713.txt', 'page_0714.txt', 'page_0718.txt', 'page_072.txt', 'page_0721.txt', 'page_0722.txt', 'page_0723.txt', 'page_073.txt', 'page_074.txt', 'page_075.txt', 'page_076.txt', 'page_077.txt', 'page_078.txt', 'page_079.txt', 'page_08.txt', 'page_080.txt', 'page_081.txt', 'page_082.txt', 'page_083.txt', 'page_084.txt', 'page_085.txt', 'page_086.txt', 'page_087.txt', 'page_088.txt', 'page_089.txt', 'page_09.txt', 'page_090.txt', 'page_091.txt', 'page_092.txt', 'page_093.txt', 'page_094.txt', 'page_095.txt', 'page_096.txt', 'page_097.txt', 'page_098.txt', 'page_099.txt', 'page_100.txt', 'page_101.txt', 'page_102.txt', 'page_103.txt', 'page_104.txt', 'page_105.txt', 'page_106.txt', 'page_107.txt', 'page_108.txt', 'page_109.txt', 'page_110.txt', 'page_111.txt', 'page_112.txt', 'page_113.txt', 'page_114.txt', 'page_115.txt', 'page_116.txt', 'page_117.txt', 'page_118.txt', 'page_119.txt', 'page_120.txt', 'page_121.txt', 'page_122.txt', 'page_123.txt', 'page_124.txt', 'page_125.txt', 'page_700.txt', 'page_701.txt', 'page_702.txt', 'page_703.txt', 'page_704.txt', 'page_706.txt', 'page_707.txt', 'page_708.txt', 'page_709.txt', 'page_710.txt', 'page_712.txt', 'page_715.txt', 'page_716.txt', 'page_717.txt', 'page_719.txt', 'page_720.txt', 'page_724.txt', 'page_725.txt', 'page_726.txt', 'page_727.txt', 'page_728.txt', 'page_729.txt', 'page_730.txt', 'page_731.txt', 'page_732.txt', 'page_733.txt', 'page_734.txt'
]

# --- Step 1: Extract links and build the graph ---
story_graph = {} # {old_page_num_str: set(linked_old_page_num_strs)}
all_valid_page_nums = set() # Store all valid page numbers found in filenames
old_page_num_to_filename = {} # Map old page number string to its original filename

for filename in page_files_and_outline:
    full_path = os.path.join(BOOK_DIR, filename)
    # Ensure content is retrieved correctly, handling potential newlines and using triple quotes for multiline strings
    content = page_contents.get(full_path, "").strip() 

    match = FILENAME_PATTERN.match(filename)
    if match:
        old_page_num_str = match.group(1)
        all_valid_page_nums.add(old_page_num_str)
        old_page_num_to_filename[old_page_num_str] = filename
        
        current_page_links = set()
        # Use the more robust LINK_PATTERN
        for link_match in LINK_PATTERN.finditer(content):
            linked_page_num_str = link_match.group(2) # The number part
            current_page_links.add(linked_page_num_str)
        story_graph[old_page_num_str] = current_page_links
    elif filename == "outline.txt":
        # Special handling for outline.txt: extract links but don't treat it as a numbered page
        outline_links = set()
        for link_match in LINK_PATTERN.finditer(content):
            linked_page_num_str = link_match.group(2)
            outline_links.add(linked_page_num_str)
        story_graph["outline"] = outline_links # Use "outline" as a key

# --- Step 2: Determine the order and assign new page numbers ---
new_page_map = {} # {old_page_num_str: new_page_num_int}
visited_old_pages = set()
queue = deque()
new_page_counter = 1

# Start BFS from page_01.txt
start_page_filename = "page_01.txt"
start_old_page_num_str = None
for old_num_str, fname in old_page_num_to_filename.items():
    if fname == start_page_filename:
        start_old_page_num_str = old_num_str
        break

if start_old_page_num_str:
    queue.append(start_old_page_num_str)
    visited_old_pages.add(start_old_page_num_str)
    new_page_map[start_old_page_num_str] = new_page_counter
    new_page_counter += 1
else:
    print("Error: Starting page 'page_01.txt' not found or invalid.")
    # If page_01.txt is missing, we might need a different starting point or error out.

# Perform BFS to determine the order and new page numbers
while queue:
    current_old_page_num_str = queue.popleft()

    # Get links from the current page
    linked_pages = story_graph.get(current_old_page_num_str, set())

    # Sort linked pages numerically to maintain a somewhat consistent order
    sorted_linked_pages = sorted(list(linked_pages), key=lambda x: int(x) if x.isdigit() else float('inf'))

    for next_old_page_num_str in sorted_linked_pages:
        # Check if the linked page is a valid page in our collection
        if next_old_page_num_str in all_valid_page_nums:
            if next_old_page_num_str not in visited_old_pages:
                visited_old_pages.add(next_old_page_num_str)
                new_page_map[next_old_page_num_str] = new_page_counter
                new_page_counter += 1
                queue.append(next_old_page_num_str)
        # else: This link points to a page number not found in the files, ignore for remapping.

# --- Step 3: Handle pages not reached by BFS (e.g., orphaned branches) ---
# Sort remaining unvisited pages by their original number to ensure they are processed.
unvisited_pages = sorted([p for p in all_valid_page_nums if p not in visited_old_pages], key=lambda x: int(x) if x.isdigit() else float('inf'))

for old_page_num_str in unvisited_pages:
    if old_page_num_str not in new_page_map:
        new_page_map[old_page_num_str] = new_page_counter
        new_page_counter += 1

# Create a mapping from old page number string to new page number int for convenience.
old_page_num_to_new_page_num = new_page_map

# Create a mapping from old page number string to its new padded string format (e.g., "001", "010").
old_page_num_to_new_page_num_str_padded = {}
for old_num_str, new_num in old_page_num_to_new_page_num.items():
    old_page_num_to_new_page_num_str_padded[old_num_str] = f"{new_num:03d}"

# --- Step 4: Update content and prepare for renaming/writing ---

new_page_data = {} # {new_filename: new_content}
files_to_delete = [] # List of original files to delete

# Helper function to replace links in text
def replace_links_in_text(text, mapping_old_to_new_str_padded):
    def replacer(match):
        prefix = match.group(1) # "turn to page " or "Turn to page "
        old_num_str = match.group(2)
        period = match.group(3) # "." or ""

        if old_num_str in mapping_old_to_new_str_padded:
            new_num_str_padded = mapping_old_to_new_str_padded[old_num_str]
            return f"{prefix}{new_num_str_padded}{period}"
        else:
            # If the linked page number is not in our remapped pages, return original link.
            return match.group(0)
    
    # Apply the replacement using the robust pattern and replacer function
    return LINK_PATTERN.sub(replacer, text)

# Process outline.txt
if "outline.txt" in page_contents:
    outline_content = page_contents[os.path.join(BOOK_DIR, "outline.txt")]
    updated_outline_content = replace_links_in_text(outline_content, old_page_num_to_new_page_num_str_padded)
    new_page_data[os.path.join(BOOK_DIR, "outline.txt")] = updated_outline_content

# Process page files
for old_page_num_str, original_filename in old_page_num_to_filename.items():
    content = page_contents.get(os.path.join(BOOK_DIR, original_filename), "")
    
    # Generate new filename, padded to 3 digits
    new_page_num = old_page_num_to_new_page_num[old_page_num_str]
    new_filename = f"page_{new_page_num:03d}.txt" # e.g., page_001.txt, page_010.txt, page_100.txt
    new_full_path = os.path.join(BOOK_DIR, new_filename)
    
    updated_content = replace_links_in_text(content, old_page_num_to_new_page_num_str_padded)
    new_page_data[new_full_path] = updated_content
    
    # Mark original file for deletion
    files_to_delete.append(os.path.join(BOOK_DIR, original_filename))

# --- Step 5: Execute writes and deletions ---
write_operations = []
for file_path, content in new_page_data.items():
    write_operations.append(default_api.write_file(file_path=file_path, content=content))

# Wait for all writes to complete before deleting
if write_operations:
    # Execute write operations, then delete old files
    print(json.dumps(write_operations)) # This prints the operations for the interpreter
    # After writes are confirmed successful, proceed to delete

    # Construct the delete commands.
    # For safety, ensure we are only deleting files that were originally present.
    delete_commands = []
    for old_file_path in files_to_delete:
        # Use single quotes inside the f-string for the file path, then escape them for bash.
        # Or, use bash's ability to handle single-quoted strings.
        # Example: rm -f '$old_file_path'
        # To handle potential spaces or special characters in old_file_path robustly in bash:
        escaped_path = old_file_path.replace("'", "'''") # Escapes single quotes for bash
        delete_commands.append(f"rm -f '{escaped_path}'")
    
    if delete_commands:
        # Combine delete commands into a single shell command
        combined_delete_command = " && ".join(delete_commands)
        print(f"Executing deletion of old files: {combined_delete_command}")
        print(json.dumps([default_api.run_shell_command(command=combined_delete_command, description="Delete old CYOA page files after renaming.")]))
else:
    print("No new files to write. Nothing to do.")

# Cleanup old page_contents if needed, or just let it be garbage collected.
# The script itself is now defined. The execution of writes and deletions will follow.
