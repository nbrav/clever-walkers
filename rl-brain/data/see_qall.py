import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import math

plt.close('all')

agent_num = 4;

#tag = "goal"; state_size = 100;
tag = "collide"; state_size = 1000;

fig, axes = plt.subplots(nrows=1, ncols=agent_num)

for agent in range(0,agent_num):

    # Q-value
    FILE_NAME = "qvalue."+tag+"."+str(agent)+".log";
    qtable = np.fromfile(FILE_NAME,np.float32)

    if(os.path.isfile(FILE_NAME)==False):
        print "Run the brain first..."
        
    action_size = 8;

    Q = qtable.reshape(len(qtable)/state_size/action_size,state_size,action_size)

    if(Q.shape[0]==0):
        print "Not enough learning.. Wait longer"

    S = range(0,state_size);
    T = Q.shape[0];

    im = plt.subplot(1,agent_num,agent+1)
    plt.imshow(Q[T-1,S,:])
    plt.title("Agent "+str(agent)+"\n"+str((T-1)*100)+"s")
    plt.clim(np.min(Q),np.max(Q))

plt.suptitle("Q-values (learning)\nwith "+str(agent_num)+" agents")

plt.colorbar()
#plt.show()
