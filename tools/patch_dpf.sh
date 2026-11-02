#!/bin/sh
# Patches applied to DPF before building.
# 1) VST3 hosts send the typed character as UTF-16. Stock DPF drops every character >= 0x7f,
#    so Cyrillic (and any non-English) typing and Ctrl+V on a Russian layout never reach the UI.
set -e
F="${1:-dpf}/distrho/src/DistrhoUIVST3.cpp"
if grep -q 'keychar >= 0 && keychar < 0x7f' "$F"; then
    sed -i.bak 's/DISTRHO_SAFE_ASSERT_INT_RETURN(keychar >= 0 \&\& keychar < 0x7f, keychar, V3_FALSE);/DISTRHO_SAFE_ASSERT_INT_RETURN(keychar >= 0, keychar, V3_FALSE);/' "$F"
    rm -f "$F.bak"
    echo "patched $F (unicode keys)"
fi
grep -q 'keychar < 0x7f' "$F" && { echo "patch failed"; exit 1; } || true
