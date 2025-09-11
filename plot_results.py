
import pandas as pd
import matplotlib.pyplot as plt
import glob
import os

files = glob.glob("assets/results_*.csv")

data = []
for file in files:
    base = os.path.basename(file)
    parts = base.replace("results_", "").replace(".csv", "").split("_")
    nth, ntasks = int(parts[0]), int(parts[1])

    df = pd.read_csv(file, header=None,
                     names=["width","height","channels","NTHREADS","NTASKS","time_ms"])

    mean = df["time_ms"].mean()
    std = df["time_ms"].std()
    runs = len(df)

    data.append([nth, ntasks, mean, std, runs])

agg = pd.DataFrame(data, columns=["NTHREADS","NTASKS","mean_time","std_time","runs"])
agg = agg.sort_values(["NTHREADS","NTASKS"]).reset_index(drop=True)

print("Resumo das execuções:")
print(agg)

baseline = agg[(agg["NTHREADS"]==32) & (agg["NTASKS"]==32)]["mean_time"].values[0]

agg["speedup"] = baseline / agg["mean_time"]
agg["efficiency"] = agg["speedup"] / (agg["NTHREADS"]/16)

plt.errorbar(
    agg.index, agg["mean_time"], yerr=agg["std_time"],
    fmt="o-", capsize=5
)
plt.xticks(agg.index, [f"{r.NTHREADS}×{r.NTASKS}" for _,r in agg.iterrows()], rotation=45)
plt.ylabel("Tempo médio (ms)")
plt.title("Tempo de processamento por configuração")
plt.grid(True)
plt.tight_layout()
plt.savefig("tempo_config.png", dpi=150)
plt.show()

plt.bar(agg.index, agg["speedup"])
plt.xticks(agg.index, [f"{r.NTHREADS}×{r.NTASKS}" for _,r in agg.iterrows()], rotation=45)
plt.ylabel("Speedup (baseline 16×32)")
plt.title("Speedup por configuração")
plt.grid(True, axis="y")
plt.tight_layout()
plt.savefig("speedup_config.png", dpi=150)
plt.show()

plt.plot(agg.index, agg["efficiency"], "s-")
plt.xticks(agg.index, [f"{r.NTHREADS}×{r.NTASKS}" for _,r in agg.iterrows()], rotation=45)
plt.ylabel("Eficiência (%)")
plt.title("Eficiência de paralelização")
plt.grid(True)
plt.tight_layout()
plt.savefig("eficiencia_config.png", dpi=150)
plt.show()
