## Objective

In a quasi-Earth fixed cell NR-NTN environments,
having UEs with different access capabilities and with different delay budget requirements,
find an optimal resource allocation strategy (access type and time-frequency domain resources) by a target satellite

## Inputs

- Altitude
- Elevation angle

- Footprint radius
- Cell (spot beam) radius
- Number of active cells at a time
- Bandwidth of a cell
- Incoming UEs
	- Supported access type & capabilities
		- 4-step Contention-based/-free Random Access (CBRA/CFRA) (mandatory)
		- 2-step CBRA/CFRA (optional)
		- RACH-less Configured Grant (CG) (optional)
		- RACH-less Dynamic Grant (DG) (optional)
	- Delay budget requirement
- Incumbent UEs
	- Throughput requirements
- (Random access related available resources)

## Derived inputs

- Propagation delay (derived from altitude and elevation angle)
- Total number of cells (derived from radii)

## Outputs

- Selected access type for each UE
- Selected resource (time-frequency domain) for each UE

## Observations & Measurements (KPI)

- RACH re-attempts count
- Handover success/failure rate (failure: max RACH re-attempt reached, RACH-less CG/DG timeout)
- Time to handover completion
- Delay budget violation rate

## Out of scope

- Satellite movements
- Doppler shift
- (Conditional) handover preparation procedure

## Abstracted Simulation Design

TBD
