#!/usr/bin/env python3

import json
import re
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


LIBRARY_LABELS = {
    "BoostAdapter":         "boost",
    "JoinAdapter":          "join",
    "JsonConsAdapter":      "jsoncons",
    "JsonCppAdapter":       "jsoncpp",
    "NlohmannAdapter":      "nlohmann",
    "PocoAdapter":          "poco",
    "RapidJsonAutoUtfAdapter":     "rapidjson/auto-utf",
    "RapidJsonAdapter":     "rapidjson",
    "RapidJsonFullPrecAdapter":    "rapidjson/full-prec",
    "RapidJsonInsituAdapter":      "rapidjson/insitu",
    "RapidJsonIterativeAdapter":   "rapidjson/iterative",
    "SimdJsonAdapter":      "simdjson",
    "TaoCppAdapter":        "taocpp",
}

LIBRARY_ORDER = list(LIBRARY_LABELS.values())

DATASET_ORDER = ["Canada", "Citm", "Twitter"]

_PALETTE = plt.rcParams["axes.prop_cycle"].by_key()["color"]
_LIB_COLORS: dict[str, str] = {}

def _lib_color(name: str) -> str:
    if name not in _LIB_COLORS:
        idx = LIBRARY_ORDER.index(name) if name in LIBRARY_ORDER else len(_LIB_COLORS)
        _LIB_COLORS[name] = _PALETTE[idx % len(_PALETTE)]
    return _LIB_COLORS[name]

def load_results(paths: list[str]) -> pd.DataFrame:
    rows = []
    for path in paths:
        with open(path) as f:
            data = json.load(f)
        for b in data.get("benchmarks", []):
            if b.get("run_type") != "aggregate" or b.get("aggregate_name") != "median":
                continue
            name  = re.sub(r"_(mean|median|stddev|cv)$", "", b["name"])
            parts = name.split("/")
            if len(parts) < 3:
                continue
            rows.append({
                "type":    parts[0],
                "dataset": parts[1],
                "library": LIBRARY_LABELS.get(parts[2], parts[2]),
                "time_ms": b.get("real_time", 0),
            })
    return pd.DataFrame(rows)

def _style_ax(ax: plt.Axes) -> None:
    ax.grid(axis="x", alpha=0.3, linestyle="--")
    ax.set_axisbelow(True)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)

def _barh_ax(ax: plt.Axes, libs: list, values: list, title: str) -> None:
    colors = [_lib_color(lib) for lib in libs]
    bars = ax.barh(libs, values, color=colors, alpha=0.87, height=0.65, edgecolor="white")
    ax.bar_label(bars, fmt="%.0f", padding=4, fontsize=8)
    ax.set_title(title, fontsize=11, fontweight="bold", loc="left", pad=6)
    ax.set_xlabel("Time (ms)")
    ax.set_xlim(0, max(values) * 1.18 if values else 1)
    _style_ax(ax)

def plot_deserialize(df: pd.DataFrame, output_dir: Path) -> None:
    parse_df = df[df["type"] == "Parse"]
    stats_df = df[df["type"] == "Statistics"]
    if parse_df.empty:
        return

    datasets  = [d for d in DATASET_ORDER if d in parse_df["dataset"].unique()]
    has_stats = not stats_df.empty
    n_subs    = 1 + len(datasets) + (1 if has_stats else 0)
    n_libs    = parse_df["library"].nunique()
    row_h     = max(3.5, n_libs * 0.38 + 0.8)

    fig, axes = plt.subplots(n_subs, 1, figsize=(10, row_h * n_subs))
    if n_subs == 1:
        axes = [axes]
    fig.suptitle("JSON Benchmark — Parse", fontsize=14, fontweight="bold")

    idx = 0
    totals = parse_df.groupby("library")["time_ms"].sum().sort_index(ascending=False)
    _barh_ax(axes[idx], totals.index.tolist(), totals.values.tolist(), "1. Parse (overall)")
    idx += 1

    for dataset in datasets:
        sub = parse_df[parse_df["dataset"] == dataset].groupby("library")["time_ms"].sum().sort_index(ascending=False)
        _barh_ax(axes[idx], sub.index.tolist(), sub.values.tolist(), f"{idx + 1}. Parse — {dataset}")
        idx += 1

    if has_stats:
        totals = stats_df.groupby("library")["time_ms"].sum().sort_index(ascending=False)
        _barh_ax(axes[idx], totals.index.tolist(), totals.values.tolist(), f"{idx + 1}. DOM Statistics")

    fig.tight_layout(rect=[0, 0, 1, 0.97])
    out = output_dir / "performance_parse.png"
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print(f"  Saved: {out}")
    plt.close(fig)

def _plot_write_bench(df: pd.DataFrame, output_dir: Path, bench_type: str, filename: str, title: str) -> None:
    sub_df = df[df["type"] == bench_type]
    if sub_df.empty:
        return

    datasets  = [d for d in DATASET_ORDER if d in sub_df["dataset"].unique()]
    n_subs    = 1 + len(datasets)
    n_libs    = sub_df["library"].nunique()
    row_h     = max(3.5, n_libs * 0.38 + 0.8)

    fig, axes = plt.subplots(n_subs, 1, figsize=(10, row_h * n_subs))
    if n_subs == 1:
        axes = [axes]
    fig.suptitle(title, fontsize=14, fontweight="bold")

    totals = sub_df.groupby("library")["time_ms"].sum().sort_index(ascending=False)
    _barh_ax(axes[0], totals.index.tolist(), totals.values.tolist(), "1. Overall")

    for idx, dataset in enumerate(datasets, start=1):
        per = sub_df[sub_df["dataset"] == dataset].groupby("library")["time_ms"].sum().sort_index(ascending=False)
        _barh_ax(axes[idx], per.index.tolist(), per.values.tolist(), f"{idx + 1}. {dataset}")

    fig.tight_layout(rect=[0, 0, 1, 0.97])
    out = output_dir / filename
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print(f"  Saved: {out}")
    plt.close(fig)

def plot_serialize(df: pd.DataFrame, output_dir: Path) -> None:
    _plot_write_bench(df, output_dir, "Stringify", "performance_stringify.png", "JSON Benchmark — Stringify")
    _plot_write_bench(df, output_dir, "Prettify",  "performance_prettify.png",  "JSON Benchmark — Prettify")

CONFORMANCE_LABELS = {
    "boost":       "boost",
    "join":        "join",
    "jsoncons":    "jsoncons",
    "jsoncpp":     "jsoncpp",
    "nlohmann":    "nlohmann",
    "poco":        "poco",
    "AutoUTF":     "rapidjson/auto-utf",
    "Default":     "rapidjson",
    "FullPrec":    "rapidjson/full-prec",
    "Insitu":      "rapidjson/insitu",
    "Iterative":   "rapidjson/iterative",
    "simdjson":    "simdjson",
    "taocpp":      "taocpp",
}

_CONF_CATEGORIES = ["parse_validation", "parse_double", "parse_string", "roundtrip"]
_CONF_CAT_LABELS = ["Parse Validation", "Parse Double", "Parse String", "Roundtrip"]
_CONF_COLORS     = ["#4C72B0", "#DD8452", "#55A868", "#C44E52"]


def plot_conformance(conf_path: str, output_dir: Path) -> None:
    with open(conf_path) as f:
        data = json.load(f)
    results = data.get("results", [])
    if not results:
        return

    rows = []
    for r in results:
        lib = CONFORMANCE_LABELS.get(r["library"], r["library"])
        total_all = sum(r[c]["total"] for c in _CONF_CATEGORIES if c in r)
        if total_all == 0:
            continue
        pass_all = sum(r[c]["pass"] for c in _CONF_CATEGORIES if c in r)
        row = {"library": lib, "overall": pass_all / total_all * 100}
        for c in _CONF_CATEGORIES:
            row[c] = r[c]["pass"] / r[c]["total"] * 100 if c in r and r[c]["total"] > 0 else 0.0
        rows.append(row)

    if not rows:
        return

    rows.sort(key=lambda r: r["library"], reverse=True)

    all_keys   = ["overall"] + _CONF_CATEGORIES
    all_labels = ["Overall"] + _CONF_CAT_LABELS

    n_libs = len(rows)
    row_h  = max(3.0, n_libs * 0.38 + 0.7)

    fig, axes = plt.subplots(len(all_keys), 1,
                             figsize=(10, row_h * len(all_keys)))
    fig.suptitle("JSON Benchmark — Conformance", fontsize=14, fontweight="bold")

    for ax, c, label in zip(axes, all_keys, all_labels):
        libs   = [r["library"] for r in rows]
        vals   = [r[c] for r in rows]
        colors = [_lib_color(lib) for lib in libs]

        ax.barh(libs, vals, color=colors, alpha=0.87, height=0.65, edgecolor="white")
        for i, v in enumerate(vals):
            ax.text(min(v + 0.8, 101), i, f"{v:.0f}%", va="center", fontsize=8)

        ax.set_xlabel("%")
        ax.set_xlim(0, 115)
        ax.set_title(label, fontsize=11, loc="left", pad=6)
        _style_ax(ax)

    fig.tight_layout(rect=[0, 0, 1, 0.97])
    out = output_dir / "conformance.png"
    fig.savefig(out, dpi=150, bbox_inches="tight")
    print(f"  Saved: {out}")
    plt.close(fig)

def _is_conformance(path: str) -> bool:
    try:
        with open(path) as f:
            data = json.load(f)
        return "results" in data and "benchmarks" not in data
    except Exception:
        return False


def main() -> None:
    if len(sys.argv) < 2:
        sys.exit(1)

    output_dir = Path(__file__).parent
    print(f"Generating plots in: {output_dir}")

    bench_paths = [p for p in sys.argv[1:] if not _is_conformance(p)]
    conf_paths  = [p for p in sys.argv[1:] if _is_conformance(p)]

    if bench_paths:
        df = load_results(bench_paths)
        if not df.empty:
            plot_deserialize(df, output_dir)
            plot_serialize(df, output_dir)

    for conf_path in conf_paths:
        plot_conformance(conf_path, output_dir)

    print("Done.")


if __name__ == "__main__":
    main()
