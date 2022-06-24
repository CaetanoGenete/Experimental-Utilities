import json
import matplotlib.pyplot as plt
import numpy as np

file = open("geometric_growth_comp.json")
data = json.load(file)

darray_results = []
vector_results = []
sizes = []

for bench in data['benchmarks']:
    time = float(bench['real_time'])
    name = bench['name']

    
    if "darray" in name:
        sizes.append(int(name.split("/")[1]))
        darray_results.append(time)
    else:
        vector_results.append(time)

plt.figure()
plt.xlabel("Number of push backs")
plt.ylabel("Time elapsed (ns)")
plt.plot(np.log(sizes), np.log(darray_results), label = "darray<int>")
plt.plot(np.log(sizes), np.log(vector_results), label = "vector<int>")
plt.legend()
plt.grid()
plt.show()
