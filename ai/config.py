"""
Config loader — resolves config.json relative to THIS file's directory,
not the working directory.  This is why changing config.json had no effect
when the script was launched from a different folder.
"""

import json
import logging
import os
from pathlib import Path
from logging.handlers import RotatingFileHandler

# Always resolve relative to the directory containing this .py file
_AI_DIR = Path(__file__).resolve().parent
DEFAULT_CONFIG = _AI_DIR / "config.json"


def load_config(path: Path = DEFAULT_CONFIG) -> dict:
    try:
        with open(path, "r", encoding="utf-8") as f:
            cfg = json.load(f)
            logging.getLogger("water_station").info(f"Config loaded from {path}")
            return cfg
    except FileNotFoundError:
        print(f"[WARN] Config not found: {path} — using defaults")
        return {}
    except json.JSONDecodeError as e:
        print(f"[WARN] Config parse error: {e} — using defaults")
        return {}


def setup_logging(config: dict) -> logging.Logger:
    lc = config.get("logging", {})
    log_file = lc.get("file", "logs/water_station.log")

    # Make log path relative to ai/ folder too
    if not os.path.isabs(log_file):
        log_file = str(_AI_DIR / log_file)

    d = os.path.dirname(log_file)
    if d and not os.path.exists(d):
        os.makedirs(d)

    logger = logging.getLogger("water_station")
    logger.setLevel(getattr(logging, lc.get("level", "INFO")))

    if not logger.handlers:
        ch = logging.StreamHandler()
        ch.setFormatter(logging.Formatter(
            "%(asctime)s | %(levelname)-8s | %(message)s", datefmt="%H:%M:%S"
        ))
        logger.addHandler(ch)

        if lc.get("enabled", True):
            fh = RotatingFileHandler(
                log_file,
                maxBytes=lc.get("max_size_mb", 10) * 1024 * 1024,
                backupCount=lc.get("backup_count", 3),
            )
            fh.setFormatter(logging.Formatter(
                "%(asctime)s | %(levelname)-8s | %(name)s | %(message)s"
            ))
            logger.addHandler(fh)

    return logger
