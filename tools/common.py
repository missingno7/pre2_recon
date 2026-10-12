"""Paths and deterministic serialization; standard library only."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]

def sha(data):
    return hashlib.sha256(data).hexdigest()

def write_json(path, obj):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2, sort_keys=True) + '\n', encoding='utf-8')

def read_json(path):
    return json.loads(path.read_text(encoding='utf-8'))
