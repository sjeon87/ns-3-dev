# Remaining Work: Measurement Request/Report Elements

## Report Element -- Missing Subelements

All defined optional subelements for the five implemented report types are now complete.

Beacon (5), Channel Load (3), Noise Histogram (4), Frame (6), and STA Statistics (7) are complete.

Notes:
- Table 9-165 (Channel Load report) and Table 9-167 (Noise Histogram report) define only
  WBC (ID 163) and Vendor Specific (ID 221). IDs 0-162 are Reserved -- there is no
  "Reporting" subelement in these report tables. The corresponding Reporting subelement
  (ID 1) exists only in the request tables (9-137 and 9-139).
- Table 9-171 (STA Statistics report) calls ID 1 "Reporting Reason", not "Triggered Reporting".

## Report Element -- Unimplemented Types (12/17)

BASIC (0), CCA (1), RPI Histogram (2), LCI (8), Transmit Stream Category (9),
Multicast Diagnostics (10), Location Civic (11), Location Identifier (12),
Directional Channel Quality (13), Directional Measurement (14),
Directional Statistics (15), FTM Range (16).

## Request Element -- Missing Subelements

| Type | Missing subelements |
|------|---------------------|
| Channel Load (3) | WBC (ID 163) |
| Noise Histogram (4) | WBC (ID 163) |
| Beacon (5) | Request (ID 10), Extended Request (ID 11), WBC (ID 163), Last Beacon Report Indication Request (ID 164) |
| Frame (6) | WBC (ID 163) |
| STA Statistics (7) | Triggered Reporting (ID 1) |
| LCI (8) | Originator Requesting STA MAC (ID 2), Target MAC (ID 3), Maximum Age (ID 4) |
| Transmit Stream (9) | Triggered Reporting (ID 1) |
| Multicast Diagnostics (10) | Triggered Reporting (ID 1) |
| Location Civic (11) | Originator MAC (ID 1), Target MAC (ID 2) |
| Location Identifier (12) | Originator MAC (ID 1), Target MAC (ID 2) |
| Directional Channel Quality (13) | Reporting (ID 1), Config (ID 2), Extended Config (ID 3) |
| FTM Range (16) | Maximum Age (ID 4) |

Type 17 (Neighboring DMG APs) has no body implementation at all.
