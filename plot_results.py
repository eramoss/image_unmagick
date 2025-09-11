import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("results.csv", header=None,
                 names=["width","height","channels","NTHREADS","NTASKS","time_ms"])

agg = df.groupby(["width","height","NTHREADS","NTASKS"]).agg(
    mean_time=("time_ms","mean"),
    std_time=("time_ms","std"),
    runs=("time_ms","count")
).reset_index()

print("Resumo das execuções (médias):")
print(agg.head())

for size in agg.groupby(["width","height"]).groups.keys():
    subset = agg[(agg["width"]==size[0]) & (agg["height"]==size[1])]
    plt.errorbar(subset["NTHREADS"], subset["mean_time"],
                 yerr=subset["std_time"], fmt="-o", capsize=5,
                 label=f"{size[0]}x{size[1]}")

plt.xlabel("Número de Threads")
plt.ylabel("Tempo médio de processamento (ms)")
plt.title("Escalabilidade por Threads (com média de execuções)")
plt.legend()
plt.grid(True)
plt.show()

for size in agg.groupby(["width","height"]).groups.keys():
    subset = agg[(agg["width"]==size[0]) & (agg["height"]==size[1])]
    plt.errorbar(subset["NTASKS"], subset["mean_time"],
                 yerr=subset["std_time"], fmt="-s", capsize=5,
                 label=f"{size[0]}x{size[1]}")

plt.xlabel("Número de Tasks (fatias)")
plt.ylabel("Tempo médio de processamento (ms)")
plt.title("Efeito do particionamento em Tasks (com média de execuções)")
plt.legend()
plt.grid(True)
plt.show()
