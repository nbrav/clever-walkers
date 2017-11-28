import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import math

plt.close('all')
#fig = plt.figure(1,figsize=(10,10))

# plot properties
BIN_SIZE = 200;

# reward/punishment values
reward = np.fromfile("reward-punishment.0.log",np.float32)

print "Agent-Environment updates=",reward.shape

T = np.array(range(0,reward.shape[0]-1))

BIN_NUM = int(math.floor(reward.shape[0]/BIN_SIZE));

T = T[:BIN_SIZE*BIN_NUM]
R = reward[:BIN_SIZE*BIN_NUM]

T = T.reshape((BIN_SIZE,BIN_NUM))
R = R.reshape((BIN_SIZE,BIN_NUM))

R_mean = np.sum(R,0);
T_mean = T[0,:] 

fig = plt.figure()

plt.plot(T_mean,R_mean,'b',linewidth=1.5,alpha=1)
plt.title('Frequency of collisions')
plt.xlabel('timex'+str(BIN_SIZE)+' updates')
#plt.ylim(0,R_mean.max()*3.0/2.0);
plt.legend()
plt.grid()

plt.show()
