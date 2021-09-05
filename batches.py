import pandas as pd

b = 4096
j_batches = 1
j = 1

df = pd.read_csv ('infinite_samples.csv')
n = df[df.columns[0]].count()

batches = []

sample_mean = 0
sample_variance = 0

#Calc sample mean
for i in range(0,n):
	sample_mean += df[df.columns[0]].loc[i]

sample_mean = sample_mean/n

#Calc sample variance
for i in range(0,n):
	sample_variance += pow(df[df.columns[0]].loc[i] - sample_mean, 2)

sample_variance = sample_variance/n

print(f"N samples: {n}")
print(f"Sample mean: {sample_mean}")
print(f"Sample variance: {sample_variance}")

autocorrelation = 0

for i in range(0, n - j):
	x = df[df.columns[0]].loc[i]
	y = df[df.columns[0]].loc[i + j]
	autocorrelation += (x - sample_mean) * (y - sample_mean)

autocorrelation = autocorrelation/(n - j)
print(f"Autocorrelation for {j}: {autocorrelation}")

for i in range(0,n,b):
	sum_batches = 0

	if(i+b < n):
		for x in range(i,i+b):
			sum_batches += df[df.columns[0]].loc[x]
		batches.append(sum_batches/b)

sample_mean_batches = 0
autocorrelation_batches = 0

for i in range(0, len(batches)):
	sample_mean_batches += batches[i]
sample_mean_batches = sample_mean_batches/len(batches)

for i in range(0,len(batches) - j_batches):
	autocorrelation_batches += (batches[i] - sample_mean_batches) * (batches[i+j_batches] - sample_mean_batches)

autocorrelation_batches = autocorrelation_batches/(n-j_batches)

print(f"\nBatch size: {b} K: {n/b}")
print(f"Batche mean: {sample_mean_batches}")
print(f"Autocorrelation batches with lag {j_batches}: {autocorrelation_batches}")

r = autocorrelation_batches/sample_variance

print(f"r{j}:{r}")