import math, random, sys

def sphere_point(rng, R):
    while True:
        x,y,z = rng.gauss(0,1), rng.gauss(0,1), rng.gauss(0,1)
        n = math.sqrt(x*x+y*y+z*z)
        if n > 1e-9: break
    return tuple(int(round(R*c/n)) for c in (x,y,z))

def make(K, R, seed, m):
    """Each component is a triangle p, -p+d1, -p+d2 with d1 != d2 both drawn
    from [-m,m]^3 and UNIQUE ACROSS COMPONENTS.

    Why unique: the midpoint of the segment p -> (-p+d) is d/2, which does not
    depend on p at all. So two components sharing a d have long edges through
    the same point of 3-space -- they intersect exactly, and the fixture is not
    an embedding. With d from (+/-1)^3 there are only 8 choices and collisions
    are the norm, which is what the first attempt measured (spatial = 3)."""
    rng = random.Random(seed)
    used_v, used_d, comps = set(), set(), []
    guard = 0
    while len(comps) < K:
        guard += 1
        if guard > 200*K: return None
        p = sphere_point(rng, R)
        if p == (0,0,0): continue
        q = tuple(-c for c in p)
        d1 = tuple(rng.randint(-m,m) for _ in range(3))
        d2 = tuple(rng.randint(-m,m) for _ in range(3))
        if d1 == d2 or d1 in used_d or d2 in used_d: continue
        v = [p, tuple(q[i]+d1[i] for i in range(3)), tuple(q[i]+d2[i] for i in range(3))]
        if len(set(v)) != 3 or any(t in used_v for t in v): continue
        used_v.update(v); used_d.update((d1,d2)); comps.append(v)
    return comps

K, R, seed, m, out = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
comps = make(K,R,seed,m)
if comps is None: raise SystemExit("resampling failed")
open(out,"w").write("\n\n".join("\n".join(f"{a} {b} {c}" for a,b,c in c_) for c_ in comps) + "\n")
