import sys

import matplotlib.pyplot as plt
import numpy as np

# 1. Use the exact aesthetic from the CAKE paper
plt.style.use("ggplot")

# 2. Colors perfectly extracted from the official Figure 3
colors = {
    "cake": "#4DB69A",
    "cake_dst": "#E28336",
    "cake_src": "#9681B4",
    "cake_triple": "#E75B8D",
    "fq_codel": "#85B24A",
}

# 3. Read your simulation data
data = {}
try:
    with open("cake-fig3-results.dat", "r") as f:
        for line in f:
            parts = line.strip().split()
            if not parts or parts[0].startswith("#"):
                continue
            # Extract the mode name and its corresponding flow values
            data[parts[0]] = [float(x) for x in parts[1:]]
except FileNotFoundError:
    print("Error: cake-fig3-results.dat not found in the current directory.")
    sys.exit(1)

modes = ["cake", "cake_dst", "cake_src", "cake_triple", "fq_codel"]

# 4. Create the 1x5 facet grid (sharing the Y-axis just like the paper)
fig, axes = plt.subplots(1, 5, figsize=(10, 4), sharey=True)
fig.subplots_adjust(wspace=0.0)  # Removes gaps between subplots

# Fallback labels (dynamically scales to how many columns your .dat file actually has)
x_labels = ["A->A", "A->B", "A->C", "A->D", "B->C", "B->D"]

for i, (ax, mode) in enumerate(zip(axes, modes)):
    if mode not in data:
        continue

    vals = data[mode]
    x_pos = np.arange(len(vals))

    # Plot the solid colored bars
    ax.bar(x_pos, vals, color=colors.get(mode, "#333333"), width=0.9)

    # Plot the black error-bar "caps" on top of the bars
    ax.plot(x_pos, vals, "_", color="black", markersize=12, markeredgewidth=2)

    # Set titles and X-axis labels
    ax.set_title(mode, fontsize=12)
    ax.set_xticks(x_pos)
    ax.set_xticklabels(x_labels[: len(vals)], rotation=90, fontsize=10)

    # Add the vertical dotted separator lines between modes
    if i > 0:
        ax.axvline(x=-0.5, color="gray", linestyle=":", linewidth=1.5)

# 5. Y-Axis Formatting
axes[0].set_ylabel("Mbits/s", fontsize=12, fontweight="bold")
axes[0].set_ylim(0, 2.65)
axes[0].set_yticks(np.arange(0, 3.0, 0.5))

# 6. Save the final image
plt.savefig("cake-fig3-pyplot.png", bbox_inches="tight", dpi=300)
print("Graph successfully generated: cake-fig3-pyplot.png")
