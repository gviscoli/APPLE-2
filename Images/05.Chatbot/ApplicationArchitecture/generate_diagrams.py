"""
Generate professional architecture diagrams for PCH.CHATBOT project.
Produces three PNG images:
  1. architecture_overview.png  — full system showing both chatbot variants
  2. sequence_buffered.png      — sequence diagram, buffered chatbot
  3. sequence_streaming.png     — sequence diagram, streaming chatbot
"""

import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch
import matplotlib.patheffects as pe
import numpy as np

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

# ─── Palette ──────────────────────────────────────────────────────────────────
C_APPLE   = "#2B5F8E"    # Apple IIe blue
C_LAN     = "#4A4A4A"    # neutral dark
C_PROXY   = "#1A7E5A"    # proxy green
C_LLM     = "#7B3FA0"    # LLM purple
C_STREAM  = "#C0392B"    # streaming red accent
C_BUFF    = "#1F618D"    # buffered blue accent
C_LIGHT   = "#F5F5F5"    # box fill
C_BORDER  = "#CCCCCC"
WHITE     = "#FFFFFF"
BG        = "#FAFAFA"
TXT       = "#1A1A1A"
TXT_L     = "#FFFFFF"

FONT_TITLE  = dict(fontsize=11, fontweight="bold", color=TXT)
FONT_BODY   = dict(fontsize=8,  color=TXT)
FONT_SMALL  = dict(fontsize=7,  color="#555555")
FONT_LABEL  = dict(fontsize=7.5, fontweight="bold", color=TXT_L)


def box(ax, x, y, w, h, fc, ec=None, radius=0.015, lw=1.2, alpha=1.0, zorder=2):
    ec = ec or fc
    p = FancyBboxPatch(
        (x, y), w, h,
        boxstyle=f"round,pad=0,rounding_size={radius}",
        facecolor=fc, edgecolor=ec, linewidth=lw,
        alpha=alpha, zorder=zorder, transform=ax.transData
    )
    ax.add_patch(p)
    return p


def label(ax, x, y, text, **kw):
    defaults = dict(ha="center", va="center", zorder=5, wrap=False)
    defaults.update(kw)
    return ax.text(x, y, text, **defaults)


def arrow(ax, x0, y0, x1, y1, color="#555555", lw=1.5,
          style="->", lbl="", lbl_side="top", zorder=4):
    ax.annotate(
        "", xy=(x1, y1), xytext=(x0, y0),
        arrowprops=dict(
            arrowstyle=style,
            color=color,
            lw=lw,
            connectionstyle="arc3,rad=0.0"
        ),
        zorder=zorder
    )
    if lbl:
        mx, my = (x0 + x1) / 2, (y0 + y1) / 2
        offset = 0.012 if lbl_side == "top" else -0.018
        ax.text(mx, my + offset, lbl, ha="center", va="center",
                fontsize=6.5, color=color, zorder=6,
                bbox=dict(fc=BG, ec="none", pad=1))


# ══════════════════════════════════════════════════════════════════════════════
# 1.  ARCHITECTURE OVERVIEW
# ══════════════════════════════════════════════════════════════════════════════
def make_architecture_overview():
    fig = plt.figure(figsize=(16, 10), facecolor=BG)
    ax  = fig.add_axes([0, 0, 1, 1])
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    ax.set_facecolor(BG)

    # ── Title ─────────────────────────────────────────────────────────────────
    ax.text(0.5, 0.965, "PCH.CHATBOT — Architettura di Sistema",
            ha="center", va="center", fontsize=15, fontweight="bold", color=TXT)
    ax.text(0.5, 0.945, "Apple IIe (MOS 6502 @ 1 MHz) ↔ Python Proxy ↔ LLM Cloud",
            ha="center", va="center", fontsize=9, color="#555555")

    # ── Legend ────────────────────────────────────────────────────────────────
    for i, (fc, txt) in enumerate([
        (C_BUFF,   "Versione Bufferizzata (porta 8080)"),
        (C_STREAM, "Versione Streaming   (porta 8081)"),
        (C_PROXY,  "Layer Proxy / Server"),
        (C_LLM,    "LLM Cloud / Backend"),
    ]):
        lx = 0.04 + i * 0.235
        box(ax, lx, 0.010, 0.018, 0.022, fc=fc, radius=0.005)
        ax.text(lx + 0.022, 0.021, txt, va="center",
                fontsize=7.5, color=TXT)

    # ══ COLUMN HEADERS ══════════════════════════════════════════════════════
    cols_x   = [0.04,  0.285, 0.530, 0.755]
    col_w    = 0.225
    col_lbls = ["Apple IIe  (Client)", "LAN / Protocollo",
                "Python Proxy  (Server)", "LLM Backend  (Cloud)"]

    for cx, lbl_txt in zip(cols_x, col_lbls):
        box(ax, cx, 0.885, col_w, 0.048, fc="#E8EEF4", ec=C_BORDER, radius=0.008)
        ax.text(cx + col_w / 2, 0.909, lbl_txt,
                ha="center", va="center", fontsize=9.5, fontweight="bold", color=C_LAN)

    # ══ ROW LABELS ══════════════════════════════════════════════════════════
    rows = [
        (0.790, "Versione\nBufferizzata",  C_BUFF),
        (0.580, "Versione\nStreaming",     C_STREAM),
        (0.340, "Layer Comune\n(Hardware)", C_APPLE),
    ]
    for ry, rtxt, rc in rows:
        ax.text(0.018, ry, rtxt, ha="center", va="center",
                fontsize=7.5, fontweight="bold", color=rc, rotation=90)

    # ─────────────────────────────────────────────────────────────────────────
    # ROW 1 — Buffered
    # ─────────────────────────────────────────────────────────────────────────
    r1_top, r1_h = 0.853, 0.155

    # chatbot.c
    box(ax, 0.055, 0.700, 0.190, 0.140, fc=C_BUFF, ec="#1A4A7A", radius=0.010)
    label(ax, 0.150, 0.790, "chatbot.c", fontsize=9.5, fontweight="bold", color=WHITE)
    label(ax, 0.150, 0.771, "UI & Logica Conversazione", fontsize=7.5, color="#DDEEFF")
    for i, line in enumerate([
        "• read_line() — input tastiera",
        "• build_json() — payload JSON",
        "• send_message() → do_post()",
        "• print_wrapped() — 40 col.",
    ]):
        ax.text(0.065, 0.752 - i * 0.014, line, fontsize=6.8, color="#DDEEFF")

    # rest_lib.c
    box(ax, 0.055, 0.625, 0.190, 0.070, fc="#154360", ec="#1A4A7A", radius=0.010)
    label(ax, 0.150, 0.667, "rest_lib.c", fontsize=9, fontweight="bold", color=WHITE)
    label(ax, 0.150, 0.651, "HTTP/TCP Layer (bufferizzato)", fontsize=7, color="#DDEEFF")
    label(ax, 0.150, 0.638, "do_post()  •  tcp_recv_cb()", fontsize=6.5, color="#AACCEE")

    # LAN arrow row1
    box(ax, 0.285, 0.650, 0.225, 0.070, fc="#EEF2F7", ec=C_BORDER, radius=0.008)
    label(ax, 0.397, 0.695, "HTTP/1.0  plain-text", fontsize=8.5, fontweight="bold", color=C_BUFF)
    label(ax, 0.397, 0.678, "POST /api/data  →  porta 8080", fontsize=7, color=TXT)
    label(ax, 0.397, 0.663, "TCP close = EOF implicito", fontsize=7, color="#888888")

    # Proxy buffered
    box(ax, 0.530, 0.625, 0.200, 0.108, fc=C_PROXY, ec="#0D5C3E", radius=0.010)
    label(ax, 0.630, 0.700, "llm_proxy.py", fontsize=9, fontweight="bold", color=WHITE)
    label(ax, 0.630, 0.684, "Porta 8080  •  Flask", fontsize=7.5, color="#CCFFEE")
    for i, line in enumerate([
        "call_nova() / call_anthropic()",
        "call_ollama()",
        "clean_for_apple2()  •  .upper()",
        "max 250 char  •  Risposta intera",
    ]):
        ax.text(0.540, 0.668 - i * 0.013, line, fontsize=6.5, color="#CCFFEE")

    # AWS Bedrock
    box(ax, 0.755, 0.625, 0.210, 0.108, fc=C_LLM, ec="#551A7A", radius=0.010)
    label(ax, 0.860, 0.700, "LLM Backend", fontsize=9, fontweight="bold", color=WHITE)
    for i, line in enumerate([
        "Amazon Nova Lite v1  (AWS Bedrock)",
        "Anthropic Claude Haiku",
        "Ollama  (locale)",
        "HTTPS / AWS Signature V4",
    ]):
        ax.text(0.763, 0.679 - i * 0.014, line, fontsize=6.8, color="#EEDDFF")

    # Arrows row1
    arrow(ax, 0.245, 0.660, 0.285, 0.660, color=C_BUFF, lw=2.0)
    arrow(ax, 0.510, 0.660, 0.530, 0.660, color=C_BUFF, lw=2.0)
    arrow(ax, 0.730, 0.660, 0.755, 0.660, color="#888888", lw=1.5)
    arrow(ax, 0.755, 0.685, 0.730, 0.685, color="#888888", lw=1.5, style="<-")
    arrow(ax, 0.510, 0.685, 0.530, 0.685, color=C_BUFF, lw=2.0, style="<-")
    arrow(ax, 0.245, 0.685, 0.285, 0.685, color=C_BUFF, lw=2.0, style="<-")

    # ─────────────────────────────────────────────────────────────────────────
    # ROW 2 — Streaming
    # ─────────────────────────────────────────────────────────────────────────

    # chatbot_streaming.c
    box(ax, 0.055, 0.410, 0.190, 0.140, fc=C_STREAM, ec="#8B0000", radius=0.010)
    label(ax, 0.150, 0.500, "chatbot_streaming.c", fontsize=8.5, fontweight="bold", color=WHITE)
    label(ax, 0.150, 0.482, "UI Streaming Real-Time", fontsize=7.5, color="#FFE0DD")
    for i, line in enumerate([
        "• Stampa 'BOT:' prima della risposta",
        "• do_post_stream() — non attende",
        "• stream_putchar() — carattere/carattere",
        "• Pager integrato (ogni 20 righe)",
    ]):
        ax.text(0.065, 0.464 - i * 0.014, line, fontsize=6.8, color="#FFE0DD")

    # rest_lib_streaming.c
    box(ax, 0.055, 0.335, 0.190, 0.070, fc="#7B241C", ec="#8B0000", radius=0.010)
    label(ax, 0.150, 0.377, "rest_lib_streaming.c", fontsize=8.5, fontweight="bold", color=WHITE)
    label(ax, 0.150, 0.361, "HTTP/TCP Layer (streaming)", fontsize=7, color="#FFE0DD")
    label(ax, 0.150, 0.348, "do_post_stream()  •  tcp_stream_cb()", fontsize=6.5, color="#FFBBAA")

    # LAN arrow row2
    box(ax, 0.285, 0.355, 0.225, 0.095, fc="#FEF5F5", ec=C_STREAM, radius=0.008, lw=1.5)
    label(ax, 0.397, 0.420, "HTTP/1.1  Chunked TE", fontsize=8.5, fontweight="bold", color=C_STREAM)
    label(ax, 0.397, 0.404, "POST /api/data  →  porta 8081", fontsize=7, color=TXT)
    label(ax, 0.397, 0.389, "Token → chunk → stampa diretta", fontsize=7, color="#888888")
    label(ax, 0.397, 0.374, "Pager: pausa ogni 20 righe", fontsize=6.5, color="#888888")

    # Proxy streaming
    box(ax, 0.530, 0.335, 0.200, 0.125, fc="#0D5C3E", ec="#073E2A", radius=0.010)
    label(ax, 0.630, 0.424, "llm_proxy_streaming.py", fontsize=8.5, fontweight="bold", color=WHITE)
    label(ax, 0.630, 0.408, "Porta 8081  •  Flask  •  Generator", fontsize=7, color="#CCFFEE")
    for i, line in enumerate([
        "stream_nova() / stream_anthropic()",
        "stream_ollama()",
        "yield token by token",
        "Transfer-Encoding: chunked auto",
        "clean_for_apple2()  •  .upper()",
    ]):
        ax.text(0.540, 0.392 - i * 0.013, line, fontsize=6.5, color="#CCFFEE")

    # Same LLM Backend (shared) — draw bracket
    box(ax, 0.755, 0.335, 0.210, 0.125, fc=C_LLM, ec="#551A7A", radius=0.010, alpha=0.75)
    label(ax, 0.860, 0.424, "LLM Backend", fontsize=9, fontweight="bold", color=WHITE)
    label(ax, 0.860, 0.408, "(condiviso)", fontsize=7.5, color="#EEDDFF")
    for i, line in enumerate([
        "converse_stream()  (Nova)",
        "invoke_model_with_response_stream()",
        "Ollama streaming",
    ]):
        ax.text(0.763, 0.390 - i * 0.014, line, fontsize=6.8, color="#EEDDFF")

    # Arrows row2
    arrow(ax, 0.245, 0.370, 0.285, 0.370, color=C_STREAM, lw=2.0)
    arrow(ax, 0.510, 0.370, 0.530, 0.370, color=C_STREAM, lw=2.0)
    arrow(ax, 0.730, 0.370, 0.755, 0.370, color="#888888", lw=1.5)
    # streaming arrows back — dashed
    for x0, x1, y in [(0.730, 0.755, 0.395), (0.510, 0.530, 0.395)]:
        ax.annotate("", xy=(x0, y), xytext=(x1, y),
                    arrowprops=dict(arrowstyle="->", color=C_STREAM, lw=1.8,
                                   linestyle="dashed"))
    ax.annotate("", xy=(0.245, 0.395), xytext=(0.285, 0.395),
                arrowprops=dict(arrowstyle="->", color=C_STREAM, lw=1.8,
                                linestyle="dashed"))
    ax.text(0.392, 0.450, "chunk…chunk…chunk…", ha="center", fontsize=6.5,
            color=C_STREAM, style="italic")

    # ─────────────────────────────────────────────────────────────────────────
    # ROW 3 — Common Hardware
    # ─────────────────────────────────────────────────────────────────────────
    box(ax, 0.055, 0.070, 0.190, 0.225, fc=C_APPLE, ec="#1A3E5A", radius=0.010)
    label(ax, 0.150, 0.278, "Apple IIe Hardware", fontsize=9, fontweight="bold", color=WHITE)
    for i, line in enumerate([
        "MOS 6502  @  1 MHz",
        "64 KB RAM  (≈40 KB utili)",
        "Stack fisico  256 byte",
        "Display  40 × 24  ASCII 7-bit",
        "Uthernet II  (slot espansione)",
        "Chip W5100  (Ethernet)",
        "",
        "ip65  —  TCP/IP stack in Assembly",
        "dhcp_init()  •  tcp_connect()",
        "tcp_send()   •  ip65_process()",
        "ip65_error=0x84 → EOF/close",
    ]):
        ax.text(0.065, 0.260 - i * 0.016, line, fontsize=6.8, color="#DDEEFF")

    # Compilation box
    box(ax, 0.285, 0.070, 0.225, 0.140, fc="#F0F4F8", ec=C_BORDER, radius=0.008)
    label(ax, 0.397, 0.195, "Compilazione & Deploy", fontsize=8.5, fontweight="bold", color=TXT)
    for i, line in enumerate([
        "cl65  (cc65 cross-compiler)",
        "Target:  apple2",
        "Ottimizzazione:  -O",
        "",
        "Output:  pch_chatbot.bin  (6502)",
        "Disco:   pch_chatbot.dsk  (Apple)",
        "Tool:    AppleCommander",
    ]):
        ax.text(0.295, 0.177 - i * 0.014, line, fontsize=6.8, color=TXT)

    # Constraints box
    box(ax, 0.530, 0.070, 0.200, 0.140, fc="#F9F5FF", ec="#CCAAFF", radius=0.008, lw=1.2)
    label(ax, 0.630, 0.195, "Vincoli Hardware & Buffer", fontsize=8.5, fontweight="bold", color=C_LLM)
    for i, line in enumerate([
        "INPUT_MAX       = 120 byte",
        "JSON_BUF_SIZE   = 160 byte",
        "request_buf     = 512 byte",
        "RESPONSE_BUF_SIZE = 2048 byte",
        "MAX_RESPONSE    = 250 char",
        "Stack call chain: max 5 livelli",
        "No malloc() — solo buffer statici",
    ]):
        ax.text(0.540, 0.177 - i * 0.013, line, fontsize=6.8, color=TXT,
                family="monospace")

    # Config box
    box(ax, 0.755, 0.070, 0.210, 0.140, fc="#F5FFF9", ec="#AAFFCC", radius=0.008, lw=1.2)
    label(ax, 0.860, 0.195, "Configurazione (.env)", fontsize=8.5, fontweight="bold", color=C_PROXY)
    for i, line in enumerate([
        "INFERENCE_MODE=cloud|anthropic|ollama",
        "",
        "LLM_MODEL_CLOUD=",
        "  amazon.nova-lite-v1:0",
        "LLM_MODEL_ANTHROPIC=",
        "  claude-haiku-4-5",
        "LLM_MODEL_OLLAMA=llama3.2",
        "AWS_DEFAULT_REGION=eu-central-1",
    ]):
        ax.text(0.762, 0.177 - i * 0.013, line, fontsize=6.3, color=TXT,
                family="monospace")

    # Vertical connectors: hardware to both rows
    ax.annotate("", xy=(0.150, 0.625), xytext=(0.150, 0.550),
                arrowprops=dict(arrowstyle="->", color=C_BUFF, lw=1.5))
    ax.annotate("", xy=(0.150, 0.410), xytext=(0.150, 0.295),
                arrowprops=dict(arrowstyle="->", color=C_STREAM, lw=1.5))

    # Separator lines
    for y in [0.608, 0.320]:
        ax.axhline(y, xmin=0.04, xmax=0.96, color=C_BORDER, lw=1.0, linestyle="--")

    # Footer
    ax.text(0.5, 0.025, "© PCH.CHATBOT Project — Apple IIe + LLM (2024–2025)",
            ha="center", va="center", fontsize=7, color="#AAAAAA")

    plt.tight_layout(pad=0)
    out = os.path.join(OUT_DIR, "architecture_overview.png")
    fig.savefig(out, dpi=150, bbox_inches="tight", facecolor=BG)
    plt.close(fig)
    print(f"Saved: {out}")


# ══════════════════════════════════════════════════════════════════════════════
# 2.  SEQUENCE DIAGRAM — BUFFERED
# ══════════════════════════════════════════════════════════════════════════════
def make_sequence_buffered():
    fig = plt.figure(figsize=(14, 11), facecolor=BG)
    ax  = fig.add_axes([0.02, 0.02, 0.96, 0.96])
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    ax.set_facecolor(BG)

    # Title
    ax.text(0.5, 0.975, "Sequence Diagram — Chatbot Bufferizzato (HTTP/1.0)",
            ha="center", fontsize=14, fontweight="bold", color=TXT)
    ax.text(0.5, 0.958, "Porta 8080  •  Risposta intera in un blocco  •  Word-wrap 40 colonne",
            ha="center", fontsize=8.5, color="#555555")

    # ── Actors ────────────────────────────────────────────────────────────────
    actors = [
        (0.10, "Utente",               "#E8EEF4",   C_LAN),
        (0.28, "chatbot.c",            C_BUFF,      C_BUFF),
        (0.46, "rest_lib.c\n+ ip65",   "#154360",   "#154360"),
        (0.64, "llm_proxy.py\n:8080",  C_PROXY,     C_PROXY),
        (0.82, "LLM Backend\n(Cloud)",  C_LLM,       C_LLM),
    ]
    ACTOR_Y = 0.920
    BOX_W, BOX_H = 0.115, 0.046
    xs = [a[0] for a in actors]

    for ax_x, name, fc, ec in actors:
        box(ax, ax_x - BOX_W / 2, ACTOR_Y, BOX_W, BOX_H,
            fc=fc, ec=ec, radius=0.012, lw=1.5)
        tc = WHITE if fc != "#E8EEF4" else TXT
        for i, part in enumerate(name.split("\n")):
            ax.text(ax_x, ACTOR_Y + BOX_H - 0.011 - i * 0.014, part,
                    ha="center", va="center",
                    fontsize=8 if "\n" not in name else 7.5,
                    fontweight="bold", color=tc)

    # Lifelines
    LIFE_TOP  = ACTOR_Y
    LIFE_BOT  = 0.040
    for ax_x, *_ in actors:
        ax.plot([ax_x, ax_x], [LIFE_TOP, LIFE_BOT],
                color=C_BORDER, lw=1.2, linestyle="--", zorder=1)

    # ── Messages ──────────────────────────────────────────────────────────────
    def msg(y, x0, x1, txt, color=C_LAN, dashed=False, lw=1.5, lbl_side="top"):
        style = "dashed" if dashed else "solid"
        arr_style = "<-" if x1 < x0 else "->"
        ax.annotate("", xy=(x1, y), xytext=(x0, y),
                    arrowprops=dict(arrowstyle=arr_style, color=color,
                                   lw=lw, linestyle=style), zorder=4)
        offset = 0.013 if lbl_side == "top" else -0.017
        ax.text((x0 + x1) / 2, y + offset, txt,
                ha="center", va="center", fontsize=7.5, color=color, zorder=5,
                bbox=dict(fc=BG, ec="none", pad=1.5))

    def note(y, x, txt, fc="#FFFDE7", ec="#F9A825", w=0.16, h=0.040):
        box(ax, x - w / 2, y - h / 2, w, h, fc=fc, ec=ec, radius=0.006, lw=1)
        for i, line in enumerate(txt.split("\n")):
            ax.text(x, y + 0.010 - i * 0.015, line,
                    ha="center", va="center", fontsize=6.8, color=TXT)

    def activation(y_top, y_bot, x, w=0.018, fc=C_BUFF):
        box(ax, x - w / 2, y_bot, w, y_top - y_bot, fc=fc, ec="white",
            radius=0.003, lw=0.5, zorder=3)

    def divider(y, txt):
        ax.axhline(y, xmin=0.03, xmax=0.97, color="#DDDDDD", lw=0.8, linestyle=":")
        ax.text(0.5, y + 0.008, txt, ha="center", fontsize=7,
                color="#888888", style="italic",
                bbox=dict(fc=BG, ec="none", pad=1))

    # Step sequence (y values decreasing from top)
    y = 0.890

    # Phase 1: User input
    divider(y - 0.010, "① Fase: Input Utente")
    y -= 0.045

    msg(y, xs[0], xs[1], "Digita domanda (tastiera)", color=C_LAN)
    activation(y + 0.008, y - 0.025, xs[1], fc=C_BUFF)
    y -= 0.040

    note(y, xs[1], "read_line()\nInput 120 char max\ncgetc() & 0x7F", w=0.17, h=0.052)
    y -= 0.055

    note(y, xs[1], "build_json()\n{\"prompt\":\"...\"}  160 byte max", w=0.19, h=0.040)
    y -= 0.048

    # Phase 2: HTTP Request
    divider(y - 0.006, "② Fase: Richiesta HTTP/1.0")
    y -= 0.040

    msg(y, xs[1], xs[2], "do_post(\"/api/data\", json_body)", color=C_BUFF, lw=2)
    activation(y + 0.008, y - 0.090, xs[2], fc="#154360")
    y -= 0.038

    note(y, xs[2], "Costruisce header HTTP\n512 byte buffer statico", w=0.18, h=0.040)
    y -= 0.045

    msg(y, xs[2], xs[3],
        "POST /api/data HTTP/1.0\\r\\n"
        "Content-Type: application/json\\r\\n"
        "Connection: close  ← EOF implicito",
        color=C_BUFF, lw=1.8)
    y -= 0.040

    note(y, xs[2], "tcp_connect() → tcp_send()\nip65_process() polling loop", w=0.19, h=0.040)
    y -= 0.048

    # Phase 3: Proxy → LLM
    divider(y - 0.006, "③ Fase: Proxy → LLM Backend")
    y -= 0.040

    activation(y + 0.008, y - 0.090, xs[3], fc=C_PROXY)
    msg(y, xs[3], xs[4], "invoke_model() HTTPS/API", color=C_PROXY, lw=1.8)
    activation(y + 0.008, y - 0.070, xs[4], fc=C_LLM)
    y -= 0.038

    note(y, xs[3], "POST a Bedrock / Anthropic / Ollama\nmaxTokens: 150, temperature: 0.4",
         w=0.19, fc="#F0FFF0", ec=C_PROXY, h=0.040)
    y -= 0.048

    msg(y, xs[4], xs[3], "Risposta JSON completa", color=C_LLM, dashed=True, lw=1.8)
    y -= 0.040

    note(y, xs[3],
         "clean_for_apple2()  accenti→ASCII\ntruncate 250 char  •  .upper()",
         w=0.20, fc="#F0FFF0", ec=C_PROXY, h=0.040)
    y -= 0.048

    # Phase 4: Response back
    divider(y - 0.006, "④ Fase: Risposta HTTP → Apple IIe")
    y -= 0.040

    msg(y, xs[3], xs[2], "HTTP/1.0 200 OK\\r\\n\\r\\n<TESTO>", color=C_PROXY, dashed=True, lw=1.8)
    y -= 0.040

    note(y, xs[2],
         "tcp_recv_cb() accumula 2048 byte\nip65_error=0x84 → TCP close → EOF",
         w=0.21, h=0.040)
    y -= 0.048

    msg(y, xs[2], xs[1], "Ritorna puntatore al body", color=C_BUFF, dashed=True, lw=1.8)
    y -= 0.040

    # Phase 5: Display
    divider(y - 0.006, "⑤ Fase: Visualizzazione Word-Wrap")
    y -= 0.040

    note(y, xs[1],
         "print_wrapped(risposta, 40 col)\nSpezza alle parole, non ai caratteri",
         w=0.21, fc="#EEF4FF", ec=C_BUFF, h=0.040)
    y -= 0.048

    msg(y, xs[1], xs[0], "Stampa risposta (40 col word-wrap)", color=C_BUFF, dashed=True, lw=1.8)
    y -= 0.040

    note(y, xs[0],
         "BOT:\nUNA RETE NEURALE E' UN MODELLO\nDI CALCOLO ISPIRATO AL CERVELLO",
         w=0.20, fc="#E8EEF4", ec=C_LAN, h=0.052)

    # Footer
    ax.text(0.5, 0.018,
            "NOTA: Tutta la risposta è bufferizzata prima di essere visualizzata. "
            "Latenza percepita = tempo LLM + trasmissione rete.",
            ha="center", fontsize=7, color="#888888",
            bbox=dict(fc="#FFF9E6", ec="#FFCC00", pad=4, boxstyle="round"))

    out = os.path.join(OUT_DIR, "sequence_buffered.png")
    fig.savefig(out, dpi=150, bbox_inches="tight", facecolor=BG)
    plt.close(fig)
    print(f"Saved: {out}")


# ══════════════════════════════════════════════════════════════════════════════
# 3.  SEQUENCE DIAGRAM — STREAMING
# ══════════════════════════════════════════════════════════════════════════════
def make_sequence_streaming():
    fig = plt.figure(figsize=(14, 13), facecolor=BG)
    ax  = fig.add_axes([0.02, 0.02, 0.96, 0.96])
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    ax.set_facecolor(BG)

    ax.text(0.5, 0.979, "Sequence Diagram — Chatbot Streaming (HTTP/1.1 Chunked TE)",
            ha="center", fontsize=14, fontweight="bold", color=TXT)
    ax.text(0.5, 0.963,
            "Porta 8081  •  Token per token in tempo reale  •  Pager ogni 20 righe",
            ha="center", fontsize=8.5, color="#555555")

    actors = [
        (0.09,  "Utente",                    "#E8EEF4",   C_LAN),
        (0.265, "chatbot_\nstreaming.c",     C_STREAM,    C_STREAM),
        (0.445, "rest_lib_\nstreaming.c",    "#7B241C",   "#7B241C"),
        (0.625, "llm_proxy_\nstreaming.py",  "#0D5C3E",   "#0D5C3E"),
        (0.810, "LLM Backend\n(Cloud)",       C_LLM,       C_LLM),
    ]
    ACTOR_Y = 0.928
    BOX_W, BOX_H = 0.120, 0.048
    xs = [a[0] for a in actors]

    for ax_x, name, fc, ec in actors:
        box(ax, ax_x - BOX_W / 2, ACTOR_Y, BOX_W, BOX_H,
            fc=fc, ec=ec, radius=0.012, lw=1.5)
        tc = WHITE if fc != "#E8EEF4" else TXT
        for i, part in enumerate(name.split("\n")):
            ax.text(ax_x, ACTOR_Y + BOX_H - 0.012 - i * 0.016, part,
                    ha="center", va="center",
                    fontsize=7.5, fontweight="bold", color=tc)

    LIFE_TOP = ACTOR_Y
    LIFE_BOT = 0.038
    for ax_x, *_ in actors:
        ax.plot([ax_x, ax_x], [LIFE_TOP, LIFE_BOT],
                color=C_BORDER, lw=1.2, linestyle="--", zorder=1)

    def msg(y, x0, x1, txt, color=C_LAN, dashed=False, lw=1.5):
        style = "dashed" if dashed else "solid"
        arr_style = "<-" if x1 < x0 else "->"
        ax.annotate("", xy=(x1, y), xytext=(x0, y),
                    arrowprops=dict(arrowstyle=arr_style, color=color,
                                   lw=lw, linestyle=style), zorder=4)
        offset = 0.012
        ax.text((x0 + x1) / 2, y + offset, txt,
                ha="center", va="center", fontsize=7.3, color=color, zorder=5,
                bbox=dict(fc=BG, ec="none", pad=1.5))

    def note(y, x, txt, fc="#FFFDE7", ec="#F9A825", w=0.17, h=0.038):
        box(ax, x - w / 2, y - h / 2, w, h, fc=fc, ec=ec, radius=0.006, lw=1)
        for i, line in enumerate(txt.split("\n")):
            ax.text(x, y + 0.008 - i * 0.014, line,
                    ha="center", va="center", fontsize=6.7, color=TXT)

    def activation(y_top, y_bot, x, w=0.018, fc=C_STREAM):
        box(ax, x - w / 2, y_bot, w, y_top - y_bot, fc=fc, ec="white",
            radius=0.003, lw=0.5, zorder=3)

    def divider(y, txt):
        ax.axhline(y, xmin=0.02, xmax=0.98, color="#DDDDDD", lw=0.8, linestyle=":")
        ax.text(0.5, y + 0.008, txt, ha="center", fontsize=7,
                color="#888888", style="italic",
                bbox=dict(fc=BG, ec="none", pad=1))

    y = 0.905

    # Phase 1
    divider(y - 0.008, "① Fase: Input Utente")
    y -= 0.042

    msg(y, xs[0], xs[1], "Digita domanda (tastiera)", color=C_LAN)
    activation(y + 0.008, y - 0.020, xs[1], fc=C_STREAM)
    y -= 0.038

    note(y, xs[1], "read_line()  •  build_json()\nSame as versione bufferizzata",
         w=0.20, h=0.038)
    y -= 0.045

    # Phase 2: Immediate BOT: header
    divider(y - 0.006, "② Fase: Stampa Immediata Header (prima della risposta LLM!)")
    y -= 0.038

    activation(y + 0.008, y - 0.020, xs[1], fc=C_STREAM)
    note(y, xs[1], "printf(\"BOT:\\n---...---\\n\")\nStampa PRIMA di inviare la richiesta",
         w=0.23, fc="#FFF0F0", ec=C_STREAM, h=0.040)
    msg(y, xs[1], xs[0], "→ Schermo: 'BOT:'  (immediato)", color=C_STREAM, dashed=True)
    y -= 0.048

    # Phase 3: HTTP Request
    divider(y - 0.006, "③ Fase: Richiesta HTTP/1.0 → Proxy Streaming")
    y -= 0.038

    msg(y, xs[1], xs[2], "do_post_stream(\"/api/data\", json_body)", color=C_STREAM, lw=2)
    activation(y + 0.008, y - 0.110, xs[2], fc="#7B241C")
    y -= 0.036

    msg(y, xs[2], xs[3],
        "POST /api/data HTTP/1.0  →  porta 8081",
        color=C_STREAM, lw=1.8)
    activation(y + 0.008, y - 0.090, xs[3], fc="#0D5C3E")
    y -= 0.038

    # Phase 4: Proxy → LLM streaming
    divider(y - 0.006, "④ Fase: Proxy → LLM (chiamata streaming)")
    y -= 0.038

    msg(y, xs[3], xs[4],
        "converse_stream() / response_stream()  HTTPS",
        color="#0D5C3E", lw=1.8)
    activation(y + 0.008, y - 0.100, xs[4], fc=C_LLM)
    y -= 0.038

    note(y, xs[3],
         "LLM genera token per token\nProxy è un generator Python (yield)",
         w=0.22, fc="#F0FFF0", ec="#0D5C3E", h=0.040)
    y -= 0.048

    # Phase 5: Chunked streaming back
    divider(y - 0.006, "⑤ Fase: Streaming Token-per-Token (cuore dell'architettura)")
    y -= 0.038

    # Draw a loop bracket
    loop_y_top = y + 0.005
    for j in range(3):
        tok_y = y - j * 0.058

        msg(tok_y, xs[4], xs[3], f"Token #{j+1}  (es. 'UNA', 'RETE', 'NEURALE')",
            color=C_LLM, dashed=True, lw=1.6)
        tok_y -= 0.024

        note(tok_y, xs[3],
             f"clean_for_apple2()  •  .upper()\nyield token → Flask aggiunge chunk HTTP",
             w=0.23, fc="#F0FFF0", ec="#0D5C3E", h=0.036)
        tok_y -= 0.032

        msg(tok_y, xs[3], xs[2],
            f"HTTP/1.1  chunk #{j+1}: <hex>\\r\\n<TOKEN>\\r\\n",
            color=C_STREAM, dashed=True, lw=1.8)
        tok_y -= 0.028

        note(tok_y, xs[2],
             "State machine: CK_SIZE→CK_DATA\ntcp_stream_cb()→stream_putchar()",
             w=0.22, fc="#FFF0F0", ec=C_STREAM, h=0.036)
        tok_y -= 0.032

        msg(tok_y, xs[2], xs[0],
            "→ Schermo: carattere per carattere",
            color=C_STREAM, dashed=True, lw=1.6)

        y = tok_y - 0.012

    loop_y_bot = y + 0.006
    ax.add_patch(mpatches.FancyBboxPatch(
        (0.02, loop_y_bot), 0.90, loop_y_top - loop_y_bot,
        boxstyle="round,pad=0,rounding_size=0.008",
        fc="none", ec="#FFAAAA", lw=1.5, linestyle="--", zorder=1
    ))
    ax.text(0.022, (loop_y_top + loop_y_bot) / 2,
            "loop\nper ogni\ntoken",
            ha="left", va="center", fontsize=6.5, color=C_STREAM,
            style="italic", rotation=90)

    y -= 0.012

    # Phase 6: End chunk
    divider(y - 0.006, "⑥ Fase: Fine Stream (chunk terminatore)")
    y -= 0.038

    msg(y, xs[3], xs[2],
        "HTTP/1.1  chunk terminatore:  0\\r\\n\\r\\n",
        color="#0D5C3E", dashed=True, lw=1.8)
    y -= 0.036

    note(y, xs[2],
         "State: CK_DONE\nChiusura connessione TCP",
         w=0.20, fc="#FFF0F0", ec=C_STREAM, h=0.036)
    y -= 0.044

    # Phase 7: Pager
    divider(y - 0.006, "⑦ Fase: Pager (ogni 20 righe)")
    y -= 0.036

    note(y, xs[2],
         "line_count >= 20\n'-- PIU\\': PREMI UN TASTO --'",
         w=0.22, fc="#FFF9E6", ec="#F9A825", h=0.038)
    msg(y, xs[2], xs[0], "→ Pausa, attende keypress", color="#888888", dashed=True)
    y -= 0.044

    # Footer
    ax.text(0.5, 0.018,
            "NOTA: Il primo carattere appare sul display prima che LLM abbia finito di generare. "
            "Latenza percepita ≈ latenza primo token (TTFT).",
            ha="center", fontsize=7, color="#888888",
            bbox=dict(fc="#FFF0F0", ec=C_STREAM, pad=4, boxstyle="round"))

    out = os.path.join(OUT_DIR, "sequence_streaming.png")
    fig.savefig(out, dpi=150, bbox_inches="tight", facecolor=BG)
    plt.close(fig)
    print(f"Saved: {out}")


# ══════════════════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    print("Generating diagrams...")
    make_architecture_overview()
    make_sequence_buffered()
    make_sequence_streaming()
    print("All done.")
