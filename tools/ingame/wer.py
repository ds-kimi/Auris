"""WER / CER of auris_load result JSONs against refs.tsv.

References come from the clip sources (JFK inaugural, LibriSpeech, its French
translation) cut at 1/3/8 s. Left out of the main score:
- the 1 s cuts end mid-word, their reference is approximate (reported apart);
- en_8s / fr_8s are one sentence said twice: whisper merges the repeat, which
  says nothing about game chat.
Each text is scored against the closest reference of its language: under load
dropped clips shift the per-talker FIFO that pairs results with clips, and a
wrong pairing would count as word errors.
Usage: python3 wer.py refs.tsv result.json [...] -> one line per file.
"""
import json
import re
import sys

SKIP = {"en_8s", "fr_8s"}


def norm(text):
    text = text.lower().replace("-", " ").replace("'", " ").replace("’", " ")
    return re.sub(r"[^\w ]", " ", text).split()


def edits(ref, hyp):
    row = list(range(len(hyp) + 1))
    for i, r in enumerate(ref, 1):
        prev, row[0] = row[0], i
        for j, h in enumerate(hyp, 1):
            cur = min(row[j] + 1, row[j - 1] + 1, prev + (r != h))
            prev, row[j] = row[j], cur
    return row[-1]


def score(refs, texts, short):
    words = errs = chars = cerrs = 0
    for t in texts:
        clip = t["clip"].split("/")[-1][:-4]
        if clip not in refs or clip in SKIP or clip.endswith("_1s") != short:
            continue
        hyp = norm(t.get("text") or "")
        lang = clip.split("_")[0][:2]
        pool = [k for k in refs if k.startswith(lang) and k not in SKIP and k.endswith("_1s") == short]
        ref = min((norm(refs[k]) for k in pool), key=lambda r: edits(r, hyp) / max(len(r), 1))
        words, errs = words + len(ref), errs + edits(ref, hyp)
        rc, hc = list(" ".join(ref)), list(" ".join(hyp))
        chars, cerrs = chars + len(rc), cerrs + edits(rc, hc)
    return (errs / words if words else None), (cerrs / chars if chars else None)


if __name__ == "__main__":
    refs = dict(l.rstrip("\n").split("\t", 1) for l in open(sys.argv[1], encoding="utf-8") if "\t" in l)
    for path in sys.argv[2:]:
        texts = json.load(open(path, encoding="utf-8")).get("texts") or []
        wer, cer = score(refs, texts, False)
        wer1, _ = score(refs, texts, True)
        fmt = lambda v: "n/a" if v is None else f"{v * 100:.1f}%"
        print(f"{path.split('/')[-1]}\tclips={len(texts)}\tWER={fmt(wer)}\tCER={fmt(cer)}\tWER_1s={fmt(wer1)}")
