import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import math

plt.close('all')

# Q-value
qtable = np.fromfile("qvalue.log",np.float32)

state_size = 256;
action_size = 8;

Q = qtable.reshape(len(qtable)/state_size/action_size,state_size,action_size)

print Q.shape, Q.max(), Q.min()

fig, axes = plt.subplots(nrows=5, ncols=1)

T = Q.shape[0];
T1 = int(T/4); T2 = int(T/2); T3 = int(3*T/4);

im1 = plt.subplot(1,5,1)
plt.imshow(Q[0,:,:])
plt.title(str(0*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im2 = plt.subplot(1,5,2)
plt.imshow(Q[T1,:,:])
plt.title(str(T1*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im3 = plt.subplot(1,5,3)
plt.imshow(Q[T2,:,:])
plt.title(str(T2*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im4 = plt.subplot(1,5,4)
plt.imshow(Q[T3,:,:])
plt.title(str(T3*100)+"s")
plt.clim(np.min(Q),np.max(Q))

im5 = plt.subplot(1,5,5)
plt.imshow(Q[-1,:,:])
plt.title(str(T*100)+"s")
plt.clim(np.min(Q),np.max(Q))

plt.suptitle("Q-values (learning)")

plt.colorbar()

#plt.show()
