import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.patches import Wedge 
import numpy as np
import math
import os
import argparse

plt.close('all')

# parse world propeties
parser = argparse.ArgumentParser(description='Plot reward for n agents.')
parser.add_argument("-n", "--num_agents", type=int, help='number of agents')
parser.add_argument("-o", "--num_objectives", type=str, help='number of objectives')
args = parser.parse_args()
NUM_AGENTS = args.num_agents; tag = args.num_objectives;

if(tag=="goal"):
    state_size_x = 15; state_size_y = 15; action_size = 8*3;
else:
    state_size_x = 100; state_size_y = 10; action_size=8*3;

#plot_timed_q()
#vmin = 0.0; vmax = 250.0;

plt.figure(figsize=(10,4))

for agent in range(0,NUM_AGENTS):

    FILE_NAME = "critic."+tag+"."+str(agent)+".log";
    vtable = np.fromfile(FILE_NAME,np.float32)
    
    #qmax = np.max(qtable); qmin = np.min(qtable);

    if(os.path.isfile(FILE_NAME)==False):
        print("Run the brain first...")
        
    V = vtable.reshape(int(len(vtable)/state_size_x/state_size_y),state_size_x,state_size_y)
 
    if(V.shape[0]==0):
        print("Not enough learning.. Wait longer")

    T = V.shape[0];

    print("agent:"+ str(agent) +" #updates:",V.shape[0], "range:",[V.min(),V.max()])
    
    im = plt.subplot(NUM_AGENTS/2,NUM_AGENTS/5,agent+1)
    plt.imshow(V[T-1,:,:].T,cmap='Wistia',interpolation="none")
    plt.axis('off')
    plt.ylabel('place cell index')
    plt.xlabel('action index')
    plt.title("Agent "+str(agent))
    #plt.clim(vmin,vmax)
    plt.colorbar(orientation="horizontal")

#plt.suptitle("Values-function (learning)\nwith "+str(NUM_AGENTS)+" agents")
plt.show()
