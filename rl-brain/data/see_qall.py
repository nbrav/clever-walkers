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
tag = ["goal","collide"];
parser = argparse.ArgumentParser(description='Plot reward for n agents.')
parser.add_argument("-n", "--num_agents", type=int, help='number of agents')
parser.add_argument("-o", "--num_objectives", type=str, help='number of objectives')
args = parser.parse_args()
NUM_AGENTS = args.num_agents; tag = args.num_objectives;

if(tag=="goal"):
    state_size_x = 15; state_size_y = 15; action_size = 8*3;
else:
    state_size_x = 100; state_size_y = 10; action_size=8*3; agent = 0;

def plot_polar():

    num_sec = 24; num_ring = 4; num_relvel = 10; agent=0;
    fig, axes = plt.subplots(nrows=1, ncols=1)
    qtable = np.fromfile("../results/b.collision/qvalue."+tag+"."+str(agent)+".log",np.float32)        
    Q = qtable.reshape(len(qtable)/state_size/action_size,state_size,action_size)
    del qtable

    Q0 = Q[0,:,:].reshape(num_sec+1, num_ring, num_relvel, action_size);    
    #plot_obj = plt.imshow(np.average(np.average(Q0,2),2).T)
    plot_obj = plt.imshow(np.average(Q0,2)[:,:,0].T)
    plot_obj.autoscale();
        
    for t in range(Q.shape[0]):
        Qt = Q[t,:,:].reshape(num_sec+1, num_ring, num_relvel, action_size);
        plot_obj.set_data(np.average(np.average(Qt,2),2).T)
        plot_obj.autoscale();
        plt.draw();        
        plt.pause(0.1);
        plt.title("#epochs:"+str(t))

def plot_timed_q():
    num_sec = 24; num_ring = 4; num_relvel = 10;
    fig, axes = plt.subplots(nrows=1, ncols=1)
    qtable = np.fromfile("../results/b.collision/qvalue."+tag+"."+str(agent)+".log",np.float32)        

    Q = qtable.reshape(len(qtable)/state_size/action_size,(num_sec+1)*num_ring*num_relvel,action_size)
    
    Q_mu = np.average(Q,1)
    Q_std = np.std(Q,1)

    plt.plot(range(Q.shape[0]),Q_mu)
    plt.plot(range(Q.shape[0]),Q_mu-Q_std,'--')
    plt.plot(range(Q.shape[0]),Q_mu+Q_std,'--')
    #plt.fill_between(range(Q.shape[0]), Q_mu-Q_std, Q_mu+Q_std, alpha=0.2, edgecolor='#1B2ACC', facecolor='#089FFF',linewidth=4, linestyle='dashdot', antialiased=True)

#plot_timed_q()
qmin = -1.0; qmax = 1.0;

for agent in range(0,NUM_AGENTS):

    FILE_NAME = "qvalue."+tag+"."+str(agent)+".log";
    qtable = np.fromfile(FILE_NAME,np.float32)
    
    #qmax = np.max(qtable); qmin = np.min(qtable);

    if(os.path.isfile(FILE_NAME)==False):
        print("Run the brain first...")
        
    Q = qtable.reshape(int(len(qtable)/state_size_x/state_size_y/action_size),state_size_x*state_size_y,action_size)
 
    if(Q.shape[0]==0):
        print("Not enough learning.. Wait longer")

    T = Q.shape[0];

    print("agent:"+ str(agent) +" #updates:",Q.shape[0])
    print(Q.sum())

    im = plt.subplot(1,NUM_AGENTS,agent+1)
    plt.imshow(Q[T-1,:,:],cmap='binary',interpolation="none")
    plt.ylabel('place cell index')
    plt.xlabel('action index')
    plt.title("Agent "+str(agent))
    plt.clim(qmin,qmax)
    plt.colorbar(orientation="horizontal")

plt.suptitle("Q-values (learning)\nwith "+str(NUM_AGENTS)+" agents")
plt.show()
