# IEEE 802.11k Radio Resource Management: Frames and Information Elements

Complete reference for all Action Frames and Information Elements required for 802.11k implementation in ns-3. All references are to IEEE 802.11-2024.

---

## 1. Action Frames (Category 5: Radio Measurement)

Section 9.6.6. Six action values defined in Table 9-470.

### 1.1 Radio Measurement Request (Action 0)

**Section 9.6.6.2, Figure 9-1185**

| Field | Octets | Notes |
|-------|--------|-------|
| Category | 1 | RADIO_MEASUREMENT = 5 |
| Radio Measurement Action | 1 | 0 |
| Dialog Token | 1 | Nonzero, chosen by sender |
| Number of Repetitions | 2 | 0 = once, 65535 = until cancelled |
| Measurement Request Elements | variable | Zero or more IE 38 instances |

Number of Repetitions applies to all Measurement Request elements in the frame. Limited by max MMPDU size.

**ns-3 status:** `WifiActionHeader::RADIO_MEASUREMENT_REQUEST` enum exists. No frame body class.

### 1.2 Radio Measurement Report (Action 1)

**Section 9.6.6.3, Figure 9-1186**

| Field | Octets | Notes |
|-------|--------|-------|
| Category | 1 | 5 |
| Radio Measurement Action | 1 | 1 |
| Dialog Token | 1 | From corresponding request, or 0 if autonomous |
| Measurement Report Elements | variable | One or more IE 39 instances |

Multiframe responses described in 11.10.6.

**ns-3 status:** `WifiActionHeader::RADIO_MEASUREMENT_REPORT` enum exists. No frame body class.

### 1.3 Link Measurement Request (Action 2)

**Section 9.6.6.4, Figure 9-1187**

| Field | Octets | Notes |
|-------|--------|-------|
| Category | 1 | 5 |
| Radio Measurement Action | 1 | 2 |
| Dialog Token | 1 | Nonzero |
| Transmit Power Used | 1 | dBm (9.4.1.20) |
| Max Transmit Power | 1 | dBm (9.4.1.19) |
| Extended Link Measurement | variable | Optional Extended Link Measurement element |

**ns-3 status:** DONE. `LinkMeasurementRequestHeader` in `link-measurement.h`. Does not implement optional Extended Link Measurement element.

### 1.4 Link Measurement Report (Action 3)

**Section 9.6.6.5, Figure 9-1188**

| Field | Octets | Notes |
|-------|--------|-------|
| Category | 1 | 5 |
| Radio Measurement Action | 1 | 3 |
| Dialog Token | 1 | From request |
| TPC Report element | 4 | IE 35 (9.4.2.15) |
| Receive Antenna ID | 1 | (9.4.2.38) |
| Transmit Antenna ID | 1 | (9.4.2.38) |
| RCPI | 1 | (9.4.2.36) |
| RSNI | 1 | (9.4.2.39) |
| DMG Link Margin | variable | Optional (9.4.2.141) |
| DMG Link Adaptation Ack | variable | Optional (9.4.2.141.3) |
| Extended Link Measurement | variable | Optional |

**ns-3 status:** DONE. `LinkMeasurementReportHeader` in `link-measurement.h`. Does not implement optional DMG or Extended Link Measurement elements.

### 1.5 Neighbor Report Request (Action 4)

**Section 9.6.6.6, Figure 9-1189**

| Field | Octets | Notes |
|-------|--------|-------|
| Category | 1 | 5 |
| Radio Measurement Action | 1 | 4 |
| Dialog Token | 1 | Nonzero |
| SSID | variable | Optional SSID element. Absent = request for current ESS |
| LCI Measurement Request | variable | Optional IE 38 with Type=LCI(8). LocationSubject=Remote. Enable=0 |
| Location Civic Measurement Request | variable | Optional IE 38 with Type=Location Civic(11). Enable=0 |
| Neighbor DMG Request | variable | Optional IE 38 with Type=Neighboring DMG APs(17). Enable=0 |

**ns-3 status:** DONE. `NeighborReportRequestHeader` in `neighbor-report.h`. Optional Measurement Request elements deferred pending IE 38.

### 1.6 Neighbor Report Response (Action 5)

**Section 9.6.6.7, Figure 9-1190**

| Field | Octets | Notes |
|-------|--------|-------|
| Category | 1 | 5 |
| Radio Measurement Action | 1 | 5 |
| Dialog Token | 1 | From request, or 0 if unsolicited |
| Neighbor Report Elements | variable | Zero or more IE 52 instances |

Limited by max MMPDU size. Under some circumstances includes the reporting STA's own BSS (see 11.10.10.3).

**ns-3 status:** DONE. `NeighborReportResponseHeader` in `neighbor-report.h`. Carries dialog token and zero or more `NeighborReportElement` instances.

---

## 2. Measurement Request Element (IE 38)

**Section 9.4.2.19, Figure 9-241**

```
Element ID (1) | Length (1) | Measurement Token (1) | Measurement Request Mode (1) |
Measurement Type (1) | Measurement Request (variable)
```

IE define: `IE_MEASUREMENT_REQUEST = 38`

### 2.1 Measurement Request Mode (Figure 9-242)

| Bit | Field | Notes |
|-----|-------|-------|
| B0 | Parallel | 1 = start at same time as next MeasReq in frame |
| B1 | Enable | Differentiates measurement vs control request (Table 9-135) |
| B2 | Request | See Table 9-135 |
| B3 | Report | See Table 9-135 |
| B4 | Duration Mandatory | 0 = max duration, 1 = mandatory duration |
| B5-B7 | Reserved | |

**Enable/Request/Report semantics (Table 9-135):**
- Enable=0: Request and Report are reserved. Standard measurement request.
- Enable=1, Req=0, Rep=0: Stop sending requests/reports of this type.
- Enable=1, Req=1, Rep=0: Accept requests, stop autonomous/triggered reports.
- Enable=1, Req=0, Rep=1: Stop requests, accept autonomous/triggered reports.
- Enable=1, Req=1, Rep=1: Accept both requests and reports.

### 2.2 Measurement Types (Table 9-136)

Types 0-2 are spectrum management only (not 802.11k). Types 3-17 are radio measurement (802.11k).

| Type | Name | Request Section | Request Figure |
|------|------|-----------------|----------------|
| 0 | Basic | 9.4.2.19.2 | 9-243 |
| 1 | CCA | 9.4.2.19.3 | 9-244 |
| 2 | RPI Histogram | 9.4.2.19.4 | 9-245 |
| 3 | Channel Load | 9.4.2.19.5 | 9-246 |
| 4 | Noise Histogram | 9.4.2.19.6 | 9-248 |
| 5 | Beacon | 9.4.2.19.7 | 9-250 |
| 6 | Frame | 9.4.2.19.8 | 9-252 |
| 7 | STA Statistics | 9.4.2.19.9 | 9-253 |
| 8 | LCI | 9.4.2.19.10 | 9-260 |
| 9 | Transmit Stream/Category | 9.4.2.19.11 | 9-266 |
| 10 | Multicast Diagnostics | 9.4.2.19.12.2 | 9-273 |
| 11 | Location Civic | 9.4.2.19.12.3 | 9-275 |
| 12 | Location Identifier | 9.4.2.19.12.4 | 9-277 |
| 13 | Directional Channel Quality | 9.4.2.19.12.5 | 9-278 |
| 14 | Directional Measurement | 9.4.2.19.12.6 | 9-284 |
| 15 | Directional Statistics | 9.4.2.19.12.7 | 9-286 |
| 16 | FTM Range | 9.4.2.19.12.8 | 9-288 |
| 17 | Neighboring DMG APs | — | — |
| 255 | Measurement Pause | 9.4.2.19.12.1 | — |

### 2.3 Type-Specific Request Fields

#### Type 3: Channel Load Request (Section 9.4.2.19.5, Figure 9-246)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | Annex E channel set |
| Channel Number | 1 | |
| Randomization Interval | 2 | Max random delay before measurement, in TUs |
| Measurement Duration | 2 | In TUs |
| Optional Subelements | variable | Table 9-137 |

**Optional subelements (Table 9-137):**
- ID 1: Channel Load Reporting (Figure 9-247) -- Reporting Condition(1) + Reference Value(1). Conditions in Table 9-138.
- ID 163: Wide Bandwidth Channel Switch
- ID 221: Vendor Specific

#### Type 4: Noise Histogram Request (Section 9.4.2.19.6, Figure 9-248)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| Randomization Interval | 2 | In TUs |
| Measurement Duration | 2 | In TUs |
| Optional Subelements | variable | Table 9-139 |

**Optional subelements (Table 9-139):**
- ID 1: Noise Histogram Reporting (Figure 9-249) -- Reporting Condition(1) + ANPI Reference Value(1). Conditions in Table 9-140.
- ID 163: Wide Bandwidth Channel Switch
- ID 221: Vendor Specific

#### Type 5: Beacon Request (Section 9.4.2.19.7, Figure 9-250)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | 0 or 255 = special, else specific channel |
| Randomization Interval | 2 | In TUs |
| Measurement Duration | 2 | In TUs |
| Measurement Mode | 1 | 0=Passive, 1=Active, 2=Beacon Table (Table 9-141) |
| BSSID | 6 | FF:FF:FF:FF:FF:FF = wildcard |
| Optional Subelements | variable | Table 9-142 |

**Optional subelements (Table 9-142):**
- ID 0: SSID (target SSID, absent = wildcard)
- ID 1: Beacon Reporting (Figure 9-251) -- Reporting Condition(1) + Threshold/Offset(1). Conditions in Table 9-143.
- ID 2: Reporting Detail -- 0=no fixed fields/elements, 1=all fixed fields and requested elements, 2=all fields and elements (Table 9-144)
- ID 10: Request element (list of requested IE IDs)
- ID 11: Extended Request element
- ID 51: AP Channel Report
- ID 163: Wide Bandwidth Channel Switch
- ID 164: Last Beacon Report Indication Request
- ID 221: Vendor Specific

#### Type 6: Frame Request (Section 9.4.2.19.8, Figure 9-252)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| Randomization Interval | 2 | In TUs |
| Measurement Duration | 2 | In TUs |
| Frame Request Type | 1 | 1 = Frame Count Report |
| MAC Address | 6 | TA to match |
| Optional Subelements | variable | Table 9-145 |

#### Type 7: STA Statistics Request (Section 9.4.2.19.9, Figure 9-253)

| Field | Octets | Notes |
|-------|--------|-------|
| Peer MAC Address | 6 | |
| Randomization Interval | 2 | In TUs |
| Measurement Duration | 2 | In TUs |
| Group Identity | 1 | Table 9-146: 0=dot11Counters, 1=dot11MACStatistics, etc. |
| Optional Subelements | variable | Table 9-147 |

**Group Identity values (Table 9-146):**
- 0: dot11CountersTable
- 1: dot11MACStatistics
- 2-9: dot11QosCountersTable (per UP)
- 10: dot11BSSAverageAccessDelay
- 11-15: dot11RSNAStatsTable (various)
- 16-255: Reserved

#### Type 8: LCI Request (Section 9.4.2.19.10, Figure 9-260)

| Field | Octets | Notes |
|-------|--------|-------|
| Location Subject | 1 | Table 9-148: 0=Local, 1=Remote, 2=Third Party |
| Optional Subelements | variable | Table 9-149 |

Note: No Randomization Interval or Duration fields. Duration Mandatory bit is reserved for LCI.

**Optional subelements (Table 9-149):**
- ID 1: Azimuth Request (Figure 9-261)
- ID 2: Originator Requesting STA MAC Address
- ID 3: Target MAC Address
- ID 221: Vendor Specific

#### Type 9: Transmit Stream/Category Measurement Request (Section 9.4.2.19.11, Figure 9-266)

| Field | Octets | Notes |
|-------|--------|-------|
| Randomization Interval | 2 | In TUs, 0 if triggered |
| Measurement Duration | 2 | In TUs, 0 if triggered |
| Peer STA Address | 6 | |
| Traffic Identifier | 1 | TID |
| Bin 0 Range | 1 | Defines delay range for Bin 0 |
| Optional Subelements | variable | Table 9-150 |

**Optional subelements (Table 9-150):**
- ID 1: Triggered Reporting (Figure 9-267) -- condition + threshold fields
- ID 221: Vendor Specific

#### Type 10: Multicast Diagnostics Request (Section 9.4.2.19.12.2, Figure 9-273)

| Field | Octets | Notes |
|-------|--------|-------|
| Randomization Interval | 2 | |
| Measurement Duration | 2 | |
| Group MAC Address | 6 | |
| Optional Subelements | variable | Table 9-153 |

#### Type 11: Location Civic Request (Section 9.4.2.19.12.3, Figure 9-275)

| Field | Octets | Notes |
|-------|--------|-------|
| Location Subject | 1 | Table 9-148 |
| Civic Location Type | 1 | Table 9-154 |
| Location Service Interval Units | 1 | Table 9-155 |
| Location Service Interval | 2 | |
| Optional Subelements | variable | Table 9-156 |

#### Type 12: Location Identifier Request (Section 9.4.2.19.12.4, Figure 9-277)

| Field | Octets | Notes |
|-------|--------|-------|
| Location Subject | 1 | Table 9-148 |
| Location Service Interval Units | 1 | Table 9-155 |
| Location Service Interval | 2 | |
| Optional Subelements | variable | Table 9-157 |

#### Type 13: Directional Channel Quality Request (Section 9.4.2.19.12.5, Figure 9-278)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| AID | 2 | |
| Randomization Interval | 2 | |
| Measurement Duration | 2 | |
| Optional Subelements | variable | Table 9-158 |

#### Type 14: Directional Measurement Request (Section 9.4.2.19.12.6, Figure 9-284)

| Field | Octets | Notes |
|-------|--------|-------|
| Randomization Interval | 2 | |
| Measurement Duration | 2 | |
| Optional Subelements | variable | Table 9-160 |

#### Type 15: Directional Statistics Request (Section 9.4.2.19.12.7, Figure 9-286)

| Field | Octets | Notes |
|-------|--------|-------|
| Randomization Interval | 2 | |
| Measurement Duration | 2 | |
| Optional Subelements | variable | Table 9-161 |

#### Type 16: FTM Range Request (Section 9.4.2.19.12.8, Figure 9-288)

| Field | Octets | Notes |
|-------|--------|-------|
| Randomization Interval | 2 | |
| Minimum AP Count | 1 | |
| Optional Subelements | variable | Table 9-162 |

**Subelements (Table 9-162):**
- ID 4: Neighbor Report (IE 52 instances listing target APs for FTM)
- ID 6: Maximum Age subelement
- ID 221: Vendor Specific

#### Type 255: Measurement Pause Request (Section 9.4.2.19.12.1)

| Field | Octets | Notes |
|-------|--------|-------|
| Pause Time | 1 | In units of 10 TUs |
| Optional Subelements | variable | Table 9-152 |

---

## 3. Measurement Report Element (IE 39)

**Section 9.4.2.20, Figure 9-289**

```
Element ID (1) | Length (1) | Measurement Token (1) | Measurement Report Mode (1) |
Measurement Type (1) | Measurement Report (variable)
```

IE define: `IE_MEASUREMENT_REPORT = 39`

### 3.1 Measurement Report Mode (Figure 9-290)

| Bit | Field |
|-----|-------|
| B0 | Late (measurement request received too late) |
| B1 | Incapable (STA cannot generate this report type) |
| B2 | Refused (STA refuses to generate report) |
| B3-B7 | Reserved |

At most one of Late/Incapable/Refused may be set. If any is set, the Measurement Report field is absent.

### 3.2 Report Types (Table 9-163)

Same type numbering as Table 9-136.

| Type | Name | Report Section | Report Figure |
|------|------|----------------|---------------|
| 0 | Basic | 9.4.2.20.2 | 9-291 |
| 1 | CCA | 9.4.2.20.3 | 9-293 |
| 2 | RPI Histogram | 9.4.2.20.4 | 9-294 |
| 3 | Channel Load | 9.4.2.20.5 | 9-295 |
| 4 | Noise Histogram | 9.4.2.20.6 | 9-296 |
| 5 | Beacon | 9.4.2.20.7 | 9-297 |
| 6 | Frame | 9.4.2.20.8 | 9-300 |
| 7 | STA Statistics | 9.4.2.20.9 | 9-303 |
| 8 | LCI | 9.4.2.20.10 | 9-312 |
| 9 | Transmit Stream/Category | 9.4.2.20.11 | 9-328 |
| 10 | Multicast Diagnostics | 9.4.2.20.12.2 | 9-330 |
| 11 | Location Civic | 9.4.2.20.12.3 | 9-331 |
| 12 | Location Identifier | 9.4.2.20.12.4 | 9-332 |
| 13 | Directional Channel Quality | 9.4.2.20.12.5 | 9-349 |
| 14 | Directional Measurement | 9.4.2.20.12.6 | 9-357 |
| 15 | Directional Statistics | 9.4.2.20.12.7 | 9-360 |
| 16 | FTM Range | 9.4.2.20.12.8 | 9-361 |

### 3.3 Type-Specific Report Fields

#### Type 3: Channel Load Report (Section 9.4.2.20.5, Figure 9-295)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| Actual Measurement Start Time | 8 | TSF at measurement start |
| Measurement Duration | 2 | In TUs |
| Channel Load | 1 | Proportion of time channel busy (0-255). See 11.10.9.3 |
| Optional Subelements | variable | Table 9-165 |

**Optional subelements (Table 9-165):**
- ID 163: Wide Bandwidth Channel Switch
- ID 221: Vendor Specific

#### Type 4: Noise Histogram Report (Section 9.4.2.20.6, Figure 9-296)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| Actual Measurement Start Time | 8 | TSF |
| Measurement Duration | 2 | In TUs |
| Antenna ID | 1 | (9.4.2.38) |
| ANPI | 1 | Average Noise Plus Interference (dBm, 0-255) |
| IPI 0 Density | 1 | IPI <= -92 dBm |
| IPI 1 Density | 1 | -92 < IPI <= -89 dBm |
| IPI 2 Density | 1 | -89 < IPI <= -86 dBm |
| IPI 3 Density | 1 | -86 < IPI <= -83 dBm |
| IPI 4 Density | 1 | -83 < IPI <= -80 dBm |
| IPI 5 Density | 1 | -80 < IPI <= -75 dBm |
| IPI 6 Density | 1 | -75 < IPI <= -70 dBm |
| IPI 7 Density | 1 | -70 < IPI <= -65 dBm |
| IPI 8 Density | 1 | -65 < IPI <= -60 dBm |
| IPI 9 Density | 1 | -60 < IPI <= -55 dBm |
| IPI 10 Density | 1 | -55 dBm < IPI |
| Optional Subelements | variable | Table 9-167 |

IPI level definitions from Table 9-166. Each density is proportion of measurement duration at that power level.

**Optional subelements (Table 9-167):**
- ID 163: Wide Bandwidth Channel Switch
- ID 221: Vendor Specific

#### Type 5: Beacon Report (Section 9.4.2.20.7, Figure 9-297)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| Actual Measurement Start Time | 8 | TSF |
| Measurement Duration | 2 | In TUs |
| Reported Frame Information | 1 | Figure 9-298: PHY Type(7 bits) + Condensed PHY Type(1 bit) |
| RCPI | 1 | (9.4.2.36) |
| RSNI | 1 | (9.4.2.39) |
| BSSID | 6 | |
| Antenna ID | 1 | (9.4.2.38) |
| Parent TSF | 4 | Lower 32 bits of serving AP's TSF |
| Optional Subelements | variable | Table 9-168 |

**Optional subelements (Table 9-168):**
- ID 1: Reported Frame Body (beacon/probe response body fragment)
- ID 2: Reported Frame Body Fragment ID (Figure 9-299)
- ID 163: Wide Bandwidth Channel Switch
- ID 164: Last Beacon Report Indication
- ID 221: Vendor Specific

#### Type 6: Frame Report (Section 9.4.2.20.8, Figure 9-300)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| Actual Measurement Start Time | 8 | TSF |
| Measurement Duration | 2 | In TUs |
| Optional Subelements | variable | Table 9-169 |

**Includes Frame Count Report subelements (Figure 9-301):**
- Transmit Address(6), BSSID(6), PHY Type(1), Average RCPI(1), Last RSNI(1), Last RCPI(1), Antenna ID(1), Frame Count(2)

#### Type 7: STA Statistics Report (Section 9.4.2.20.9, Figure 9-303)

| Field | Octets | Notes |
|-------|--------|-------|
| Measurement Duration | 2 | In TUs |
| Group Identity | 1 | Table 9-170 (same as Table 9-146) |
| Statistics Group Data | variable | Figures 9-304 to 9-308 depending on GroupID |
| Optional Subelements | variable | Table 9-171 |

**Group Data formats:**
- GroupID 0 (dot11Counters, Figure 9-304): 8 counters x 4 octets = 32 octets
- GroupID 1 (dot11MACStatistics, Figure 9-305): 7 counters x 4 octets = 28 octets
- GroupID 2-9 (dot11QosCounters, Figure 9-306): per-UP counters
- GroupID 10 (dot11BSSAverageAccessDelay, Figure 9-307): delay metrics
- GroupID 11-15 (RSNA, Figure 9-308): security counters

#### Type 8: LCI Report (Section 9.4.2.20.10, Figure 9-312)

Complex structure with multiple subelements (Table 9-172):
- ID 0: LCI subelement (Figure 9-313) -- contains LCI field (Figure 9-314): latitude, longitude, altitude with uncertainty encoding
- ID 1: Z subelement (Figure 9-317) -- floor/altitude info
- ID 2: Usage Rules/Policy (Figure 9-322)
- ID 3: Colocated BSSID List (Figure 9-323)
- ID 4: Azimuth Report (Figures 9-315, 9-316)
- ID 221: Vendor Specific

#### Type 9: Transmit Stream/Category Report (Section 9.4.2.20.11, Figure 9-328)

| Field | Octets | Notes |
|-------|--------|-------|
| Actual Measurement Start Time | 8 | TSF |
| Measurement Duration | 2 | In TUs |
| Peer STA Address | 6 | |
| Traffic Identifier | 1 | |
| Bin 0 Range | 1 | |
| Transmitted MSDU Count | 4 | |
| MSDU Discarded Count | 4 | |
| MSDU Failed Count | 4 | |
| MSDU Multiple Retry Count | 4 | |
| QoS CF-Polls Lost Count | 4 | |
| Average Queue Delay | 4 | |
| Average Transmit Delay | 4 | |
| Bin 0-5 | 4 each = 24 | Delay histogram bins |
| Optional Subelements | variable | Table 9-174 |

#### Type 10: Multicast Diagnostics Report (Section 9.4.2.20.12.2, Figure 9-330)

| Field | Octets | Notes |
|-------|--------|-------|
| Measurement Duration | 2 | |
| Group MAC Address | 6 | |
| Multicast Diagnostics Reason | 1 | |
| Multicast Diagnostics Result | 4 | |
| Optional Subelements | variable | Table 9-175 |

#### Type 11: Location Civic Report (Section 9.4.2.20.12.3, Figure 9-331)

| Field | Octets | Notes |
|-------|--------|-------|
| Location Civic Type | 1 | Table 9-154 |
| Location Civic Subelements | variable | Table 9-177 |

#### Type 12: Location Identifier Report (Section 9.4.2.20.12.4, Figure 9-332)

| Field | Octets | Notes |
|-------|--------|-------|
| Expiration | 2 | |
| Public ID URI Subelement | variable | |
| Subelements | variable | Table 9-180 |

#### Type 13: Directional Channel Quality Report (Section 9.4.2.20.12.5, Figure 9-349)

| Field | Octets | Notes |
|-------|--------|-------|
| Operating Class | 1 | |
| Channel Number | 1 | |
| AID | 2 | |
| Actual Measurement Start Time | 8 | |
| Measurement Duration | 2 | |
| Directional Channel Quality Reports | variable | Per-direction quality data |
| Optional Subelements | variable | Table 9-182 |

#### Type 14: Directional Measurement Report (Section 9.4.2.20.12.6, Figure 9-357)

| Field | Octets | Notes |
|-------|--------|-------|
| Actual Measurement Start Time | 8 | |
| Measurement Duration | 2 | |
| Directional Measurement Reports | variable | |
| Optional Subelements | variable | Table 9-184 |

#### Type 15: Directional Statistics Report (Section 9.4.2.20.12.7, Figure 9-360)

| Field | Octets | Notes |
|-------|--------|-------|
| Measurement Duration | 2 | |
| Group Identity | 1 | |
| Statistics Group Data | variable | |
| Optional Subelements | variable | Table 9-185 |

#### Type 16: FTM Range Report (Section 9.4.2.20.12.8, Figure 9-361)

| Field | Octets | Notes |
|-------|--------|-------|
| Range Entry Count | variable | |
| Range Entries | variable | Per-AP results: BSSID, range, max range error, etc. |
| Error Code | 1 | Table 9-186 |
| Optional Subelements | variable | Table 9-187 |

---

## 4. RM Enabled Capabilities Element (IE 70)

**Section 9.4.2.43, Figure 9-432**

```
Element ID (1) | Length (1) | RM Enabled Capabilities (5)
```

IE define: `IE_RM_ENABLED_CAPACITIES = 70`

5-octet (40-bit) capability bitmap. Key bits from Table 9-218:

| Bit | Field |
|-----|-------|
| 0 | Link Measurement Capability Enabled |
| 1 | Neighbor Report Capability Enabled |
| 2 | Parallel Measurements Capability Enabled |
| 3 | Repeated Measurements Capability Enabled |
| 4 | Beacon Passive Measurement Capability Enabled |
| 5 | Beacon Active Measurement Capability Enabled |
| 6 | Beacon Table Measurement Capability Enabled |
| 7 | Beacon Measurement Reporting Conditions Capability Enabled |
| 8 | Frame Measurement Capability Enabled |
| 9 | Channel Load Measurement Capability Enabled |
| 10 | Noise Histogram Measurement Capability Enabled |
| 11 | STA Statistics Measurement Capability Enabled |
| 12 | LCI Measurement Capability Enabled |
| 13 | LCI Azimuth Capability Enabled |
| 14 | Transmit Stream/Category Measurement Capability Enabled |
| 15 | Triggered Transmit Stream/Category Measurement Capability Enabled |
| 16 | AP Channel Report Capability Enabled |
| 17 | RM MIB Capability Enabled |
| 18-20 | Operating Channel Max Measurement Duration (3 bits) |
| 21-23 | Nonoperating Channel Max Measurement Duration (3 bits) |
| 24-26 | Measurement Pilot Capability (3 bits) |
| 27 | Measurement Pilot Transmission Information Capability Enabled |
| 28 | Neighbor Report TSF Offset Capability Enabled |
| 29 | RCPI Measurement Capability Enabled |
| 30 | RSNI Measurement Capability Enabled |
| 31 | BSS Average Access Delay Capability Enabled |
| 32 | BSS Available Admission Capacity Capability Enabled |
| 33 | Antenna Capability Enabled |
| 34 | FTM Range Report Capability Enabled |
| 35 | Civic Location Measurement Capability Enabled |
| 36-39 | Reserved |

**ns-3 status:** IE define exists. No class implementation.

---

## 5. Neighbor Report Element (IE 52)

**Section 9.4.2.35, Figure 9-416**

**ns-3 status:** DONE. `NeighborReportElement` in `neighbor-report-element.h`.

Implemented subelements: TSF Information (1), Condensed Country String (2), BSS Transition Candidate Preference (3), BSS Termination Duration (4), Bearing (5), Wide Bandwidth Channel (6), HT Capabilities (45), HT Operation (61), VHT Capabilities (191), VHT Operation (192), Vendor Specific (221).

---

## 6. TPC Report Element (IE 35)

**Section 9.4.2.15**

**ns-3 status:** DONE. `TpcReportElement` in `tpc-report-element.h`.

---

## 7. Encoding Rules

**Time units:**
- TU (Time Unit) = 1024 microseconds
- TSF (Timing Synchronization Function) = microsecond resolution, 8 octets

**Measurement Duration:** Always in TUs.

**Randomization Interval:** Max random delay before starting measurement, in TUs.

**RCPI (Section 9.4.2.36):** 0 = power < -109.5 dBm, 220 = power >= 0 dBm, 255 = not available. Encoded as 0.5 dB steps from -110 dBm.

**RSNI (Section 9.4.2.39):** 0 = -10 dB, 255 = not available. Encoded as 0.5 dB steps from -10 dB.

**Channel Load:** 0-255 representing fraction of measurement duration where CCA indicated busy. 255 = 100%.

**ANPI:** Average Noise Plus Interference in dBm, encoded per 11.10.9.4.

---

## 8. Implementation Summary

### Already done

| Item | Class | File |
|------|-------|------|
| Link Measurement Request (Action 2) | `LinkMeasurementRequestHeader` | `link-measurement.h` |
| Link Measurement Report (Action 3) | `LinkMeasurementReportHeader` | `link-measurement.h` |
| TPC Report Element (IE 35) | `TpcReportElement` | `tpc-report-element.h` |
| Neighbor Report Element (IE 52) | `NeighborReportElement` | `neighbor-report-element.h` |
| Neighbor Report Request (Action 4) | `NeighborReportRequestHeader` | `neighbor-report.h` |
| Neighbor Report Response (Action 5) | `NeighborReportResponseHeader` | `neighbor-report.h` |
| WifiActionHeader enums | `RadioMeasurementActionValue` | `mgt-action-headers.h` |

### Needs implementation

**Action Frame Bodies:**
1. `RadioMeasurementRequestHeader` (Action 0)
2. `RadioMeasurementReportHeader` (Action 1)

**Information Elements:**
1. `MeasurementRequestElement` (IE 38) -- generic container
2. `MeasurementReportElement` (IE 39) -- generic container
3. `RmEnabledCapabilities` (IE 70) -- 5-octet bitmap

**Measurement Type Request Fields (14 radio measurement types):**
- Type 3: Channel Load Request
- Type 4: Noise Histogram Request
- Type 5: Beacon Request
- Type 6: Frame Request
- Type 7: STA Statistics Request
- Type 8: LCI Request
- Type 9: Transmit Stream/Category Request
- Type 10: Multicast Diagnostics Request
- Type 11: Location Civic Request
- Type 12: Location Identifier Request
- Type 13: Directional Channel Quality Request
- Type 14: Directional Measurement Request
- Type 15: Directional Statistics Request
- Type 16: FTM Range Request
- Type 255: Measurement Pause

**Measurement Type Report Fields (14 radio measurement types):**
- Type 3: Channel Load Report
- Type 4: Noise Histogram Report
- Type 5: Beacon Report
- Type 6: Frame Report
- Type 7: STA Statistics Report
- Type 8: LCI Report
- Type 9: Transmit Stream/Category Report
- Type 10: Multicast Diagnostics Report
- Type 11: Location Civic Report
- Type 12: Location Identifier Report
- Type 13: Directional Channel Quality Report
- Type 14: Directional Measurement Report
- Type 15: Directional Statistics Report
- Type 16: FTM Range Report
