import pandas as pd

b = 1024
j = 1

df = pd.read_csv ('infinite_samples.csv')
n = df[df.columns[0]].count()

batches = []

sample_mean = 0;
sample_variance = 0;

#Calc sample mean
for i in range(0,n):
	sample_mean += df[df.columns[0]].loc[i]

sample_mean = sample_mean/n

#Calc sample variance
for i in range(0,n):
	sample_variance += pow(df[df.columns[0]].loc[i] - sample_mean, 2)

sample_variance = sample_variance/n

print(f"N samples: {n}")
print(f"Batch size: {b} K: {n/b}")
print(f"Sample mean: {sample_mean}")
print(f"Sample variance: {sample_variance}")

for i in range(0,n,b):
	sum_batches = 0

	if(i+b < n):
		for x in range(i,i+b):
			sum_batches += df[df.columns[0]].loc[x]
		batches.append(sum_batches/b)

autocorrelation = 0;

for i in range(0,len(batches) - j):
	autocorrelation += (batches[i] - sample_mean) * (batches[i+j] - sample_mean)

autocorrelation = autocorrelation/(n-j)

print(f"Autocorrelation: {autocorrelation}")

r = autocorrelation/sample_variance

print(f"r{j}:{r}")