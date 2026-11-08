"""Quasi-Earth-fixed NTN handover resource allocation study (DESIGN.md v5.1)."""

__version__ = "1.0.0"
DOCUMENT_VERSION = "5.1"
SCHEMA_VERSION = 6
CSV_HEADER_VERSION = 1

ACCESS_TYPES = (
    "4step-cbra",
    "4step-cfra",
    "2step-cbra",
    "2step-cfra",
    "cg",
    "dg",
)

# Capability profiles P1..P7 of Table 6.2. 4-step is mandatory.
CAPABILITY_PROFILES = {
    "P1": ("4step-cbra", "4step-cfra"),
    "P2": ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra"),
    "P3": ("4step-cbra", "4step-cfra", "cg"),
    "P4": ("4step-cbra", "4step-cfra", "dg"),
    "P5": ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra", "cg"),
    "P6": ("4step-cbra", "4step-cfra", "2step-cbra", "2step-cfra", "dg"),
    "P7": ACCESS_TYPES,
}

BUDGET_CLASSES = ("tight", "short", "medium", "loose")

CAPABILITY_ROW = ("P1", "P2", "P3", "P4", "P5", "P6", "P7")

TWO_STEP_TYPES = ("2step-cbra", "2step-cfra")
