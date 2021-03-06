import sndsource as s 
import numpy as np
import matplotlib.pyplot as plt
op = s.OpenOptions()
op.filename = "/home/akhlif-deezer/la.wav"

snd = s.SndSource(op)

a = np.zeros(2048, dtype=np.int16)
print snd.pull_data(a, 0)
print a.astype(np.int16)


x = a/float(2**15)

plt.plot(x)

#plt.show()
of = s.OutFormat()
s.print_out_format(of)
wt = s.SndWriter("la.mp3", of)
wt.open()
print "opened"
while snd.pull_data(a, 0) > 0:
    print "writing"
    wt.write(a)

wt.close()


