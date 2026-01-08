import numpy as np
import matplotlib.pyplot as plt

with open('debug_printout.txt') as f:
    lines = f.readlines()

data = [i.strip() for i in lines]

p_data = [i[3:].split('|')[0].strip().split() for i in data if i.startswith('p:')]

p_data = [[int(j) for j in i] for i in p_data]

arr = np.zeros((len(p_data), len(p_data[0])))

for row in range(len(p_data)):
    arr[row] = p_data[row]

arr = np.swapaxes(arr, 0, 1)

normalized = np.zeros(arr.shape)

for row in range(len(arr)):
    normalized[row] = arr[row] / max(arr[row])
plt.subplot(4,1,1)
plt.imshow(normalized, aspect='auto')

widths = arr[6]

xTicksPos = np.concatenate(([0], np.cumsum(widths)))[:-1] / np.sum(widths)
xTicksLabels = ["Sh", "e", "S", "e", "ll", "s", "S", "ea", "Sh", "e", "ll", "s", "B", "y", "Th", "e", "S", "ea", "Sh", "ore", ".", ""]
# cumulative edges
edges = np.concatenate(([0], np.cumsum(widths)))

def forward(x):
    """
    Map index-space x (pixel edges at integers)
    to physical x using column widths.
    """
    x = np.asarray(x)

    i = np.floor(x).astype(int)
    i = np.clip(i, 0, len(widths) - 1)

    frac = x - i
    return edges[i] + frac * widths[i]

def inverse(x):
    """
    Map physical x back to index-space.
    """
    x = np.asarray(x)

    i = np.searchsorted(edges, x, side="right") - 1
    i = np.clip(i, 0, len(widths) - 1)

    return i + (x - edges[i]) / widths[i]



for y in range(len(arr)):
    for x in range(len(arr[0])):
        if arr[y,x] >= max(arr[y]) / 2:
            color = 'black'
        else:
            color = 'white'
        plt.text(x, y, str(int(arr[y,x])), horizontalalignment='left', verticalalignment='center', color=color)

plt.yticks(range(17), ["synthflags", "phcode", "stresslevel", "sourceix", "wordstress", "tone_ph", "length", "env", "type", "prepause", "amp", "newword", "pitch1", "pitch2", "std_length", "phontab_addr", "sound_param"])
plt.xticks(np.arange(22), xTicksLabels)
fig = plt.gcf()
fig.set_size_inches(15, 20)
plt.subplot(4,1,2)

wave_data = [i[5:].split('|')[0].strip().split() for i in data if i.startswith('wave: ')]
harmonic_peak_data = [i[5:].split('|')[1].strip().split(",")[:-1] for i in data if i.startswith('wave: ')]

wave_data = [[int(j) for j in i] for i in wave_data]

arr = np.zeros((len(wave_data), len(wave_data[0])))

for row in range(len(wave_data)):
    arr[row] = wave_data[row]

arr = np.swapaxes(arr, 0, 1)

normalized = np.zeros(arr.shape)

for row in range(len(arr)):
    normalized[row] = arr[row] / max(arr[row])

for i, row in enumerate(normalized):
    plt.plot(row+i)
plt.xlim([0, len(normalized[0])])


plt.yticks(np.arange(7)+0.5, ("samplecount", "phaseinc", "samplecount_start", "wdata.amplitude",
		            "wdata.pitch", "wdata.amplitude_fmt", "flutter_amp"))

harmonic_peak_data = [[[float(i) for i in harm.split()] for harm in t] for t in harmonic_peak_data]
arr = np.array(harmonic_peak_data)

formant_labels = ["freq1", "height1", "left1", "right1"]
for graphi in range(4):
    plt.subplot(8,1,5+graphi)
    plt.ylabel(formant_labels[graphi])
    for i in range(6):
        plt.plot(arr[:,i,graphi])
    plt.xlim([0, len(arr[:,0,graphi])])

plt.show()
