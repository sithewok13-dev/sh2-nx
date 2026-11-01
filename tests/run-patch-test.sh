#!/bin/sh
# The exe patches (tools/patch_exe.py) must not write over each other; checked without the game's exe
# (the widescreen text-position caves are found in the exe, so their fixed cave slots are added by hand).
set -e
cd "$(dirname "$0")/../tools"
python3 -c "
import patch_exe as p, ws_patches as w
caves = [(w.CAVE_WS + 8 * i, b'\xcc' * 4, b'', 'text position cave %d' % i) for i in range(len(w.TEXT_HOOKS))]
p.overlaps(p.PATCHES + caves)
print('patch test: %d patches, no overlaps' % (len(p.PATCHES) + len(caves)))"
