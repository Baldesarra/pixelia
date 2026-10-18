"""Serielles Log des ESP32 mitlesen, ohne Interaktion.

- Öffnet den Port ohne Reset (DTR/RTS aus), optional mit Reset über RTS (--reset).
- Schickt optional Befehle an die Diagnose-Schnittstelle (--send, "+2" = 2 s warten).
- Schreibt das volle Log mit Zeitstempeln in --out, gibt nur Gefiltertes aus (--full: alles).
- Dekodiert Backtraces mit addr2line, wenn --elf und --addr2line angegeben sind.

Aufruf über tools/monitor.ps1 (wählt Python aus der ESP-IDF-Umgebung und erkennt den COM-Port).
"""

import argparse
import re
import subprocess
import sys
import time

import serial

ANSI = re.compile(r"\x1b\[[0-9;]*m")
OWN_TAGS = r"(app|sys|i2c|touch|bl|lcd|gfx|perf|cmd)"
DEFAULT_FILTER = (
    rf"^[IWE] \(\d+\) {OWN_TAGS}:"
    r"|^[WE] \(\d+\)"
    r"|Guru Meditation|Backtrace|abort\(\)|assert|panic|rst:0x|Brownout|CORRUPT HEAP|stack overflow|LCD underrun"
    r"|ESP-ROM:|Rebooting"
)


def decode_backtrace(line, elf, addr2line):
    addrs = re.findall(r"0x[0-9a-fA-F]{8}(?=:0x)", line)
    if not addrs or not elf or not addr2line:
        return []
    try:
        out = subprocess.run([addr2line, "-pfiaC", "-e", elf, *addrs], capture_output=True, text=True, timeout=20)
        return ["    -> " + l for l in out.stdout.splitlines() if l.strip()]
    except Exception as exc:  # noqa: BLE001
        return [f"    -> addr2line fehlgeschlagen: {exc}"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--seconds", type=float, default=10.0)
    ap.add_argument("--reset", action="store_true")
    ap.add_argument("--send", action="append", default=[])
    ap.add_argument("--wait", type=float, default=None, help="Sekunden bis zum ersten Befehl")
    ap.add_argument("--gap", type=float, default=0.4, help="Sekunden zwischen Befehlen")
    ap.add_argument("--out", required=True)
    ap.add_argument("--full", action="store_true")
    ap.add_argument("--filter", default=None)
    ap.add_argument("--until", default=None, help="Regex: vorzeitig beenden, sobald eine Zeile passt")
    ap.add_argument("--max-lines", type=int, default=200)
    ap.add_argument("--elf", default=None)
    ap.add_argument("--addr2line", default=None)
    args = ap.parse_args()

    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    filt = re.compile(args.filter or DEFAULT_FILTER)
    until = re.compile(args.until) if args.until else None

    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = args.baud
    ser.timeout = 0.05
    ser.dtr = False
    ser.rts = False
    try:
        ser.open()
    except serial.SerialException as exc:
        print(f"FEHLER: Port {args.port} nicht zu öffnen: {exc}")
        return 3

    start = time.monotonic()
    if args.reset:
        # EN über RTS kurz auf low ziehen (Auto-Reset-Schaltung), DTR bleibt aus = normaler Start
        ser.rts = True
        time.sleep(0.12)
        ser.rts = False
        start = time.monotonic()

    wait = args.wait if args.wait is not None else (2.5 if args.reset else 0.3)
    queue = list(args.send)
    next_send = start + wait

    printed = 0
    total = 0
    buf = b""
    stopped_early = False
    with open(args.out, "w", encoding="utf-8") as log:
        log.write(f"# Port {args.port}, {args.baud} Baud, Reset {'ja' if args.reset else 'nein'}, Start {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        while time.monotonic() - start < args.seconds:
            now = time.monotonic()
            if queue and now >= next_send:
                item = queue.pop(0)
                if item.startswith("+"):
                    next_send = now + float(item[1:])
                else:
                    ser.write((item + "\n").encode("utf-8"))
                    log.write(f"[{now - start:8.3f}] >>> {item}\n")
                    next_send = now + args.gap
            chunk = ser.read(4096)
            if not chunk:
                continue
            buf += chunk
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                line = ANSI.sub("", raw.decode("utf-8", errors="replace").rstrip("\r"))
                t = time.monotonic() - start
                log.write(f"[{t:8.3f}] {line}\n")
                total += 1
                show = args.full or filt.search(line)
                extra = decode_backtrace(line, args.elf, args.addr2line) if "Backtrace" in line else []
                for l in extra:
                    log.write(l + "\n")
                if show and printed < args.max_lines:
                    print(f"[{t:6.1f}] {line}")
                    for l in extra:
                        print(l)
                    printed += 1
                    if printed == args.max_lines:
                        print("… (Ausgabe gekürzt, volles Log in der Datei)")
                if until and until.search(line):
                    stopped_early = True
                    break
            if stopped_early:
                break
        if buf:
            log.write(f"[{time.monotonic() - start:8.3f}] {buf.decode('utf-8', errors='replace')}\n")
    ser.close()
    dauer = time.monotonic() - start
    print(f"--- {total} Zeilen in {dauer:.1f} s{' (Abbruch durch --until)' if stopped_early else ''}, {printed} angezeigt | Log: {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
