import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.patches import Wedge 
import numpy as np
import math
import os

plt.close('all')

agent_num = 10;
tag = "goal"; state_size_x = 15; state_size_y = 15; action_size = 8*3;
#tag = "collide"; state_size = 1000; action_size=36*5; agent = 0;

#plot_timed_q()
vmin = 0.0; vmax = 0.1;

plt.figure(figsize=(10,4))

for agent in range(0,agent_num):

    FILE_NAME = "critic."+tag+"."+str(agent)+".log";
    vtable = np.fromfile(FILE_NAME,np.float32)
    
    #qmax = np.max(qtable); qmin = np.min(qtable);

    if(os.path.isfile(FILE_NAME)==False):
        print("Run the brain first...")
        
    V = vtable.reshape(int(len(vtable)/state_size_x/state_size_y),state_size_x,state_size_y)
 
    if(V.shape[0]==0):
        print("Not enough learning.. Wait longer")

    T = V.shape[0];

    print("agent:"+ str(agent) +" #updates:",V.shape[0])
    
    im = plt.subplot(agent_num/5,agent_num/2,agent+1)
    plt.imshow(V[T-1,:,:],cmap='binary',interpolation="none")
    plt.axis('off')
    plt.ylabel('place cell index')
    plt.xlabel('action index')
    #plt.title("Agent "+str(agent))
    plt.clim(vmin,vmax)
    #plt.colorbar(orientation="horizontal")

#plt.suptitle("Values-function (learning)\nwith "+str(agent_num)+" agents")
plt.show()
