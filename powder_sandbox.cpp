#include <ncurses.h>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <climits>

// ===== Elements =====
enum class Element {
    EMPTY,
    // powders
    SAND, GUNPOWDER, ASH, SNOW,
    // liquids
    WATER, SALTWATER, OIL, ETHANOL, ACID, LAVA, MERCURY,
    // solids / terrain
    STONE, GLASS, WALL, WOOD, PLANT, METAL, WIRE, ICE, COAL,
    DIRT, WET_DIRT, SEAWEED,
    // gases
    SMOKE, STEAM, GAS, TOXIC_GAS, HYDROGEN, CHLORINE,
    // actors / special
    FIRE, LIGHTNING, HUMAN, ZOMBIE
};

struct Cell {
    Element type = Element::EMPTY;
    int life = 0;        // age / gas lifetime / charge / wetness / anim tick
};

static int gWidth = 0, gHeight = 0;
static std::vector<std::vector<Cell>> grid;

static std::mt19937 rng(
    (unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count()
);

static inline bool in_bounds(int x, int y){ return x>=0 && x<gWidth && y>=0 && y<gHeight; }
static inline int  rint(int a,int b){ std::uniform_int_distribution<int> d(a,b); return d(rng); }
static inline bool chance(int p){ std::uniform_int_distribution<int> d(1,100); return d(rng)<=p; }
static inline bool empty(const Cell& c){ return c.type==Element::EMPTY; }

// classification helpers
static inline bool sandlike(Element e){
    return e==Element::SAND || e==Element::GUNPOWDER || e==Element::ASH || e==Element::SNOW;
}
static inline bool liquid(Element e){
    switch(e){
        case Element::WATER: case Element::SALTWATER: case Element::OIL:
        case Element::ETHANOL: case Element::ACID: case Element::LAVA:
        case Element::MERCURY: return true;
        default: return false;
    }
}
static inline bool solid(Element e){
    switch(e){
        case Element::STONE: case Element::GLASS: case Element::WALL: case Element::WOOD:
        case Element::PLANT: case Element::METAL: case Element::WIRE: case Element::ICE:
        case Element::COAL: case Element::DIRT: case Element::WET_DIRT: case Element::SEAWEED:
            return true;
        default: return false;
    }
}
static inline bool gas(Element e){
    switch(e){
        case Element::SMOKE: case Element::STEAM: case Element::GAS:
        case Element::TOXIC_GAS: case Element::HYDROGEN: case Element::CHLORINE:
            return true;
        default: return false;
    }
}
static inline bool flammable(Element e){
    return e==Element::WOOD || e==Element::PLANT || e==Element::OIL ||
           e==Element::ETHANOL || e==Element::GUNPOWDER || e==Element::COAL ||
           e==Element::SEAWEED;
}
static inline bool conductor(Element e){
    return e==Element::METAL || e==Element::WIRE || e==Element::MERCURY || e==Element::SALTWATER;
}
static inline bool dissolvable(Element e){
    return e==Element::SAND || e==Element::STONE || e==Element::GLASS ||
           e==Element::WOOD || e==Element::PLANT || e==Element::METAL ||
           e==Element::WIRE || e==Element::ASH || e==Element::COAL ||
           e==Element::SEAWEED || e==Element::DIRT || e==Element::WET_DIRT;
}
// relative density for liquids
static inline int density(Element e){
    switch(e){
        case Element::ETHANOL: return 85;
        case Element::OIL:     return 90;
        case Element::GAS: case Element::HYDROGEN: return 1;
        case Element::STEAM:   return 2;
        case Element::SMOKE:   return 3;
        case Element::CHLORINE:return 5;
        case Element::WATER:   return 100;
        case Element::SALTWATER:return 103;
        case Element::ACID:    return 110;
        case Element::LAVA:    return 160;
        case Element::MERCURY: return 200;
        default: return 999;
    }
}

// harmful stuff for humans/zombies
static inline bool is_hazard(Element e){
    return e==Element::FIRE || e==Element::LAVA || e==Element::ACID ||
           e==Element::TOXIC_GAS || e==Element::CHLORINE || e==Element::LIGHTNING;
}

static inline std::string name_of(Element e){
    switch(e){
        case Element::EMPTY: return "Empty";
        case Element::SAND: return "Sand";
        case Element::GUNPOWDER: return "Gunpowder";
        case Element::ASH: return "Ash";
        case Element::SNOW: return "Snow";
        case Element::WATER: return "Water";
        case Element::SALTWATER: return "Salt Water";
        case Element::OIL: return "Oil";
        case Element::ETHANOL: return "Ethanol";
        case Element::ACID: return "Acid";
        case Element::LAVA: return "Lava";
        case Element::MERCURY: return "Mercury";
        case Element::STONE: return "Stone";
        case Element::GLASS: return "Glass";
        case Element::WALL: return "Wall";
        case Element::WOOD: return "Wood";
        case Element::PLANT: return "Plant";
        case Element::METAL: return "Metal";
        case Element::WIRE: return "Wire";
        case Element::ICE: return "Ice";
        case Element::COAL: return "Coal";
        case Element::DIRT: return "Dirt";
        case Element::WET_DIRT: return "Wet Dirt";
        case Element::SEAWEED: return "Seaweed";
        case Element::SMOKE: return "Smoke";
        case Element::STEAM: return "Steam";
        case Element::GAS: return "Gas";
        case Element::TOXIC_GAS: return "Toxic Gas";
        case Element::HYDROGEN: return "Hydrogen";
        case Element::CHLORINE: return "Chlorine";
        case Element::FIRE: return "Fire";
        case Element::LIGHTNING: return "Lightning";
        case Element::HUMAN: return "Human";
        case Element::ZOMBIE: return "Zombie";
    }
    return "?";
}

static inline short color_of(Element e){
    switch(e){
        case Element::EMPTY: return 1;
        // yellow-ish
        case Element::SAND:
        case Element::GUNPOWDER:
        case Element::SNOW:
        case Element::DIRT:
            return 2;
        // cyan water-ish
        case Element::WATER:
        case Element::SALTWATER:
        case Element::STEAM:
        case Element::ICE:
        case Element::ETHANOL:
            return 3;
        // white solids
        case Element::STONE:
        case Element::GLASS:
        case Element::WALL:
        case Element::METAL:
        case Element::WIRE:
        case Element::COAL:
        case Element::WET_DIRT:
            return 4;
        // green stuff & humans
        case Element::WOOD:
        case Element::PLANT:
        case Element::SEAWEED:
        case Element::HUMAN:
            return 5;
        // red danger
        case Element::FIRE:
        case Element::LAVA:
        case Element::ZOMBIE:
            return 6;
        // magenta haze
        case Element::SMOKE:
        case Element::ASH:
        case Element::GAS:
        case Element::HYDROGEN:
            return 7;
        // blue heavy liquids
        case Element::OIL:
        case Element::MERCURY:
            return 8;
        // green/yellow chem/bolt
        case Element::ACID:
        case Element::TOXIC_GAS:
        case Element::CHLORINE:
        case Element::LIGHTNING:
            return 9;
    }
    return 1;
}

static inline char glyph_of(Element e){
    switch(e){
        case Element::EMPTY: return ' ';
        case Element::SAND: return '.';
        case Element::GUNPOWDER: return '%';
        case Element::ASH: return ';';
        case Element::SNOW: return ',';
        case Element::WATER: return '~';
        case Element::SALTWATER: return ':';
        case Element::OIL: return 'o';
        case Element::ETHANOL: return 'e';
        case Element::ACID: return 'a';
        case Element::LAVA: return 'L';
        case Element::MERCURY: return 'm';
        case Element::STONE: return '#';
        case Element::GLASS: return '=';
        case Element::WALL: return '@';
        case Element::WOOD: return 'w';
        case Element::PLANT: return 'p';
        case Element::SEAWEED: return 'v';
        case Element::METAL: return 'M';
        case Element::WIRE: return '-';
        case Element::ICE: return 'I';
        case Element::COAL: return 'c';
        case Element::DIRT: return 'd';
        case Element::WET_DIRT: return 'D';
        case Element::SMOKE: return '^';
        case Element::STEAM: return '"';
        case Element::GAS: return '`';
        case Element::TOXIC_GAS: return 'x';
        case Element::HYDROGEN: return '\'';
        case Element::CHLORINE: return 'X';
        case Element::FIRE: return '*';
        case Element::LIGHTNING: return '|'; // bolt segment
        case Element::HUMAN: return 'Y';     // stick-ish guy
        case Element::ZOMBIE: return 'T';    // angry stick
    }
    return '?';
}

// ===== Grid =====
static void init_grid(int w,int h){
    gWidth=w; gHeight=h;
    grid.assign(gHeight, std::vector<Cell>(gWidth));
}
static void clear_grid(){
    for(int y=0;y<gHeight;++y)
        for(int x=0;x<gWidth;++x)
            grid[y][x]=Cell{};
}

// ===== Helpers =====
static void explode(int cx,int cy,int r){
    for(int dy=-r; dy<=r; ++dy){
        for(int dx=-r; dx<=r; ++dx){
            int x=cx+dx, y=cy+dy;
            if(!in_bounds(x,y)) continue;
            if(dx*dx+dy*dy>r*r) continue;
            Cell& c=grid[y][x];
            if(c.type==Element::WALL) continue;
            if(c.type==Element::STONE||c.type==Element::GLASS||
               c.type==Element::METAL||c.type==Element::WIRE||
               c.type==Element::ICE) continue;

            int roll=rint(1,100);
            if(roll<=50){ c.type=Element::FIRE; c.life=15+rint(0,10); }
            else if(roll<=80){ c.type=Element::SMOKE; c.life=20; }
            else { c.type=Element::GAS; c.life=20; }
        }
    }
}

static void place_brush(int cx,int cy,int rad, Element e){
    if(e==Element::LIGHTNING){
        // SPECIAL: lightning is a vertical yellow bolt striking DOWN to first surface
        if(!in_bounds(cx,cy)) return;
        int x=cx;
        int y=cy;
        // fall through air/gas until hitting non-air or bottom
        while(y+1<gHeight){
            Element below = grid[y+1][x].type;
            if(!empty(grid[y+1][x]) && !gas(below)) break;
            ++y;
        }
        for(int yy=cy; yy<=y; ++yy){
            Cell &c=grid[yy][x];
            c.type=Element::LIGHTNING;
            c.life=2; // short-lived
        }
        // if we hit water/saltwater below, electrify it
        if(y+1 < gHeight){
            Cell &below = grid[y+1][x];
            if(below.type==Element::WATER || below.type==Element::SALTWATER){
                below.life = std::max(below.life, 8);
            }
        }
        return;
    }

    for(int dy=-rad; dy<=rad; ++dy){
        for(int dx=-rad; dx<=rad; ++dx){
            int x=cx+dx, y=cy+dy;
            if(!in_bounds(x,y)) continue;
            if(dx*dx+dy*dy<=rad*rad){
                Cell &c=grid[y][x];
                c.type=e;
                c.life=0;
                if(gas(e)) c.life=25;
                if(e==Element::FIRE) c.life=20;
            }
        }
    }
}

// ===== Simulation =====
static void step_sim(){
    if(gWidth<=0||gHeight<=0) return;
    std::vector<std::vector<bool>> updated(gHeight, std::vector<bool>(gWidth,false));

    for(int y=gHeight-1; y>=0; --y){
        for(int x=0; x<gWidth; ++x){
            if(updated[y][x]) continue;
            Cell &cell = grid[y][x];
            Element t = cell.type;
            if(t==Element::EMPTY || t==Element::WALL){
                updated[y][x]=true;
                continue;
            }

            auto swap_to = [&](int nx,int ny){
                std::swap(grid[ny][nx], cell);
                updated[ny][nx]=true;
            };

            // --- powders ---
            if(sandlike(t)){
                bool moved=false;

                if(in_bounds(x,y+1)){
                    Cell &below=grid[y+1][x];
                    if(empty(below) || liquid(below.type)){
                        swap_to(x,y+1);
                        moved=true;
                    }
                }
                if(!moved){
                    int dir = rint(0,1)?1:-1;
                    for(int i=0;i<2 && !moved;++i){
                        int nx=x+(i?-dir:dir), ny=y+1;
                        if(!in_bounds(nx,ny)) continue;
                        Cell &d=grid[ny][nx];
                        if(empty(d) || liquid(d.type)){
                            swap_to(nx,ny);
                            moved=true;
                        }
                    }
                }
                if(!moved) updated[y][x]=true;

                // snow melts near heat
                if(t==Element::SNOW){
                    for(int dy=-1;dy<=1;++dy)
                        for(int dx=-1;dx<=1;++dx){
                            int nx=x+dx, ny=y+dy;
                            if(!in_bounds(nx,ny)) continue;
                            Element ne=grid[ny][nx].type;
                            if(ne==Element::FIRE || ne==Element::LAVA){
                                cell.type=Element::WATER;
                                cell.life=0;
                            }
                        }
                }

                // seaweed seed: sand under persistent water, spaced apart
                if(t==Element::SAND){
                    if(in_bounds(x,y-1) && grid[y-1][x].type==Element::WATER){
                        cell.life++;
                        if(cell.life>220){
                            bool nearbyWeed=false;
                            for(int wy=-2;wy<=2 && !nearbyWeed;++wy){
                                for(int wx=-2;wx<=2;++wx){
                                    int sx=x+wx, sy=y+wy;
                                    if(!in_bounds(sx,sy)) continue;
                                    if(grid[sy][sx].type==Element::SEAWEED){
                                        nearbyWeed=true;
                                        break;
                                    }
                                }
                            }
                            if(!nearbyWeed && in_bounds(x,y-1) && grid[y-1][x].type==Element::WATER){
                                grid[y-1][x].type=Element::SEAWEED;
                                grid[y-1][x].life=0;
                            }
                            cell.life=0;
                        }
                    }else{
                        cell.life=0;
                    }
                }

                continue;
            }

            // --- liquids ---
            if(liquid(t)){
                bool moved=false;

                if(in_bounds(x,y+1)){
                    Cell &b=grid[y+1][x];
                    if(empty(b) || gas(b.type)){
                        swap_to(x,y+1);
                        moved=true;
                    }else if(liquid(b.type) && density(t)>density(b.type)){
                        swap_to(x,y+1);
                        moved=true;
                    }
                }

                if(!moved){
                    int order[2]={-1,1};
                    if(rint(0,1)) std::swap(order[0],order[1]);
                    for(int i=0;i<2 && !moved;++i){
                        int nx=x+order[i];
                        if(!in_bounds(nx,y)) continue;
                        Cell &s=grid[y][nx];
                        if(empty(s) || gas(s.type)){
                            swap_to(nx,y);
                            moved=true;
                        }else if(liquid(s.type) && density(t)>density(s.type) && chance(50)){
                            swap_to(nx,y);
                            moved=true;
                        }
                    }
                }

                if(!moved) updated[y][x]=true;

                // interactions
                for(int dy=-1;dy<=1;++dy){
                    for(int dx=-1;dx<=1;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Cell &n=grid[ny][nx];

                        // water vs fire/lava
                        if(t==Element::WATER || t==Element::SALTWATER){
                            if(n.type==Element::FIRE){
                                n.type=Element::SMOKE;
                                n.life=15;
                            }else if(n.type==Element::LAVA){
                                n.type=Element::STONE;
                                n.life=0;
                                // sometimes big steam, sometimes fully cooled
                                if(chance(50)){
                                    cell.type=Element::STEAM;
                                    cell.life=20;
                                }else{
                                    cell.type=Element::STONE;
                                    cell.life=0;
                                }
                            }
                        }

                        // oil/ethanol ignite
                        if(t==Element::OIL || t==Element::ETHANOL){
                            if(n.type==Element::FIRE || n.type==Element::LAVA){
                                cell.type=Element::FIRE;
                                cell.life=25;
                            }
                        }

                        // acid eats stuff
                        if(t==Element::ACID){
                            if(dissolvable(n.type)){
                                if(chance(30)){
                                    n.type=Element::TOXIC_GAS;
                                    n.life=25;
                                }else{
                                    n.type=Element::EMPTY;
                                    n.life=0;
                                }
                                if(chance(25)){
                                    cell.type=Element::EMPTY;
                                    cell.life=0;
                                }
                            }
                            if(n.type==Element::WATER && chance(30)){
                                cell.type=Element::SALTWATER;
                                cell.life=0;
                                if(chance(30)){
                                    n.type=Element::STEAM;
                                    n.life=20;
                                }
                            }
                        }

                        // lava
                        if(t==Element::LAVA){
                            if(flammable(n.type)){
                                n.type=Element::FIRE;
                                n.life=25;
                            }else if(n.type==Element::SAND || n.type==Element::SNOW){
                                n.type=Element::GLASS;
                                n.life=0;
                            }else if(n.type==Element::WATER || n.type==Element::SALTWATER){
                                n.type=Element::STONE;
                                n.life=0;
                                if(chance(50)){
                                    cell.type=Element::STEAM;
                                    cell.life=20;
                                }else{
                                    cell.type=Element::STONE;
                                    cell.life=0;
                                }
                            }else if(n.type==Element::ICE){
                                n.type=Element::WATER;
                                n.life=0;
                            }
                        }
                    }
                }

                // lava cools
                if(t==Element::LAVA){
                    cell.life++;
                    if(cell.life>200){
                        cell.type=Element::STONE;
                        cell.life=0;
                    }
                }

                // hydrate dirt
                if(t==Element::WATER || t==Element::SALTWATER){
                    for(int dy=-1;dy<=1;++dy)
                        for(int dx=-1;dx<=1;++dx){
                            int nx=x+dx, ny=y+dy;
                            if(!in_bounds(nx,ny)) continue;
                            Cell &n=grid[ny][nx];
                            if(n.type==Element::DIRT || n.type==Element::WET_DIRT){
                                n.type=Element::WET_DIRT;
                                n.life=300;
                            }
                        }
                }

                // electrified water pulse (yellow, harmful)
                if((t==Element::WATER || t==Element::SALTWATER) && cell.life>0){
                    int q = cell.life;
                    for(int dy=-1;dy<=1;++dy){
                        for(int dx=-1;dx<=1;++dx){
                            if(!dx && !dy) continue;
                            int nx=x+dx, ny=y+dy;
                            if(!in_bounds(nx,ny)) continue;
                            Cell &n=grid[ny][nx];
                            if(n.type==Element::WATER || n.type==Element::SALTWATER){
                                if(n.life < q-1) n.life = q-1;
                            }
                            if(n.type==Element::HUMAN || n.type==Element::ZOMBIE){
                                n.type=Element::ASH;
                                n.life=0;
                            }
                        }
                    }
                    cell.life--;
                    if(cell.life<0) cell.life=0;
                }

                continue;
            }

            // --- gases ---
            if(gas(t)){
                bool moved=false;

                int tries = (t==Element::HYDROGEN ? 2 : 1);
                for(int i=0;i<tries && !moved;++i){
                    if(in_bounds(x,y-1) && empty(grid[y-1][x])){
                        swap_to(x,y-1);
                        moved=true;
                    }
                }

                if(!moved){
                    int order[2]={-1,1};
                    if(rint(0,1)) std::swap(order[0],order[1]);
                    for(int i=0;i<2 && !moved;++i){
                        int nx=x+order[i];
                        int ny=y-(chance(50)?1:0);
                        if(in_bounds(nx,ny) && empty(grid[ny][nx])){
                            swap_to(nx,ny);
                            moved=true;
                        }
                    }
                }

                if(t==Element::HYDROGEN || t==Element::GAS){
                    for(int dy=-1;dy<=1;++dy)
                        for(int dx=-1;dx<=1;++dx){
                            if(!dx && !dy) continue;
                            int nx=x+dx, ny=y+dy;
                            if(!in_bounds(nx,ny)) continue;
                            Element ne=grid[ny][nx].type;
                            if(ne==Element::FIRE || ne==Element::LAVA){
                                if(t==Element::HYDROGEN) explode(x,y,4);
                                else { cell.type=Element::FIRE; cell.life=12; }
                            }
                        }
                }
                if(t==Element::CHLORINE){
                    for(int dy=-1;dy<=1;++dy)
                        for(int dx=-1;dx<=1;++dx){
                            int nx=x+dx, ny=y+dy;
                            if(!in_bounds(nx,ny)) continue;
                            if(grid[ny][nx].type==Element::PLANT && chance(35)){
                                grid[ny][nx].type=Element::TOXIC_GAS;
                                grid[ny][nx].life=25;
                            }
                        }
                }

                cell.life--;
                if(cell.life<=0){
                    // much less water / ash generation
                    if(t==Element::STEAM && chance(15)){
                        cell.type=Element::WATER;
                        cell.life=0;
                    }else if(t==Element::SMOKE && chance(8)){
                        cell.type=Element::ASH;
                        cell.life=0;
                    }else{
                        cell.type=Element::EMPTY;
                        cell.life=0;
                    }
                }else{
                    if(!moved) updated[y][x]=true;
                }
                continue;
            }

            // --- fire ---
            if(t==Element::FIRE){
                // flicker upward
                if(in_bounds(x,y-1) && (empty(grid[y-1][x]) || gas(grid[y-1][x].type)) && chance(50)){
                    swap_to(x,y-1);
                }

                for(int dy=-1;dy<=1;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Cell &n=grid[ny][nx];

                        if(flammable(n.type) && chance(40)){
                            if(n.type==Element::GUNPOWDER) explode(nx,ny,5);
                            else { n.type=Element::FIRE; n.life=15+rint(0,10); }
                        }
                        if(n.type==Element::WATER || n.type==Element::SALTWATER){
                            cell.type=Element::SMOKE;
                            cell.life=15;
                        }
                        if(n.type==Element::WIRE || n.type==Element::METAL){
                            if(chance(5)) n.life=std::max(n.life,5);
                        }
                    }

                cell.life--;
                if(cell.life<=0){
                    cell.type=Element::SMOKE;
                    cell.life=15;
                }
                updated[y][x]=true;
                continue;
            }

            // --- lightning: charge & ignite, then vanish (no ash) ---
            if(t==Element::LIGHTNING){
                for(int dy=-2;dy<=2;++dy)
                    for(int dx=-2;dx<=2;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Cell &n=grid[ny][nx];
                        Element ne=n.type;
                        if(ne==Element::WIRE || ne==Element::METAL){
                            n.life=std::max(n.life,12);
                        }
                        if(ne==Element::WATER || ne==Element::SALTWATER){
                            n.life=std::max(n.life,8);
                        }
                        if(flammable(ne)){
                            if(ne==Element::GUNPOWDER) explode(nx,ny,6);
                            else { n.type=Element::FIRE; n.life=20+rint(0,10); }
                        }
                        if(ne==Element::HYDROGEN || ne==Element::GAS){
                            explode(nx,ny,4);
                        }
                    }
                cell.life--;
                if(cell.life<=0){
                    cell.type=Element::EMPTY;
                    cell.life=0;
                }
                updated[y][x]=true;
                continue;
            }

            // small helpers for creatures
            auto walk_try = [&](int tx,int ty)->bool{
                if(!in_bounds(tx,ty)) return false;
                Cell &d=grid[ty][tx];
                if(empty(d) || gas(d.type)){
                    std::swap(d,cell);
                    return true;
                }
                return false;
            };

            // --- HUMAN ---
            if(t==Element::HUMAN){
                // environmental hazards kill humans (including electrified water)
                bool killed=false;
                for(int dy=-1;dy<=1 && !killed;++dy){
                    for(int dx=-1;dx<=1 && !killed;++dx){
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Element ne=grid[ny][nx].type;
                        if(is_hazard(ne) ||
                           ((ne==Element::WATER || ne==Element::SALTWATER) && grid[ny][nx].life>0)){
                            cell.type=Element::ASH;
                            cell.life=0;
                            killed=true;
                        }
                    }
                }
                if(killed){
                    updated[y][x]=true;
                    continue;
                }

                cell.life++; // anim tick

                // gravity: only fall through air/gas (not liquids)
                if(in_bounds(x,y+1)){
                    Element b=grid[y+1][x].type;
                    if(empty(grid[y+1][x]) || gas(b)){
                        swap_to(x,y+1);
                        continue;
                    }
                }

                // look for nearest zombie
                int zx = 0, zy = 0;
                bool seen=false;
                for(int ry=-6; ry<=6 && !seen; ++ry){
                    for(int rx=-6; rx<=6; ++rx){
                        int nx=x+rx, ny=y+ry;
                        if(!in_bounds(nx,ny)) continue;
                        if(grid[ny][nx].type==Element::ZOMBIE){
                            zx=nx; zy=ny; seen=true; break;
                        }
                    }
                }

                // attack adjacent zombies
                for(int dy=-1;dy<=1;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        if(grid[ny][nx].type==Element::ZOMBIE && chance(35)){
                            if(chance(60)){
                                grid[ny][nx].type=Element::FIRE;
                                grid[ny][nx].life=10+rint(0,10);
                            }else{
                                grid[ny][nx].type=Element::ASH;
                                grid[ny][nx].life=0;
                            }
                        }
                    }

                int dir = rint(0,1)?1:-1;
                if(seen){
                    // run away
                    dir = (zx<x)?1:-1;
                }

                if(!walk_try(x+dir,y)){
                    // small jump over 1-tile obstacles
                    if(in_bounds(x+dir,y-1) && empty(grid[y-1][x+dir]) && empty(grid[y-1][x]) && chance(70)){
                        std::swap(grid[y-1][x], cell);
                    }else{
                        walk_try(x+(rint(0,1)?1:-1), y);
                    }
                }

                updated[y][x]=true;
                continue;
            }

            // --- ZOMBIE ---
            if(t==Element::ZOMBIE){
                // hazards kill/burn zombies too (including electrified water)
                for(int dy=-1;dy<=1;++dy){
                    for(int dx=-1;dx<=1;++dx){
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Element ne=grid[ny][nx].type;
                        if(is_hazard(ne) ||
                           ((ne==Element::WATER || ne==Element::SALTWATER) && grid[ny][nx].life>0)){
                            cell.type=Element::FIRE;
                            cell.life=15;
                        }
                    }
                }
                if(cell.type!=Element::ZOMBIE){
                    updated[y][x]=true;
                    continue;
                }

                cell.life++;

                // gravity: only air/gas
                if(in_bounds(x,y+1)){
                    Element b=grid[y+1][x].type;
                    if(empty(grid[y+1][x]) || gas(b)){
                        swap_to(x,y+1);
                        continue;
                    }
                }

                // look for human
                int hx=0, hy=0;
                bool seen=false;
                for(int ry=-6; ry<=6 && !seen; ++ry){
                    for(int rx=-6; rx<=6; ++rx){
                        int nx=x+rx, ny=y+ry;
                        if(!in_bounds(nx,ny)) continue;
                        if(grid[ny][nx].type==Element::HUMAN){
                            hx=nx; hy=ny; seen=true; break;
                        }
                    }
                }

                // infect/attack adjacent humans
                for(int dy=-1;dy<=1;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        if(grid[ny][nx].type==Element::HUMAN){
                            if(chance(70)){
                                grid[ny][nx].type=Element::ZOMBIE;
                                grid[ny][nx].life=0;
                            }else{
                                grid[ny][nx].type=Element::FIRE;
                                grid[ny][nx].life=10;
                            }
                        }
                    }

                int dir = seen ? ((hx>x)?1:-1) : (rint(0,1)?1:-1);
                if(!walk_try(x+dir,y)){
                    if(in_bounds(x+dir,y-1) && empty(grid[y-1][x+dir]) && empty(grid[y-1][x]) && chance(70)){
                        std::swap(grid[y-1][x], cell);
                    }else{
                        walk_try(x+(rint(0,1)?1:-1), y);
                    }
                }

                updated[y][x]=true;
                continue;
            }

            // --- wet dirt drying ---
            if(t==Element::WET_DIRT){
                bool nearWater=false;
                for(int dy=-1;dy<=1 && !nearWater;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Element ne=grid[ny][nx].type;
                        if(ne==Element::WATER || ne==Element::SALTWATER){
                            nearWater=true; break;
                        }
                    }
                if(!nearWater){
                    cell.life--;
                    if(cell.life<=0){
                        cell.type=Element::DIRT;
                        cell.life=0;
                    }
                }
                updated[y][x]=true;
                continue;
            }

            // --- plants & seaweed ---
            if(t==Element::PLANT || t==Element::SEAWEED){
                // burning
                for(int dy=-1;dy<=1;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        if(grid[ny][nx].type==Element::FIRE || grid[ny][nx].type==Element::LAVA){
                            cell.type=Element::FIRE;
                            cell.life=20;
                        }
                    }

                if(cell.type==Element::FIRE){
                    updated[y][x]=true;
                    continue;
                }

                if(t==Element::PLANT){
                    bool goodSoil = (in_bounds(x,y+1) && grid[y+1][x].type==Element::WET_DIRT);
                    // more controlled, mainly vertical growth
                    if(goodSoil && chance(2)){
                        int gx=x, gy=y-1;
                        if(in_bounds(gx,gy) && empty(grid[gy][gx])){
                            grid[gy][gx].type=Element::PLANT;
                            grid[gy][gx].life=0;
                        }
                    }
                }else{ // SEAWEED
                    bool underwater = in_bounds(x,y-1) &&
                        (grid[y-1][x].type==Element::WATER || grid[y-1][x].type==Element::SALTWATER);
                    bool isTop = !in_bounds(x,y-1) || grid[y-1][x].type!=Element::SEAWEED;
                    if(underwater && isTop && chance(2)){
                        int gy=y-1;
                        if(in_bounds(x,gy) &&
                           (grid[gy][x].type==Element::WATER || grid[gy][x].type==Element::SALTWATER)){
                            grid[gy][x].type=Element::SEAWEED;
                            grid[gy][x].life=0;
                        }
                    }
                }
                updated[y][x]=true;
                continue;
            }

            // --- wood/coal burn ---
            if(t==Element::WOOD || t==Element::COAL){
                for(int dy=-1;dy<=1;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        if(grid[ny][nx].type==Element::FIRE || grid[ny][nx].type==Element::LAVA){
                            cell.type=Element::FIRE;
                            cell.life = (t==Element::COAL ? 35 : 25);
                        }
                    }
                updated[y][x]=true;
                continue;
            }

            // --- gunpowder ---
            if(t==Element::GUNPOWDER){
                for(int dy=-1;dy<=1;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        if(!dx && !dy) continue;
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Element ne=grid[ny][nx].type;
                        if(ne==Element::FIRE || ne==Element::LAVA){
                            explode(x,y,5);
                            break;
                        }
                    }
                updated[y][x]=true;
                continue;
            }

            // --- wire / metal conduction ---
            if(t==Element::WIRE || t==Element::METAL){
                if(cell.life>0){
                    int q=cell.life;
                    for(int dy=-1;dy<=1;++dy)
                        for(int dx=-1;dx<=1;++dx){
                            if(!dx && !dy) continue;
                            int nx=x+dx, ny=y+dy;
                            if(!in_bounds(nx,ny)) continue;
                            Cell &n=grid[ny][nx];
                            if(n.type==Element::WIRE || n.type==Element::METAL){
                                if(n.life<q-1) n.life=q-1;
                            }
                            // wire can shock water too
                            if(n.type==Element::WATER || n.type==Element::SALTWATER){
                                if(n.life<q-1) n.life=q-1;
                            }
                            if(flammable(n.type) && chance(15)){
                                if(n.type==Element::GUNPOWDER) explode(nx,ny,5);
                                else { n.type=Element::FIRE; n.life=15+rint(0,10); }
                            }
                            if(n.type==Element::HYDROGEN || n.type==Element::GAS){
                                if(chance(35)) explode(nx,ny,4);
                            }
                        }
                    cell.life--;
                    if(cell.life<0) cell.life=0;
                }
                updated[y][x]=true;
                continue;
            }

            // --- ice ---
            if(t==Element::ICE){
                for(int dy=-1;dy<=1;++dy)
                    for(int dx=-1;dx<=1;++dx){
                        int nx=x+dx, ny=y+dy;
                        if(!in_bounds(nx,ny)) continue;
                        Element ne=grid[ny][nx].type;
                        if(ne==Element::FIRE || ne==Element::LAVA || ne==Element::STEAM){
                            if(chance(25)){
                                cell.type=Element::WATER;
                                cell.life=0;
                            }
                        }
                    }
                updated[y][x]=true;
                continue;
            }

            // default static
            updated[y][x]=true;
        }
    }
}

// ===== Drawing =====
static void draw_grid(int cx,int cy, Element cur, bool paused, int brush){
    for(int y=0;y<gHeight;++y){
        for(int x=0;x<gWidth;++x){
            Cell &c=grid[y][x];
            char ch = glyph_of(c.type);

            // little "animations" / stick vibes
            if(c.type==Element::HUMAN)  ch = (c.life/6)%2 ? 'y' : 'Y';
            if(c.type==Element::ZOMBIE) ch = (c.life/6)%2 ? 't' : 'T';
            if(c.type==Element::LIGHTNING) ch='|'; // straight yellow bolt

            short col = color_of(c.type);
            // electrified water pulse = yellow
            if((c.type==Element::WATER || c.type==Element::SALTWATER) && c.life>0){
                col = 9;
            }

            if(has_colors()) attron(COLOR_PAIR(col));
            mvaddch(y,x,ch);
            if(has_colors()) attroff(COLOR_PAIR(col));
        }
    }

    if(in_bounds(cx,cy)) mvaddch(cy,cx,'+');

    int maxy,maxx; getmaxyx(stdscr,maxy,maxx);
    if(gHeight<maxy) mvhline(gHeight,0,'-',maxx);

    std::string status =
        "Move: Arrows/WASD | Space: draw | E: erase | +/-: brush | C/X: clear | "
        "P: pause | M/Tab: elements | Q: quit";
    if((int)status.size()>maxx) status.resize(maxx);
    if(gHeight+1<maxy) mvaddnstr(gHeight+1,0,status.c_str(),maxx);

    std::string info =
        "Current: "+name_of(cur)+
        " | Brush r="+std::to_string(brush)+
        (paused?" [PAUSED]":"");
    if((int)info.size()>maxx) info.resize(maxx);
    if(gHeight+2<maxy) mvaddnstr(gHeight+2,0,info.c_str(),maxx);
}

// ===== Element Browser & Credits =====
enum class Category { POWDERS, LIQUIDS, SOLIDS, GASES, SPECIAL, CREDITS };
struct MenuItem { Element type; Category cat; const char* label; const char* desc; };

static const std::vector<MenuItem> MENU = {
    // Powders
    {Element::SAND,Category::POWDERS,"Sand","Classic falling grains."},
    {Element::GUNPOWDER,Category::POWDERS,"Gunpowder","Explodes when ignited."},
    {Element::ASH,Category::POWDERS,"Ash","Burnt residue."},
    {Element::SNOW,Category::POWDERS,"Snow","Melts near heat."},

    // Liquids
    {Element::WATER,Category::LIQUIDS,"Water","Flows, cools, extinguishes."},
    {Element::SALTWATER,Category::LIQUIDS,"Salt Water","Conductive water."},
    {Element::OIL,Category::LIQUIDS,"Oil","Light, flammable."},
    {Element::ETHANOL,Category::LIQUIDS,"Ethanol","Very flammable."},
    {Element::ACID,Category::LIQUIDS,"Acid","Dissolves many materials."},
    {Element::LAVA,Category::LIQUIDS,"Lava","Hot molten rock."},
    {Element::MERCURY,Category::LIQUIDS,"Mercury","Heavy liquid metal."},

    // Solids
    {Element::STONE,Category::SOLIDS,"Stone","Heavy solid block."},
    {Element::GLASS,Category::SOLIDS,"Glass","From sand + lava."},
    {Element::WALL,Category::SOLIDS,"Wall","Indestructible barrier."},
    {Element::WOOD,Category::SOLIDS,"Wood","Flammable solid."},
    {Element::PLANT,Category::SOLIDS,"Plant","Grows on wet dirt."},
    {Element::SEAWEED,Category::SOLIDS,"Seaweed","Grows in water over sand."},
    {Element::METAL,Category::SOLIDS,"Metal","Conductive solid."},
    {Element::WIRE,Category::SOLIDS,"Wire","Conductive path."},
    {Element::ICE,Category::SOLIDS,"Ice","Melts into water."},
    {Element::COAL,Category::SOLIDS,"Coal","Burns longer."},
    {Element::DIRT,Category::SOLIDS,"Dirt","Gets wet; grows plants."},
    {Element::WET_DIRT,Category::SOLIDS,"Wet Dirt","Dries over time."},

    // Gases
    {Element::SMOKE,Category::GASES,"Smoke","Rises; may fall as ash."},
    {Element::STEAM,Category::GASES,"Steam","Condenses to water."},
    {Element::GAS,Category::GASES,"Gas","Neutral rising gas."},
    {Element::TOXIC_GAS,Category::GASES,"Toxic Gas","Nasty chemical cloud."},
    {Element::HYDROGEN,Category::GASES,"Hydrogen","Very light, explosive."},
    {Element::CHLORINE,Category::GASES,"Chlorine","Harms plants."},

    // Special
    {Element::FIRE,Category::SPECIAL,"Fire","Burns & flickers upward."},
    {Element::LIGHTNING,Category::SPECIAL,"Lightning","Yellow electrical bolt."},
    {Element::HUMAN,Category::SPECIAL,"Human","Avoids zombie, fights back."},
    {Element::ZOMBIE,Category::SPECIAL,"Zombie","Chases and infects humans."},
    {Element::EMPTY,Category::SPECIAL,"Eraser","Place empty space."},

    // Credits tab
    {Element::EMPTY,Category::CREDITS,"Credits","Show credits & license."}
};

static const char* cat_name(Category c){
    switch(c){
        case Category::POWDERS: return "Powders";
        case Category::LIQUIDS: return "Liquids";
        case Category::SOLIDS:  return "Solids";
        case Category::GASES:   return "Gases";
        case Category::SPECIAL: return "Special";
        case Category::CREDITS: return "Credits";
    }
    return "?";
}

// Use a separate ncurses window for credits to avoid flicker / messing stdscr
static void show_credits_overlay(){
    int maxy,maxx; getmaxyx(stdscr,maxy,maxx);
    if(maxx<40 || maxy<12) return;

    int w = std::min(maxx-4, 70);
    int h = std::min(maxy-4, 15);
    int ty=(maxy-h)/2;
    int lx=(maxx-w)/2;

    WINDOW* win = newwin(h,w,ty,lx);
    if(!win) return;

    box(win,0,0);
    std::string title=" Credits ";
    mvwaddnstr(win, 0, (w-(int)title.size())/2, title.c_str(), w-2);

    const char* lines[] = {
        "Terminal Powder Toy-like Sandbox",
        "Author: Robert",
        "GitHub: https://github.com/RobertFlexx",
        "Language: C++17 + ncurses",
        "",
        "BSD 3-Clause License (snippet):",
        "Redistribution and use in source and binary forms,",
        "with or without modification, are permitted provided",
        "that the following conditions are met:",
        "1) Source redistributions retain this notice & disclaimer.",
        "2) Binary redistributions reproduce this notice & disclaimer.",
        "3) Names of contributors can't be used to endorse products",
        "   derived from this software without permission.",
        "",
        "Press any key to return."
    };
    int num = (int)(sizeof(lines)/sizeof(lines[0]));
    int y = 2;
    for(int i=0; i<num && y<h-1; ++i,++y){
        mvwaddnstr(win, y, 2, lines[i], w-4);
    }

    wrefresh(win);
    flushinp();
    wgetch(win);
    delwin(win);
}

static Element element_menu(Element current){
    Category tabs[] = {Category::POWDERS,Category::LIQUIDS,Category::SOLIDS,
                       Category::GASES,Category::SPECIAL,Category::CREDITS};
    const int NT = 6;

    Category curTab = Category::POWDERS;
    for(const auto& it:MENU) if(it.type==current){ curTab=it.cat; break; }

    int tabIdx=0;
    for(int i=0;i<NT;++i) if(tabs[i]==curTab){ tabIdx=i; break; }

    int sel=0;
    bool done=false;
    Element result=current;

    while(!done){
        int maxy,maxx; getmaxyx(stdscr,maxy,maxx);
        std::vector<int> idx;
        for(size_t i=0;i<MENU.size();++i)
            if(MENU[i].cat==tabs[tabIdx]) idx.push_back((int)i);

        if(sel<0) sel=0;
        if(sel>=(int)idx.size()) sel=(int)idx.size()-1;

        int boxW=std::max(44,maxx-6);
        int boxH=std::max(14,maxy-6);
        boxW=std::min(boxW,maxx);
        boxH=std::min(boxH,maxy);
        int lx=(maxx-boxW)/2;
        int ty=(maxy-boxH)/2;
        int rx=lx+boxW-1;
        int by=ty+boxH-1;

        clear();
        mvaddch(ty,lx,'+'); mvaddch(ty,rx,'+');
        mvaddch(by,lx,'+'); mvaddch(by,rx,'+');
        for(int x=lx+1;x<rx;++x){ mvaddch(ty,x,'-'); mvaddch(by,x,'-'); }
        for(int y=ty+1;y<by;++y){ mvaddch(y,lx,'|'); mvaddch(y,rx,'|'); }

        std::string title=" Element Browser ";
        mvaddnstr(ty, lx+(boxW-(int)title.size())/2, title.c_str(), boxW-2);

        int tabsY=ty+1;
        int cx=lx+2;
        for(int i=0;i<NT;++i){
            std::string tab=" ";
            tab+=cat_name(tabs[i]);
            tab+=" ";
            if(cx+(int)tab.size()>=rx) break;
            if(i==tabIdx) attron(A_REVERSE);
            mvaddnstr(tabsY,cx,tab.c_str(),rx-cx-1);
            if(i==tabIdx) attroff(A_REVERSE);
            cx+=(int)tab.size()+1;
        }

        int y=ty+3;
        int maxListY=by-3;
        for(int i=0;i<(int)idx.size() && y<=maxListY; ++i,++y){
            const auto &it = MENU[idx[i]];
            std::string line=" ";
            line+=it.label;
            line+=" - ";
            line+=it.desc;
            if((int)line.size()>boxW-4) line.resize(boxW-4);
            if(i==sel) attron(A_REVERSE);
            mvaddnstr(y,lx+2,line.c_str(),boxW-4);
            if(i==sel) attroff(A_REVERSE);
        }

        std::string hint="Left/Right: tabs | Up/Down: select | Enter: choose | ESC: back";
        mvaddnstr(by-1,lx+2,hint.c_str(),boxW-4);
        refresh();

        int ch=getch();
        if(ch==KEY_LEFT){
            tabIdx=(tabIdx+NT-1)%NT; sel=0;
        }else if(ch==KEY_RIGHT){
            tabIdx=(tabIdx+1)%NT; sel=0;
        }else if(ch==KEY_UP){
            if(!idx.empty()) sel = (sel + (int)idx.size() - 1) % (int)idx.size();
        }else if(ch==KEY_DOWN){
            if(!idx.empty()) sel = (sel + 1) % (int)idx.size();
        }else if(ch=='\n' || ch=='\r' || ch==KEY_ENTER){
            if(!idx.empty()){
                const auto &it = MENU[idx[sel]];
                if(it.cat==Category::CREDITS){
                    show_credits_overlay();   // no flicker, uses its own window
                }else{
                    result=it.type;
                    done=true;
                }
            }else{
                done=true;
            }
        }else if(ch==27){
            done=true;
        }
    }
    return result;
}

// ===== Main =====
int main(){
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr,TRUE);
    nodelay(stdscr,TRUE);

    int termH,termW; getmaxyx(stdscr,termH,termW);
    int simH = std::max(1, termH-3);
    init_grid(termW,simH);

    if(has_colors()){
        start_color();
        use_default_colors();
        init_pair(1, COLOR_BLACK,   -1);
        init_pair(2, COLOR_YELLOW,  -1); // sand/dirt/etc
        init_pair(3, COLOR_CYAN,    -1); // water-ish
        init_pair(4, COLOR_WHITE,   -1); // neutral solids
        init_pair(5, COLOR_GREEN,   -1); // plants/humans
        init_pair(6, COLOR_RED,     -1); // fire/lava/zombies
        init_pair(7, COLOR_MAGENTA, -1); // smoke/gas/ash
        init_pair(8, COLOR_BLUE,    -1); // oil/mercury
        init_pair(9, COLOR_YELLOW,  -1); // lightning/acid/etc
    }

    int cx=gWidth/2, cy=gHeight/2;
    int brush=1;
    Element current=Element::SAND;
    bool running=true, paused=false;

    while(running){
        // handle resize
        int nh,nw; getmaxyx(stdscr,nh,nw);
        int nSimH = std::max(1, nh-3);
        if(nw!=gWidth || nSimH!=gHeight){
            init_grid(nw,nSimH);
            cx=std::clamp(cx,0,gWidth-1);
            cy=std::clamp(cy,0,gHeight-1);
        }

        int ch;
        while((ch=getch())!=ERR){
            if(ch=='q'||ch=='Q'){
                running=false;
            }else if(ch==KEY_LEFT || ch=='a' || ch=='A'){
                cx = std::max(0,cx-1);
            }else if(ch==KEY_RIGHT || ch=='d' || ch=='D'){
                cx = std::min(gWidth-1,cx+1);
            }else if(ch==KEY_UP || ch=='w'){
                cy = std::max(0,cy-1);
            }else if(ch==KEY_DOWN || ch=='s' || ch=='S'){
                cy = std::min(gHeight-1,cy+1);
            }else if(ch==' '){
                place_brush(cx,cy,brush,current);
            }else if(ch=='e' || ch=='E'){
                place_brush(cx,cy,brush,Element::EMPTY);
            }else if(ch=='+' || ch=='='){
                if(brush<8) ++brush;
            }else if(ch=='-' || ch=='_'){
                if(brush>1) --brush;
            }else if(ch=='c' || ch=='C' || ch=='x' || ch=='X'){
                clear_grid();
            }else if(ch=='p' || ch=='P'){
                paused=!paused;
            }else if(ch=='m' || ch=='M' || ch=='\t'){
                flushinp();
                nodelay(stdscr,FALSE);
                current = element_menu(current);
                nodelay(stdscr,TRUE);
            }else if(ch=='1'){ current=Element::SAND; }
            else if(ch=='2'){ current=Element::WATER; }
            else if(ch=='3'){ current=Element::STONE; }
            else if(ch=='4'){ current=Element::WOOD; }
            else if(ch=='5'){ current=Element::FIRE; }
            else if(ch=='6'){ current=Element::OIL; }
            else if(ch=='7'){ current=Element::LAVA; }
            else if(ch=='8'){ current=Element::PLANT; }
            else if(ch=='9'){ current=Element::GUNPOWDER; }
            else if(ch=='0'){ current=Element::ACID; }
            else if(ch=='W'){ current=Element::WALL; }
            else if(ch=='L'){ current=Element::LIGHTNING; }
            else if(ch=='H' || ch=='h'){ current=Element::HUMAN; }
            else if(ch=='Z'){ current=Element::ZOMBIE; }
            else if(ch=='D'){ current=Element::DIRT; }
        }

        if(!paused) step_sim();

        erase();
        draw_grid(cx,cy,current,paused,brush);
        refresh();
        napms(16);
    }

    endwin();
    return 0;
}
