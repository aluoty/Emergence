# Emergence — Civilization Simulator

Build your civilization from a small settlement to a multiversal empire.
Manage 8 resources, research 14 technologies, construct 17 building types,
choose between 5 crop varieties and 4 hunting grounds, explore the stars,
and interact with alien civilizations.

## Building & Running

```bash
make           # build (requires raylib)
./build/emergence
make clean     # remove binary
```

## Keybindings

### Tab Switching
`Q` Overview | `W` Buildings | `E` Tech | `R` Civs | `A` Achievements

### Actions
`H` Hunt  | `F` Farm  | `G` Gift food  | `B` Build homes
`U` Research  | `T` Trade  | `Z` Rest  | `C` Heal
`M` Recruit  | `S` Scout  | `P` Train soldier
`I` Colonize  | `Y` Build starship  | `O` Explore galaxy
`J` Cycle crop  | `D` Cycle hunt zone
`K` Save  | `L` Load  | `N` New game

### Jobs
`1` Farmer  | `2` Woodcutter  | `3` Miner  | `4` Hunter  | `5` Scholar  | `6` Priest

### Civs Tab
`0-5` Select civ  | `T` Trade  | `W` War  | `A` Ally  | `V` Switch view

### Buildings Tab
`0-9` Build corresponding structure

### Tech Tab
`0-9` Research corresponding technology

## Resources

Food, Wood, Stone, Gold, Herbs, Tools, Knowledge, Faith

## Crop Types

`J` cycles through 5 crops. Each has different cost, grow time, and yield:

| Crop | Cost | Days | Yield | Bonus |
|------|------|------|-------|-------|
| Wheat | 5 | 3 | 8-14 | - |
| Corn | 8 | 4 | 14-24 | - |
| Rice | 7 | 3 | 10-18 | - |
| Potato | 4 | 3 | 6-12 | - |
| Herbs | 3 | 2 | 4-8 | +herbs on harvest |

## Hunting Zones

`D` cycles through 4 zones. Each has different danger and yield:

| Zone | Danger | Food | Herbs | Notes |
|------|--------|------|-------|-------|
| Plains | None | 4-10 | 5% | Safe |
| Forest | Low | 8-18 | 20% | Moderate |
| Mountains | Medium | 14-28 | 10% | Risky, big game |
| Swamp | High | 6-16 | 40% | Dangerous, rare herbs |

## Buildings (17 types)

Hut, Farm, Lumber Mill, Quarry, Market, Temple, Library, Barracks,
Wall, Workshop, Inn, Dock, **Forge**, **Granary**, **Sawmill**,
**University**, **Observatory**

## Technologies

14 technologies from Agriculture to Transcendence.

## Tiers

Planetary → Interplanetary → Interstellar → Universal → Multiversal

## Achievements

36 achievements tracking milestones of your civilization.

## Victory

Reach 100 population or survive 500 turns.
