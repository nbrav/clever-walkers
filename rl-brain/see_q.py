import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import math

plt.close('all')

agent = 0;

# Q-value
qtable = np.fromfile("qvalue."+str(agent)+".log",np.float32)

if(os.path.isfile("qvalue.log")==False):
    print "Run the brain first..."

state_size = 88;
action_size = 8;

Q = qtable.reshape(len(qtable)/state_size/action_size,state_size,action_size)

if(Q.shape[0]==0):
    print "Not enough learning.. Wait longer"

print "Epochs completed=",Q.shape[0]
print "|S|=",Q.shape[1],";|A|=",Q.shape[2],

fig, axes = plt.subplots(nrows=5, ncols=1)

S = range(0,88);
T = Q.shape[0];
T1 = int(T/4); T2 = int(T/2); T3 = int(3*T/4);

im1 = plt.subplot(1,5,1)
plt.imshow(Q[0,S,:])
plt.title(str(0*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im2 = plt.subplot(1,5,2)
plt.imshow(Q[T1,S,:])
plt.title(str(T1*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im3 = plt.subplot(1,5,3)
plt.imshow(Q[T2,S,:])
plt.title(str(T2*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im4 = plt.subplot(1,5,4)
plt.imshow(Q[T3,S,:])
plt.title(str(T3*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im5 = plt.subplot(1,5,5)
plt.imshow(Q[T-1,S,:])
plt.title(str((T-1)*100)+"s")
plt.clim(np.min(Q),np.max(Q))

plt.suptitle("Q-values (learning) agent:"+str(agent))
plt.colorbar()
#plt.show()
