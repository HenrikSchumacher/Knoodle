//! KLUT FST compression experiment (handoff/klut-fst-compression ROUND-1).
//!
//! Reads a pair file produced by export_pairs.py — repeated records of
//! [key_len bytes MacLeod key][4 bytes LE knot-type ID], sorted by key —
//! builds a minimal acyclic subsequential FST (fst::Map) for several key
//! serialization variants, and reports size + query time for each.

use fst::{Map, MapBuilder};
use std::collections::HashMap;
use std::time::Instant;

fn usage() -> ! {
    eprintln!("usage: fst_bench <pairs.bin> <key_len>");
    std::process::exit(2);
}

/// Deterministic shuffle (splitmix64-driven Fisher-Yates) so query order
/// defeats any locality from the sorted build order.
fn shuffle<T>(v: &mut [T], mut seed: u64) {
    let mut next = move || {
        seed = seed.wrapping_add(0x9e3779b97f4a7c15);
        let mut z = seed;
        z = (z ^ (z >> 30)).wrapping_mul(0xbf58476d1ce4e5b9);
        z = (z ^ (z >> 27)).wrapping_mul(0x94d049bb133111eb);
        z ^ (z >> 31)
    };
    for i in (1..v.len()).rev() {
        v.swap(i, (next() % (i as u64 + 1)) as usize);
    }
}

/// Pack symbols (dense-remapped to 0..alphabet) at `bits` bits each into a
/// big-endian bitstream. Big-endian preserves lexicographic order of the
/// underlying symbol sequence, so sorting packed keys sorts original keys.
fn bitpack(key: &[u8], remap: &HashMap<u8, u8>, bits: u32) -> Vec<u8> {
    let mut out = Vec::with_capacity((key.len() * bits as usize + 7) / 8);
    let mut acc: u64 = 0;
    let mut nbits: u32 = 0;
    for &b in key {
        acc = (acc << bits) | remap[&b] as u64;
        nbits += bits;
        while nbits >= 8 {
            out.push((acc >> (nbits - 8)) as u8);
            nbits -= 8;
        }
    }
    if nbits > 0 {
        out.push((acc << (8 - nbits)) as u8);
    }
    out
}

fn run_variant(name: &str, mut pairs: Vec<(Vec<u8>, u64)>) {
    pairs.sort_by(|a, b| a.0.cmp(&b.0));
    pairs.dedup_by(|a, b| a.0 == b.0); // paranoia; transforms are injective

    let t0 = Instant::now();
    let mut builder = MapBuilder::memory();
    for (k, v) in &pairs {
        builder.insert(k, *v).expect("insert failed (unsorted?)");
    }
    let bytes = builder.into_inner().expect("finish failed");
    let build_s = t0.elapsed().as_secs_f64();

    let map = Map::new(bytes).expect("invalid fst");
    let n = pairs.len();

    // correctness: every key maps to its ID
    for (k, v) in &pairs {
        assert_eq!(map.get(k), Some(*v), "wrong value in variant {name}");
    }
    // membership: last-byte perturbations must miss
    let mut misses = 0usize;
    for (k, _) in pairs.iter().take(10_000) {
        let mut bad = k.clone();
        bad[k.len() - 1] ^= 0xFF;
        if map.get(&bad).is_none() {
            misses += 1;
        }
    }

    // query benchmark: hit every key once in random order
    let mut order: Vec<usize> = (0..n).collect();
    shuffle(&mut order, 0x5eed);
    let t1 = Instant::now();
    let mut sink: u64 = 0;
    for &i in &order {
        sink ^= map.get(&pairs[i].0).unwrap();
    }
    let q_ns = t1.elapsed().as_nanos() as f64 / n as f64;
    std::hint::black_box(sink);

    println!(
        "{name:>10}  {size:>10} B  {bpk:>6.2} B/key  build {build_s:>5.1}s  \
         query {q_ns:>6.0} ns/key  perturbed-miss {misses}/10000",
        size = map.as_fst().as_bytes().len(),
        bpk = map.as_fst().as_bytes().len() as f64 / n as f64,
    );
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.len() != 3 {
        usage();
    }
    let blob = std::fs::read(&args[1]).expect("cannot read pairs file");
    let key_len: usize = args[2].parse().unwrap_or_else(|_| usage());
    let rec = key_len + 4;
    assert_eq!(blob.len() % rec, 0, "pair file size not a multiple of record");
    let n = blob.len() / rec;

    let mut pairs: Vec<(Vec<u8>, u64)> = Vec::with_capacity(n);
    let mut alphabet: Vec<u8> = Vec::new();
    for r in blob.chunks_exact(rec) {
        let key = r[..key_len].to_vec();
        let id = u32::from_le_bytes(r[key_len..].try_into().unwrap()) as u64;
        for &b in &key {
            if !alphabet.contains(&b) {
                alphabet.push(b);
            }
        }
        pairs.push((key, id));
    }
    alphabet.sort_unstable();
    let bits = (usize::BITS - (alphabet.len() - 1).leading_zeros()) as u32;
    let remap: HashMap<u8, u8> = alphabet
        .iter()
        .enumerate()
        .map(|(i, &b)| (b, i as u8))
        .collect();

    let types = pairs.iter().map(|p| p.1).max().unwrap() + 1;
    println!(
        "keys {n}, key_len {key_len}, types {types}, alphabet {} ({} bits/sym)",
        alphabet.len(),
        bits
    );
    println!(
        "raw keys {} B ({key_len} B/key); floor 1.23*log2(types) = {:.2} B/key",
        n * key_len,
        1.23 * (types as f64).log2() / 8.0
    );

    run_variant("forward", pairs.clone());

    let reversed: Vec<(Vec<u8>, u64)> = pairs
        .iter()
        .map(|(k, v)| (k.iter().rev().cloned().collect(), *v))
        .collect();
    run_variant("reversed", reversed);

    let packed: Vec<(Vec<u8>, u64)> = pairs
        .iter()
        .map(|(k, v)| (bitpack(k, &remap, bits), *v))
        .collect();
    run_variant("packed", packed);

    let packed_rev: Vec<(Vec<u8>, u64)> = pairs
        .iter()
        .map(|(k, v)| {
            let r: Vec<u8> = k.iter().rev().cloned().collect();
            (bitpack(&r, &remap, bits), *v)
        })
        .collect();
    run_variant("packed_rev", packed_rev);
}
