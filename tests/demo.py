"""The attract demo, original vs C: LevelRestart with AX = 1 (the entry after a game over and
at startup) through the front end, whose attract timer the tests end at its wait loop (the
oracle at loc_8E7B, the C through pre2_test_poll), the intro scene, the world map, the level
start and every demo frame, until the demo ends and the program is back at the front end (or
a frame cap). The demo is the game's own input log at ds:003F (PollInputState plays back one
entry per call): the shipped one (KEYB.SQZ), the shipped one after the late-year screen
recorded over its head (the date bug, docs/completeness.md), and recordings made with the
native host (`pre2 --demo-out FILE`): files demos/*.p2d hold ds:003F..083E (2048 bytes: the
log, then the expert flag 083Dh and the start level 083Eh).
"""
import ctypes, re, sys
from difftest import Case
from common import ROOT
from emu import DATA_SEG, CODE_SEG
from unicorn import UC_HOOK_CODE
sys.path.insert(0, str(ROOT/'tests'))
import game as G

_sym = (ROOT/'c'/'pre2_sym.h').read_text()
def _ds(name): return int(re.search(rf'#define DS_{name} 0x([0-9A-F]+)u', _sym).group(1), 16)

def demo_setup(log=None, late_polls=0, cap=4000):
    def s(m):
        G.clear_stops(m)
        G.booted(m)
        if log is not None:
            for i, v in enumerate(log): m.wb(0x3F + i, v)
        if late_polls:
            # the late-year credits screen on a modern date, left by a key after late_polls polls
            m.ww(m.data('ClockYear'), 2026)
            st = {'n': 0}
            def poll(uc, a, size, user):
                st['n'] += 1
                m.wb(m.data('KeyDownTable') + 0x1C, 0xFF if st['n'] == late_polls else 0)
            hs = [m.uc.hook_add(UC_HOOK_CODE, poll, None, CODE_SEG * 16 + m.code(l), CODE_SEG * 16 + m.code(l))
                  for l in ('loc_0BC3', 'loc_0BCF')]
            import difftest
            difftest.arm_tick_model(m)
            m.call('ShowLateYearCredits', budget=4_000_000_000)
            difftest.disarm_tick_model(m)
            for h in hs: m.uc.hook_del(h)
            m.ports.log.clear()                      # the setup's own port traffic
        a = CODE_SEG * 16 + m.code('loc_8E7B')       # the attract timer has run out
        m._stop_hooks = getattr(m, '_stop_hooks', []) + [
            m.uc.hook_add(UC_HOOK_CODE, lambda uc, ad, sz, u: m.ww(m.data('word_27F0'), 0x10E), None, a, a)]
        G.stop_after(m, 'RunFrontEnd', 2)            # the demo ended: back at the front end
        G.stop_after(m, 'MainFrameLoop', cap + 1)    # or the frame cap
    return s

def demo_c(d):
    mem = d.pre2_memory().contents
    off = DATA_SEG * 16 + _ds('word_27F0')
    @ctypes.CFUNCTYPE(None)
    def poll():
        mem[off] = 0x0E; mem[off + 1] = 0x01
    ctypes.c_void_p.in_dll(d, 'pre2_test_poll').value = ctypes.cast(poll, ctypes.c_void_p).value
    return poll

def demo_case(title, log=None, late_polls=0, cap=4000):
    return Case(f'attract demo: {title}', 'LevelRestart', 'c_AttractDemo', regs={'AX': 1, 'CX': cap},
                setup=demo_setup(log, late_polls, cap), files=True, budget=(cap + 400) * 8_000_000,
                tick_model=True, port_inputs={0x201: 0xFF}, c_setup=demo_c)

CASES = [demo_case('the shipped demo (KEYB.SQZ, a pre-1996 date), to its end'),
         demo_case('the shipped demo after the late-year screen recorded 300 idle polls over its head',
                   late_polls=300, cap=900)]
for p in sorted((ROOT/'demos').glob('*.p2d')) if (ROOT/'demos').is_dir() else ():
    data = p.read_bytes()
    if len(data) == 0x800:
        CASES.append(demo_case(f'recording {p.name}', log=data))
