#!/usr/bin/env bash
# RyzenAdj system diagnostic - run as root: sudo ./examples/diagnose.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RYZENADJ="${ROOT}/build/ryzenadj"

source_newer_than_binary() {
    find "$ROOT" \
        -path "$ROOT/.git" -prune -o \
        -path "$ROOT/build" -prune -o \
        -path "$ROOT/build-*" -prune -o \
        -type f \( -name '*.c' -o -name '*.h' -o -name 'CMakeLists.txt' \) \
        -newer "$RYZENADJ" -print -quit | grep -q .
}

echo "=== RyzenAdj diagnostic ==="
echo "Date: $(date)"
echo

echo "--- CPU ---"
LC_ALL=C lscpu | grep -E 'Model name|Vendor|CPU\(s\)|Family|Model:' || true
echo

echo "--- Permissions ---"
id
echo "root required: ryzen_smu sysfs writes and /dev/mem are restricted to root"
echo

echo "--- ryzen_smu kernel module ---"
if [[ -d /sys/kernel/ryzen_smu_drv ]]; then
    for f in drv_version version codename mp1_if_version pm_table_version pm_table_size; do
        path="/sys/kernel/ryzen_smu_drv/$f"
        if [[ -e "$path" ]]; then
            if [[ "$f" == pm_table_version ]]; then
                printf "%s: 0x%s\n" "$f" "$(od -An -tx4 -N4 "$path" | tr -d ' ')"
            elif [[ "$f" == pm_table_size ]]; then
                printf "%s: %u bytes\n" "$f" "$(od -An -tu8 -N8 "$path" | tr -d ' ')"
            else
                printf "%s: %s\n" "$f" "$(cat "$path")"
            fi
        else
            echo "$f: (missing)"
        fi
    done
    ls -la /sys/kernel/ryzen_smu_drv/smn /sys/kernel/ryzen_smu_drv/pm_table 2>/dev/null || true
else
    echo "ryzen_smu_drv sysfs not found"
fi
echo

echo "--- /dev/mem ---"
ls -la /dev/mem 2>/dev/null || echo "/dev/mem not available"
echo

echo "--- CPU hwmon ---"
found_cpu_hwmon=0
for hwmon in /sys/class/hwmon/hwmon*; do
    [[ -e "$hwmon/name" ]] || continue
    hwmon_name="$(cat "$hwmon/name")"
    case "$hwmon_name" in
    k10temp|zenpower)
        found_cpu_hwmon=1
        echo "$hwmon_name: $hwmon"
        for input in "$hwmon"/temp*_input; do
            [[ -e "$input" ]] || continue
            label="$(basename "$input" _input)"
            label_path="${input%_input}_label"
            [[ -e "$label_path" ]] && label="$(cat "$label_path")"
            raw_temp="$(cat "$input")"
            if [[ "$raw_temp" =~ ^-?[0-9]+$ ]]; then
                printf "  %s: %d.%03d C\n" "$label" "$((raw_temp / 1000))" "$((raw_temp % 1000))"
            else
                printf "  %s: %s mC\n" "$label" "$raw_temp"
            fi
        done
        ;;
    esac
done
if [[ "$found_cpu_hwmon" -eq 0 ]]; then
    echo "k10temp/zenpower hwmon not found"
fi
echo

if [[ ! -x "$RYZENADJ" ]] || source_newer_than_binary; then
    echo "Building ryzenadj..."
    cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release -DSTRIP_X86_ISA_NOTE=ON
    cmake --build "$ROOT/build" -j"$(nproc)"
fi

echo "--- ryzenadj --info ---"
"$RYZENADJ" --info 2>&1 || true
echo

echo "--- ryzenadj --dump-table (first 20 rows) ---"
"$RYZENADJ" --dump-table 2>/dev/null | head -25 || true
echo

echo "--- New monitoring (branch) ---"
echo "  GFX POWER, FreqEff, C0/CC1/C6% per core (Strix Point / desktop)"
echo "  --oc-clk-per-core for Renoir/Cezanne/Rembrandt"
echo

echo "--- Hidden presets ---"
echo "power-saving / max-performance map to ACPI ALIB CCLK boost setpoint:"
echo "  Renoir/Lucienne/Cezanne and newer APUs: ALIB 0x12 (power-saving) / 0x11 (max-performance)"
echo "  Raven/Picasso: ALIB 0x19 / 0x18"
echo

echo "=== Done ==="
