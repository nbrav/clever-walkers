import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import math

plt.close('all')

agent_num = 4;
#tag = "collide";state_size = 1000;
tag = "goal";state_size = 100; action_size = 8*3;
agent_colour = ['r','b','g','y']

fig, axes = plt.subplots(nrows=agent_num, ncols=1)

for agent in range(0,agent_num):

    # Q-value
    FILE_NAME = "qvalue."+tag+"."+str(agent)+".log";
    qtable = np.fromfile(FILE_NAME,np.float32)

    if(os.path.isfile(FILE_NAME)==False):
        print "Run the brain first..."

    Q = -qtable.reshape(len(qtable)/state_size/action_size,state_size,action_size)

    if(Q.shape[0]==0):
        print "Not enough learning.. Wait longer"

    S = range(0,state_size);
    T = Q.shape[0];

    im = plt.subplot(agent_num,1,agent+1)
    
    plt.plot(np.sum(np.sum(Q[1:,:,:]-Q[:-1,:,:],2),1)/state_size/action_size,color=agent_colour[agent])

    plt.title("Agent "+str(agent))
    plt.xlim(-10,1000)
    #plt.ylim(-0.003,0.003)

plt.suptitle("Convergence test\n"+r"$<Q>_{s,a}$ ")

