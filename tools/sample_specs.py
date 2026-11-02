# Instrument definitions for the "2000s / Windows XP visual novel" era.
# All recordings come from CC0 (public domain) libraries:
#   VCSL      https://github.com/sgossner/VCSL
#   VSCO-2-CE https://github.com/sgossner/VSCO-2-CE
# fields: key, repo, folder, regex (must capture the note name), spacing (semitones between zones),
#         rate (Hz), maxlen (s), loop (True for sustained sounds), note range (lo, hi)
SPECS = [
  dict(key="piano",    repo="vcsl", folder="Chordophones/Zithers/Grand Piano, Kawai", rx=r"^Sustains/GPiano_sus_([A-G]#?-?\d)_v3_rr1_Player\.wav$", spacing=4, rate=32000, maxlen=4.0, loop=False, rng=(28, 100)),
  dict(key="strings",  repo="vsco", folder="Strings/Violin Section/susVib", rx=r"_([A-G]#?\d)_v1(_rr1)?\.wav$", spacing=4, rate=24000, maxlen=2.6, loop=True, rng=(55, 100)),
  dict(key="viola",    repo="vsco", folder="Strings/Viola Section/susvib", rx=r"_([A-G]#?\d)_v1_1\.wav$", spacing=4, rate=24000, maxlen=2.6, loop=True, rng=(48, 76)),
  dict(key="cello",    repo="vsco", folder="Strings/Cello Section/susvib", rx=r"^susvib_([A-G]#?\d)_v1_1\.wav$", spacing=4, rate=24000, maxlen=2.6, loop=True, rng=(36, 64)),
  dict(key="pizz",     repo="vsco", folder="Strings/Violin Section/Pizz", rx=r"_([A-G]#?\d)_v2_rr1\.wav$", spacing=5, rate=24000, maxlen=1.2, loop=False, rng=(55, 100)),
  dict(key="cpizz",    repo="vsco", folder="Strings/Cello Section/pizzT", rx=r"_([A-G]#?\d)_v2_RR1\.wav$", spacing=5, rate=24000, maxlen=1.4, loop=False, rng=(36, 60)),
  dict(key="trem",     repo="vsco", folder="Strings/Violin Section/Trem", rx=r"_([A-G]#?\d)_v1(_rr1)?\.wav$", spacing=5, rate=24000, maxlen=2.4, loop=True, rng=(55, 100)),
  dict(key="ctrem",    repo="vsco", folder="Strings/Cello Section/trem", rx=r"^trem_([A-G]#?\d)_v1_1\.wav$", spacing=5, rate=24000, maxlen=2.4, loop=True, rng=(36, 64)),
  dict(key="harp",     repo="vcsl", folder="Chordophones/Composite Chordophones/Concert Harp", rx=r"^KSHarp_([A-G]#?\d)_mf1\.wav$", spacing=4, rate=32000, maxlen=3.0, loop=False, rng=(24, 103)),
  dict(key="flute",    repo="vsco", folder="Woodwinds/Flute/susvib", rx=r"_([A-G]#?\d)_v1(_1)?\.wav$", spacing=4, rate=24000, maxlen=2.4, loop=True, rng=(60, 96)),
  dict(key="clarinet", repo="vsco", folder="Woodwinds/Clarinet/susLong", rx=r"_([A-G]#?\d)_v1(_rr1)?_sum\.wav$", spacing=4, rate=24000, maxlen=2.4, loop=True, rng=(50, 91)),
  dict(key="oboe",     repo="vsco", folder="Woodwinds/Oboe/Vib", rx=r"_([A-G]#?\d)_v1(_rr1)?_Main\.wav$", spacing=4, rate=24000, maxlen=2.4, loop=True, rng=(58, 91)),
  dict(key="recorder", repo="vcsl", folder="Aerophones/Edge-blown Aerophones/Baroque Alto Recorder", rx=r"^(?:Sustain|Vibrato)[^/]*/[^/]*_([A-G]#?\d)_rr1_Main\.wav$", spacing=4, rate=24000, maxlen=2.0, loop=True, rng=(53, 86)),
  dict(key="ocarina",  repo="vcsl", folder="Aerophones/Edge-blown Aerophones/Ocarina, Typical", rx=r"^Sustains/Sus/StdOcarina_Sus_([A-G]#?\d)\.wav$", spacing=4, rate=24000, maxlen=2.0, loop=True, rng=(57, 77)),
  dict(key="glock",    repo="vcsl", folder="Idiophones/Struck Idiophones/Glockenspiel", rx=r"^glock_medium_([A-G]#?\d)_01\.wav$", spacing=5, rate=32000, maxlen=2.2, loop=False, rng=(79, 108)),
  dict(key="vibes",    repo="vcsl", folder="Idiophones/Struck Idiophones/Vibraphone", rx=r"^Soft Mallets/Vibes_soft_([A-G]#?\d)_v2_rr1_Main\.wav$", spacing=5, rate=32000, maxlen=3.0, loop=False, rng=(53, 89)),
  dict(key="bowed",    repo="vcsl", folder="Idiophones/Struck Idiophones/Vibraphone/Bowed", rx=r"^Vibes_bowed_([A-G]#?\d)_rr1_Main\.wav$", spacing=5, rate=24000, maxlen=3.0, loop=True, rng=(53, 89)),
  dict(key="chimes",   repo="vcsl", folder="Idiophones/Struck Idiophones/Hand Chimes", rx=r"^sus_([A-G]#?\d)_r01_main\.wav$", spacing=5, rate=32000, maxlen=3.0, loop=False, rng=(48, 96)),
  dict(key="kalimba",  repo="vcsl", folder="Idiophones/Plucked Idiophones/Kalimba, Tanzania", rx=r"_([A-G]#?\d)_k\d+_50_100_rr\d\.wav$", spacing=4, rate=32000, maxlen=2.0, loop=False, rng=(35, 84)),
  dict(key="zither",   repo="vcsl", folder="Chordophones/Zithers/Dan Tranh", rx=r"^Normal/([A-G]#?\d)_mf_1\.wav$", spacing=5, rate=32000, maxlen=2.5, loop=False, rng=(40, 90)),
  dict(key="harpsi",   repo="vcsl", folder="Chordophones/Zithers/Harpsichord, Flemish", rx=r"_([A-G]#?\d)_[^/]*rr1[^/]*\.wav$", spacing=5, rate=32000, maxlen=2.0, loop=False, rng=(29, 89)),
  # ---- rock / battle (pitch detected from the audio, names in these folders are unreliable)
  dict(key="trumpet",  repo="vsco", folder="Brass/Trumpet/sus", rx=r"_sus_([A-G]#?\d)_v3_rr1\.wav$", spacing=3, rate=24000, maxlen=2.4, loop=True, rng=(0, 127), detect=True),
  dict(key="horn",     repo="vsco", folder="Brass/F Horn/sus", rx=r"_sus_([A-G]#?\d)_v2_1\.wav$", spacing=4, rate=24000, maxlen=2.4, loop=True, rng=(0, 127), detect=True),
  dict(key="trombone", repo="vsco", folder="Brass/Tenor Trombone/sus", rx=r"_sus_([A-G]#?\d)_v2_1\.wav$", spacing=4, rate=24000, maxlen=2.4, loop=True, rng=(0, 127), detect=True),
  dict(key="tpstac",   repo="vsco", folder="Brass/Trumpet/stac", rx=r"_stac_([A-G]#?\d)_v3_rr1\.wav$", spacing=4, rate=24000, maxlen=0.8, loop=False, rng=(0, 127), detect=True),
  dict(key="tbstac",   repo="vsco", folder="Brass/Tenor Trombone/stac", rx=r"_stac_([A-G]#?\d)_v3_rr1\.wav$", spacing=5, rate=24000, maxlen=0.8, loop=False, rng=(0, 127), detect=True),
  dict(key="vspic",    repo="vsco", folder="Strings/Violin Section/Spic", rx=r"_([A-G]#?\d)_v2_rr1\.wav$", spacing=4, rate=24000, maxlen=0.6, loop=False, rng=(55, 100)),
  dict(key="cspic",    repo="vsco", folder="Strings/Cello Section/spic", rx=r"^spic_([A-G]#?\d)_v2_RR1\.wav$", spacing=5, rate=24000, maxlen=0.7, loop=False, rng=(36, 62)),
  # ---- real guitars / bass / drums for ROCK (CC0: Karoryfer Emily guitar, Black and Blue basses, Big Rusty Drums)
  dict(key="egtr",     repo="karoryfer.emilyguitar", folder="notes", rx=r"^([a-g]b?\d)_mf_rr1\.wav$", spacing=0, rate=32000, maxlen=3.5, loop=True, rng=(0, 127), detect=True),
  dict(key="egtrf",    repo="karoryfer.emilyguitar", folder="notes", rx=r"^([a-g]b?\d)_f_rr2\.wav$", spacing=0, rate=32000, maxlen=1.2, loop=False, rng=(0, 127), detect=True),
  dict(key="ebass",    repo="karoryfer.black-and-blue-basses", folder="Samples/darkblack/reg", rx=r"^darkblack_([a-g]b?\d)_mf_rr1\.wav$", spacing=3, rate=24000, maxlen=3.0, loop=True, rng=(0, 127), offset=-12),
  dict(key="etoms",    repo="karoryfer.big-rusty-drums", folder="Samples", rx=r"^tom_(22|18|15|14)/mallet/cl/t\d+_m_vl5_rr1\.flac$", spacing=0, rate=32000, maxlen=1.8, loop=False, rng=(0, 127), roots={"22": 36, "18": 41, "15": 46, "14": 50}),
  dict(key="taiko",    repo="vcsl", folder="Membranophones/Struck Membranophones/Bass Drum 2", rx=r"^bassdrum_hit_(ff|mf1)\.wav$", spacing=0, rate=24000, maxlen=2.2, loop=False, rng=(0, 127), roots={"ff": 36, "mf1": 30}),
  dict(key="timp",     repo="vcsl", folder="Membranophones/Struck Membranophones/Timpani 2/Hit", rx=r"^Timpani([1-8])A_hit_v3_rr1_main\.wav$", spacing=0, rate=24000, maxlen=2.5, loop=False, rng=(0, 127), detect=True, nonote=True),
  # ---- v8: school + horror (CC0, VCSL)
  dict(key="xylo",     repo="vcsl", folder="Idiophones/Struck Idiophones/Xylophone", rx=r"^Hard Mallets/Xylo_Hard_([A-G]#?\d)_ff_01_far\.wav$", spacing=0, rate=32000, maxlen=1.3, loop=False, rng=(0, 127)),
  dict(key="tbell",    repo="vcsl", folder="Idiophones/Struck Idiophones/Tubular Bells 2", rx=r"^TB_hit_([A-G]#?\d)_v4_\d\.wav$", spacing=0, rate=24000, maxlen=4.5, loop=False, rng=(0, 127)),
  dict(key="cymrev",   repo="vcsl", folder="Idiophones/Struck Idiophones/Suspended Cymbal 1", rx=r"^susCymb1_(hit_fff1)\.wav$", spacing=0, rate=24000, maxlen=3.2, loop=False, rng=(0, 127), roots={"hit_fff1": 60}),
]
# drum hits for the "Live (2000s)" kit: lane -> (repo, path regex)
DRUMS = [
  ("kick",  "vcsl", r"Bass Drum 1/BDrumNew_hit_v3_rr1_Sum\.wav$"),
  ("snare", "vcsl", r"Snare Drum, Modern 1/Snare2_HitNS_v4_rr1_Mid\.wav$"),
  ("chat",  "vcsl", r"Hi-Hat Cymbal/HiHat_HitC_v3_rr1_Mid\.wav$"),
  ("ohat",  "vcsl", r"Hi-Hat Cymbal/HiHat_HitO_rr1_Mid\.wav$"),
  ("clap",  "vcsl", r"Claps/Clap_rr1\.wav$"),
  ("toml",  "vcsl", r"Tom 2/Mallet/TomL_HitM_v3_rr1_Mid\.wav$"),
  ("tomh",  "vcsl", r"Tom 1/Mallet/TomH_HitM_v3_rr1_Mid\.wav$"),
  ("rim",   "vcsl", r"Claves/Claves1_Hit_v2_rr1_Mid\.wav$"),
  ("crash", "vcsl", r"Suspended Cymbal 1/susCymb1_hit_f1\.wav$"),
]
