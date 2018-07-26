import matplotlib as mpl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import math
import argparse

plt.close('all')

# parse world propeties
tag = ["goal","collide"];
parser = argparse.ArgumentParser(description='Plot reward for n agents.')
parser.add_argument("-n", "--num_agents", type=int, help='number of agents')
args = parser.parse_args()
NUM_AGENTS = args.num_agents;

# plot properties
REWARD_SIGN = {"goal":1,"collide":-1}
PATH = "./data/trial.";

BIN_SIZE    = 100;

idx_colour = ['r','b','g','y','k','r','b','g','y','k']
idx_legend = []

fig, axes = plt.subplots(nrows=1, ncols=len(tag), figsize=(7, 5), dpi=80, facecolor='w', edgecolor='k')

for goal_tag in tag:
    
    reward_vec = [];
    for idx in range(0,NUM_AGENTS):            

        reward = np.fromfile(PATH+goal_tag+"."+str(idx)+".log",np.float32)[:] * REWARD_SIGN[goal_tag]
        
        T = np.array(range(0,reward.shape[0]))
        BIN_NUM = int(math.floor(reward.shape[0]/BIN_SIZE));
        
        R_smooth = np.convolve(reward, np.ones(BIN_SIZE)/BIN_SIZE)
        R_smooth = R_smooth[BIN_SIZE:BIN_SIZE*BIN_NUM]
    
        reward_vec.append(reward)

    ax = plt.subplot(len(tag),1,tag.index(goal_tag)+1)

    reward_avg = np.mean(np.asarray(reward_vec),0)
    reward_std = np.std(np.asarray(reward_vec),0)
    ax.plot(T,reward_avg,'g',linewidth=2.5,alpha=0.5)        
    ax.fill_between(T, reward_avg+reward_std, reward_avg-reward_std, linewidth=0.0, facecolor='gray', alpha=0.5)
    
    ax.set_xlabel(r'time-step $\times 10^3$',fontsize=20)
    ax.set_ylabel(r'$<$'+goal_tag+r'$>$', fontsize=20)

    plt.tick_params(labelsize=15)
    if goal_tag == "goal":
        plt.ylim(-2.5,30)
        ax.legend(['mean','std'],loc='upper left')
    else:
        plt.ylim(-10,50)
        ax.legend(['mean','std'],loc='upper right')
        
        
        #plt.xlim(-2,2000)

        #ax.plot(T,np.zeros(T.shape),'--',linewidth=2.5,alpha=0.5)

print(np.mean(reward_avg), np.mean(reward_std))
            
print("Number of updates: ",reward.shape[0])        
fig.tight_layout()
plt.show()
